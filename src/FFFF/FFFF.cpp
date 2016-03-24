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
#include "AudioHost.h"

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

void FFFF_error(void* data,const char* msg) {

	// NEEDS to be static, since we return the c_str() of it.
	// This also assumes that JSON API calls are serialized.
	static std::string result;

	FFFF* ffff = (FFFF*)data;
	ffff->ErrorPopup(msg);
}

FFFF::FFFF(cJSON* config) {

	m_output_framedata = NULL;
	m_output_lastwrite = 0.0;
	m_output_framenum = 0;
	m_spoutsender = NULL;

	m_img1 = NULL;
	m_img2 = NULL;
	m_img_into_pipeline = NULL;
	m_record = false;
	m_capture = NULL;
	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		m_ffglpipeline[pipenum].m_name = "";
		m_ffglpipeline[pipenum].clear();
		m_ff10pipeline[pipenum].clear();
	}
	hidden = false;

	m_window_width = jsonNeedInt(config, "window_width", 800);
	m_window_height = jsonNeedInt(config, "window_height", 600);
	m_showfps = jsonNeedInt(config, "showfps", 0) ? true : false;
	m_audiohost_type = jsonNeedString(config, "audiohost_type", "");
	m_desired_FPS = jsonNeedInt(config, "fps", 30);
	m_spout = jsonNeedBool(config, "spout", true);
	if (m_audiohost_type != "") {
		m_audiohost = new AudioHost(m_audiohost_type, jsonNeedJSON(config, "audiohost_config", NULL));
	}
	else {
		m_audiohost = NULL;
	}
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
	// m_desired_FPS = 60.0;
	// m_desired_FPS = 120.0;
	m_throttle_timePerFrame = 1.0 / m_desired_FPS;
	m_throttle_lasttime = 0.0;

	m_fps_accumulator = 0;
	m_fps_lasttime = -1.0;

}

void
FFFF::InsertKeystroke(int key,int updown) {
	m_vizserver->InsertKeystroke(key,updown);
}

bool
FFFF::StartStuff() {

	m_vizserver = VizServer::GetServer();

	m_vizserver->SetErrorCallback(FFFF_error,this);

	if (!m_vizserver->Start()) {
		NosuchErrorOutput("Unable to start VizServer!?");
		return false;
	}

	m_vizserver->AddJsonCallback((void*)this,"ffff",FFFF_json,(void*)this);

	if (m_audiohost) {
	 	m_audiohost->Start();
	}

	return true;
}


