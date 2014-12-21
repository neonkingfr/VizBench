/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <pthread.h>

#include "NosuchUtil.h"

#include "FFFF.h"
#include "NosuchJSON.h"
#include "Timer.h"

#include "opencv/cv.h"
#include "opencv/highgui.h"

#include <iostream>
#include <fstream>

#include "VizServer.h"

extern "C" {
	int zre_msg_test(bool);
	int zre_log_msg_test(bool);
	int zre_uuid_test(bool);
};

const char* FFFF_json(void* data,const char *method, cJSON* params, const char* id) {

	// NEEDS to be static, since we return the c_str() of it.
	// This also assumes that JSON API calls are serialized.
	static std::string result;

	FFFF* ffff = (FFFF*)data;
	ffff->_json_result = ffff->submitJson(std::string(method),params,id);
	return ffff->_json_result.c_str();
}

FFFF::FFFF() {
	_showfps = true;
	_capture = NULL;
	_pipeline = NULL;
	hidden = false;

#ifdef ZRE_TEST
	zre_msg_test(true);
    zre_log_msg_test (true);
    zre_uuid_test (true);

	void *context = zmq_ctx_new();
	void *responder = zmq_socket(context, ZMQ_REP);
	int rc = zmq_bind(responder, "tcp://*:5555");
	DEBUGPRINT(("zmq_bind rc=%d",rc));
	while (1) {
        char buffer [10];
        zmq_recv (responder, buffer, 10, 0);
        printf ("Received Hello\n");
        Sleep (1);          //  Do some 'work'
        zmq_send (responder, "World", 5, 0);
    }
	DEBUGPRINT(("zmq test done"));
#endif

	NosuchLockInit(&_json_mutex,"json");
	_json_cond = PTHREAD_COND_INITIALIZER;
	_json_pending = false;
	_timer = Timer::New();
	desired_FPS = 60.0;
	throttle_timePerFrame = 1.0 / desired_FPS;
	throttle_lasttime = 0.0;

	fps_accumulator = 0;
	fps_lasttime = -1.0;
}

void
FFFF::InsertKeystroke(int key,int updown) {
	_vizserver->InsertKeystroke(key,updown);
}

void
FFFF::StartStuff() {

	// Error popups during API execution will
	// hang the API (unless we make it non-modal),
	// so don't do it.  Error messages should
	// end up in the log anyway.
	NosuchErrorPopup = NULL;

	_vizserver = VizServer::GetServer();
	_vizserver->Start();
	ApiFilter af = ApiFilter("ffff");
	_vizserver->AddJsonCallback((void*)this,af,FFFF_json,(void*)this);
}

void
FFFF::StopStuff() {
	_vizserver->Stop();
}

void
FFFF::CheckFPS()
{
	// Put out FPS
    curFrameTime = _timer->GetElapsedTime();
	if ( fps_lasttime < 0 || (curFrameTime-fps_lasttime) > 1.0 ) {
		if ( _showfps ) {
			DEBUGPRINT(("FPS = %d\n",fps_accumulator));
		}
		fps_lasttime = curFrameTime;
		fps_accumulator = 0;
	} else {
		fps_accumulator++;
	}

	// Throttle things so we only do desired_FPS frames per second
	// XXX - BOGUS!  There's got to be a better way.
	while (1) {
	    curFrameTime = _timer->GetElapsedTime();
		if ( (curFrameTime-throttle_lasttime) > throttle_timePerFrame ) {
			throttle_lasttime += throttle_timePerFrame;
			break;
		} else {
			Sleep(1);
		}
	}

}

std::string
FFFF::submitJson(std::string method, cJSON *params, const char* id) {

	// We want JSON requests to be interpreted in the main thread of the FFGL plugin,
	// so we stuff the request into _json_* variables and wait for the main thread to
	// pick it up (in ProcessOpenGL)
	NosuchLock(&_json_mutex,"json");

	_json_pending = true;
	_json_method = std::string(method);
	_json_params = params;
	_json_id = id;

	bool err = false;
	while ( _json_pending ) {
		DEBUGPRINT2(("####### Waiting for _json_cond!"));
		int e = pthread_cond_wait(&_json_cond, &_json_mutex);
		if ( e ) {
			DEBUGPRINT(("####### ERROR from pthread_cond_wait e=%d",e));
			err = true;
			break;
		}
	}
	std::string result;
	if ( err ) {
		result = jsonError(-32000,"Error waiting for json!?",id);
	} else {
		result = _json_result;
	}

	NosuchUnlock(&_json_mutex,"json");

	return result;
}

