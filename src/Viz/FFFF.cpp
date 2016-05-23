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

#include <gl/glew.h>

#include "glstuff.h"

#include "drawtext.h"

#include "FFGLPlugin.h"

#include "FFFF.h"
#include "FFGLPipeline.h"

#include "NosuchJSON.h"
#include "Timer.h"
#include "AudioHost.h"

#include "opencv/cv.h"
#include "opencv/highgui.h"

#include <GLFW/glfw3.h>

#include "spout.h"

#include <iostream>
#include <fstream>

#include "VizServer.h"
#include "VizParams.h"

#include <sys/stat.h>

VizServer* FFFF::m_vizserver = NULL;

char* FfffOutput = NULL;
FILE* FfffOutputFile = NULL;
std::string FfffOutputPrefix;
int FfffOutputFPS;

const char* FFFF_json(void* data,const char *method, cJSON* params, const char* id) {

	// NEEDS to be static, since we return the c_str() of it.
	// This also assumes that JSON API calls are serialized.
	static std::string result;

	FFFF* ffff = (FFFF*)data;
	ffff->m_json_result = ffff->submitJson(std::string(method),params,id);
	return ffff->m_json_result.c_str();
}

void FFFF_error(void* data,const char* msg) {
	FFFF* ffff = (FFFF*)data;
	ffff->ErrorPopup(msg);
}

FFFF::FFFF() {

	m_vizserver = VizServer::GetServer();

	m_vizserver->SetErrorCallback(FFFF_error,this);

	glfwSetTime(0.0);

	std::string err;
	std::string fname = VizConfigPath("FFFF.json");
	cJSON* config = jsonReadFile(fname, err);
	if (!config) {
		throw NosuchException("Hey!  Error in reading JSON from %s!  err=%s", fname.c_str(), err.c_str());
	}

	jsonSetDebugConfig(config);

	m_output_framedata = NULL;
	m_output_lastwrite = 0.0;
	m_output_framenum = 0;
	m_spoutsender = NULL;

	m_camera_image_raw = NULL;
	m_camera_image_flipped = NULL;
	m_img_into_pipeline = NULL;
	m_record = false;
	m_capture = NULL;
	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		m_ffglpipeline[pipenum].clear();
		m_pipeline_enabled[pipenum] = false;
		m_pipeline_camera_enabled[pipenum] = false;
	}
	hidden = false;

	// Allow the config to override the default paths for these
	m_ffglpath = jsonNeedString(config, "ffglpath", "ffglplugins");

	m_window_width = jsonNeedInt(config, "window_width", 800);
	m_window_height = jsonNeedInt(config, "window_height", 600);
	m_window_x = jsonNeedInt(config, "window_x", 0);
	m_window_y = jsonNeedInt(config, "window_y", 0);

	m_preview_width = jsonNeedInt(config, "preview_width", 800);
	m_preview_height = jsonNeedInt(config, "preview_height", 600);
	m_preview_x = jsonNeedInt(config, "preview_x", 1000);
	m_preview_y = jsonNeedInt(config, "preview_y", 0);

	m_pipesetname = jsonNeedString(config, "pipeset", "");

	m_fontname = jsonNeedString(config, "fontname", "arial.ttf");
	m_fontsize = jsonNeedInt(config, "fontsize", 24);

	m_camera_index = jsonNeedInt(config, "camera", -1);  // -1 for no camera, 0+ for camera
	m_camera_flipx = jsonNeedBool(config, "camera_flipx", false);
	m_camera_flipy = jsonNeedBool(config, "camera_flipy", false);

	m_showfps = jsonNeedInt(config, "showfps", 0) ? true : false;
	m_autoload = jsonNeedInt(config, "autoload", 1) ? true : false;
	m_autosave = jsonNeedInt(config, "autosave", 1) ? true : false;
	m_audiohost_type = jsonNeedString(config, "audiohost_type", "");
	m_desired_FPS = jsonNeedInt(config, "fps", 30);
	m_spout = jsonNeedBool(config, "spout", true);
	if (m_audiohost_type != "") {
		m_audiohost = new AudioHost(m_audiohost_type, jsonNeedJSON(config, "audiohost_config", NULL));
	}
	else {
		m_audiohost = NULL;
	}

	FfffOutputPrefix = jsonNeedString(config, "outputprefix", "");
	FfffOutputFPS = jsonNeedInt(config, "outputfps", 30);

	NosuchLockInit(&_json_mutex,"json");
	m_json_cond = PTHREAD_COND_INITIALIZER;
	m_json_pending = false;
	m_timer = Timer::New();
	m_throttle_timePerFrame = 1.0 / m_desired_FPS;
	m_throttle_lasttime = 0.0;

	m_fps_accumulator = 0;
	m_fps_lasttime = -1.0;

	m_state = new FFFFState();
}