void
FFFF::StopStuff() {

	m_vizserver->Stop();
	VizServer::DeleteServer();

	if (FfffOutputFile) {
		fclose(FfffOutputFile);
		FfffOutputFile = NULL;
	}
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
	DEBUGPRINT(("CAMERA detail FPS=%f wid=%f hgt=%f msec=%f ratio=%f fourcc=%f",
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
FFGLPipeline::FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string viztag)
{
	FFGLPluginInstance* np = new FFGLPluginInstance(plugin,viztag);
	// DEBUGPRINT(("----- MALLOC new FFGLPluginInstance"));
	if ( np->InstantiateGL(&fboViewport)!=FF_SUCCESS ) {
		delete np;
		throw NosuchException("Unable to InstantiateGL !?");
	}
	return np;
}

FF10PluginInstance*
FFFF::FF10NewPluginInstance(FF10PluginDef* plugdef, std::string viztag)
{
	FF10PluginInstance* np = new FF10PluginInstance(plugdef,viztag);
	// DEBUGPRINT(("--- MALLOC new FF10PluginInstance = %d",(long)np));

	const PluginInfoStruct *pi = plugdef->GetInfo();
	char nm[17];
	memcpy(nm, pi->PluginName, 16);
	nm[16] = 0;
	VideoInfoStruct vis = { ffWidth, ffHeight, FF_DEPTH_24, FF_ORIENTATION_TL };

	if ( np->Instantiate(&vis)!=FF_SUCCESS ) {
		delete np;
		throw NosuchException("Unable to Instantiate !?");
	}
	return np;
}

void
FFFF::clearPipeline(int pipenum)
{
	// Are we locked, here?

	m_ffglpipeline[pipenum].m_name = "";

	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline[pipenum].begin(); it != m_ff10pipeline[pipenum].end(); it++) {
		 DEBUGPRINT1(("--- deleting FF10PluginInstance *it=%ld", (long)(*it)));
		delete *it;
	}

	m_ff10pipeline[pipenum].clear();
	m_ffglpipeline[pipenum].clear();

	// Forget all the callbacks

	// m_vizserver->ClearJsonCallbacks();
	m_vizserver->ClearJsonCallbacksOfPipeline(pipenum);

	m_vizserver->AddJsonCallback((void*)this,"ffff",FFFF_json,(void*)this);
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
FFFF::FFGLFindPluginInstance(int pipenum, std::string viztag) {
	return m_ffglpipeline[pipenum].find_plugin(viztag);
}

FF10PluginInstance*
FFFF::FF10FindPluginInstance(int pipenum, std::string viztag) {
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline[pipenum].begin(); it != m_ff10pipeline[pipenum].end(); it++) {
		if ( viztag == (*it)->viztag() ) {
			return *it;
		}
	}
	return NULL;
}

#if 0
// Return a new plugin viztag name, making sure it doesn't match any existing ones
std::string
FFFF::newInstanceName() {
	int inum = 0;
	while ( true ) {
		std::string nm = NosuchSnprintf("p%d",inum++);
		FFGLPluginInstance* p=m_ffglpipeline;
		for ( ; p!=NULL; p=p->next ) {
			if ( nm == p->viztag() ) {
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
FFFF::FFGLAddToPipeline(int pipenum, std::string pluginName, std::string viztag, bool autoenable, cJSON* params) {

	// See if this viztag is already used in the pipeline
	if (FFGLFindPluginInstance(pipenum,viztag) != NULL) {
		DEBUGPRINT(("Plugin with viztag '%s' already exists",viztag.c_str()));
		return NULL;
	}

    FFGLPluginDef* plugindef = findffglplugindef(pluginName);
	if ( plugindef == NULL ) {
		throw NosuchException("There is no plugin named '%s'", pluginName.c_str());
		// DEBUGPRINT(("There is no plugin named '%s'",pluginName.c_str()));
		// return NULL;
	}

	FFGLPipeline& pipeline = m_ffglpipeline[pipenum];

	FFGLPluginInstance* np = pipeline.FFGLNewPluginInstance(plugindef,viztag);

	// If the plugin's first parameter is "viztag", set it
	int pnum = np->plugindef()->getParamNum("viztag");
	if ( pnum >= 0 ) {
		np->setparam("viztag",viztag);
	}

	// Add it to the end of the pipeline
	pipeline.append_plugin(np);

	if (params) {
		for (cJSON* pn = params->child; pn != NULL; pn = pn->next) {
			NosuchAssert(pn->type == cJSON_Object);
			std::string nm = jsonNeedString(pn, "name", "");
			if (nm == "") {
				throw NosuchException("Missing parameter name");
			}
			cJSON *t = cJSON_GetObjectItem(pn,"value");
			if (t == NULL) {
				throw NosuchException("Missing parameter value");
			}
			if (t->type == cJSON_String ) {
				// In the saved pipeline json, it saves a "vtag" value which
				// is the viztag without the pipeline number prefix.
				if (nm == "vtag" || nm == "viztag") {
					// np->setparam("viztag", viztag);
				}
				else {
					np->setparam(nm, t->valuestring);
				}
			}
			else if (t->type == cJSON_Number) {
				np->setparam(nm, (float)(t->valuedouble));
			}
			else {
				throw NosuchException("FFGLAddToPipeline unable to handle type=%d",t->type);
			}
		}
	}

	if ( autoenable ) {
		np->setEnable(true);
	}

	return np;
}

FF10PluginInstance*
FFFF::FF10AddToPipeline(int pipenum, std::string pluginName, std::string viztag, bool autoenable, cJSON* params) {

	// First scan existing pipeline to see if this viztag is already used
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline[pipenum].begin(); it != m_ff10pipeline[pipenum].end(); it++) {
		if ( viztag == (*it)->viztag() ) {
			DEBUGPRINT(("Plugin with viztag '%s' already exists",viztag.c_str()));
			return NULL;
		}
	}

    FF10PluginDef* plugin = findff10plugindef(pluginName);
	if ( plugin == NULL ) {
		DEBUGPRINT(("There is no plugin named '%s'",pluginName.c_str()));
		return NULL;
	}

	FF10PluginInstance* np = FF10NewPluginInstance(plugin,viztag);

	// If the plugin's first parameter is "viztag", set it
	int pnum = np->plugindef()->getParamNum("viztag");
	if ( pnum >= 0 ) {
		DEBUGPRINT1(("In FF10AddToPipeline of %s, setting viztag to %s",pluginName.c_str(),viztag.c_str()));
		np->setparam("viztag",viztag);
	}

	m_ff10pipeline[pipenum].insert(m_ff10pipeline[pipenum].end(),np);

	if (params) {
		for (cJSON* pn = params->child; pn != NULL; pn = pn->next) {
			NosuchAssert(pn->type == cJSON_Object);
			std::string nm = jsonNeedString(pn, "name", "");
			if (nm == "") {
				throw NosuchException("Missing parameter name");
			}
			double v = jsonNeedDouble(pn, "value", -1.0);
			if (v < 0) {
				throw NosuchException("Missing parameter value");
			}
			np->setparam(nm, (float)v);
		}
	}

	if ( autoenable ) {
		np->enable();
	}

	return np;
}

void
FFFF::FF10DeleteFromPipeline(int pipenum, std::string viztag) {

	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline[pipenum].begin(); it != m_ff10pipeline[pipenum].end(); ) {
		if (viztag == (*it)->viztag()) {
			delete *it;
			it = m_ff10pipeline[pipenum].erase(it);
		}
		else {
			it++;
		}
	}
}

void FFFF::ErrorPopup(const char* msg) {
	MessageBoxA(NULL, msg, "FFFF", MB_OK);
}

void
FFFF::loadPipeline(int pipenum, std::string configname, bool synthesize, std::string sidrange)
{
	if (configname == "") {
		throw NosuchException("configname is blank!?");
	}
	if (!NosuchEndsWith(configname, ".json")) {
		configname += ".json";
	}
	std::string fname = VizConfigPath("pipelines",configname);
	std::string err;

	DEBUGPRINT(("loadPipeline configname=%s sidrange=%s fname=%s",
		configname.c_str(), sidrange.c_str(), fname.c_str()));

	bool exists = NosuchFileExists(fname);
	cJSON* json;
	if (!exists) {
		if (!synthesize) {
			throw NosuchException("No such file: fname=%s", fname.c_str());
		}
		// If the configfile doesn't exist, synthesize one.
		// This assumes that the configname is a vizlet name.
		// First take off the trailing .json
		if (NosuchEndsWith(configname, ".json")) {
			int pos = configname.length() - 5;
			configname = configname.substr(0, pos);
		}
		DEBUGPRINT(("Synthesizing FFFF pipeline for %s", configname.c_str(), configname.c_str()));
		std::string jstr = NosuchSnprintf("{\"pipeline\": [ { \"plugin\": \"%s\", \"params\": [ ] } ] }", configname.c_str());
		json = cJSON_Parse(jstr.c_str());
		if (!json) {
			throw NosuchException("Unable to parse synthesized config!? jstr=%s", jstr.c_str());
		}
	} else {
		json = jsonReadFile(fname,err);
		if (!json) {
			std::string err = NosuchSnprintf("Unable to parse file!? fname=%s", fname.c_str());
			throw NosuchException(err.c_str());
		}
	}
	loadPipelineJson(pipenum,configname,json);
	jsonFree(json);

	m_ffglpipeline[pipenum].setSidrange(sidrange);
}

void
FFFF::loadPipelineJson(int pipenum, std::string name, cJSON* json) {

	clearPipeline(pipenum);

	m_ffglpipeline[pipenum].m_name = name;

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

		// If an explicit vtag (note, not viztag) isn't given, use plugin name
		const char* name = plugin->valuestring;
		std::string vtag = NosuchToLower(jsonNeedString(p, "vtag", name));
		std::string viztag = NosuchSnprintf("%d:%s", pipenum, vtag.c_str());

		bool enabled = jsonNeedBool(p, "enabled", true);  // optional, default is 1 (true)
		bool moveable = jsonNeedBool(p, "moveable", true);  // optional, default is 1 (true)
		cJSON* params = jsonGetArray(p, "params");  // optional, params can be NULL

		// Default is to enable the plugin as soon as we added it, but you
		// can add an "enabled" value to change that.

		// XXX - someday there should be a base class for both
		// FFGL and FF10 plugins
		if (plugintype == "ffgl") {
			FFGLPluginInstance* pi = FFGLAddToPipeline(pipenum, name, viztag, enabled, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}
			pi->setMoveable(moveable);
		}
		else {  // "ff10"
			FF10PluginInstance* pi = FF10AddToPipeline(pipenum, name, viztag, enabled, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}

		}
		DEBUGPRINT1(("Pipeline, loaded plugin=%s viztag=%s", name, viztag.c_str()));

		cJSON* vizletdump = jsonGetArray(p, "vizletdump");
		if (vizletdump != NULL) {
			int nvals = cJSON_GetArraySize(vizletdump);
			for (int n = 0; n < nvals; n++) {
				cJSON *p = cJSON_GetArrayItem(vizletdump, n);
				NosuchAssert(p);
				if (p->type != cJSON_Object) {
					DEBUGPRINT(("non-Object in vizletdump array!?"));
					continue;
				}
				std::string meth = jsonNeedString(p,"method","");
				cJSON* params = jsonGetObject(p, "params");
				if (!params) {
					throw NosuchException("No params value in vizletdump?");
				}
				DEBUGPRINT1(("Pipeline load meth=%s params=%s", meth.c_str(),cJSON_PrintUnformatted(params)));
				std::string fullmethod = NosuchSnprintf("%d:%s.%s",pipenum,name,meth.c_str());
				const char* s = m_vizserver->ProcessJson(fullmethod.c_str(), params, "12345");
			}
		}

	}
}

void
doCameraFrame(IplImage* frame)
{
}

void
do_ff10pipeline(std::vector<FF10PluginInstance*> pipeline, unsigned char* pixels) {
	for (std::vector<FF10PluginInstance*>::iterator it = pipeline.begin(); it != pipeline.end(); it++) {
		FF10PluginInstance* p = *it;
		if (p->isEnabled()) {
			p->plugindef()->Process(p->instanceid(), pixels);
		}
	}
}

bool
FFFF::doCameraAndFF10Pipeline(int pipenum, bool use_camera, GLuint texturehandle) {

	int interp;
	unsigned char *pixels_into_pipeline;
	IplImage* camframe = NULL;

	//bind the gl texture so we can upload the next video frame
	glBindTexture(GL_TEXTURE_2D, texturehandle);

	if (use_camera) {
		camframe = getCameraFrame();
	}

	CvSize fbosz = cvSize(fboWidth, fboHeight);

	if (camframe != NULL) {
		unsigned char* pixels = (unsigned char *)(camframe->imageData);

		CvSize camsz;
		CvSize ffsz;

		camsz = cvSize(camWidth, camHeight);
		ffsz = cvSize(ffWidth, ffHeight);

		if (m_img1 == NULL) {
			m_img1 = cvCreateImageHeader(camsz, IPL_DEPTH_8U, 3);
		}
		cvSetImageData(m_img1, pixels, camWidth * 3);

		if (m_img2 == NULL) {
			m_img2 = cvCreateImage(ffsz, IPL_DEPTH_8U, 3);
		}

		if (m_img_into_pipeline == NULL) {
			m_img_into_pipeline = cvCreateImage(fbosz, IPL_DEPTH_8U, 3);
			if (m_img_into_pipeline == NULL) {
				DEBUGPRINT(("UNABLE TO CREATE m_img_into_pipeline!???"));
				return false;
			}
			// DEBUGPRINT(("----- MALLOC m_img_into_pipeline = %d", (long)m_img_into_pipeline));
		}


		interp = CV_INTER_LINEAR;  // or CV_INTER_NN
		interp = CV_INTER_NN;  // This produces some artifacts compared to CV_INTER_LINEAR, but is faster

		// XXX - If the camera size is the same as the ff (ffgl) size,
		// we shouldn't have to do this resize
		cvResize(m_img1, m_img2, interp);

		unsigned char *resizedpixels = NULL;
		cvGetImageRawData(m_img2, &resizedpixels, NULL, NULL);
		if (resizedpixels != 0) {
			do_ff10pipeline(m_ff10pipeline[pipenum], resizedpixels);
		}

		cvResize(m_img2, m_img_into_pipeline, interp);
		cvGetImageRawData(m_img_into_pipeline, &pixels_into_pipeline, NULL, NULL);

#ifdef NOMORERELEASE
		DEBUGPRINT(("---- RELEASE m_img1 %d", (long)m_img1));
		cvReleaseImageHeader(&m_img1);
		DEBUGPRINT(("---- RELEASE img2 %d", (long)img2));
		cvReleaseImage(&img2);
#endif
	}
	else {
		if (m_img_into_pipeline == NULL) {
			m_img_into_pipeline = cvCreateImage(fbosz, IPL_DEPTH_8U, 3);
			if (m_img_into_pipeline == NULL) {
				DEBUGPRINT(("UNABLE TO CREATE m_img_into_pipeline!???"));
				return false;
			}
		}

		pixels_into_pipeline = (unsigned char *)(m_img_into_pipeline->imageData);
		cvZero(m_img_into_pipeline);

		do_ff10pipeline(m_ff10pipeline[pipenum], pixels_into_pipeline);
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

	return true;
}

void
FFFF::doPipeline(int pipenum, int width, int height)
{
	m_ffglpipeline[pipenum].doPipeline(width, height);
}

void
FFGLPipeline::setSidrange(std::string sidrange) {
	int sidmin, sidmax;
	if (sscanf(sidrange.c_str(), "%d-%d", &sidmin, &sidmax) != 2) {
		throw NosuchException("Invalid format of sidrange: %s", sidrange.c_str());
	}
	m_sidmin = sidmin;
	m_sidmax = sidmax;

	// Now go through all the vizlets in the pipeline, and set the sidrange within them

	VizServer* viz = VizServer::GetServer();
	FFGLPluginList& ffglplugins = m_pluginlist;
	for (FFGLPluginList::iterator it = ffglplugins.begin(); it != ffglplugins.end(); it++) {
		FFGLPluginInstance* p = *it;
		bool isvizlet = viz->IsVizlet(p->viztag().c_str());
		if (!isvizlet) {
			continue;
		}

		// This code just blindly calls a set_sidrange method on every vizlet.

		std::string fullmethod = NosuchSnprintf("%s.set_sidrange",p->viztag().c_str());
		std::string jsonstr = NosuchSnprintf("{ \"sidrange\": \"%d-%d\" }", m_sidmin, m_sidmax);
		cJSON* params = cJSON_Parse(jsonstr.c_str());
		if (!params) {
			throw NosuchException("Internal error in parsing sidrange json!?");
		}
		std::string s = viz->ProcessJson(fullmethod.c_str(), params, "12345");
		DEBUGPRINT1(("result of set_sidrange=%s",s.c_str()));
		// XXX - should really check the result for success (or missing api), here.
		cJSON_free(params);
	}
}

void
FFGLPipeline::paintTexture() {

	FFGLTexCoords maxCoords = GetMaxGLTexCoords(m_texture);

	//bind the texture handle
	glBindTexture(GL_TEXTURE_2D, m_texture.Handle);
	
	glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_ONE, GL_ONE);

#ifdef XXX_SOMEDAY
	// glBlendEquation(GL_FUNC_ADD);
	glBlendEquation(GL_FUNC_SUBTRACT);
#endif
	
	glEnable(GL_TEXTURE_2D);
	
	// glColor4f(0.0,0.0,1.0,0.5);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glLineWidth((GLfloat)10.0f);
	
	glBegin(GL_QUADS);
	glTexCoord2d(0.0, maxCoords.t);
	glVertex3f(-1.0f, 1.0f, 0.0f);	// Top Left
	
	glTexCoord2d(maxCoords.s, maxCoords.t);
	glVertex3f(1.0f, 1.0f, 0.0f);	// Top Right
	
	glTexCoord2d(maxCoords.s, 0.0);
	glVertex3f(1.0f, -1.0f, 0.0f);	// Bottom Right
	
	glTexCoord2d(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 0.0f);	// Bottom Left
	
	glEnd();
	
	glDisable(GL_TEXTURE_2D);
}

void
FFFF::sendSpout(int width, int height) {

	if (m_spoutsender != NULL && spoutTexture.Handle != 0) {

		// Copy screen into spoutTexture
		glBindTexture(GL_TEXTURE_2D, spoutTexture.Handle);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Send it.
		bool b = m_spoutsender->SendTexture(spoutTexture.Handle, GL_TEXTURE_2D, width, height);
		if (!b) {
			DEBUGPRINT(("Error in spout SendTexture!?"));
		}
	}
}

void
FFGLPipeline::doPipeline(int window_width, int window_height)
{
	if ( ! m_pipeline_enabled ) {
		return;
	}
	FFGLPluginInstance* lastplugin = NULL;
	FFGLPluginInstance* firstplugin = NULL;
	int num_ffgl_active = 0;

	for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
		FFGLPluginInstance* p = *it;
		if (p->isEnabled()) {
			num_ffgl_active++;
			lastplugin = p;
			if (firstplugin == NULL) {
				firstplugin = p;
			}
		}
	}

	if (num_ffgl_active == 0 ) {
		if (!do_ffgl_plugin(NULL, 4)) {
			DEBUGPRINT(("Error A in do_ffgl_plugin"));
			// return 0;
		}
	}

	for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
		FFGLPluginInstance* pi = *it;
		if (!pi->isEnabled()) {
			continue;
		}
		if (num_ffgl_active == 1) {
			if ( ! do_ffgl_plugin(pi,3) ) {
				DEBUGPRINT(("DISABLING plugin - Error B in do_ffgl_plugin for plugin=%s",pi->viztag().c_str()));
				pi->setEnable(false);
			}
			continue;
		}
		if ( pi == firstplugin ) {
			if (!do_ffgl_plugin(pi, 0)) {
				DEBUGPRINT(("DISABLING plugin - Error D in do_ffgl_plugin for plugin=%s", pi->viztag().c_str()));
				pi->setEnable(false);
			}
		}
		else if (pi == lastplugin) {
			//deactivate rendering to the fbo
			//(this re-activates rendering to the window)
			fbo_output->UnbindAsRenderTarget(glExtensions);
			if (!do_ffgl_plugin(pi, 2)) {
				DEBUGPRINT(("DISABLING plugin - Error D in do_ffgl_plugin for plugin=%s", pi->viztag().c_str()));
				pi->setEnable(false);
			}
		} 
		else {
			if (!do_ffgl_plugin(pi, 1)) {
				DEBUGPRINT(("DISABLING plugin - Error E in do_ffgl_plugin for plugin=%s", pi->viztag().c_str()));
				pi->setEnable(false);
			}
		}
	}

#ifdef SAVE_THIS_MIGHT_BE_NEEDED_SOMEDAY
	// This old stuff that writes out individual frames is no longer needed,
	// but it might come in handy for something someday, so I'm not deleting it completely yet
	// Now read the pixels back and save them
	if (m_record && SaveFrames && *SaveFrames!='\0') {

		std::string path = VizPath("recordings") + NosuchSnprintf("/%s_%06d.ppm", SaveFrames, m_output_framenum);
		if (m_output_framedata == NULL) {
			m_output_framedata = (GLubyte*)malloc(4 * window_width*window_height);  // 4, should work for both 3 and 4 bytes
			// DEBUGPRINT(("---- MALLOC SaveFrames data %d", data));
		}
		// The glReadPixles changes it from 60fps (on a blank screen) to 45fps
		glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, m_output_framedata);
		IplImage* img = cvCreateImageHeader(cvSize(window_width, window_height), IPL_DEPTH_8U, 3);
		// DEBUGPRINT(("---- MALLOC SaveFrames img %d", (long)img));
		img->origin = 1;  // ???
		img->imageData = (char*)m_output_framedata;
		// The cvSaveImage (with glReadPixels) changes it from 60fps (on a blank screen) to 20fps
		// Perhaps async file writing will help
		if (!cvSaveImage(path.c_str(), img)) {
			DEBUGPRINT(("Unable to save image: %s", path.c_str()));
		}
		// free(data);  it's static, now
		img->imageData = NULL;
		DEBUGPRINT1(("---- RELEASE SaveFrames img %d", (long)img));
		cvReleaseImageHeader(&img);
	}