void
FFFF::checkAndExecuteJSON() {
	NosuchLock(&_json_mutex,"json");
	if (_json_pending) {
		// Execute json stuff and generate response
		_json_result = executeJsonAndCatchExceptions(_json_method, _json_params, _json_id);
		_json_pending = false;
		int e = pthread_cond_signal(&_json_cond);
		if ( e ) {
			DEBUGPRINT(("ERROR from pthread_cond_signal e=%d\n",e));
		}
	}
	NosuchUnlock(&_json_mutex,"json");

}

std::string FFFF::executeJsonAndCatchExceptions(std::string meth, cJSON *params, const char* id) {
	std::string r;
	try {
		CATCH_NULL_POINTERS;

		r = executeJson(meth,params,id);
	} catch (NosuchException& e) {
		std::string s = NosuchSnprintf("Exception in '%s' API - %s",meth.c_str(),e.message());
		r = jsonError(-32000,s.c_str(),id);
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		std::string s = NosuchSnprintf("Other exception in '%s' API",meth.c_str());
		r = jsonError(-32000,s.c_str(),id);
	}
	return r;
}

IplImage*
FFFF::getCameraFrame()
{
	if ( _capture == NULL ) {
		return NULL;
	}
	return cvQueryFrame(_capture);
}

bool
FFFF::initCamera(int camindex) {

	if ( camindex < 0 ) {
		DEBUGPRINT(("CAMERA disabled (camera < 0)"));
		_capture = NULL;
		return FALSE;
	}
	_capture = cvCreateCameraCapture(camindex);
	if ( !_capture ) {
	    DEBUGPRINT(("Unable to initialize capture from camera index=%d\n",camindex));
	    return FALSE;
	}
	DEBUGPRINT1(("CAMERA detail FPS=%f wid=%f hgt=%f msec=%f ratio=%f fourcc=%f",
		cvGetCaptureProperty(_capture,CV_CAP_PROP_FPS),
		cvGetCaptureProperty(_capture,CV_CAP_PROP_FRAME_WIDTH),
		cvGetCaptureProperty(_capture,CV_CAP_PROP_FRAME_HEIGHT),
		cvGetCaptureProperty(_capture,CV_CAP_PROP_POS_MSEC),
		cvGetCaptureProperty(_capture,CV_CAP_PROP_POS_AVI_RATIO),
		cvGetCaptureProperty(_capture,CV_CAP_PROP_FOURCC)));

	// This code is an attempt to figure out when the camera is way too slow - on
	// my laptop, for example, if you try to use camera index 1 (when there's only
	// one camera, i.e. when you should be using camera index 0), the open
	// succeeds, but it takes 1 second per frame.  Hence the code below.
	// If the camera's working properly, this code will execute
	// quickly, but if not, this will cause a 2-second delay and
	// then the camera will be disabled and we'll continue on.
	double t1 = glfwGetTime();
	IplImage* cami = cvQueryFrame(_capture);
	double t2 = glfwGetTime();
	cami = cvQueryFrame(_capture);
	double t3 = glfwGetTime();
	if ( (t2-t1) > 0.75 && (t3-t2) > 0.75 ) {
		cvReleaseCapture(&_capture);
		_capture = NULL;
		DEBUGPRINT(("Getting Camera Frames is too slow!  Disabling camera! (times=%lf %lf %lf",t1,t2,t3));
		return FALSE;
	}

#define CV_CAP_PROP_FRAME_WIDTH    3
#define CV_CAP_PROP_FRAME_HEIGHT   4

    /* retrieve or set capture properties */
    double fwidth = cvGetCaptureProperty( _capture, CV_CAP_PROP_FRAME_WIDTH );
    double fheight = cvGetCaptureProperty( _capture, CV_CAP_PROP_FRAME_HEIGHT );

    camWidth = (int)fwidth;
    camHeight = (int)fheight;

    DEBUGPRINT(("CAMERA CV says width=%d height=%d\n",camWidth,camHeight));
	return TRUE;
}

