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
	ffff->m_json_result = ffff->submitJson(std::string(method),params,id);
	return ffff->m_json_result.c_str();
}

FFFF::FFFF() {
	m_showfps = true;
	m_capture = NULL;
	m_ffglpipeline = NULL;
	m_ff10pipeline = NULL;
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
	m_json_cond = PTHREAD_COND_INITIALIZER;
	m_json_pending = false;
	m_timer = Timer::New();
	m_desired_FPS = 60.0;
	m_desired_FPS = 120.0;
	m_throttle_timePerFrame = 1.0 / m_desired_FPS;
	m_throttle_lasttime = 0.0;

	m_fps_accumulator = 0;
	m_fps_lasttime = -1.0;

	m_dotrail = true;
	m_trailamount = 0.90f;
}

void
FFFF::setupTrails() {
    FFGLPluginDef* pdef = findffglplugin("Trails");
	if ( pdef == NULL ) {
		DEBUGPRINT(("There is no plugin named Trails!?"));
		return;
	}
	m_trailsplugin = FFGLNewPluginInstance(pdef,"Trails");
	setTrailParams();
}

void
FFFF::InsertKeystroke(int key,int updown) {
	m_vizserver->InsertKeystroke(key,updown);
}

void
FFFF::StartStuff() {

	// Error popups during API execution will
	// hang the API (unless we make it non-modal),
	// so don't do it.  Error messages should
	// end up in the log anyway.
	NosuchErrorPopup = NULL;

	m_vizserver = VizServer::GetServer();
	m_vizserver->Start();
	ApiFilter af = ApiFilter("ffff");
	m_vizserver->AddJsonCallback((void*)this,af,FFFF_json,(void*)this);
}

void
FFFF::StopStuff() {
	m_vizserver->Stop();
	VizServer::DeleteServer();
}