#endif

#ifdef SAVE_THIS_FOR_RECORDING_MIGHT_BE_NEEDED_SOMEDAY
	if (m_record && FfffOutputFile) {

		if (m_output_framedata == NULL) {
			m_output_framedata = (GLubyte*)malloc(4 * window_width*window_height);  // 4, should work for both 3 and 4 bytes
			// DEBUGPRINT(("---- MALLOC FfffOutputFile data %d", data));
		}

		glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, m_output_framedata);

		double tm = NosuchTimeElapsedInSeconds();
		double timePerFrame = 1.0 / FfffOutputFPS;
		// Pehaps this should be a loop, but handle first one, too
		if (m_output_framenum == 0) {
			// First frame of a recording
			fwrite(m_output_framedata, window_width*window_height, 3, FfffOutputFile);
			DEBUGPRINT1(("Writing frame %d  time=%lf",m_output_framenum,NosuchTimeElapsedInSeconds()));
			m_output_framenum++;
			m_output_lastwrite = tm;
		}
		else {
			int frameswritten = 0;
			while ((tm - m_output_lastwrite) > timePerFrame) {
				fwrite(m_output_framedata, window_width*window_height, 3, FfffOutputFile);
				frameswritten++;
				m_output_lastwrite += timePerFrame;
			}
			if (frameswritten > 0) {
				m_output_framenum += frameswritten;
				m_output_lastwrite = tm;
				DEBUGPRINT1(("Wrote %d frames, framenum is now %d  time=%lf", frameswritten, m_output_framenum, NosuchTimeElapsedInSeconds()));
			}
		}
	}