FFGLPluginInstance*
FFFF::newPluginInstance(FFGLPluginDef* plugin, std::string inm)
{
	FFGLPluginInstance* np = new FFGLPluginInstance(plugin,inm);
	if ( np->InstantiateGL(&fboViewport)!=FF_SUCCESS ) {
		delete np;
		throw NosuchException("Unable to InstantiateGL !?");
	}
	return np;
}

void
FFFF::deletePluginInstance(FFGLPluginInstance* p)
{
	DEBUGPRINT(("DELETING FFGLPluginInstance p=%ld",(long)p));
	p->DeInstantiateGL();
	delete p;
}

void
FFFF::clearPipeline()
{
    for ( int n=0; n<NPREPLUGINS; n++ ) {
        if ( preplugins[n] ) {
			delete preplugins[n];
		}
		preplugins[n] = NULL;
    }
	FFGLPluginInstance* next;
	for ( FFGLPluginInstance* p=_pipeline; p!=NULL; p=next ) {
		next = p->next;
		deletePluginInstance(p);
    }
	_pipeline = NULL;
}

void
FFFF::loadPluginDefs(std::string ffpath, std::string ffglpath, int ffgl_width, int ffgl_height)
{
    nffplugindefs = 0;
    nffglplugindefs = 0;

    loadffglpath(ffglpath);
    if ( FFGLinit2(ffgl_width,ffgl_height) != 1 ) {
		DEBUGPRINT(("HEY!  FFGLinit2 failed!?"));
		return;
	}

    // loadplugins();
    loadffpath(ffpath);

    DEBUGPRINT(("%d FF plugins, %d FFGL plugins\n",nffplugindefs,nffglplugindefs));
}

FFGLPluginInstance*
FFFF::findInstance(std::string inm) {
	for ( FFGLPluginInstance* p=_pipeline; p!=NULL; p=p->next ) {
		if ( inm == p->name() ) {
			return p;
		}
	}
	return NULL;
}

// Return a new plugin instance name, making sure it doesn't match any existing ones
std::string
FFFF::newInstanceName() {
	int inum = 0;
	while ( true ) {
		std::string nm = NosuchSnprintf("p%d",inum++);
		FFGLPluginInstance* p=_pipeline;
		for ( ; p!=NULL; p=p->next ) {
			if ( nm == p->name() ) {
				break;
			}
		}
		if ( p == NULL ) {
			return nm;
		}
	}
}

FFGLPluginInstance*
FFFF::addToPipeline(std::string pluginName, std::string inm, bool autoenable = true) {

	// First scan existing pipeline to see if this instanceName is already used
	FFGLPluginInstance* last = NULL;
	for ( FFGLPluginInstance* p=_pipeline; p!=NULL; p=p->next ) {
		if ( inm == p->name() ) {
			DEBUGPRINT(("Plugin instance named '%s' already exists",inm.c_str()));
			return NULL;
		}
		last = p;
	}

    FFGLPluginDef* plugin = findffglplugin(pluginName);
	if ( plugin == NULL ) {
		DEBUGPRINT(("There is no plugin named '%s'",pluginName.c_str()));
		return NULL;
	}

	FFGLPluginInstance* np = newPluginInstance(plugin,inm);

	// If the plugin's first parameter is "viztag", set it
	int pnum = np->plugindef()->getParamNum("viztag");
	if ( pnum >= 0 ) {
		DEBUGPRINT1(("In addToPipeline of %s, setting viztag to %s",pluginName.c_str(),inm.c_str()));
		np->setparam("viztag",inm);
	}

	// Add it to the end of the pipeline
	if ( last == NULL ) {
		_pipeline = np;
	} else {
		last->next = np;
	}

	if ( autoenable ) {
		np->enable();
	}
	return np;
}

