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
	m_img1 = NULL;
	m_img2 = NULL;
	m_img_into_pipeline = NULL;
	m_showfps = false;
	m_capture = NULL;
	m_ffglpipeline.clear();
	m_ff10pipeline.clear();
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

	m_trail_enable = false;
	m_trail_amount = 0.0f;
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
	m_vizserver->AddJsonCallback((void*)this,"ffff",FFFF_json,(void*)this);
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
FFFF::FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string inm)
{
	FFGLPluginInstance* np = new FFGLPluginInstance(plugin,inm);
	// DEBUGPRINT(("----- MALLOC new FFGLPluginInstance"));
	if ( np->InstantiateGL(&fboViewport)!=FF_SUCCESS ) {
		delete np;
		throw NosuchException("Unable to InstantiateGL !?");
	}
	return np;
}

FF10PluginInstance*
FFFF::FF10NewPluginInstance(FF10PluginDef* plugdef, std::string inm)
{
	FF10PluginInstance* np = new FF10PluginInstance(plugdef,inm);
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
FFFF::shufflePipeline()
{
	size_t sz = m_ffglpipeline.size();
	// Just swap things randomly
	for (size_t n = 1; n<sz; n++ ) {
		if ((rand() % 2) == 0) {
			if (m_ffglpipeline[n-1]->isMoveable() && m_ffglpipeline[n]->isMoveable()) {
				FFGLPluginInstance* t = m_ffglpipeline[n - 1];
				m_ffglpipeline[n - 1] = m_ffglpipeline[n];
				m_ffglpipeline[n] = t;
			}
		}
	}
}

void
FFFF::randomizePipeline()
{
	size_t sz = m_ffglpipeline.size();
	// Just swap things randomly
	for (size_t n = 1; n<sz; n++ ) {
		int n1 = rand() % sz;
		int n2 = rand() % sz;
		if ( n1 != n2 ) {
			if (m_ffglpipeline[n1]->isMoveable() && m_ffglpipeline[n2]->isMoveable()) {
				FFGLPluginInstance* t = m_ffglpipeline[n1];
				m_ffglpipeline[n1] = m_ffglpipeline[n2];
				m_ffglpipeline[n2] = t;
			}
		}
	}
}

void
FFFF::clearPipeline()
{
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline.begin(); it != m_ff10pipeline.end(); it++) {
		 DEBUGPRINT1(("--- deleting FF10PluginInstance *it=%ld", (long)(*it)));
		delete *it;
	}
	m_ff10pipeline.clear();

	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
		DEBUGPRINT1(("--- deleteing FFGLPluginInstance *it=%ld", (long)(*it)));
		delete *it;
	}
	m_ffglpipeline.clear();
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
	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
		if ( inm == (*it)->name() ) {
			return *it;
		}
	}
	return NULL;
}

FF10PluginInstance*
FFFF::FF10FindPluginInstance(std::string inm) {
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline.begin(); it != m_ff10pipeline.end(); it++) {
		if ( inm == (*it)->name() ) {
			return *it;
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
	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
		if ( inm == (*it)->name() ) {
			DEBUGPRINT(("Plugin instance named '%s' already exists",inm.c_str()));
			return NULL;
		}
	}

    FFGLPluginDef* plugin = findffglplugindef(pluginName);
	if ( plugin == NULL ) {
		DEBUGPRINT(("There is no plugin named '%s'",pluginName.c_str()));
		return NULL;
	}

	FFGLPluginInstance* np = FFGLNewPluginInstance(plugin,inm);

	// If the plugin's first parameter is "viztag", set it
	int pnum = np->plugindef()->getParamNum("viztag");
	if ( pnum >= 0 ) {
		np->setparam("viztag",inm);
	}

	// Add it to the end of the pipeline
	m_ffglpipeline.insert(m_ffglpipeline.end(),np);

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
		np->setEnable(true);
	}

	return np;
}

FF10PluginInstance*
FFFF::FF10AddToPipeline(std::string pluginName, std::string inm, bool autoenable, cJSON* params) {

	// First scan existing pipeline to see if this instanceName is already used
	FF10PluginInstance* last = NULL;
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline.begin(); it != m_ff10pipeline.end(); it++) {
		if ( inm == (*it)->name() ) {
			DEBUGPRINT(("Plugin instance named '%s' already exists",inm.c_str()));
			return NULL;
		}
	}

    FF10PluginDef* plugin = findff10plugindef(pluginName);
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

	m_ff10pipeline.insert(m_ff10pipeline.end(),np);

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
FFFF::FFGLMoveUpInPipeline(std::string inm) {
	size_t sz = m_ffglpipeline.size();
	for (size_t n = 0; n<sz; n++ ) {
		if (inm == m_ffglpipeline[n]->name()) {
			if (n > 0) {
				FFGLPluginInstance* t = m_ffglpipeline[n - 1];
				m_ffglpipeline[n - 1] = m_ffglpipeline[n];
				m_ffglpipeline[n] = t;
			}
			return;
		}
	}
}