#endif

	// Copy screen into m_texture
	glBindTexture(GL_TEXTURE_2D, m_texture.Handle);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, window_width, window_height);
	glBindTexture(GL_TEXTURE_2D, 0);

	return;
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

std::string FFFF::FFGLPipelineList(int pipenum, bool only_enabled) {
	std::string r = "[";
	std::string sep = "";

	FFGLPluginList& ffglplugins = m_ffglpipeline[pipenum].m_pluginlist;


	for (FFGLPluginList::iterator it = ffglplugins.begin(); it != ffglplugins.end(); it++) {
		FFGLPluginInstance* p = *it;

		bool isvizlet = m_vizserver->IsVizlet(p->viztag().c_str());

		std::string isviz;
		if (isvizlet) {
			isviz = std::string(" \"vizlet\": ") + (isvizlet ? "1" : "0") + ", ";
		}
		bool enabled = p->isEnabled();
		if ( only_enabled==false || enabled==true ) {
			r += (sep + "{ \"plugin\":\""+p->plugindef()->GetPluginName()+"\","
				+ " \"viztag\":\"" + p->viztag() + "\", "
				+ isviz
				+ " \"moveable\": " + (p->isMoveable() ? "1" : "0") + ","
				+ " \"enabled\": " + (p->isEnabled() ? "1" : "0")
				+ "  }");
			sep = ", ";
		}
	}
	r = r + "]";
	return r;
}

std::string FFFF::FF10PipelineList(int pipenum, bool only_enabled) {
	std::string r = "[";
	std::string sep = "";
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline[pipenum].begin(); it != m_ff10pipeline[pipenum].end(); it++) {
		FF10PluginInstance* p = *it;
		bool enabled = p->isEnabled();
		if ( only_enabled==false || enabled==true ) {
			r += (sep + "{ \"plugin\":\""+p->plugindef()->GetPluginName()+"\", \"viztag\":\"" + p->viztag() + "\", "
				+ " \"moveable\": " + (p->isMoveable() ? "1" : "0") + ","
				+ " \"enabled\": " + (p->isEnabled()?"1":"0")
				+ "  }");
			sep = ", ";
		}
	}
	r = r + "]";
	return r;
}