void
FFFF::deleteFromPipeline(std::string inm) {

	// First scan existing pipeline to see if this instanceName is already used
	FFGLPluginInstance* prev = NULL;
	FFGLPluginInstance* p = _pipeline;
	while ( p ) {
		if ( inm == p->name() ) {
			break;
		}
		prev = p;
		p=p->next;
	}
	if ( p == NULL ) {
		// don't complain when deleting an instance that doesn't exist
		return;
	}
	if ( prev == NULL ) {
		_pipeline = _pipeline->next;
	} else {
		prev->next = p->next;
	}
	deletePluginInstance(p);
}

void
FFFF::loadPipeline(std::string configname)
{
	std::string fname = ManifoldPath("config/ffff/"+configname+".json");
	std::string err;

	cJSON* json = jsonReadFile(fname,err);
	if ( !json ) {
		throw NosuchException("Unable to loadPipeline of config: %s, err=%s",
			fname.c_str(),err.c_str());
	}
	loadPipelineJson(json);
	jsonFree(json);
}

void
FFFF::loadPipelineJson(cJSON* json)
{

	clearPipeline();

	cJSON* plugins = jsonGetArray(json,"pipeline");
	if ( !plugins ) {
		throw NosuchException("No 'pipeline' value in config");
	}

	int nplugins = cJSON_GetArraySize(plugins);
	for ( int n=0; n<nplugins; n++ ) {
		cJSON *p = cJSON_GetArrayItem(plugins,n);
		NosuchAssert(p);
		if ( p->type != cJSON_Object ) {
			throw NosuchException("Hey! Item #%d in pipeline isn't an object?",n);
		}
		cJSON* plugin = jsonGetString(p,"plugin");

		// If an explicit viztag isn't given, use plugin name
		std::string viztag = jsonNeedString(p,"viztag",plugin->valuestring);
		cJSON* enabled = jsonGetNumber(p,"enabled");  // optional, default is 1 (true)
		cJSON* params = jsonGetObject(p,"params");
		NosuchAssert(plugin && params);

		// Default is to enable the plugin as soon as we added it, but you
		// can add an "enabled" value to change that.
		bool autoenable = (enabled==NULL || enabled->valuedouble != 0.0);
		FFGLPluginInstance* pi = addToPipeline(plugin->valuestring,viztag,autoenable);
		if ( ! pi ) {
			DEBUGPRINT(("Pipeline, unable to add viztag=%s plugin=%s",
				viztag.c_str(),plugin->valuestring));
			continue;
		}
		DEBUGPRINT(("Pipeline, loaded viztag=%s plugin=%s",
			viztag.c_str(),plugin->valuestring));
		for ( cJSON* pn=params->child; pn!=NULL; pn=pn->next ) {
			NosuchAssert(pn->type == cJSON_Number);
			pi->setparam(pn->string,(float)(pn->valuedouble));
		}
	}
}

void
doCameraFrame(IplImage* frame)
{
}