void
FFFF::FFGLMoveDownInPipeline(std::string inm) {
	size_t sz = m_ffglpipeline.size();
	for (size_t n = 0; n<sz; n++ ) {
		if (inm == m_ffglpipeline[n]->name()) {
			if (n < (sz-1)) {
				FFGLPluginInstance* t = m_ffglpipeline[n + 1];
				m_ffglpipeline[n + 1] = m_ffglpipeline[n];
				m_ffglpipeline[n] = t;
			}
			return;
		}
	}
}

void
FFFF::FFGLDeleteFromPipeline(std::string inm) {

	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); ) {
		if (inm == (*it)->name()) {
			// DEBUGPRINT1(("--- deleteing BB FFGLPluginInstance *it=%ld", (long)(*it)));
			delete *it;
			it = m_ffglpipeline.erase(it);
		}
		else {
			it++;
		}
	}
}

void
FFFF::FF10DeleteFromPipeline(std::string inm) {

	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline.begin(); it != m_ff10pipeline.end(); ) {
		if (inm == (*it)->name()) {
			delete *it;
			it = m_ff10pipeline.erase(it);
		}
		else {
			it++;
		}
	}
}

void
FFFF::loadPipeline(std::string configname, bool synthesize)
{
	if (!NosuchEndsWith(configname, ".json")) {
		configname += ".json";
	}
	std::string fname = VizConfigPath("ffff\\" + configname);
	std::string err;

	DEBUGPRINT(("loadPipeline configname=%s fname=%s", configname.c_str(), fname.c_str()));

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
			throw NosuchException("Unable to parse file!? fname=%s", fname.c_str());
		}
	}
	loadPipelineJson(json);
	jsonFree(json);
	m_pipelinename = configname;
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
		bool enabled = jsonNeedBool(p, "enabled", true);  // optional, default is 1 (true)
		bool moveable = jsonNeedBool(p, "moveable", true);  // optional, default is 1 (true)
		cJSON* params = jsonGetArray(p, "params");
		NosuchAssert(plugin && params);

		// Default is to enable the plugin as soon as we added it, but you
		// can add an "enabled" value to change that.

		// XXX - someday there should be a base class for both
		// FFGL and FF10 plugins
		if (plugintype == "ffgl") {
			FFGLPluginInstance* pi = FFGLAddToPipeline(name, viztag, enabled, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}
			pi->setMoveable(moveable);
		}
		else {  // "ff10"
			FF10PluginInstance* pi = FF10AddToPipeline(name, viztag, enabled, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}

		}
		DEBUGPRINT(("Pipeline, loaded plugin=%s viztag=%s", name, viztag.c_str()));

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
				DEBUGPRINT(("vizletdump meth=%s", meth.c_str()));
				std::string fullmethod = std::string(name) + "." + meth;
				const char* s = m_vizserver->ProcessJson(fullmethod.c_str(), params, "12345");
			}
		}

	}

	if (m_trail_enable) {
		FFGLPluginInstance* pi = FFGLAddToPipeline("Trails", "Trails", true, NULL);
		if (!pi) {
			DEBUGPRINT(("Unable to add Trails plugin!?"));
		}
		else {
			pi->setparam("Opacity", (float)m_trail_amount);
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
FFFF::doOneFrame(bool use_camera, int window_width, int window_height)
{
	int interp;
	IplImage* camframe = NULL;
	unsigned char *pixels_into_pipeline;
	// IplImage* img_into_pipeline;

	//bind the gl texture so we can upload the next video frame
	glBindTexture(GL_TEXTURE_2D, mapTexture.Handle);

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
			// DEBUGPRINT(("----- MALLOC m_img1 = %d", (long)m_img1));
		}
		cvSetImageData(m_img1, pixels, camWidth * 3);

		if (m_img2 == NULL) {
			m_img2 = cvCreateImage(ffsz, IPL_DEPTH_8U, 3);
			// DEBUGPRINT(("----- MALLOC m_img2 = %d", (long)m_img2));
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
			do_ff10pipeline(m_ff10pipeline, resizedpixels);
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

		do_ff10pipeline(m_ff10pipeline, pixels_into_pipeline);
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
	int num_ffgl_active = 0;

	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
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

	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
		FFGLPluginInstance* pi = *it;
		if (!pi->isEnabled()) {
			continue;
		}
		if (num_ffgl_active == 1) {
			if ( ! do_ffgl_plugin(pi,3) ) {
				DEBUGPRINT(("DISABLING plugin - Error B in do_ffgl_plugin for plugin=%s",pi->name().c_str()));
				pi->setEnable(false);
			}
			continue;
		}
		if ( pi == firstplugin ) {
			if (!do_ffgl_plugin(pi, 0)) {
				DEBUGPRINT(("DISABLING plugin - Error D in do_ffgl_plugin for plugin=%s", pi->name().c_str()));
				pi->setEnable(false);
			}
		}
		else if (pi == lastplugin) {
			//deactivate rendering to the fbo
			//(this re-activates rendering to the window)
			fbo_output->UnbindAsRenderTarget(glExtensions);
			if (!do_ffgl_plugin(pi, 2)) {
				DEBUGPRINT(("DISABLING plugin - Error D in do_ffgl_plugin for plugin=%s", pi->name().c_str()));
				pi->setEnable(false);
			}
		} 
		else {
			if (!do_ffgl_plugin(pi, 1)) {
				DEBUGPRINT(("DISABLING plugin - Error E in do_ffgl_plugin for plugin=%s", pi->name().c_str()));
				pi->setEnable(false);
			}
		}
	}

#if 0
	//deactivate rendering to the fbo
	//(this re-activates rendering to the window)
	if (num_ffgl_active > 0) {
		fbo_output->UnbindAsRenderTarget(glExtensions);
	}
#endif

	static GLubyte *m_data = NULL;

	extern char* SaveFrames;
	extern FILE* Ffmpeg;

	// Now read the pixels back and save them
	if (SaveFrames && *SaveFrames!='\0') {
		static int filenum = 0;
		std::string path = VizPath("recording") + NosuchSnprintf("/%s%06d.ppm", SaveFrames, filenum++);
		if (m_data == NULL) {
			m_data = (GLubyte*)malloc(4 * window_width*window_height);  // 4, should work for both 3 and 4 bytes
			// DEBUGPRINT(("---- MALLOC SaveFrames data %d", data));
		}
		glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, m_data);
		IplImage* img = cvCreateImageHeader(cvSize(window_width, window_height), IPL_DEPTH_8U, 3);
		// DEBUGPRINT(("---- MALLOC SaveFrames img %d", (long)img));
		img->origin = 1;  // ???
		img->imageData = (char*)m_data;
		if (!cvSaveImage(path.c_str(), img)) {
			DEBUGPRINT(("Unable to save image: %s", path.c_str()));
		}
		// free(data);  it's static, now
		img->imageData = NULL;
		DEBUGPRINT(("---- RELEASE SaveFrames img %d", (long)img));
		cvReleaseImageHeader(&img);
	}
	if (Ffmpeg) {
		if (m_data == NULL) {
			m_data = (GLubyte*)malloc(4 * window_width*window_height);  // 4, should work for both 3 and 4 bytes
			// DEBUGPRINT(("---- MALLOC Ffmpeg data %d", data));
		}
		glReadPixels(0, 0, window_width, window_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, m_data);
		fwrite(m_data, window_width*window_height, 3, Ffmpeg);
	}

#ifdef NOMORERELEASE
	if ( camframe != NULL ) {
		DEBUGPRINT(("---- RELEASE m_img_into_pipeline %d", (long)m_img_into_pipeline));
		cvReleaseImage(&m_img_into_pipeline);
	} else {
		DEBUGPRINT(("---- RELEASE m_img_into_pipeline %d", (long)m_img_into_pipeline));
		cvReleaseImage(&m_img_into_pipeline);
	}
#endif

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
	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
		FFGLPluginInstance* p = *it;
		bool isvizlet = m_vizserver->IsVizlet(p->name().c_str());
		std::string isviz;
		if (isvizlet) {
			isviz = std::string(" \"vizlet\": ") + (isvizlet ? "1" : "0") + ", ";
		}
		bool enabled = p->isEnabled();
		if ( only_enabled==false || enabled==true ) {
			r += (sep + "{ \"plugin\":\""+p->plugindef()->GetPluginName()+"\","
				+ " \"instance\":\"" + p->name() + "\", "
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

std::string FFFF::FF10PipelineList(bool only_enabled) {
	std::string r = "[";
	std::string sep = "";
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline.begin(); it != m_ff10pipeline.end(); it++) {
		FF10PluginInstance* p = *it;
		bool enabled = p->isEnabled();
		if ( only_enabled==false || enabled==true ) {
			r += (sep + "{ \"plugin\":\""+p->plugindef()->GetPluginName()+"\", \"instance\":\"" + p->name() + "\", "
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
			r += sep+linebreak+NosuchSnprintf("{\"name\":\"%s\", \"value\":\"%s\" }",nm,pi->GetParameterDisplay(pd->num).c_str());
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

std::string FFFF::savePipeline(std::string fname, const char* id)
{
	if ( ! NosuchEndsWith(fname, ".json") ) {
		fname += ".json";
	}
	std::string fpath = VizConfigPath("ffff\\"+fname);

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
	for (std::vector<FF10PluginInstance*>::iterator it = m_ff10pipeline.begin(); it != m_ff10pipeline.end(); it++) {
		f << sep;
		f << "\t{\n";
		f << "\t\t\"ff10plugin\": \"" + (*it)->name() + "\",\n";
		f << "\t\t\"enabled\": " + std::string((*it)->isEnabled()?"1":"0") + ",\n";
		f << "\t\t\"moveable\": " + std::string((*it)->isMoveable()?"1":"0") + ",\n";
		f << "\t\t\"params\": " + FF10ParamVals(*it,"\n\t\t\t") + "\n";
		f << "\t}";
		sep = ",\n";
	}
	for (std::vector<FFGLPluginInstance*>::iterator it = m_ffglpipeline.begin(); it != m_ffglpipeline.end(); it++) {
		FFGLPluginInstance* p = *it;
		f << sep;
		f << "\t{\n";
		f << "\t\t\"ffglplugin\": \"" + (*it)->name() + "\",\n";
		f << "\t\t\"enabled\": " + std::string((*it)->isEnabled()?"1":"0") + ",\n";
		f << "\t\t\"moveable\": " + std::string((*it)->isMoveable()?"1":"0") + ",\n";
		if (m_vizserver->IsVizlet((*it)->name().c_str())) {
			std::string name = (*it)->name();
			f << "\t\t\"vizlet\": " + std::string(m_vizserver->IsVizlet(name.c_str()) ? "1" : "0") + ",\n";

			std::string fullmethod = name + "." + "dump";
			const char* s = m_vizserver->ProcessJson(fullmethod.c_str(), NULL, "12345");
			cJSON* json = cJSON_Parse(s);
			if (!json) {
				throw NosuchException("Unable to parse .dump json in savePipeline!?");
			}
			std::string result = jsonNeedString(json, "result","");
			f << "\t\t\"vizletdump\": " + result + ",\n";
			jsonFree(json);
		}
		f << "\t\t\"params\": " + FFGLParamVals(*it,"\n\t\t\t") + "\n";
		f << "\t}";
		sep = ",\n";
	}
	f << "\n\t]\n}\n";
	f.close();
	return jsonOK(id);
}

std::string FFFF::executeJson(std::string meth, cJSON *params, const char* id)
{
	std::string err;

	if (meth == "apis") {
		return jsonStringResult("time;clicknow;show;hide;echo;"
			"enable(instance,onoff);delete(instance);"
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
			"fps(onoff);"
			"trail_enable(onoff);"
			"trail_amount(value);"
			"moveup(instance);"
			"movedown(instance);"
			"shufflepipeline;"
			"randomizepipeline;"
			, id);
	}
	// DEBUGPRINT(("FFFF api = %s", meth.c_str()));
	if (meth == "fps") {
		m_showfps = jsonNeedBool(params, "onoff");
		return jsonOK(id);
	}
	if (meth == "trail_enable") {
		m_trail_enable = jsonNeedBool(params, "onoff");
		return jsonOK(id);
	}
	if (meth == "trail_amount") {
		m_trail_amount = jsonNeedDouble(params, "value");
		return jsonOK(id);
	}

	if (meth == "time") {
		return jsonDoubleResult(m_vizserver->GetTime(), id);
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

	if (meth == "add" || meth == "ffgladd") {
		std::string plugin = jsonNeedString(params, "plugin");
		std::string iname = jsonNeedString(params, "instance");
		bool autoenable = jsonNeedBool(params, "autoenable", true);
		bool moveable = jsonNeedBool(params, "moveable", true);
		cJSON* pparams = jsonGetArray(params, "params");
		FFGLPluginInstance* pi = FFGLAddToPipeline(plugin, iname, autoenable, pparams);
		if (!pi) {
			throw NosuchException("Unable to add plugin '%s'", plugin.c_str());
		}
		pi->setMoveable(moveable);
		return jsonStringResult(iname, id);
	}
	if (meth == "ff10add") {
		std::string plugin = jsonNeedString(params, "plugin");
		std::string iname = jsonNeedString(params, "instance");
		bool autoenable = jsonNeedBool(params, "autoenable", true);
		cJSON* pparams = jsonGetArray(params, "params");
		FF10PluginInstance* pi = FF10AddToPipeline(plugin, iname, autoenable, pparams);
		if (!pi) {
			throw NosuchException("Unable to add plugin '%s'", plugin.c_str());
		}
		return jsonStringResult(iname, id);
	}
	if (meth == "ffglenable" || meth == "enable" ) {
		std::string instance = jsonNeedString(params, "instance");
		bool onoff = jsonNeedBool(params, "onoff");
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);   // throws an exception if it doesn't exist
		pi->setEnable(onoff);
		return jsonOK(id);
	}
	if (meth == "ffglmoveable" ) {
		std::string instance = jsonNeedString(params, "instance");
		bool onoff = jsonNeedBool(params, "onoff");
		FFGLPluginInstance* pi = FFGLNeedPluginInstance(instance);   // throws an exception if it doesn't exist
		pi->setMoveable(onoff);
		return jsonOK(id);
	}
	if (meth == "about") {
		std::string inst = jsonNeedString(params, "instance");
		FFGLPluginInstance* pgl = FFGLFindPluginInstance(inst);
		if (pgl != NULL) {
			FFGLPluginDef* def = pgl->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->About, id);
		}
		FF10PluginInstance* p10 = FF10NeedPluginInstance(inst);
		if (p10 != NULL) {
			FF10PluginDef* def = p10->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->About, id);
		}
		throw NosuchException("No such instance: %s",inst.c_str());
	}
	if (meth == "description") {
		std::string inst = jsonNeedString(params, "instance");
		FFGLPluginInstance* pgl = FFGLFindPluginInstance(inst);
		if (pgl != NULL) {
			FFGLPluginDef* def = pgl->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->Description, id);
		}
		FF10PluginInstance* p10 = FF10NeedPluginInstance(inst);
		if (p10 != NULL) {
			FF10PluginDef* def = p10->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->Description, id);
		}
		throw NosuchException("No such instance: %s",inst.c_str());
	}
	if ( meth == "delete" ) {
		// It's okay if instance doesn't exist, so don't use needFFGLPluginInstance
		std::string iname = jsonNeedString(params,"instance");
		FFGLDeleteFromPipeline(iname);
		return jsonOK(id);
	}
	if ( meth == "moveup" ) {
		// It's okay if instance doesn't exist, so don't use needFFGLPluginInstance
		FFGLMoveUpInPipeline(jsonNeedString(params,"instance"));
		return jsonOK(id);
	}
	if ( meth == "movedown" ) {
		// It's okay if instance doesn't exist, so don't use needFFGLPluginInstance
		FFGLMoveDownInPipeline(jsonNeedString(params,"instance"));
		return jsonOK(id);
	}
	if (meth == "shufflepipeline") {
		shufflePipeline();
		return jsonOK(id);
	}
	if (meth == "randomizepipeline") {
		randomizePipeline();
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
		bool only_enabled = jsonNeedBool(params,"only_enabled",false);  // default is to list everything
		return jsonResult(FFGLPipelineList(only_enabled).c_str(),id);
	}
	if ( meth == "ff10pipeline" ) {
		bool only_enabled = jsonNeedBool(params,"only_enabled",false);  // default is to list everything
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
		std::string r = FFGLParamVals(pi);
		return jsonResult(r,id);
	}
	if ( meth == "ff10paramvals" ) {
		std::string instance = jsonNeedString(params,"instance","");
		FF10PluginInstance* pi = FF10NeedPluginInstance(instance);   // throws an exception if it doesn't exist
		std::string r = FF10ParamVals(pi);
		return jsonResult(r,id);
	}
	if ( meth == "savepipeline" ) {
		std::string fname =  jsonNeedString(params,"filename");
		return savePipeline(fname,id);
	}
	if (meth == "pipelinefilename") {
		std::string s = m_pipelinename;
		// Take off any trailing .json
		int pos = s.find(".json");
		if (pos != s.npos) {
			s = s.substr(0, pos);
		}
		return jsonStringResult(s, id);
	}
	if ( meth == "loadpipeline" ) {
		std::string fname =  jsonNeedString(params,"filename");
		loadPipeline(fname, true);  // this will throw exceptions on failure
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