std::string FFFF::FF10ParamList(std::string plugin, const char* id) {
    FF10PluginDef* p = findff10plugindef(plugin);
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
    FFGLPluginDef* p = findffglplugindef(pluginx);
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

std::string FFFF::FFGLParamVals(FFGLPluginInstance* pi, std::string linebreak = "") {
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
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":%f }",nm,v);
			break;
		case FF_TYPE_TEXT:
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"%s\", \"type\":\"text\" }",nm,pi->GetParameterDisplay(pd->num).c_str());
			break;
		default:
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"???\" }",nm);
			break;
		}
		sep = ", ";
	}
	r += linebreak;
	r += "]";
	return r;
}

std::string FFFF::FF10ParamVals(FF10PluginInstance* pi, std::string linebreak = "") {
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
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":%f }",nm,v);
			break;
		case FF_TYPE_TEXT:
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"%s\" }",nm,pi->GetParameterDisplay(pd->num).c_str());
			break;
		default:
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"???\" }",nm);
			break;
		}
		sep = ", ";
	}
	r += linebreak + "]";
	return r;
}

FFGLPluginDef*
needFFGLPlugin(std::string name)
{
		FFGLPluginDef* p = findffglplugindef(name);
		if ( p == NULL ) {
			throw NosuchSnprintf("No plugin named '%s'",name.c_str());
		}
		return p;
}

FFGLPluginInstance*
FFFF::FFGLNeedPluginInstance(int pipenum, std::string viztag)
{
		FFGLPluginInstance* p = FFGLFindPluginInstance(pipenum, viztag);
		if ( p == NULL ) {
			throw NosuchException("There is no viztag '%s' in pipeline %d",viztag.c_str(),pipenum);
		}
		return p;
}

FF10PluginInstance*
FFFF::FF10NeedPluginInstance(int pipenum, std::string viztag)
{
		FF10PluginInstance* p = FF10FindPluginInstance(pipenum, viztag);
		if ( p == NULL ) {
			throw NosuchException("There is no viztag '%s' in pipeline %d",viztag.c_str(),pipenum);
		}
		return p;
}

std::string FFFF::savePipeline(int pipenum, std::string fname, const char* id)
{
	if ( ! NosuchEndsWith(fname, ".json") ) {
		fname += ".json";
	}
	std::string fpath = VizConfigPath("pipelines",fname);

	DEBUGPRINT(("savePipeline fpath=%s",fpath.c_str()));
	std::string err;

	std::ofstream f;
	f.open(fpath.c_str(),std::ios::trunc);
	if ( ! f.is_open() ) {
		err = NosuchSnprintf("Unable to open file - %s",fpath.c_str());
		return false;
	}
	f << "{\n\"pipeline\": [\n";
	std::string sep = "";
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline[pipenum].begin(); it != m_ff10pipeline[pipenum].end(); it++) {
		f << sep;
		f << "\t{\n";
		f << "\t\t\"ff10plugin\": \"" + (*it)->viztag() + "\",\n";
		f << "\t\t\"enabled\": " + std::string((*it)->isEnabled()?"1":"0") + ",\n";
		f << "\t\t\"moveable\": " + std::string((*it)->isMoveable()?"1":"0") + ",\n";
		f << "\t\t\"params\": " + FF10ParamVals(*it,"\n\t\t\t") + "\n";
		f << "\t}";
		sep = ",\n";
	}
	FFGLPluginList& ffglplugins = m_ffglpipeline[pipenum].m_pluginlist;
	for (FFGLPluginList::iterator it = ffglplugins.begin(); it != ffglplugins.end(); it++) {
		FFGLPluginInstance* p = *it;
		f << sep;
		f << "\t{\n";
		f << "\t\t\"ffglplugin\": \"" + p->viztag() + "\",\n";
		f << "\t\t\"ffglplugin\": \"" + p->plugindef()->GetPluginName() + "\",\n";
		f << "\t\t\"enabled\": " + std::string(p->isEnabled()?"1":"0") + ",\n";
		f << "\t\t\"moveable\": " + std::string(p->isMoveable()?"1":"0") + ",\n";

		if (m_vizserver->IsVizlet(p->viztag().c_str())) {
			std::string name = p->viztag();
			f << "\t\t\"vizlet\": " + std::string(m_vizserver->IsVizlet(name.c_str()) ? "1" : "0") + ",\n";

			std::string fullmethod = name + "." + "dump";
			const char* s = m_vizserver->ProcessJson(fullmethod.c_str(), NULL, "12345");
			cJSON* json = cJSON_Parse(s);
			if (!json) {
				throw NosuchException("Unable to parse .dump json in savePipeline!?");
			}
			std::string result = jsonNeedString(json, "result","");
			// The result should be a big long json value.  We want to pretty-print it.
			cJSON* prettyj = cJSON_Parse(result.c_str());
			if (prettyj == NULL) {
				throw NosuchException("Bad format of json result!? result = %s", result.c_str());
			}
			const char* prettys = cJSON_Print(prettyj);
			f << "\t\t\"vizletdump\": " + NosuchReplaceAll(prettys,"\n","\n\t\t") + ",\n";
			cJSON_free((void*)prettys);
			jsonFree(json);
		}
		f << "\t\t\"params\": " + FFGLParamVals(p,"\n\t\t\t") + "\n";
		f << "\t}";
		sep = ",\n";
	}
	f << "\n\t]\n}\n";
	f.close();
	return jsonOK(id);
}

void FFFF::savePipeset(std::string fname)
{
	if (fname == "") {
		throw NosuchException("Pipeset name is blank!?");
	}
	if ( ! NosuchEndsWith(fname, ".json") ) {
		fname += ".json";
	}
	std::string fpath = VizConfigPath("pipesets",fname);

	DEBUGPRINT(("savePipeset fpath=%s",fpath.c_str()));
	std::string err;

	std::ofstream f;
	f.open(fpath.c_str(),std::ios::trunc);
	if ( ! f.is_open() ) {
		throw NosuchException("Unable to open file - %s",fpath.c_str());
	}
	f << "{\n\"pipeset\": [\n";
	std::string sep = "";
	for (int n = 0; n < NPIPELINES; n++) {
		FFGLPipeline& pipeline = m_ffglpipeline[n];
		f << sep;
		f << "\t{\n";
		f << "\t\t\"pipeline\": \"" << pipeline.m_name << "\",\n";
		f << "\t\t\"enabled\": " << pipeline.m_pipeline_enabled << ",\n";
		f << "\t\t\"sidrange\": \"" << pipeline.m_sidmin << "-" << pipeline.m_sidmax << "\"\n";
		f << "\t}";
		sep = ",\n";
	}
	f << "\n\t]\n}\n";
	f.close();
}