bool
FFFF::doOneFrame(bool use_camera, int window_width, int window_height)
{
	int interp;
    unsigned char * pixels;
    IplImage* img1;
    IplImage* img2;
	IplImage* camframe = NULL;
    unsigned char *pixels_into_pipeline;
    IplImage* img_into_pipeline;

	DEBUGPRINT1(("DOONEFRAME START"));
	
    //bind the gl texture so we can upload the next video frame
    glBindTexture(GL_TEXTURE_2D, mapTexture.Handle);

	if ( use_camera ) {
		camframe = getCameraFrame();
	}

	if ( camframe != NULL ) {
	    pixels = (unsigned char *)(camframe->imageData);
	
	    CvSize camsz;
	    CvSize ffsz;
	    CvSize fbosz;

	    camsz = cvSize(camWidth,camHeight);
	    ffsz = cvSize(ffWidth,ffHeight);
	    fbosz = cvSize(fboWidth,fboHeight);
	
		img1 = cvCreateImageHeader(camsz, IPL_DEPTH_8U, 3);
	    cvSetImageData(img1,pixels,camWidth*3);
	
	    img2 = cvCreateImage(ffsz, IPL_DEPTH_8U, 3);
	    img_into_pipeline = cvCreateImage(fbosz, IPL_DEPTH_8U, 3);
	
		interp = CV_INTER_LINEAR;  // or CV_INTER_NN
		interp = CV_INTER_NN;  // This produces some artifacts compared to CV_INTER_LINEAR, but is faster
	
		// XXX - If the camera size is the same as the ff (ffgl) size,
		// we shouldn't have to do this resize
	    cvResize(img1, img2, interp);
	
	    unsigned char *resizedpixels = NULL;
	    cvGetImageRawData( img2, &resizedpixels, NULL, NULL );
	    pixels = resizedpixels;

	    if (pixels != 0) {
	        for ( int n=0; n<NPREPLUGINS; n++ ) {
	            FFPluginDef* p = preplugins[n];
	            if ( p ) {
	                p->Process(p->m_instanceid,pixels);
	            }
	        }
	    }

	    cvResize(img2, img_into_pipeline, interp);
	    cvGetImageRawData( img_into_pipeline, &pixels_into_pipeline, NULL, NULL );

	} else {
	    // WAS - img_into_pipeline = cvCreateImage(cvSize(window_width,window_height), IPL_DEPTH_8U, 3);
	    img_into_pipeline = cvCreateImage(cvSize(fboWidth,fboHeight), IPL_DEPTH_8U, 3);

	    pixels_into_pipeline = (unsigned char *)(img_into_pipeline->imageData);
		cvZero(img_into_pipeline);
	
        for ( int n=0; n<NPREPLUGINS; n++ ) {
            FFPluginDef* p = preplugins[n];
            if ( p ) {
                p->Process(p->m_instanceid,pixels_into_pipeline);
            }
	    }
	}

    //upload it to the gl texture. use subimage because
    //the video frame size is probably smaller than the
    //size of the texture on the gpu hardware
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0,
                    fboWidth,
                    fboHeight,
                    GL_BGR_EXT,
                    GL_UNSIGNED_BYTE,
                    pixels_into_pipeline);

    //unbind the gl texture
    glBindTexture(GL_TEXTURE_2D, 0);

    FFGLPluginInstance* lastplugin = NULL;
	FFGLPluginInstance* firstplugin = NULL;
    int nactive = 0;
	FFGLPluginInstance* pi;

	for ( pi = _pipeline; pi!=NULL; pi=pi->next ) {
		if ( pi->isEnabled() ) {
			nactive++;
			lastplugin = pi;
			if ( firstplugin == NULL ) {
				firstplugin = pi;
			}
		}
    }

	if ( nactive == 0 ) {
		if ( ! do_ffgl_plugin(NULL,4) ) {
			DEBUGPRINT(("Error A in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
			// return 0;
		}
	}

	for ( pi = _pipeline; pi!=NULL; pi=pi->next ) {
		if ( ! pi->isEnabled() ) {
			continue;
		}
		if ( nactive == 1 ) {
			if ( ! do_ffgl_plugin(pi,3) ) {
				DEBUGPRINT(("DISABLING plugin - Error B in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
				pi->disable();
				// return 0;
			}
			continue;
		}
		if ( pi == lastplugin ) {
			//deactivate rendering to the fbo
			//(this re-activates rendering to the window)
			if ( nactive > 1 ) {
				fbo_output->UnbindAsRenderTarget(glExtensions);
			}
			if ( ! do_ffgl_plugin(pi,2) ) {
				DEBUGPRINT(("DISABLING plugin - Error C in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
				pi->disable();
			}
		} else if ( pi == firstplugin ) {
			if ( ! do_ffgl_plugin(pi,0) ) {
				DEBUGPRINT(("DISABLING plugin - Error D in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
				pi->disable();
			}
		} else {
			if ( ! do_ffgl_plugin(pi,1) ) {
				DEBUGPRINT(("DISABLING plugin - Error E in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
				pi->disable();
			}
		}
	}

	if ( camframe != NULL ) {
		cvReleaseImageHeader(&img1);
		cvReleaseImage(&img2);
		cvReleaseImage(&img_into_pipeline);
	} else {
		cvReleaseImage(&img_into_pipeline);
	}

	DEBUGPRINT1(("DOONEFRAME END"));
	return true;
}

std::string FFFF::FFList() {
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<nffplugindefs; n++ ) {
		r += (sep + "\"" + ffplugindefs[n]->name + "\"");
		sep = ", ";
	}
	r = r + "]";
	return r;
}

std::string FFFF::FFGLList() {
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<nffglplugindefs; n++ ) {
		r += (sep + "\"" + ffglplugindefs[n]->name + "\"");
		sep = ", ";
	}
	r = r + "]";
	return r;
}

std::string FFFF::PipelineList(bool only_enabled) {
	std::string r = "[";
	std::string sep = "";
	for ( FFGLPluginInstance* p=_pipeline; p!=NULL; p=p->next ) {
		bool enabled = p->isEnabled();
		if ( only_enabled==false || enabled==true ) {
			r += (sep + "{ \"plugin\":\""+p->plugindef()->GetPluginName()+"\", \"instance\":\"" + p->name() + "\", "
				+ " \"enabled\": " + (p->isEnabled()?"1":"0") + "  }");
			sep = ", ";
		}
	}
	r = r + "]";
	return r;
}

std::string FFFF::FFParamList(std::string plugin, const char* id) {
    FFPluginDef* p = findffplugin(plugin);
	if ( !p ) {
		return jsonError(-32000,"No plugin by that name",id);
	}
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<p->m_numparams; n++ ) {
		FFParameterDef* ps = &(p->m_paramdefs[n]);
		const char* nm = ps->name.c_str();
		switch(ps->type){
		case -1: // 
		case FF_TYPE_STANDARD:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"standard\", \"default\":%f}",nm,ps->default_float_val);
			break;
		case FF_TYPE_BOOLEAN:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"boolean\", \"default\":%f}",nm,ps->default_float_val);
			break;
		case FF_TYPE_TEXT:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"text\", \"default\":%s}",nm,ps->default_string_val.c_str());
			break;
		default:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"%d\", \"default\":%f}",nm,ps->type,ps->default_float_val);
			break;
		}
		sep = ", ";
	}
	r = r + "]";
	return jsonResult(r,id);
}

std::string FFFF::FFGLParamList(std::string plugin, const char* id) {
    FFGLPluginDef* p = findffglplugin(plugin);
	if ( !p ) {
		return jsonError(-32000,"No plugin by that name",id);
	}
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<p->m_numparams; n++ ) {
		FFGLParameterDef* ps = &(p->m_paramdefs[n]);
		const char* nm = ps->name.c_str();
		switch(ps->type){
		case -1: // 
		case FF_TYPE_STANDARD:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"standard\", \"default\":%f, \"num\":%d }",nm,ps->default_float_val,ps->num);
			break;
		case FF_TYPE_BOOLEAN:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"boolean\", \"default\":%f, \"num\":%d }",nm,ps->default_float_val,ps->num);
			break;
		case FF_TYPE_TEXT:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"text\", \"default\":\"%s\", \"num\":%d}",nm,ps->default_string_val.c_str(),ps->num);
			break;
		default:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"type\":\"%d\", \"default\":%f, \"num\":%d}",nm,ps->type,ps->default_float_val,ps->num);
			break;
		}
		sep = ", ";
	}
	r = r + "]";
	return jsonResult(r,id);
}