void FFFF::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	switch (action) {
	case GLFW_PRESS:
		m_vizserver->InsertKeystroke(key,KEYSTROKE_DOWN);
		break;
	case GLFW_RELEASE:
		m_vizserver->InsertKeystroke(key,KEYSTROKE_UP);
		break;
	}
}


void
FFFF::CreateWindows() {

	GLFWmonitor* monitor;
	int nmonitors;
	GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);

	for (int n = 0; n < nmonitors; n++) {
		const char* nm = glfwGetMonitorName(monitors[n]);
		const GLFWvidmode* mode = glfwGetVideoMode(monitors[n]);
		DEBUGPRINT(("GLFW Monitor %d - name=%s  width=%d height=%d refresh=%d", n, nm ? nm : "NULL?", mode->width, mode->height, mode->refreshRate));
	}

	// Only set monitor if you're doing fullscreen
	// monitor = glfwGetPrimaryMonitor();
	monitor = NULL;

	glfwWindowHint(GLFW_DECORATED, GL_FALSE);

	window = glfwCreateWindow(m_window_width, m_window_height, "FFFF", monitor, NULL);
	if (window == NULL) {
		throw NosuchException("Unable to create main window!?");
	}
	glfwSetWindowPos(window, m_window_x, m_window_y);
	glfwSetWindowSize(window, m_window_width, m_window_height);
	glfwShowWindow(window);

	// MAKE PREVIEW WINDOW, be sure to share resources with main window
	preview = glfwCreateWindow(m_preview_width, m_preview_height, "Preview", monitor, window);
	if (preview == NULL) {
		throw NosuchException("Unable to create preview window!?");
	}
	glfwSetWindowPos(preview, m_preview_x, m_preview_y);
	glfwSetWindowSize(preview, m_preview_width, m_preview_height);
	glfwShowWindow(preview);
	// END MAKE PREVIEW WINDOW

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	InitGlExtensions();

	std::string fontpath = "c:\\windows\\fonts\\" + m_fontname;
	m_font = dtx_open_font(fontpath.c_str(), m_fontsize);
	if (m_font == NULL) {
		throw NosuchException("Unable to load font '%s'!?",fontpath.c_str());
	}

	if (m_img_into_pipeline == NULL) {
		CvSize fbosz = cvSize(m_window_width, m_window_height);
		m_img_into_pipeline = cvCreateImage(fbosz, IPL_DEPTH_8U, 3);
		if (m_img_into_pipeline == NULL) {
			throw NosuchException("Unable to create image for img_into_pipeline!???");
		}
	}

}

void FFFF::drawWindowPipelines() {
	// MAIN WINDOW
	glfwMakeContextCurrent(window);
	// glfwGetFramebufferSize(window, &width, &height);

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];
		if (m_pipeline_enabled[pipenum]) {
			if (m_pipeline_camera_enabled[pipenum]) {
				IplImage* camframe = getCameraFrame();
				paintInitialTexture(camframe,mapTexture.Handle,m_camera_flipx,m_camera_flipy);
			}
			else {

			}
			pipeline.doPipeline(m_window_width, m_window_height);
		}
	}
	glClearColor(0, 0, 0, 0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];
		if (m_pipeline_enabled[pipenum]) {
			pipeline.paintTexture();
		}
	}

}

void
FFFF::drawWindowFinish() {

#if 0
	// Draw a rectangle just to show that we're alive.
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glLineWidth((GLfloat)2.0f);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.1f, 0.1f, 0.0f);	// Top Left
	glVertex3f(0.1f, 0.9f, 0.0f);	// Top Right
	glVertex3f(0.9f, 0.9f, 0.0f);	// Bottom Right
	glVertex3f(0.9f, 0.1f, 0.0f);	// Bottom Left
	glEnd();

	glColor4f(1.0, 0.0, 0.0, 1.0);
	glPushMatrix();
	glTranslatef(-0.5, -0.5, 0.0);
	glScalef(0.002f, 0.004f, 1.0f);
	dtx_use_font(m_font, m_fontsize);
	dtx_string("Space Puddle");
	glPopMatrix();