void
FFFF::loadPipeset(std::string pipesetname)
{
	if (pipesetname == "") {
		throw NosuchException("Pipeset name is blank!?");
	}
	if (!NosuchEndsWith(pipesetname, ".json")) {
		pipesetname += ".json";
	}
	std::string fname = VizConfigPath("pipesets",pipesetname);
	std::string err;

	DEBUGPRINT(("loadPipeset configname=%s fname=%s", pipesetname.c_str(), fname.c_str()));

	bool exists = NosuchFileExists(fname);
	cJSON* json;
	if (!exists) {
		throw NosuchException("No such file: fname=%s", fname.c_str());
	}

	json = jsonReadFile(fname,err);
	if (!json) {
		std::string err = NosuchSnprintf("Unable to parse file!? fname=%s", fname.c_str());
		throw NosuchException(err.c_str());
	}
	loadPipesetJson(json);
	m_pipeset_filename = pipesetname;
	jsonFree(json);
}

void
FFFF::loadPipesetJson(cJSON* json)
{

	// XXX - should perhaps really surround this with a try/except
	// and make sure the pipeset gets completely cleared if there's any problem loading it.

	cJSON* pipeset = jsonGetArray(json,"pipeset");
	if ( !pipeset ) {
		throw NosuchException("No 'pipeset' value in config");
	}

	int npipelines = cJSON_GetArraySize(pipeset);
	if (npipelines != NPIPELINES) {
		throw NosuchException("Number of pipelines in pipeset is %d - expected %d", npipelines, NPIPELINES);
	}

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {

		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];

		clearPipeline(pipenum);

		cJSON *p = cJSON_GetArrayItem(pipeset, pipenum);
		NosuchAssert(p);
		if (p->type != cJSON_Object) {
			throw NosuchException("Hey! Item #%d in pipeset isn't an object?", pipenum);
		}

		std::string name = jsonNeedString(p, "pipeline");
		bool enabled = jsonNeedBool(p, "enabled");
		std::string sidrange = jsonNeedString(p, "sidrange");

		pipeline.m_name = name;
		pipeline.m_pipeline_enabled = enabled;
		DEBUGPRINT(("Set pipenum=%d enabled to %d",pipenum,m_ffglpipeline[pipenum].m_pipeline_enabled));
		if (name != "") {
			loadPipeline(pipenum, name, true, sidrange);
		}
	}
}

void
FFFF::parseVizTag(std::string fulltag, int& pipenum, std::string& vtag)
{
	const char* p = fulltag.c_str();
	if (!(*p >= '0' && *p <= '9')) {
		throw NosuchException("Expected viztag to start with an integer: %s", fulltag.c_str());
	}
	if (fulltag.length() < 3 || *(p + 1) != ':') {
		throw NosuchException("Unexpected format of viztag: %s", fulltag.c_str());
	}
	pipenum = *p - '0';
	vtag = std::string(p + 2);
}