std::string FFFF::FFGLParamVals(FFGLPluginInstance* pi, const char* id) {
	FFGLPluginDef* p = pi->plugindef();
	int numparams = p->m_numparams;
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<p->m_numparams; n++ ) {
		FFGLParameterDef* pd = &(p->m_paramdefs[n]);
		const char* nm = pd->name.c_str();
		float v;
		switch(pd->type){
		case -1: // 
		case FF_TYPE_STANDARD:
		case FF_TYPE_BOOLEAN:
			v = pi->GetFloatParameter(pd->num);
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"value\":%f }",nm,v);
			break;
		case FF_TYPE_TEXT:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"%s\" }",nm,pi->GetParameterDisplay(pd->num).c_str());
			break;
		default:
			r += sep+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"???\" }",nm);
			break;
		}
		sep = ", ";
	}
	r = r + "]";
	return jsonResult(r,id);
}

FFGLPluginDef*
needFFGLPlugin(std::string name)
{
		FFGLPluginDef* p = findffglplugin(name);
		if ( p == NULL ) {
			throw NosuchSnprintf("No plugin named '%s'",name.c_str());
		}
		return p;
}

FFGLPluginInstance*
FFFF::needFFGLPluginInstance(std::string name)
{
		FFGLPluginInstance* p = findInstance(name);
		if ( p == NULL ) {
			throw NosuchException("There is no plugin instance named '%s'",name.c_str());
		}
		return p;
}