void
FFFF::CheckFPS()
{
	// Put out FPS
    curFrameTime = m_timer->GetElapsedTime();
	if ( m_fps_lasttime < 0 || (curFrameTime - m_fps_lasttime) > 1.0 ) {
		if ( m_showfps ) {
			DEBUGPRINT(("FPS = %d\n",m_fps_accumulator));
		}
		 m_fps_lasttime = curFrameTime;
		 m_fps_accumulator = 0;
	} else {
		 m_fps_accumulator++;
	}

	// Throttle things so we only do desired_FPS frames per second
	// XXX - BOGUS!  There's got to be a better way.
	while (1) {
	    curFrameTime = m_timer->GetElapsedTime();
		if ( (curFrameTime - m_throttle_lasttime) > m_throttle_timePerFrame ) {
			m_throttle_lasttime += m_throttle_timePerFrame;
			// DEBUGPRINT(("m_throttle_lasttime=%f", m_throttle_lasttime));
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
	DEBUGPRINT1(("FFFF::submitJson A meth=%s",method.c_str()));
	NosuchLock(&_json_mutex,"json");

	DEBUGPRINT1(("FFFF::submitJson B meth=%s",method.c_str()));
	m_json_pending = true;
	m_json_method = std::string(method);
	m_json_params = params;
	m_json_id = id;

	bool err = false;
	while ( m_json_pending ) {
		DEBUGPRINT2(("####### Waiting for _json_cond!"));
		int e = pthread_cond_wait(&m_json_cond, &_json_mutex);
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
		result = m_json_result;
	}

	NosuchUnlock(&_json_mutex,"json");

	return result;
}

void
FFFF::checkAndExecuteJSON() {
	NosuchLock(&_json_mutex,"json");
	if (m_json_pending) {
		// Execute json stuff and generate response
		DEBUGPRINT1(("FFFF:executing JSON method=%s",m_json_method.c_str()));
		m_json_result = executeJsonAndCatchExceptions(m_json_method, m_json_params, m_json_id);
		m_json_pending = false;
		int e = pthread_cond_signal(&m_json_cond);
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
		r = jsonError(-32000,s,id);
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		std::string s = NosuchSnprintf("Other exception in '%s' API",meth.c_str());
		r = jsonError(-32000,s,id);
	}
	return r;
}

IplImage*
FFFF::getCameraFrame()
{
	if ( m_capture == NULL ) {
		return NULL;
	}
	return cvQueryFrame(m_capture);
}

bool
FFFF::initCamera(int camindex) {

	if ( camindex < 0 ) {
		DEBUGPRINT(("CAMERA disabled (camera < 0)"));
		m_capture = NULL;
		return FALSE;
	}
	m_capture = cvCreateCameraCapture(camindex);
	if ( !m_capture ) {
	    DEBUGPRINT(("Unable to initialize capture from camera index=%d\n",camindex));
	    return FALSE;
	}
	DEBUGPRINT1(("CAMERA detail FPS=%f wid=%f hgt=%f msec=%f ratio=%f fourcc=%f",
		cvGetCaptureProperty(m_capture,CV_CAP_PROP_FPS),
		cvGetCaptureProperty(m_capture,CV_CAP_PROP_FRAME_WIDTH),
		cvGetCaptureProperty(m_capture,CV_CAP_PROP_FRAME_HEIGHT),
		cvGetCaptureProperty(m_capture,CV_CAP_PROP_POS_MSEC),
		cvGetCaptureProperty(m_capture,CV_CAP_PROP_POS_AVI_RATIO),
		cvGetCaptureProperty(m_capture,CV_CAP_PROP_FOURCC)));

	// This code is an attempt to figure out when the camera is way too slow - on
	// my laptop, for example, if you try to use camera index 1 (when there's only
	// one camera, i.e. when you should be using camera index 0), the open
	// succeeds, but it takes 1 second per frame.  Hence the code below.
	// If the camera's working properly, this code will execute
	// quickly, but if not, this will cause a 2-second delay and
	// then the camera will be disabled and we'll continue on.
	double t1 = glfwGetTime();
	IplImage* cami = cvQueryFrame(m_capture);
	double t2 = glfwGetTime();
	cami = cvQueryFrame(m_capture);
	double t3 = glfwGetTime();
	if ( (t2-t1) > 0.75 && (t3-t2) > 0.75 ) {
		cvReleaseCapture(&m_capture);
		m_capture = NULL;
		DEBUGPRINT(("Getting Camera Frames is too slow!  Disabling camera! (times=%lf %lf %lf",t1,t2,t3));
		return FALSE;
	}

#define CV_CAP_PROP_FRAME_WIDTH    3
#define CV_CAP_PROP_FRAME_HEIGHT   4

    /* retrieve or set capture properties */
    double fwidth = cvGetCaptureProperty( m_capture, CV_CAP_PROP_FRAME_WIDTH );
    double fheight = cvGetCaptureProperty( m_capture, CV_CAP_PROP_FRAME_HEIGHT );

    camWidth = (int)fwidth;
    camHeight = (int)fheight;

    DEBUGPRINT(("CAMERA CV says width=%d height=%d\n",camWidth,camHeight));
	return TRUE;
}

FFGLPluginInstance*
FFFF::FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string inm)
{
	FFGLPluginInstance* np = new FFGLPluginInstance(plugin,inm);
	if ( np->InstantiateGL(&fboViewport)!=FF_SUCCESS ) {
		delete np;
		throw NosuchException("Unable to InstantiateGL !?");
	}
	return np;
}

void
FFFF::FFGLDeletePluginInstance(FFGLPluginInstance* p)
{
	DEBUGPRINT(("DELETING FFGLPluginInstance p=%ld",(long)p));
	p->DeInstantiateGL();
	delete p;
}

FF10PluginInstance*
FFFF::FF10NewPluginInstance(FF10PluginDef* plugdef, std::string inm)
{
	FF10PluginInstance* np = new FF10PluginInstance(plugdef,inm);

	const PluginInfoStruct *pi = plugdef->GetInfo();
	char nm[17];
	memcpy(nm, pi->PluginName, 16);
	nm[16] = 0;
	VideoInfoStruct vis = { ffWidth, ffHeight, FF_DEPTH_24, FF_ORIENTATION_TL };

	if ( np->Instantiate(&vis)!=FF_SUCCESS ) {
		delete np;
		throw NosuchException("Unable to Instantiate !?");
	}
	// np->setInstanceID(i);
	return np;
}

void
FFFF::FF10DeletePluginInstance(FF10PluginInstance* p)
{
	DEBUGPRINT(("DELETING FF10PluginInstance p=%ld",(long)p));
	// p->plugindef()->DeInstantiate(p->instanceid());
	p->DeInstantiate();
	delete p;
}

void
FFFF::clearPipeline()
{
	FF10PluginInstance* nextff10;
	for ( FF10PluginInstance* p=m_ff10pipeline; p!=NULL; p=nextff10 ) {
		nextff10 = p->next;
		FF10DeletePluginInstance(p);
    }
	m_ff10pipeline = NULL;

	FFGLPluginInstance* nextffgl;
	for ( FFGLPluginInstance* p=m_ffglpipeline; p!=NULL; p=nextffgl ) {
		nextffgl = p->next;
		FFGLDeletePluginInstance(p);
    }
	m_ffglpipeline = NULL;
}

void
FFFF::loadAllPluginDefs(std::string ff10path, std::string ffglpath, int ffgl_width, int ffgl_height)
{
    nff10plugindefs = 0;
    nffglplugindefs = 0;

    loadffglpath(ffglpath);
    if ( FFGLinit2(ffgl_width,ffgl_height) != 1 ) {
		DEBUGPRINT(("HEY!  FFGLinit2 failed!?"));
		return;
	}

    loadff10path(ff10path);

    DEBUGPRINT(("%d FF plugins, %d FFGL plugins\n",nff10plugindefs,nffglplugindefs));
}

FFGLPluginInstance*
FFFF::FFGLFindPluginInstance(std::string inm) {
	for ( FFGLPluginInstance* p=m_ffglpipeline; p!=NULL; p=p->next ) {
		if ( inm == p->name() ) {
			return p;
		}
	}
	return NULL;
}

FF10PluginInstance*
FFFF::FF10FindPluginInstance(std::string inm) {
	for ( FF10PluginInstance* p=m_ff10pipeline; p!=NULL; p=p->next ) {
		if ( inm == p->name() ) {
			return p;
		}
	}
	return NULL;
}

#if 0
// Return a new plugin instance name, making sure it doesn't match any existing ones
std::string
FFFF::newInstanceName() {
	int inum = 0;
	while ( true ) {
		std::string nm = NosuchSnprintf("p%d",inum++);
		FFGLPluginInstance* p=m_ffglpipeline;
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
#endif

FFGLPluginInstance*
FFFF::FFGLAddToPipeline(std::string pluginName, std::string inm, bool autoenable, cJSON* params) {

	// First scan existing pipeline to see if this instanceName is already used
	FFGLPluginInstance* last = NULL;
	for ( FFGLPluginInstance* p=m_ffglpipeline; p!=NULL; p=p->next ) {
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

	FFGLPluginInstance* np = FFGLNewPluginInstance(plugin,inm);

	// If the plugin's first parameter is "viztag", set it
	int pnum = np->plugindef()->getParamNum("viztag");
	if ( pnum >= 0 ) {
		DEBUGPRINT1(("In FFGLAddToPipeline of %s, setting viztag to %s",pluginName.c_str(),inm.c_str()));
		np->setparam("viztag",inm);
	}

	// Add it to the end of the pipeline
	if ( last == NULL ) {
		m_ffglpipeline = np;
	} else {
		last->next = np;
	}

	if ( autoenable ) {
		np->enable();
	}

	if (params) {
		for (cJSON* pn = params->child; pn != NULL; pn = pn->next) {
			NosuchAssert(pn->type == cJSON_Number);
			np->setparam(pn->string, (float)(pn->valuedouble));
		}
	}

	return np;
}

FF10PluginInstance*
FFFF::FF10AddToPipeline(std::string pluginName, std::string inm, bool autoenable, cJSON* params) {

	// First scan existing pipeline to see if this instanceName is already used
	FF10PluginInstance* last = NULL;
	for ( FF10PluginInstance* p=m_ff10pipeline; p!=NULL; p=p->next ) {
		if ( inm == p->name() ) {
			DEBUGPRINT(("Plugin instance named '%s' already exists",inm.c_str()));
			return NULL;
		}
		last = p;
	}

    FF10PluginDef* plugin = findff10plugin(pluginName);
	if ( plugin == NULL ) {
		DEBUGPRINT(("There is no plugin named '%s'",pluginName.c_str()));
		return NULL;
	}

	FF10PluginInstance* np = FF10NewPluginInstance(plugin,inm);

	// If the plugin's first parameter is "viztag", set it
	int pnum = np->plugindef()->getParamNum("viztag");
	if ( pnum >= 0 ) {
		DEBUGPRINT1(("In FF10AddToPipeline of %s, setting viztag to %s",pluginName.c_str(),inm.c_str()));
		np->setparam("viztag",inm);
	}

	// Add it to the end of the pipeline
	if ( last == NULL ) {
		m_ff10pipeline = np;
	} else {
		last->next = np;
	}

	if ( autoenable ) {
		np->enable();
	}

	if (params) {
		for (cJSON* pn = params->child; pn != NULL; pn = pn->next) {
			NosuchAssert(pn->type == cJSON_Number);
			np->setparam(pn->string, (float)(pn->valuedouble));
		}
	}

	return np;
}

void
FFFF::FFGLDeleteFromPipeline(std::string inm) {

	// First scan existing pipeline to see if this instanceName is already used
	FFGLPluginInstance* prev = NULL;
	FFGLPluginInstance* p = m_ffglpipeline;
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
		m_ffglpipeline = m_ffglpipeline->next;
	} else {
		prev->next = p->next;
	}
	FFGLDeletePluginInstance(p);
}

void
FFFF::FF10DeleteFromPipeline(std::string inm) {

	// First scan existing pipeline to see if this instanceName is already used
	FF10PluginInstance* prev = NULL;
	FF10PluginInstance* p = m_ff10pipeline;
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
		m_ff10pipeline = m_ff10pipeline->next;
	} else {
		prev->next = p->next;
	}
	FF10DeletePluginInstance(p);
}

void
FFFF::loadPipeline(std::string configname)
{
	std::string fname = VizConfigPath("ffff\\"+configname+".json");
	std::string err;

	cJSON* json = jsonReadFile(fname,err);
	if ( !json ) {
		// If an FFFF configfile doesn't exist, synthesize one
		// assuming that the configname is a vizlet name
		DEBUGPRINT(("Synthesizing FFFF config for %s",configname.c_str()));
		std::string jstr = NosuchSnprintf("{\"pipeline\": [ { \"plugin\": \"%s\", \"params\": { } } ] }",configname.c_str());
		json = cJSON_Parse(jstr.c_str());
		if ( !json ) {
			throw NosuchException("Unable to parse synthesized config!? jstr=%s",jstr.c_str());
		}
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
	for (int n = 0; n < nplugins; n++) {
		cJSON *p = cJSON_GetArrayItem(plugins, n);
		NosuchAssert(p);
		if (p->type != cJSON_Object) {
			throw NosuchException("Hey! Item #%d in pipeline isn't an object?", n);
		}

		std::string plugintype = "ffgl";

		// We expect either ffglplugin or ff10plugin, but allow
		// plugin as an alias for ffglplugin.
		cJSON* plugin = jsonGetString(p, "plugin");
		if (!plugin) {
			plugin = jsonGetString(p, "ffglplugin");
		}
		if (!plugin) {
			plugin = jsonGetString(p, "ff10plugin");
			if (plugin) {
				plugintype = "ff10";
			}
		}

		if (plugin == NULL) {
			throw NosuchException("Hey! Item #%d in pipeline doesn't specify plugin?", n);
		}

		// If an explicit viztag isn't given, use plugin name
		const char* name = plugin->valuestring;
		std::string viztag = jsonNeedString(p, "viztag", name);
		cJSON* enabled = jsonGetNumber(p, "enabled");  // optional, default is 1 (true)
		cJSON* params = jsonGetObject(p, "params");
		NosuchAssert(plugin && params);

		// Default is to enable the plugin as soon as we added it, but you
		// can add an "enabled" value to change that.
		bool autoenable = (enabled == NULL || enabled->valuedouble != 0.0);

		// XXX - someday there should be a base class for both
		// FFGL and FF10 plugins
		if (plugintype == "ffgl") {
			FFGLPluginInstance* pi = FFGLAddToPipeline(name, viztag, autoenable, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}
		}
		else {  // "ff10"
			FF10PluginInstance* pi = FF10AddToPipeline(name, viztag, autoenable, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}

		}
		DEBUGPRINT(("Pipeline, loaded plugin=%s viztag=%s", name, viztag.c_str()));
	}
}

void
doCameraFrame(IplImage* frame)
{
}

void
do_ff10pipeline(FF10PluginInstance* pipeline, unsigned char* pixels) {
	FF10PluginInstance* p;
	for ( p = pipeline; p!=NULL; p=p->next ) {
		if (p->isEnabled()) {
			p->plugindef()->Process(p->instanceid(), pixels);
		}
	}
}

bool
FFFF::doOneFrame(bool use_camera, int window_width, int window_height)
{
	int interp;
    IplImage* img1;
    IplImage* img2;
	IplImage* camframe = NULL;
    unsigned char *pixels_into_pipeline;
    IplImage* img_into_pipeline;

    //bind the gl texture so we can upload the next video frame
    glBindTexture(GL_TEXTURE_2D, mapTexture.Handle);

	if ( use_camera ) {
		camframe = getCameraFrame();
	}

	if ( camframe != NULL ) {
	    unsigned char* pixels = (unsigned char *)(camframe->imageData);
	
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
	    if (resizedpixels != 0) {
			do_ff10pipeline(m_ff10pipeline,resizedpixels);
	    }

	    cvResize(img2, img_into_pipeline, interp);
	    cvGetImageRawData( img_into_pipeline, &pixels_into_pipeline, NULL, NULL );

	} else {
	    img_into_pipeline = cvCreateImage(cvSize(fboWidth,fboHeight), IPL_DEPTH_8U, 3);

	    pixels_into_pipeline = (unsigned char *)(img_into_pipeline->imageData);
		cvZero(img_into_pipeline);
	
		do_ff10pipeline(m_ff10pipeline,pixels_into_pipeline);
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

	for ( pi = m_ffglpipeline; pi!=NULL; pi=pi->next ) {
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

	for ( pi = m_ffglpipeline; pi!=NULL; pi=pi->next ) {
		if ( ! pi->isEnabled() ) {
			continue;
		}
#if 0
		if ( nactive == 1 ) {
			if ( ! do_ffgl_plugin(pi,3) ) {
				DEBUGPRINT(("DISABLING plugin - Error B in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
				pi->disable();
			}
			continue;
		}
#endif
		if ( pi == firstplugin ) {
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

	// The Trails plugin is always the last one

	//deactivate rendering to the fbo
	//(this re-activates rendering to the window)
	if ( nactive > 0 ) {
		fbo_output->UnbindAsRenderTarget(glExtensions);
	}
	if ( ! do_ffgl_plugin(m_trailsplugin,2) ) {
		DEBUGPRINT(("Error C in do_ffgl_plugin for Trails plugin!?"));
	}

	static GLubyte *data = NULL;

	extern char* SaveFrames;
	extern FILE* Ffmpeg;

	// Now read the pixels back and save them
	if (SaveFrames && *SaveFrames!='\0') {
		static int filenum = 0;
		std::string path = VizPath("recording") + NosuchSnprintf("/%s%06d.ppm", SaveFrames, filenum++);
		if (data == NULL) {
			data = (GLubyte*)malloc(4 * window_width*window_height);  // 4, should work for both 3 and 4 bytes
		}
		glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
		IplImage* img = cvCreateImageHeader(cvSize(window_width, window_height), IPL_DEPTH_8U, 3);
		img->origin = 1;  // ???
		img->imageData = (char*)data;
		if (!cvSaveImage(path.c_str(), img)) {
			DEBUGPRINT(("Unable to save image: %s", path.c_str()));
		}
		// free(data);
		img->imageData = NULL;
		cvReleaseImageHeader(&img);
	}
	if (Ffmpeg) {
		if (data == NULL) {
			data = (GLubyte*)malloc(4 * window_width*window_height);  // 4, should work for both 3 and 4 bytes
		}
		// glReadPixels(0, 0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE, data);

		// glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_3_BYTES, data);
		glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

		// glReadPixels(0, 0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		// glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
		fwrite(data, window_width*window_height, 3, Ffmpeg);
	}

	if ( camframe != NULL ) {
		cvReleaseImageHeader(&img1);
		cvReleaseImage(&img2);
		cvReleaseImage(&img_into_pipeline);
	} else {
		cvReleaseImage(&img_into_pipeline);
	}

	return true;
}

std::string FFFF::FF10List() {
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<nff10plugindefs; n++ ) {
		r += (sep + "\"" + ff10plugindefs[n]->name + "\"");
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

std::string FFFF::FFGLPipelineList(bool only_enabled) {
	std::string r = "[";
	std::string sep = "";
	for ( FFGLPluginInstance* p=m_ffglpipeline; p!=NULL; p=p->next ) {
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

std::string FFFF::FF10PipelineList(bool only_enabled) {
	std::string r = "[";
	std::string sep = "";
	for ( FF10PluginInstance* p=m_ff10pipeline; p!=NULL; p=p->next ) {
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

std::string FFFF::FF10ParamList(std::string plugin, const char* id) {
    FF10PluginDef* p = findff10plugin(plugin);
	if ( !p ) {
		return jsonError(-32000,"No plugin by that name",id);
	}
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<p->m_numparams; n++ ) {
		FF10ParameterDef* ps = &(p->m_paramdefs[n]);
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

std::string FFFF::FFGLParamList(std::string pluginx, const char* id) {
    FFGLPluginDef* p = findffglplugin(pluginx);
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

std::string FFFF::FF10ParamVals(FF10PluginInstance* pi, const char* id) {
	FF10PluginDef* p = pi->plugindef();
	int numparams = p->m_numparams;
	std::string r = "[";
	std::string sep = "";
    for ( int n=0; n<p->m_numparams; n++ ) {
		FF10ParameterDef* pd = &(p->m_paramdefs[n]);
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
FFFF::FFGLNeedPluginInstance(std::string name)
{
		FFGLPluginInstance* p = FFGLFindPluginInstance(name);
		if ( p == NULL ) {
			throw NosuchException("There is no plugin instance named '%s'",name.c_str());
		}
		return p;
}

FF10PluginInstance*
FFFF::FF10NeedPluginInstance(std::string name)
{
		FF10PluginInstance* p = FF10FindPluginInstance(name);
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
		return jsonError(-32000,e,id);
	}
	return jsonOK(id);
}

std::string FFFF::loadFfffPatch(std::string nm, const char* id)
{
	std::string err;
	std::string fname = VizConfigPath("ffff\\"+nm);
	DEBUGPRINT(("loadPatch nm=%s fname=%s",nm.c_str(),fname.c_str()));
	cJSON* j = jsonReadFile(fname,err);
	if ( ! j ) {
		throw NosuchException("Unable to open file (name=%s, err=%s)",fname.c_str(),err.c_str());
	}
	loadPipelineJson(j);
	jsonFree(j);
	return jsonOK(id);
}

void
FFFF::setTrailParams()
{
	if (m_trailsplugin) {
		if (m_dotrail) {
			m_trailsplugin->setparam("Opacity", (float)m_trailamount);
		}
		else {
			m_trailsplugin->setparam("Opacity", 0.0);
		}
	}
}

std::string FFFF::executeJson(std::string meth, cJSON *params, const char* id)
{
	std::string err;

	if (meth == "apis") {
		return jsonStringResult("time;clicknow;show;hide;echo;"
			"enable(instance);disable(instance);delete(instance);"
			"ffgladd(plugin,instance,autoenable,params);"
			"ff10add(plugin,instance,autoenable,params);"
			"ffglparamset(instance,param,val);"
			"ff10paramset(instance,param,val);"
			"ffglparamget(instance,param);"
			"ff10paramget(instance,param);"
			"ff10paramlist(plugin);"
			"ffglparamlist(plugin);"
			"ff10paramvals(instance);"
			"ffglparamvals(instance);"
			"ffglpipeline;"
			"ff10pipeline;"
			"ffglplugins;"
			"ff10plugins;"
			"dotrail(onoff);"
			"trail(amount);"
			, id);
	}
	if (meth == "dotrail") {
		m_dotrail = (jsonNeedInt(params, "onoff") != 0);
		setTrailParams();
		return jsonOK(id);
	}
	if (meth == "trail") {
		m_trailamount = jsonNeedDouble(params, "amount");
		setTrailParams();
		return jsonOK(id);
	}

	if ( meth == "time"  ) {
		return jsonDoubleResult(m_vizserver->GetTime(),id);
	}
	if ( meth == "clicknow"  ) {
		return jsonIntResult(m_vizserver->SchedulerCurrentClick(),id);
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
	if ( meth == "add" || meth == "ffgladd" ) {
		std::string plugin = jsonNeedString(params,"plugin");
		std::string iname = jsonNeedString(params,"instance");
		bool autoenable = (jsonNeedInt(params,"autoenable",1) != 0);
		cJSON* pparams = jsonGetObject(params, "params");
		FFGLPluginInstance* pi = FFGLAddToPipeline(plugin,iname,autoenable,pparams);
		if ( ! pi ) {
			throw NosuchException("Unable to add plugin '%s'",plugin.c_str());
		}
		return jsonStringResult(iname,id);
	}
	if ( meth == "ff10add" ) {
		std::string plugin = jsonNeedString(params,"plugin");
		std::string iname = jsonNeedString(params,"instance");
		bool autoenable = (jsonNeedInt(params,"autoenable",1) != 0);
		cJSON* pparams = jsonGetObject(params, "params");
		FF10PluginInstance* pi = FF10AddToPipeline(plugin,iname,autoenable,pparams);
		if ( ! pi ) {
			throw NosuchException("Unable to add plugin '%s'",plugin.c_str());
		}
		return jsonStringResult(iname,id);
	}
	if ( meth == "enable" || meth == "disable" ) {
		std::string instance = jsonNeedString(params,"instance");
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);   // throws an exception if it doesn't exist
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
		FFGLDeleteFromPipeline(iname);
		return jsonOK(id);
	}
	if ( meth == "clearpipeline" ) {
		clearPipeline();
		return jsonOK(id);
	}
	if ( meth == "ffglparamset" || meth == "set" ) {
		std::string instance = jsonNeedString(params,"instance");
		std::string param = jsonNeedString(params,"param");
		float val = (float) jsonNeedDouble(params,"val");
		DEBUGPRINT1(("ffglparamset %s %s %f", instance.c_str(), param.c_str(), val));
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);
		if ( ! pi->setparam(param,val) ) {
			throw NosuchException("Unable to find or set parameter '%s' in instance '%s'",param.c_str(),instance.c_str());
		}
		return jsonOK(id);
	}
	if ( meth == "ffglparamget" || meth == "get" ) {
		std::string instance = jsonNeedString(params,"instance");
		std::string param = jsonNeedString(params,"param");
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);
		FFGLParameterDef* pd = pi->plugindef()->findparamdef(param);
		if ( pd == NULL ) {
			throw NosuchException("No parameter '%s' in plugin '%s'",param.c_str(),instance.c_str());
		}
		return pi->getParamJsonResult(pd, pi, id);
	}
	if ( meth == "ff10paramset" ) {
		std::string instance = jsonNeedString(params,"instance");
		std::string param = jsonNeedString(params,"param");
		float val = (float) jsonNeedDouble(params,"val");
		FF10PluginInstance* pi = FF10NeedPluginInstance(instance);
		if ( ! pi->setparam(param,val) ) {
			throw NosuchException("Unable to find or set parameter '%s' in instance '%s'",param.c_str(),instance.c_str());
		}
		return jsonOK(id);
	}
	if ( meth == "ff10paramget" ) {
		std::string instance = jsonNeedString(params,"instance");
		std::string param = jsonNeedString(params,"param");
		FF10PluginInstance* pi = FF10NeedPluginInstance(instance);
		FF10ParameterDef* pd = pi->plugindef()->findparamdef(param);
		if ( pd == NULL ) {
			throw NosuchException("No parameter '%s' in plugin '%s'",param.c_str(),instance.c_str());
		}
		return pi->getParamJsonResult(pd, pi, id);
	}
	if ( meth == "ff10plugins" ) {
		return jsonResult(FF10List().c_str(),id);
	}
	if ( meth == "ffglplugins" ) {
		return jsonResult(FFGLList().c_str(),id);
	}
	if ( meth == "ffglpipeline" ) {
		bool only_enabled = (jsonNeedInt(params,"enabled",0) != 0);  // default is to list everything
		return jsonResult(FFGLPipelineList(only_enabled).c_str(),id);
	}
	if ( meth == "ff10pipeline" ) {
		bool only_enabled = (jsonNeedInt(params,"enabled",0) != 0);  // default is to list everything
		return jsonResult(FF10PipelineList(only_enabled).c_str(),id);
	}
	if ( meth == "ffglparamlist" ) {
		return FFGLParamList(jsonNeedString(params,"plugin"),id);
	}
	if ( meth == "ff10paramlist" ) {
		return FF10ParamList(jsonNeedString(params,"plugin"),id);
	}
#if 0
	if ( meth == "ffglparamlist" ) {
		std::string plugin = jsonNeedString(params,"plugin","");
		std::string instance = jsonNeedString(params,"instance","");
		if ( plugin == "" && instance != "" ) {
			FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);   // throws an exception if it doesn't exist
			plugin = pi->plugindef()->GetPluginName();
		}
		return FFGLParamList(plugin,id);
	}
	if ( meth == "ff10paramlist" ) {
		std::string plugin = jsonNeedString(params,"plugin","");
		std::string instance = jsonNeedString(params,"instance","");
		if ( plugin == "" && instance != "" ) {
			FF10PluginInstance* pi = FF10NeedPluginInstance(instance);   // throws an exception if it doesn't exist
			plugin = pi->plugindef()->GetPluginName();
		}
		return FF10ParamList(plugin,id);
	}
#endif
	if ( meth == "ffglparamvals" ) {
		std::string instance = jsonNeedString(params,"instance","");
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);   // throws an exception if it doesn't exist
		return FFGLParamVals(pi,id);
	}
	if ( meth == "ff10paramvals" ) {
		std::string instance = jsonNeedString(params,"instance","");
		FF10PluginInstance* pi = FF10NeedPluginInstance(instance);   // throws an exception if it doesn't exist
		return FF10ParamVals(pi,id);
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