std::string FFFF::executeJson(std::string meth, cJSON *params, const char* id)
{
	std::string err;

	if (meth == "apis") {
		// XXX - this is out of date - need some better way of
		// XXX - maintaining this.
		return jsonStringResult("time;clicknow;show;hide;echo;"
			"enable(viztag,onoff);delete(viztag);"
			"ffgladd(plugin,viztag,autoenable,params);"
			"ff10add(plugin,viztag,autoenable,params);"
			"ffglparamset(viztag,param,val);"
			"ff10paramset(viztag,param,val);"
			"ffglparamget(viztag,param);"
			"ff10paramget(viztag,param);"
			"ff10paramlist(plugin);"
			"ffglparamlist(plugin);"
			"ff10paramvals(viztag);"
			"ffglparamvals(viztag);"
			"ffglpipeline;"
			"ff10pipeline;"
			"ffglplugins;"
			"ff10plugins;"
			"record(onoff);"
			"audio(onoff);"
			"moveup(viztag);"
			"movedown(viztag);"
			"shufflepipeline;"
			"randomizepipeline;"
			"set_fps(onoff);"
			, id);
	}
	// DEBUGPRINT(("FFFF api = %s", meth.c_str()));
#if 0
	if (meth == "fps") {
		m_showfps = jsonNeedBool(params, "onoff");
		return jsonOK(id);
	}
#endif
	if (meth == "audio") {
		bool onoff = jsonNeedBool(params, "onoff",true);
		if (m_audiohost == NULL) {
			throw NosuchException("No audio host!?");
		}
		if (!m_audiohost->Playing(onoff)) {
			throw NosuchException("Unable to control audio playback?!");
		}
		return jsonOK(id);
	}
	if (meth == "record") {
		bool onoff = jsonNeedBool(params, "onoff");
		if (m_audiohost) {
			if (!m_audiohost->Recording(onoff)) {
				throw NosuchException("Unable to %s audio recording?!",onoff?"start":"stop");
			}
			DEBUGPRINT(("Audio recording %s",onoff?"started":"stopped"));
		}
		else {
			DEBUGPRINT(("No audio host, only recording video frames"));
		}
		if (m_record == true && onoff == false) {
			// recording is being turned off
			if (FfffOutputFile == NULL) {
					DEBUGPRINT(("Video recording was already stopped"));
			}
			// If we've got an FfffOutputFile, close it
			if (FfffOutputFile) {
				fclose(FfffOutputFile);
				FfffOutputFile = NULL;
				DEBUGPRINT(("Video recording stopped"));
			}
			// Execute the "recordingfinished.bat" script which can do things like
			// rename or copy the files to another machine for encoding, uploading, etc.
			STARTUPINFOA info = { 0 };
			PROCESS_INFORMATION processInfo;

			ZeroMemory(&info, sizeof(info));
			info.cb = sizeof(info);
			ZeroMemory(&processInfo, sizeof(processInfo));

			info.dwFlags = STARTF_USESHOWWINDOW;
			info.wShowWindow = TRUE;

			std::string batpath = VizPath("bin\\recordingfinished.bat");
			std::string cmdexe = "c:\\windows\\system32\\cmd.exe";
			char* cmdline = _strdup(NosuchSnprintf("%s /c \"%s %s\"",cmdexe.c_str(),batpath.c_str(),FfffOutputPrefix.c_str()).c_str());
			// Second argument must be writable
			if (CreateProcessA(cmdexe.c_str(), cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
			{
				DEBUGPRINT(("Started: %s",batpath.c_str()));
				// We don't wait, we want it running separately
				// ::WaitForSingleObject(processInfo.hProcess, INFINITE);
				CloseHandle(processInfo.hProcess);
				CloseHandle(processInfo.hThread);
			}
			else {
				DEBUGPRINT(("Unable to execute: %s",batpath.c_str()));
			}
		}
		else if (m_record == false && onoff == true) {
			// recording is being turned on.  If we're writing output, open it
			if (FfffOutputPrefix == "") {
				throw NosuchException("Recording not turned on - there is no outputprefix value");
			}
			if ( FfffOutputFile ) {
				DEBUGPRINT(("Video recording was already started"));
			} else {
				m_output_framenum = 0;
				// raw video gets saved in this file
				std::string path = VizPath("recordings") + NosuchSnprintf("\\%s-%dx%d-%ld.rawvideo", FfffOutputPrefix.c_str(), m_window_width, m_window_height, time(NULL));
				FfffOutputFile = fopen(path.c_str(), "wb");
				if (FfffOutputFile == NULL) {
					NosuchErrorOutput("Unable to open output file: %s",path.c_str());
					DEBUGPRINT(("Unable to open output file: %s",path.c_str()));
				}
				DEBUGPRINT(("Video recording started, path=%s",path.c_str()));
			}
		}
		m_record = onoff;	// This will cause video frames to be recorded
		return jsonOK(id);
	}
	if (meth == "time") {
		return jsonDoubleResult(m_vizserver->SchedulerCurrentTimeInSeconds(), id);
	}
	if (meth == "clicknow") {
		return jsonIntResult(m_vizserver->SchedulerCurrentClick(), id);
	}
	if (meth == "show") {
		if (!window) {
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
	if (meth == "hide") {
		if (!window) {
			throw NosuchException("No window?!");
		}
		hidden = true;
		glfwIconifyWindow(window);
		return ok_json(id);
	}
	if (meth == "_echo" || meth == "echo") {
		std::string val = jsonNeedString(params, "value");
		return jsonStringResult(val, id);
	}
	if (meth == "name") {
		return jsonStringResult("FFFF", id);
	}

	{ static _CrtMemState s1, s2, s3;
	if (meth == "memdump") {
		_CrtMemState s;
		_CrtMemCheckpoint(&s);
		if (_CrtMemDifference(&s3, &s1, &s)) {
			_CrtMemDumpStatistics(&s3);
		}
		// _CrtDumpMemoryLeaks();
		return jsonOK(id);
	}
	if (meth == "memcheckpoint") {
		_CrtMemState s1;
		_CrtMemCheckpoint(&s1);
		return jsonOK(id);
	}
	}

	// Many of the methods below take pipenum, so grab it up front if it's there
	int pipenum = jsonNeedInt(params, "pipenum", -1);

	// viztags consist of a pipeline number and a unique (within the pipeline) vtag
	std::string viztag = jsonNeedString(params, "viztag","");

	// viztags are case-insensitive, always converted to lower case
	viztag = NosuchToLower(viztag);

	std::string vtag;
	if (viztag != "") {
		int pipenum2;
		parseVizTag(viztag, pipenum2, vtag);
		if (pipenum >= 0 && pipenum2 != pipenum) {
			throw NosuchException("Inconsistency between pipenum and viztag parameters!?");
		}
		pipenum = pipenum2;
	}

	// At this point, if pipenum is -1, it means it wasn't specified,
	// either in an explicit parameter or in a viztag value.
	// Any method that uses pipenum should make sure it's non-negative.

	FFGLPipeline* ppipeline = NULL;
	if (pipenum >= 0){
		ppipeline = &m_ffglpipeline[pipenum];
	}

	if (meth == "add" || meth == "ffgladd") {
		std::string plugin = jsonNeedString(params, "plugin");
		bool autoenable = jsonNeedBool(params, "autoenable", true);
		bool moveable = jsonNeedBool(params, "moveable", true);
		cJSON* pparams = jsonGetArray(params, "params");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pi = FFGLAddToPipeline(pipenum, plugin, viztag, autoenable, pparams);
		if (!pi) {
			throw NosuchException("Unable to add plugin '%s'", plugin.c_str());
		}
		pi->setMoveable(moveable);
		return jsonStringResult(viztag, id);
	}

	if (meth == "ff10add") {
		std::string plugin = jsonNeedString(params, "plugin");
		bool autoenable = jsonNeedBool(params, "autoenable", true);
		cJSON* pparams = jsonGetArray(params, "params");
		NosuchAssert(pipenum >= 0);
		FF10PluginInstance* pi = FF10AddToPipeline(pipenum, plugin, viztag, autoenable, pparams);
		if (!pi) {
			throw NosuchException("Unable to add plugin '%s'", plugin.c_str());
		}
		return jsonStringResult(viztag, id);
	}
	if (meth == "ffglenable" || meth == "enable" ) {
		// std::string viztag = jsonNeedString(params, "viztag");
		bool onoff = jsonNeedBool(params, "onoff");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(pipenum,viztag);   // throws an exception if it doesn't exist
		pi->setEnable(onoff);
		if (!onoff) {
			pi->ProcessDisconnect();
		} else {
			pi->ProcessConnect();
		}
		return jsonOK(id);
	}
	if (meth == "ffglmoveable" ) {
		// std::string viztag = jsonNeedString(params, "viztag");
		bool onoff = jsonNeedBool(params, "onoff");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(pipenum,viztag);   // throws an exception if it doesn't exist
		pi->setMoveable(onoff);
		return jsonOK(id);
	}
	if (meth == "about") {
		// std::string inst = jsonNeedString(params, "viztag");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pgl = FFGLFindPluginInstance(pipenum,viztag);
		if (pgl != NULL) {
			FFGLPluginDef* def = pgl->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->About, id);
		}
		FF10PluginInstance* p10 = FF10NeedPluginInstance(pipenum,viztag);
		if (p10 != NULL) {
			FF10PluginDef* def = p10->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->About, id);
		}
		throw NosuchException("No such viztag: %s",viztag.c_str());
	}
	if (meth == "description") {
		// std::string viztag = jsonNeedString(params, "viztag");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pgl = FFGLFindPluginInstance(pipenum,viztag);
		if (pgl != NULL) {
			FFGLPluginDef* def = pgl->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->Description, id);
		}
		FF10PluginInstance* p10 = FF10NeedPluginInstance(pipenum,viztag);
		if (p10 != NULL) {
			FF10PluginDef* def = p10->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->Description, id);
		}
		throw NosuchException("No such viztag: %s",viztag.c_str());
	}
	if ( meth == "delete" ) {
		// It's okay if it doesn't exist
		ppipeline->delete_plugin(viztag);
		return jsonOK(id);
	}
	if ( meth == "moveup" ) {
		// It's okay if it doesn't exist
		ppipeline->moveup(viztag);
		return jsonOK(id);
	}
	if ( meth == "movedown" ) {
		// It's okay if it doesn't exist
		ppipeline->movedown(viztag);
		return jsonOK(id);
	}
	if (meth == "shufflepipeline") {
		ppipeline->shuffle();
		return jsonOK(id);
	}
	if (meth == "randomizepipeline") {
		ppipeline->randomize();
		return jsonOK(id);
	}
	if ( meth == "clearpipeline" ) {
		NosuchAssert(pipenum >= 0);
		clearPipeline(pipenum);
		return jsonOK(id);
	}
	if (meth == "ffglparamset" || meth == "set") {

		// std::string viztag = jsonNeedString(params, "viztag");
		std::string param = jsonNeedString(params, "param");
		cJSON *jv = cJSON_GetObjectItem(params, "val");

		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(pipenum,viztag);
		if (pi == NULL) {
			throw NosuchException("Unable find find plugin with viztag='%s'", viztag.c_str());
		}
		if (jv == NULL) {
			throw NosuchException("Missing 'val' parameter in ffglparamset call");
		}
		if (jv->type == cJSON_Number) {
			float val = (float)jsonNeedDouble(params, "val");
			if (!pi->setparam(param, val)) {
				throw NosuchException("Unable to find or set parameter '%s' in viztag '%s'", param.c_str(), viztag.c_str());
			}
		}
		else if (jv->type == cJSON_String) {
			std::string val = jsonNeedString(params,"val");
			if ( ! pi->setparam(param,val) ) {
				throw NosuchException("Unable to find or set parameter '%s' in viztag '%s'",param.c_str(),viztag.c_str());
			}
		}
		else {
			throw NosuchException("Unable to handle jv->type=%d", jv->type);
		}
		return jsonOK(id);
	}
	if ( meth == "ffglparamget" || meth == "get" ) {
		// std::string viztag = jsonNeedString(params,"viztag");
		std::string param = jsonNeedString(params,"param");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(pipenum,viztag);
		FFGLParameterDef* pd = pi->plugindef()->findparamdef(param);
		if ( pd == NULL ) {
			throw NosuchException("No parameter '%s' in viztag '%s'",param.c_str(),viztag.c_str());
		}
		return pi->getParamJsonResult(pd, pi, id);
	}
	if ( meth == "ff10paramset" ) {
		// std::string viztag = jsonNeedString(params,"viztag");
		std::string param = jsonNeedString(params,"param");
		float val = (float) jsonNeedDouble(params,"val");
		NosuchAssert(pipenum >= 0);
		FF10PluginInstance* pi = FF10NeedPluginInstance(pipenum,viztag);
		if ( ! pi->setparam(param,val) ) {
			throw NosuchException("Unable to find or set parameter '%s' in viztag '%s'",param.c_str(),viztag.c_str());
		}
		return jsonOK(id);
	}
	if ( meth == "ff10paramget" ) {
		// std::string viztag = jsonNeedString(params,"viztag");
		std::string param = jsonNeedString(params,"param");
		NosuchAssert(pipenum >= 0);
		FF10PluginInstance* pi = FF10NeedPluginInstance(pipenum,viztag);
		FF10ParameterDef* pd = pi->plugindef()->findparamdef(param);
		if ( pd == NULL ) {
			throw NosuchException("No parameter '%s' in plugin '%s'",param.c_str(),viztag.c_str());
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
		bool only_enabled = jsonNeedBool(params,"only_enabled",false);  // default is to list everything
		NosuchAssert(pipenum >= 0);
		return jsonResult(FFGLPipelineList(pipenum,only_enabled).c_str(),id);
	}
	if ( meth == "ff10pipeline" ) {
		bool only_enabled = jsonNeedBool(params,"only_enabled",false);  // default is to list everything
		NosuchAssert(pipenum >= 0);
		return jsonResult(FF10PipelineList(pipenum,only_enabled).c_str(),id);
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
		std::string viztag = jsonNeedString(params,"viztag","");
		if ( plugin == "" && viztag != "" ) {
			FFGLPluginInstance* pi = FFGLNeedPluginInstance(viztag);   // throws an exception if it doesn't exist
			plugin = pi->plugindef()->GetPluginName();
		}
		return FFGLParamList(plugin,id);
	}
	if ( meth == "ff10paramlist" ) {
		std::string plugin = jsonNeedString(params,"plugin","");
		std::string viztag = jsonNeedString(params,"viztag","");
		if ( plugin == "" && viztag != "" ) {
			FF10PluginInstance* pi = FF10NeedPluginInstance(viztag);   // throws an exception if it doesn't exist
			plugin = pi->plugindef()->GetPluginName();
		}
		return FF10ParamList(plugin,id);
	}
#endif
	if ( meth == "ffglparamvals" ) {
		// std::string viztag = jsonNeedString(params,"viztag","");
		NosuchAssert(pipenum >= 0);
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(pipenum,viztag);   // throws an exception if it doesn't exist
		std::string r = FFGLParamVals(pi);
		return jsonResult(r,id);
	}
	if ( meth == "ff10paramvals" ) {
		// std::string viztag = jsonNeedString(params,"viztag","");
		NosuchAssert(pipenum >= 0);
		FF10PluginInstance* pi = FF10NeedPluginInstance(pipenum,viztag);   // throws an exception if it doesn't exist
		std::string r = FF10ParamVals(pi);
		return jsonResult(r,id);
	}
	if ( meth == "savepipeline" ) {
		std::string fname =  jsonNeedString(params,"filename");
		NosuchAssert(pipenum >= 0);
		return savePipeline(pipenum,fname,id);
	}
	if ( meth == "set_enablepipeline" ) {
		NosuchAssert(pipenum >= 0);
		m_ffglpipeline[pipenum].m_pipeline_enabled = jsonNeedBool(params, "onoff", true);
		return jsonOK(id);
	}
	if ( meth == "get_enablepipeline" ) {
		NosuchAssert(pipenum >= 0);
		return jsonIntResult(m_ffglpipeline[pipenum].m_pipeline_enabled, id);
	}
	if (meth == "get_pipeset") {
		std::string s = m_pipeset_filename;
		// Take off any trailing .json
		int pos = s.find(".json");
		if (pos != s.npos) {
			s = s.substr(0, pos);
		}
		return jsonStringResult(s, id);
	}
	if ( meth == "load_pipeset" ) {
		std::string fname =  jsonNeedString(params,"filename");
		loadPipeset(fname);
		return jsonOK(id);
	}
	if ( meth == "save_pipeset" ) {
		std::string fname =  jsonNeedString(params,"filename");
		savePipeset(fname);
		return jsonOK(id);
	}
	if ( meth == "set_showfps" ) {
		m_showfps = jsonNeedBool(params, "onoff", true);
		return jsonOK(id);
	}
	if ( meth == "get_showfps" ) {
		return jsonIntResult(m_showfps,id);
	}
	if (meth == "pipelinefilename") {
		NosuchAssert(pipenum >= 0);
		std::string s = m_ffglpipeline[pipenum].m_name;
		// Take off any trailing .json
		int pos = s.find(".json");
		if (pos != s.npos) {
			s = s.substr(0, pos);
		}
		return jsonStringResult(s, id);
	}
	if ( meth == "load_pipeline" ) {
		std::string fname =  jsonNeedString(params,"filename");
		NosuchAssert(pipenum >= 0);
		std::string sidrange = NosuchSnprintf("%d-%d", ppipeline->m_sidmin, ppipeline->m_sidmax);
		loadPipeline(pipenum, fname, true, sidrange);  // this will throw exceptions on failure
		return jsonOK(id);
	}
	if (meth == "get_sidrange") {
		NosuchAssert(pipenum >= 0);
		std::string s = NosuchSnprintf("%d-%d", ppipeline->m_sidmin, ppipeline->m_sidmax);
		return jsonStringResult(s, id);
	}
	if (meth == "set_sidrange") {
		int sidmin, sidmax;
		std::string range =  jsonNeedString(params,"sidrange");
		if (sscanf(range.c_str(), "%d-%d", &sidmin, &sidmax) != 2) {
			return jsonError(-32000,"Bad format of sidrange value",id);
		}
		NosuchAssert(pipenum >= 0);
		ppipeline->m_sidmin = sidmin;
		ppipeline->m_sidmax = sidmax;
		return jsonOK(id);
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
	// DEBUGPRINT(("---- MALLOC new Timer"));
    return new GlfwTimer();
}