std::string FFFF::saveFfffPatch(std::string nm, const char* id)
{
	DEBUGPRINT(("saveFfffPatch nm=%s",nm.c_str()));
	std::string err;
	cJSON* j = cJSON_Parse("{}");
	if ( ! jsonWriteFile(nm,j,err) ) {
		std::string e = NosuchSnprintf("Unable to save patch (name=%s, err=%s)",nm.c_str(),err.c_str());
		return jsonError(-32000,e.c_str(),id);
	}
	return jsonOK(id);
}

std::string FFFF::loadFfffPatch(std::string nm, const char* id)
{
	std::string err;
	std::string fname = ManifoldPath("config/ffff/"+nm);
	DEBUGPRINT(("loadPatch nm=%s fname=%s",nm.c_str(),fname.c_str()));
	cJSON* j = jsonReadFile(fname,err);
	if ( ! j ) {
		throw NosuchException("Unable to open file (name=%s, err=%s)",fname.c_str(),err.c_str());
	}
	loadPipelineJson(j);
	jsonFree(j);
	return jsonOK(id);
}

std::string FFFF::executeJson(std::string meth, cJSON *params, const char* id)
{
	std::string err;

	if ( meth == "millinow"  ) {
		return jsonIntResult(_vizserver->MilliNow(),id);
	}
	if ( meth == "clicknow"  ) {
		return jsonIntResult(_vizserver->CurrentClick(),id);
	}
	if ( meth == "show" ) {
		if ( ! window ) {
			throw NosuchException("No window?!");
		}
		hidden = false;
		// For some reason, it doesn't work without
		// iconifying, first.  At least, right after
		// starting it up with the web interface.
		glfwIconifyWindow(window);
		glfwShowWindow(window);
		return ok_json(id);
	}
	if ( meth == "hide" ) {
		if ( ! window ) {
			throw NosuchException("No window?!");
		}
		hidden = true;
		glfwIconifyWindow(window);
		return ok_json(id);
	}
	if ( meth == "_echo" || meth == "echo" ) {
		std::string val = jsonNeedString(params,"value");
		return jsonStringResult(val,id);
	}
	if ( meth == "name" ) {
		return jsonStringResult("FFFF",id);
	}
	if ( meth == "add" ) {
		std::string plugin = jsonNeedString(params,"plugin");
		std::string iname = jsonNeedString(params,"instance","");
		if ( iname == "" ) {
			iname = newInstanceName();
		}
		bool autoenable = (jsonNeedInt(params,"autoenable",1) != 0);
		FFGLPluginInstance* pi = addToPipeline(plugin,iname,autoenable);
		if ( ! pi ) {
			throw NosuchException("Unable to add plugin '%s'",plugin.c_str());
		}
		return jsonStringResult(iname,id);
	}
	if ( meth == "enable" || meth == "disable" ) {
		std::string instance = jsonNeedString(params,"instance");
		FFGLPluginInstance* pi = needFFGLPluginInstance(instance);   // throws an exception if it doesn't exist
		if ( meth == "enable" ) {
			pi->enable();
		} else {
			pi->disable();
		}
		return jsonOK(id);
	}
	if ( meth == "delete" ) {
		// It's okay if instance doesn't exist, so don't use needFFGLPluginInstance
		std::string iname = jsonNeedString(params,"instance");
		deleteFromPipeline(iname);
		return jsonOK(id);
	}
	if ( meth == "ffglparamset" || meth == "set" ) {
		std::string instance = jsonNeedString(params,"instance");
		std::string param = jsonNeedString(params,"param");
		float val = (float) jsonNeedDouble(params,"val");
		FFGLPluginInstance* pi = needFFGLPluginInstance(instance);
		if ( ! pi->setparam(param,val) ) {
			throw NosuchException("Unable to find or set parameter '%s' in instance '%s'",param.c_str(),instance.c_str());
		}
		return jsonOK(id);
	}
	if ( meth == "ffglparamget" || meth == "get" ) {
		std::string instance = jsonNeedString(params,"instance");
		std::string param = jsonNeedString(params,"param");
		FFGLPluginInstance* pi = needFFGLPluginInstance(instance);
		FFGLParameterDef* pd = pi->plugindef()->findparamdef(param);
		if ( pd == NULL ) {
			throw NosuchException("No parameter '%s' in plugin '%s'",param.c_str(),instance.c_str());
		}

		std::string s = pi->GetParameterDisplay(pd->num);
		DEBUGPRINT(("get API, display=%s",s.c_str()));
		float v;
		switch(pd->type){
		case FF_TYPE_TEXT:
			return jsonStringResult(s,id);
			break;
		case FF_TYPE_BOOLEAN:
			v = pi->GetBoolParameter(pd->num);
			return jsonDoubleResult(v,id);
		case FF_TYPE_STANDARD:
		default:
			v = pi->GetFloatParameter(pd->num);
			return jsonDoubleResult(v,id);
		}
		throw NosuchException("UNIMPLEMENTED parameter type (%d) in get API!",pd->type);
	}
	if ( meth == "fflist" || meth == "ffplugins" ) {
		return jsonResult(FFList().c_str(),id);
	}
	if ( meth == "ffgllist" || meth == "ffglplugins" ) {
		return jsonResult(FFGLList().c_str(),id);
	}
	if ( meth == "pipelinelist" || meth == "instancelist" ) {
		bool only_enabled = (jsonNeedInt(params,"enabled",0) != 0);  // default is to list everything
		return jsonResult(PipelineList(only_enabled).c_str(),id);
	}
	if ( meth == "ffparamlist" ) {
		return FFParamList(jsonNeedString(params,"plugin"),id);
	}
	if ( meth == "ffglparamlist" ) {
		std::string plugin = jsonNeedString(params,"plugin","");
		std::string instance = jsonNeedString(params,"instance","");
		if ( plugin == "" && instance != "" ) {
			FFGLPluginInstance* pi = needFFGLPluginInstance(instance);   // throws an exception if it doesn't exist
			plugin = pi->plugindef()->GetPluginName();
		}
		return FFGLParamList(plugin,id);
	}
	if ( meth == "ffglparamvals" ) {
		std::string instance = jsonNeedString(params,"instance","");
		FFGLPluginInstance* pi = needFFGLPluginInstance(instance);   // throws an exception if it doesn't exist
		return FFGLParamVals(pi,id);
	}
	if ( meth == "save" ) {
		std::string fname =  jsonNeedString(params,"patch");
		return saveFfffPatch(fname,id);
	}
	if ( meth == "load" ) {
		std::string fname =  jsonNeedString(params,"patch");
		return loadFfffPatch(fname,id);
	}

	throw NosuchException("Unrecognized method '%s'",meth.c_str());

// 	return jsonError(-32000,err.c_str(),id);
}

class GlfwTimer :
    public Timer
{
public:
    double start_time;

    GlfwTimer()
    {
        Reset();
    }

    void Reset()
    {
        start_time = glfwGetTime();
    }

    double GetElapsedTime()
    {
        double systime = glfwGetTime();
        return ((double)(systime - start_time));
    }

    virtual ~GlfwTimer()
    {
    }
};

Timer *Timer::New()
{
    return new GlfwTimer();
}