#endif

	glfwSwapBuffers(window);

	sendSpout();
}

void
FFFF::drawPreviewPipelines() {

	glfwMakeContextCurrent(preview);

	glClearColor(0, 0.0, 0, 0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];
		if (m_pipeline_enabled[pipenum]) {
			m_ffglpipeline[pipenum].paintTexture();
		}
	}
}

void
FFFF::drawPrefixFinish() {

#if 0
	// Draw a rectangle just to show that we're alive.
	glColor4f(1.0, 0.0, 0.0, 1.0);
	glLineWidth((GLfloat)2.0f);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.1f, 0.1f, 0.0f);	// Top Left
	glVertex3f(0.1f, 0.9f, 0.0f);	// Top Right
	glVertex3f(0.9f, 0.9f, 0.0f);	// Bottom Right
	glVertex3f(0.9f, 0.1f, 0.0f);	// Bottom Left
	glEnd();

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glPushMatrix();
	glTranslatef(-0.5, -0.5, 0.0);
	glScalef(0.002f, 0.004f, 1.0f);
	dtx_use_font(m_font, m_fontsize);
	dtx_string("Space Puddle");
	glPopMatrix();
#endif

	glfwSwapBuffers(preview);
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

void
FFFF::StartStuff() {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		throw NosuchException("glfwInit failed!?");
	}

	if (!m_vizserver->Start()) {
		throw NosuchException("Unable to start VizServer!?");
	}

	m_vizserver->AddJsonCallback((void*)this,"ffff",FFFF_json,(void*)this);

	if (m_audiohost) {
	 	m_audiohost->Start();
	}

	CreateWindows();

	InitCamera();

	loadAllPluginDefs();

	spoutInit();

#ifdef DUMPOBJECTS
	_CrtMemCheckpoint(&m_s0);
#endif

}

void
FFFF::RunStuff() {	

	// int count = 0;
	while (!glfwWindowShouldClose(window))
	{
		checkAndExecuteJSON();

		if (hidden == false) {

			drawWindowPipelines();
			drawWindowFinish();

			drawPreviewPipelines();
			drawPrefixFinish();
		}

		glfwPollEvents();

		CheckFPS();
		CheckAutoload();
	}
}

void
FFFF::StopStuff() {

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];
		pipeline.clear();
	}

	m_vizserver->Stop();
	VizServer::DeleteServer();

	if (FfffOutputFile) {
		fclose(FfffOutputFile);
		FfffOutputFile = NULL;
	}

#ifdef _DEBUG
#ifdef DUMPOBJECTS
	_CrtMemDumpAllObjectsSince(&m_s0);
#endif
#endif

	// clearPipeline();

	glfwDestroyWindow(window);
	glfwDestroyWindow(preview);

	glfwTerminate();

	Pt_Stop();

	// _CrtDumpMemoryLeaks();
}

