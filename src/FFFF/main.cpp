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

#include "NosuchDebug.h"
#include "mmtt_sharedmem.h"
#include "FFFF.h"
#include "NosuchJSON.h"
#include "NosuchMidi.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gl/gl.h>
#include <gl/glu.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#include "mmsystem.h"

#include "resource.h"

#include "stdint.h"
#include "FFGLPlugin.h"
#include "FF10Plugin.h"
#include "ffffutil.h"
#include "Timer.h"

// #include "Python.h"

FFFF* F;

int 				camWidth;
int 				camHeight;

// Function prototypes.
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow );

//////////////// NEW GLFW CODE START

#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

	switch (action) {
	case GLFW_PRESS:
		F->InsertKeystroke(key,KEYSTROKE_DOWN);
		break;
	case GLFW_RELEASE:
		F->InsertKeystroke(key,KEYSTROKE_UP);
		break;
	}
}

char* SaveFrames = NULL;
char* FfmpegFile = NULL;
FILE* Ffmpeg = NULL;

int ffffMain(std::string config)
{
	SaveFrames = getenv("SAVEFRAMES");
	FfmpegFile = getenv("FFMPEGFILE");

	if (FfmpegFile) {
		// start ffmpeg telling it to expect raw rgba 720p-60hz frames
		// -i - tells it to read frames from stdin
		// XXX - should replace newest.mp4 with FFMPEGFILE value
		std::string path = VizPath("recording") + NosuchSnprintf("\\%s.mp4", FfmpegFile);

		std::string cmd = NosuchSnprintf("ffmpeg -r 60 -f rawvideo -pix_fmt bgr24 -s 1024x768 -i - "
			"-threads 0 -preset fast -y -r 60 -vf vflip %s", path.c_str());
#ifdef EXPERIMENT
		std::string cmd = NosuchSnprintf("ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s 1024x768 -i - "
			"-threads 0 -preset fast -y -r 60 -vf vflip %s", path.c_str());
#endif

//	const char* cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt bgr8 -s 1024x768 -i - "
// "-threads 0 -preset fast -y -crf 21 -vf vflip newest.mp4";

		// open pipe to ffmpeg's stdin in binary write mode
		Ffmpeg = _popen(cmd.c_str(), "wb");
	}

	NosuchDebugInit();
	NosuchDebugSetLogDirFile(VizPath("log"), "ffff.debug");
	DEBUGPRINT(("FFFF is starting."));

	if (NosuchNetworkInit()) {
		DEBUGPRINT(("Unable to initialize networking?"));
		return false;
	}

	glfwSetTime(0.0);

	std::string err;
	std::string fname = VizPath("config\\FFFF.json");
	cJSON* j = jsonReadFile(fname, err);
	if (!j) {
		DEBUGPRINT(("Hey!  Error in reading JSON from %s!  err=%s", fname.c_str(), err.c_str()));
		return false;
	}

	jsonSetDebugConfig(j);

	// Allow the config to override the default paths for these
	std::string ff10path = jsonNeedString(j, "ff10path", "ffplugins");
	std::string ffglpath = jsonNeedString(j, "ffglpath", "ffglplugins");

	// Remove shell expansion, because I want things to be the same between the
	// Vizbench and Vizlets repositories.  If anything, a general environment
	// variable subsitution mechanism should be put here.

	int camera_index = jsonNeedInt(j, "camera", -1);  // -1 for no camera, 0+ for camera

	F = new FFFF();
	F->SetShowFPS(jsonNeedInt(j, "showfps", 0) ? true : false);

	F->StartStuff();

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWmonitor* monitor;
	bool fullscreen = false;
	if (fullscreen) {
		monitor = glfwGetPrimaryMonitor();
	}
	else {
		monitor = NULL;
	}

	int window_width = jsonNeedInt(j, "window_width", 800);
	int window_height = jsonNeedInt(j, "window_height", 600);

	int window_x = jsonNeedInt(j, "window_x", 100);
	int window_y = jsonNeedInt(j, "window_y", 100);

	int ffgl_width = window_width;
	int ffgl_height = window_height;

	F->window = glfwCreateWindow(window_width, window_height, "FFFF", monitor, NULL);
	if (F->window == NULL) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(F->window);

	glfwSetKeyCallback(F->window, key_callback);

	F->loadFFPlugins(ff10path, ffglpath, ffgl_width, ffgl_height);

	bool use_camera = FALSE;
	if (camera_index < 0) {
		DEBUGPRINT(("Camera is disabled (camera < 0)"));
	}
	else if (!F->initCamera(camera_index)) {
		DEBUGPRINT(("Camera is disabled (init failed)"));
	}
	else {
		use_camera = TRUE;
	}

	try {
		CATCH_NULL_POINTERS;
		F->loadPipeline(config);
	}
	catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException while loading Pipeline!! - %s", e.message());
	}
	catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured while loading Pipeline!?");
	}


	// #define DUMPOBJECTS
#ifdef DUMPOBJECTS
	_CrtMemState s0;
	_CrtMemCheckpoint(&s0);
#endif

	glfwSetWindowPos(F->window, window_x, window_y);
	glfwSetWindowSize(F->window, window_width, window_height);
	glfwIconifyWindow(F->window);
	glfwShowWindow(F->window);

	// int count = 0;
	while (!glfwWindowShouldClose(F->window))
	{
		F->checkAndExecuteJSON();

		if (F->hidden == false) {
			int width, height;
			glfwGetFramebufferSize(F->window, &width, &height);

			F->doOneFrame(use_camera, width, height);

			glfwSwapBuffers(F->window);
		}

		glfwPollEvents();

		F->CheckFPS();
	}

	F->clearPipeline();

	F->StopStuff();

	if ( Ffmpeg ) {
		_pclose(Ffmpeg);
	}

#ifdef _DEBUG
#ifdef DUMPOBJECTS
	_CrtMemDumpAllObjectsSince( &s0 );
#endif
#endif

	// F->clearPipeline();

    glfwDestroyWindow(F->window);

    glfwTerminate();

	Pt_Stop();

	// _CrtDumpMemoryLeaks();

    exit(EXIT_SUCCESS);
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow )
{
	int r = -1;

	char pathbuff[MAX_PATH];
	HMODULE module = GetModuleHandleA(NULL);
	GetModuleFileNameA(module, pathbuff, MAX_PATH);
	std::string path = std::string(pathbuff);
	// We want to take off the final filename AND the directory.
	// This assumes that the DLL is in either a bin or ffglplugins
	// subdirectory of the main Vizpath
	size_t pos = path.find_last_of("/\\");
	if ( pos != path.npos && pos > 0 ) {
		std::string parent = path.substr(0,pos);
		pos = path.substr(0,pos-1).find_last_of("/\\");
		if ( pos != parent.npos && pos > 0) {
			SetVizPath(parent.substr(0,pos));
		}
	}

	std::string config = "";
	if (__argc > 1) {
		config = __argv[1];
	}
	else {
		std::string msg = NosuchSnprintf("Usage: %s {configname}\n",__argv[0]);
		MessageBoxA(NULL,msg.c_str(),"FFFF",MB_OK);
		return r;
	}
	try {
		CATCH_NULL_POINTERS;
		r = ffffMain(config);
	} catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException in ffffMain!! - %s",e.message());
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured in ffffMain!?");
	}
	return r;
}