void
FFFF::CheckAutoload()
{
	if (!m_autoload) {
		return;
	}
	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {

		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];

		pipeline.CheckAutoload();
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

std::string FFFF::executeJsonAndCatchExceptions(const std::string meth, cJSON *params, const char* id) {
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

void
FFFF::InitCamera() {

	m_use_camera = FALSE;
	if (m_camera_index < 0) {
		DEBUGPRINT(("Camera is disabled (camera < 0)"));
		m_capture = NULL;
		return;
	}

	m_capture = cvCreateCameraCapture(m_camera_index);
	if ( !m_capture ) {
	    DEBUGPRINT(("Unable to initialize capture from camera index=%d\n",m_camera_index));
		return;
	}

	m_use_camera = TRUE;

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
		return;
	}

#define CV_CAP_PROP_FRAME_WIDTH    3
#define CV_CAP_PROP_FRAME_HEIGHT   4

    /* retrieve or set capture properties */
    double fwidth = cvGetCaptureProperty( m_capture, CV_CAP_PROP_FRAME_WIDTH );
    double fheight = cvGetCaptureProperty( m_capture, CV_CAP_PROP_FRAME_HEIGHT );

    m_camWidth = (int)fwidth;
    m_camHeight = (int)fheight;

    DEBUGPRINT(("CAMERA CV says width=%d height=%d\n",m_camWidth,m_camHeight));
	return;
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

void
FFFF::loadAllPluginDefs()
{
    nffglplugindefs = 0;

    loadffglpath(m_ffglpath);
	FFGLinit2();

    DEBUGPRINT(("%d FFGL plugins\n",nffglplugindefs));
}

void
FFFF::ErrorPopup(const char* msg) {
	MessageBoxA(NULL, msg, "FFFF", MB_OK);
}

#define FLIP_X 1
#define FLIP_Y 0
#define FLIP_XY -1
#define FLIP_NONE -2

// Something is strange here - the values for horizonta
int flipMode(bool flipx, bool flipy) {
	if (flipx) {
		if (flipy) {
			return FLIP_XY;
		}
		else {
			return FLIP_X;
		}
	}
	else {
		if (flipy) {
			return FLIP_Y;
		}
		else {
			return FLIP_NONE;
		}
	}
}

bool
FFFF::paintInitialTexture(IplImage* camframe, GLuint texturehandle, bool flipx, bool flipy) {

	int interp;
	unsigned char *pixels_into_pipeline;

	glBindTexture(GL_TEXTURE_2D, texturehandle);

	if (camframe != NULL) {

		unsigned char* pixels = (unsigned char *)(camframe->imageData);
		CvSize camsz = cvSize(m_camWidth, m_camHeight);

		// The image header is static, it's only created once.
		if (m_camera_image_raw == NULL) {
			m_camera_image_raw = cvCreateImageHeader(camsz, IPL_DEPTH_8U, 3);
		}

		cvSetImageData(m_camera_image_raw, pixels, m_camWidth * 3);

		interp = CV_INTER_LINEAR;  // or CV_INTER_NN
		interp = CV_INTER_NN;  // This produces some artifacts compared to CV_INTER_LINEAR, but is faster

		// XXX - If the camera size is the same as the ff (ffgl) size,
		// we shouldn't have to do this resize
		int flipmode = flipMode(m_camera_flipx, m_camera_flipy);
		if (flipmode != FLIP_NONE) {
			// Flip the camera image first - it's typically low-rez, like 640x480
			// Ideally, we'd 
			if (m_camera_image_flipped == NULL) {
				m_camera_image_flipped = cvCreateImage(camsz, IPL_DEPTH_8U, 3);
			}
			cvFlip(m_camera_image_raw, m_camera_image_flipped, flipmode);
			cvResize(m_camera_image_flipped, m_img_into_pipeline, interp);
		}
		else {
			cvResize(m_camera_image_raw, m_img_into_pipeline, interp);
		}
		cvGetImageRawData(m_img_into_pipeline, &pixels_into_pipeline, NULL, NULL);
	}
	else {
		cvZero(m_img_into_pipeline);
		pixels_into_pipeline = (unsigned char *)(m_img_into_pipeline->imageData);
	}

	//upload it to the gl texture. use subimage because
	//the video frame size is probably smaller than the
	//size of the texture on the gpu hardware
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		0, 0,
		m_window_width,
		m_window_height,
		GL_BGR_EXT,
		GL_UNSIGNED_BYTE,
		pixels_into_pipeline);

	//unbind the gl texture
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

#if 0
void
FFFF::doPipeline(int pipenum, int width, int height)
{
	m_ffglpipeline[pipenum].doPipeline(width, height);
}
#endif

void
FFGLPipeline::setEnableInput(bool onoff) {

	// Go through all the vizlets in the pipeline, and enable/disable input

	DEBUGPRINT1(("setEnableInput pipe=%s onoff=%d ",m_name.c_str(),onoff));

	VizServer* viz = VizServer::GetServer();
	FFGLPluginList& ffglplugins = m_pluginlist;
	for (FFGLPluginList::iterator it = ffglplugins.begin(); it != ffglplugins.end(); it++) {
		FFGLPluginInstance* p = *it;
		bool isvizlet = viz->IsVizlet(p->viztag().c_str());
		if (!isvizlet) {
			continue;
		}

		// This code just blindly calls a set_enableinput method on every vizlet.

		std::string fullmethod = NosuchSnprintf("%s.set_enableinput",p->viztag().c_str());
		std::string jsonstr = NosuchSnprintf("{ \"onoff\": \"%d\" }", onoff);
		cJSON* params = cJSON_Parse(jsonstr.c_str());
		if (!params) {
			throw NosuchException("Internal error in parsing enableinput json!?");
		}
		std::string s = viz->ProcessJson(fullmethod.c_str(), params, "12345");
		DEBUGPRINT1(("result of set_enableinput=%s",s.c_str()));
		// XXX - should really check the result for success (or missing api), here.
		cJSON_free(params);
	}
}

void
FFGLPipeline::setSidrange(int sidmin, int sidmax) {
	m_sidmin = sidmin;
	m_sidmax = sidmax;
	std::string jsonstr = NosuchSnprintf("{ \"sidrange\": \"%d-%d\" }", m_sidmin, m_sidmax);
	cJSON* params = cJSON_Parse(jsonstr.c_str());
	if (!params) {
		throw NosuchException("Internal error in parsing sidrange json!?");
	}
	applyAllPlugins("set_sidrange", params);
}

void
FFGLPipeline::applyAllPlugins(std::string meth, cJSON* params) {

	// Now go through all the vizlets in the pipeline, and set the sidrange within them

	DEBUGPRINT1(("applyAllPlugins meth=%s", meth.c_str()));

	VizServer* viz = VizServer::GetServer();
	FFGLPluginList& ffglplugins = m_pluginlist;
	for (FFGLPluginList::iterator it = ffglplugins.begin(); it != ffglplugins.end(); it++) {
		FFGLPluginInstance* p = *it;
		bool isvizlet = viz->IsVizlet(p->viztag().c_str());
		if (!isvizlet) {
			continue;
		}
		// This code just blindly calls the method on every vizlet.

		std::string fullmethod = p->viztag() + "." + meth;
		std::string s = viz->ProcessJson(fullmethod.c_str(), params, "12345");
		DEBUGPRINT1(("result of meth=%s",s.c_str()));
		// XXX - should really check the result for success (or missing api), here
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
FFFF::spoutInit() {

	if (m_spout) {

		spoutInitTexture();

		strcpy(m_sendername, "FFFF");
		m_spoutsender = new SpoutSender;
		bool b = m_spoutsender->CreateSender(m_sendername, m_window_width, m_window_height);
		if (!b) {
			DEBUGPRINT(("Unable to CreateSender for Spout!?"));
			NosuchErrorOutput("Unable to CreateSender for Spout!?");
			delete m_spoutsender;
			m_spoutsender = NULL;
		}
	}
}

void
FFFF::sendSpout() {

	if (m_spoutsender != NULL && spoutTexture.Handle != 0) {

		// Copy screen into spoutTexture
		glBindTexture(GL_TEXTURE_2D, spoutTexture.Handle);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_window_width, m_window_height);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Send it.
		bool b = m_spoutsender->SendTexture(spoutTexture.Handle, GL_TEXTURE_2D, m_window_width, m_window_height);
		if (!b) {
			DEBUGPRINT(("Error in spout SendTexture!?"));
		}
	}
}

void
FFGLPipeline::doPipeline(int window_width, int window_height)
{
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

FFGLPluginDef*
needFFGLPlugin(std::string name)
{
		FFGLPluginDef* p = findffglplugindef(name);
		if ( p == NULL ) {
			throw NosuchSnprintf("No plugin named '%s'",name.c_str());
		}
		return p;
}

std::string FFFF::savePipeline(int pipenum, std::string fname, const char* id)
{
	std::string fpath = pipelinePath(fname);

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
	FFGLPluginList& ffglplugins = m_ffglpipeline[pipenum].m_pluginlist;
	for (FFGLPluginList::iterator it = ffglplugins.begin(); it != ffglplugins.end(); it++) {
		FFGLPluginInstance* p = *it;

		// Take off the pipeline# part of the viztag, and save it as a "vtag".
		// When the pipeline is read back in, the pipeline# will be added back.
		std::string vtag = p->viztag().substr(2);

		f << sep;
		f << "\t{\n";
		f << "\t\t\"vtag\": \"" + vtag + "\",\n";
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
	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		FFGLPipeline& pipeline = m_ffglpipeline[pipenum];
		f << sep;
		f << "\t{\n";
		f << "\t\t\"pipeline\": \"" << pipeline.m_name << "\",\n";
		f << "\t\t\"enabled\": " << m_pipeline_enabled[pipenum] << ",\n";
		f << "\t\t\"camera_enabled\": " << m_pipeline_camera_enabled[pipenum] << ",\n";
		f << "\t\t\"sidrange\": \"" << pipeline.m_sidmin << "-" << pipeline.m_sidmax << "\"\n";
		f << "\t}";
		sep = ",\n";
	}
	f << "\n\t]\n}\n";
	f.close();
}

void
FFFF::LoadPipeset(std::string pipeset)
{
	// The passed-in command-line pipeset value presides.
	if (pipeset == "") {
		// Then we look at FFFF.json for a pipeset value
		pipeset = m_pipesetname;
	}
	if (pipeset == "" ) {
		// Last is the pipeset value in the saved state
		pipeset = m_state->pipeset();
	}
	if (pipeset == "") {
		pipeset = "default";
	}

	std::string fname = pipeset;
	if (!NosuchEndsWith(fname, ".json")) {
		fname += ".json";
	}
	std::string fpath = VizConfigPath("pipesets",fname);
	std::string err;

	DEBUGPRINT(("LoadPipeset %s path=%s", pipeset.c_str(), fpath.c_str()));

	bool exists = NosuchFileExists(fpath);
	cJSON* json;
	if (!exists) {
		// make a copy of the current one
		if (NosuchFileExists(m_pipesetpath)) {
			NosuchFileCopy(m_pipesetpath, fpath);
		}
		else {
			throw NosuchException("No such file: fpath=%s", fpath.c_str());
		}
	}

	json = jsonReadFile(fpath,err);
	if (!json) {
		std::string err = NosuchSnprintf("Unable to parse file!? fpath=%s", fpath.c_str());
		throw NosuchException(err.c_str());
	}
	loadPipesetJson(json);
	m_pipesetname = pipeset;
	m_pipesetpath = fpath;

	m_state->set_pipeset(pipeset);
	m_state->save();

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

		pipeline.clear();

		cJSON *p = cJSON_GetArrayItem(pipeset, pipenum);
		NosuchAssert(p);
		if (p->type != cJSON_Object) {
			throw NosuchException("Hey! Item #%d in pipeset isn't an object?", pipenum);
		}

		std::string name = jsonNeedString(p, "pipeline");
		bool enabled = jsonNeedBool(p, "enabled");
		bool camera_enabled = jsonNeedBool(p, "camera_enabled", false);
		std::string sidrange = jsonNeedString(p, "sidrange");
		std::string spriteparams = jsonNeedString(p, "spriteparams", "");
		if (spriteparams == "") {
			spriteparams = name;
		}

		int sidmin, sidmax;
		if (sscanf(sidrange.c_str(), "%d-%d", &sidmin, &sidmax) != 2) {
			throw NosuchException("Invalid format of sidrange: %s", sidrange.c_str());
		}

		std::string fpath = pipelinePath(name);

		// The default instance name of a pipe is the pipenum
		std::string piname = NosuchSnprintf("%d", pipenum);

		pipeline.load(piname, name, fpath, sidmin, sidmax);

		m_pipeline_enabled[pipenum] = enabled;
		m_pipeline_camera_enabled[pipenum] = camera_enabled;

		pipeline.setEnableInput(enabled);
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

std::string
FFFF::copyFile(cJSON *params, PathGenerator pathgen, const char* id)
{
	std::string fromfile =  jsonNeedString(params,"fromfile");
	std::string tofile =  jsonNeedString(params,"tofile");
	DEBUGPRINT(("Making copy %s to %s",fromfile.c_str(),tofile.c_str()));
	std::string frompath = pathgen(fromfile);
	std::string topath = pathgen(tofile);
	bool r = NosuchFileCopy(frompath, topath);
	if (!r) {
		return jsonError(-32000,"Unable to copy!?",id);
	}
	return jsonOK(id);
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
			"ffglparamset(viztag,param,val);"
			"ffglparamget(viztag,param);"
			"ffglparamlist(plugin);"
			"ffglparamvals(viztag);"
			"ffglpipeline;"
			"ffglplugins;"
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

	if (meth == "get_pipeset") {
		return jsonStringResult(m_pipesetname, id);
	}
	if ( meth == "load_pipeset" ) {
		std::string fname =  jsonNeedString(params,"name");
		LoadPipeset(fname);
		return jsonOK(id);
	}
	if ( meth == "save_pipeset" ) {
		std::string fname =  jsonNeedString(params,"name");
		savePipeset(fname);
		return jsonOK(id);
	}
	if ( meth == "set_autoload" ) {
		// controls autoloading of pipelines when their config files change
		m_autoload = jsonNeedBool(params, "onoff", true);
		return jsonOK(id);
	}
	if ( meth == "get_autoload" ) {
		return jsonIntResult(m_autoload,id);
	}
	if ( meth == "set_autosave" ) {
		// controls autosaving of pipeset
		m_autosave = jsonNeedBool(params, "onoff", true);
		return jsonOK(id);
	}
	if ( meth == "get_autosave" ) {
		return jsonIntResult(m_autosave,id);
	}
	if ( meth == "set_showfps" ) {
		m_showfps = jsonNeedBool(params, "onoff", true);
		return jsonOK(id);
	}
	if ( meth == "get_showfps" ) {
		return jsonIntResult(m_showfps,id);
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

	if (pipenum < 0) {
		throw NosuchException("No pipenum parameter provided");
	}
	if (pipenum >= NPIPELINES) {
		throw NosuchException("pipenum value is too large");
	}

	// ALL apis from here on down can assume that pipenum is >=0
	// and that ppipeline is non-NULL

	FFGLPipeline* ppipeline = &m_ffglpipeline[pipenum];

	if (meth == "add" || meth == "ffgladd") {
		std::string plugin = jsonNeedString(params, "plugin");
		bool autoenable = jsonNeedBool(params, "autoenable", true);
		bool moveable = jsonNeedBool(params, "moveable", true);
		cJSON* pparams = jsonGetArray(params, "params");
		FFGLPluginInstance* pi = ppipeline->addToPipeline(plugin, viztag, autoenable, pparams);
		if (!pi) {
			throw NosuchException("Unable to add plugin '%s'", plugin.c_str());
		}
		pi->setMoveable(moveable);
		return jsonStringResult(viztag, id);
	}

	if (meth == "ffglenable" || meth == "enable" ) {
		// std::string viztag = jsonNeedString(params, "viztag");
		bool onoff = jsonNeedBool(params, "onoff");
		FFGLPluginInstance* pi = ppipeline->find_plugin(viztag,true);   // throws an exception if it doesn't exist
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
		FFGLPluginInstance* pi = ppipeline->find_plugin(viztag,true);   // throws an exception if it doesn't exist
		pi->setMoveable(onoff);
		return jsonOK(id);
	}
	if (meth == "about") {
		// std::string inst = jsonNeedString(params, "viztag");
		FFGLPluginInstance* pgl = m_ffglpipeline[pipenum].find_plugin(viztag);
		if (pgl != NULL) {
			FFGLPluginDef* def = pgl->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->About, id);
		}
		throw NosuchException("No such viztag: %s",viztag.c_str());
	}
	if (meth == "description") {
		// std::string viztag = jsonNeedString(params, "viztag");
		FFGLPluginInstance* pgl = m_ffglpipeline[pipenum].find_plugin(viztag);
		if (pgl != NULL) {
			FFGLPluginDef* def = pgl->plugindef();
			return jsonStringResult(def->GetExtendedInfo()->Description, id);
		}
		throw NosuchException("No such viztag: %s",viztag.c_str());
	}
	if ( meth == "delete" ) {
		// It's okay if it doesn't exist
		ppipeline->delete_plugin(viztag);
		return jsonOK(id);
	}
	if ( meth == "moveplugin" ) {
		// It's okay if it doesn't exist
		int n = jsonNeedInt(params, "n", 1);
		ppipeline->moveplugin(viztag,n);
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
		ppipeline->clear();
		return jsonOK(id);
	}
	if (meth == "ffglparamset" || meth == "set") {

		// std::string viztag = jsonNeedString(params, "viztag");
		std::string param = jsonNeedString(params, "param");
		cJSON *jv = cJSON_GetObjectItem(params, "val");

		FFGLPluginInstance* pi = m_ffglpipeline[pipenum].find_plugin(viztag);
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
		FFGLPluginInstance* pi = m_ffglpipeline[pipenum].find_plugin(viztag);
		FFGLParameterDef* pd = pi->plugindef()->findparamdef(param);
		if ( pd == NULL ) {
			throw NosuchException("No parameter '%s' in viztag '%s'",param.c_str(),viztag.c_str());
		}
		return pi->getParamJsonResult(pd, pi, id);
	}
	if ( meth == "ffglplugins" ) {
		return jsonResult(FFGLList().c_str(),id);
	}
	if ( meth == "ffglpipeline" ) {
		bool only_enabled = jsonNeedBool(params,"only_enabled",false);  // default is to list everything
		return jsonResult(ppipeline->pipelineList(only_enabled).c_str(),id);
	}
	if ( meth == "ffglparamlist" ) {
		return FFGLParamList(jsonNeedString(params,"plugin"),id);
	}
	if ( meth == "ffglparamvals" ) {
		// std::string viztag = jsonNeedString(params,"viztag","");
		FFGLPluginInstance* pi = ppipeline->find_plugin(viztag,true);   // throws an exception if it doesn't exist
		std::string r = FFGLParamVals(pi);
		return jsonResult(r,id);
	}
	if ( meth == "save_pipeline" ) {
		std::string fname =  jsonNeedString(params,"name");
		return savePipeline(pipenum,fname,id);
	}
	if ( meth == "set_enablepipeline" ) {
		bool enabled = jsonNeedBool(params, "onoff", true);
		m_pipeline_enabled[pipenum] = enabled;
		ppipeline->setEnableInput(enabled);
		return jsonOK(id);
	}
	if ( meth == "get_enablepipeline" ) {
		return jsonIntResult(m_pipeline_enabled[pipenum], id);
	}
	if ( meth == "set_enablecamera" ) {
		m_pipeline_camera_enabled[pipenum] = jsonNeedBool(params, "onoff", true);
		return jsonOK(id);
	}
	if ( meth == "get_enablecamera" ) {
		return jsonIntResult(m_pipeline_camera_enabled[pipenum], id);
	}
	if (meth == "pipelinename") {
		return jsonStringResult(ppipeline->m_name, id);
	}
	if (meth == "pipesetname") {
		return jsonStringResult(m_pipesetname, id);
	}
	if ( meth == "load_pipeline" ) {
		std::string fname =  jsonNeedString(params,"name");
		// Keep the same sidmin/sidmax
		std::string fpath = pipelinePath(fname);
		ppipeline->load(ppipeline->m_piname, fname, fpath,
			ppipeline->m_sidmin, ppipeline->m_sidmax);
		ppipeline->setEnableInput(m_pipeline_enabled[pipenum]);
		return jsonOK(id);
	}

	if ( meth == "copy_sprite" ) {
		return copyFile(params, SpriteVizParamsPath, id);
	}
	if ( meth == "copy_midi" ) {
		return copyFile(params, MidiVizParamsPath, id);
	}
	if ( meth == "copy_pipeline" ) {
		return copyFile(params, pipelinePath, id);
	}
	if ( meth == "copy_pipeset" ) {
		return copyFile(params, pipesetPath, id);
	}
	if (meth == "get_sidrange") {
		std::string s = NosuchSnprintf("%d-%d", ppipeline->m_sidmin, ppipeline->m_sidmax);
		return jsonStringResult(s, id);
	}
	if (meth == "set_sidrange") {
		int sidmin, sidmax;
		std::string range =  jsonNeedString(params,"sidrange");
		if (sscanf(range.c_str(), "%d-%d", &sidmin, &sidmax) != 2) {
			return jsonError(-32000,"Bad format of sidrange value",id);
		}
		// ppipeline->m_sidmin = sidmin;
		// ppipeline->m_sidmax = sidmax;
		ppipeline->setSidrange(sidmin,sidmax);
		return jsonOK(id);
	}
	if (meth == "get_spriteparams") {
		return jsonStringResult(ppipeline->m_spriteparams, id);
	}
	if (meth == "set_spriteparams") {
		ppipeline->m_spriteparams = jsonNeedString(params, "name");
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

