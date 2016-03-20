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

// #include "mmtt_sharedmem.h"
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
// #include "mmsystem.h"

#include "resource.h"

#include "stdint.h"
#include "FFGLPlugin.h"
#include "FF10Plugin.h"
#include "ffffutil.h"
#include "Timer.h"
#include "XGetopt.h"

#include "spout.h"

// #include "Python.h"

FFFF* F;
void tjtdebug();

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

char* FfffOutput = NULL;
FILE* FfffOutputFile = NULL;
std::string FfffOutputPrefix;
int FfffOutputFPS;

int ffffMain(std::string pipelinename[4], bool fullscreen)
{
	NosuchDebugInit();
	NosuchDebugSetLogDirFile(VizPath("log"), "ffff.debug");
	DEBUGPRINT(("FFFF is starting."));

	if (NosuchNetworkInit()) {
		DEBUGPRINT(("Unable to initialize networking?"));
		return false;
	}

	glfwSetTime(0.0);

	std::string err;
	std::string fname = VizConfigPath("FFFF.json");
	cJSON* config = jsonReadFile(fname, err);
	if (!config) {
		DEBUGPRINT(("Hey!  Error in reading JSON from %s!  err=%s", fname.c_str(), err.c_str()));
		return false;
	}

	jsonSetDebugConfig(config);

	// Allow the config to override the default paths for these
	std::string ff10path = jsonNeedString(config, "ff10path", "ff10plugins");
	std::string ffglpath = jsonNeedString(config, "ffglpath", "ffglplugins");

	// Remove shell expansion, because I want things to be the same between the
	// Vizbench and Vizlets repositories.  If anything, a general environment
	// variable subsitution mechanism should be put here.

	int camera_index = jsonNeedInt(config, "camera", -1);  // -1 for no camera, 0+ for camera
	int window_width = jsonNeedInt(config, "window_width", 800);
	int window_height = jsonNeedInt(config, "window_height", 600);
	int window_x = jsonNeedInt(config, "window_x", 0);
	int window_y = jsonNeedInt(config, "window_y", 0);
	FfffOutputPrefix = jsonNeedString(config, "outputprefix", "");
	FfffOutputFPS = jsonNeedInt(config, "outputfps", 30);

	F = new FFFF(config);

	if (!F->StartStuff()) {
		exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWmonitor* monitor;
	int nmonitors;
	GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);

	if (fullscreen) {
		if (nmonitors == 0) {
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		monitor = monitors[nmonitors - 1];
		// monitor = glfwGetPrimaryMonitor();
	}
	else {
		monitor = NULL;
	}

	int ffgl_width = window_width;
	int ffgl_height = window_height;

	glfwWindowHint(GLFW_DECORATED, GL_FALSE);

	F->window = glfwCreateWindow(window_width, window_height, "FFFF", monitor, NULL);
	if (F->window == NULL) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(F->window);

	glfwSetKeyCallback(F->window, key_callback);

	F->loadAllPluginDefs(ff10path, ffglpath, ffgl_width, ffgl_height);

	// glfwShowWindow(F->window);

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
		for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
			if (pipelinename[pipenum] != "") {
				F->loadPipeline(pipenum, pipelinename[pipenum], true);
			}
		}
	}
	catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException while loading Pipeline!! - %s", e.message());
	}
	catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured while loading Pipeline!?");
	}

#ifdef DUMPOBJECTS
	_CrtMemState s0;
	_CrtMemCheckpoint(&s0);
#endif

	glfwSetWindowPos(F->window, window_x, window_y);
	glfwSetWindowSize(F->window, window_width, window_height);
	// glfwIconifyWindow(F->window);
	glfwShowWindow(F->window);

	if (F->m_spout) {
		strcpy(F->m_sendername, "FFFF");
		F->m_spoutsender = new SpoutSender;
		bool b = F->m_spoutsender->CreateSender(F->m_sendername,window_width,window_height);
		if (!b) {
			DEBUGPRINT(("Unable to CreateSender for Spout!?"));
			NosuchErrorOutput("Unable to CreateSender for Spout!?");
			delete F->m_spoutsender;
			F->m_spoutsender = NULL;
		}
	}

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

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		F->clearPipeline(pipenum);
	}

	F->StopStuff();

	if ( FfffOutputFile ) {
		_pclose(FfffOutputFile);
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

	bool fullscreen = false;
	std::string pipeline[4];

	std::string cmdline = std::string(szCmdLine);
	std::vector<std::string> args = NosuchSplitOnAnyChar(cmdline, " \t\n");

	int pipenum = 0;

	for (unsigned int n = 0; n < args.size(); n++) {
		std::string arg = args[n];
		if (arg.size() > 1 && arg[0] == '-') {
			switch (arg[1]) {
			case 'f':
				fullscreen = true;
				break;
			case 'c':
				if ((n + 1) < args.size()) {
					pipeline[pipenum] = args[n + 1];
					n++;
				}
				break;
			}
		}
	}

	if (pipeline[0] == "" ) {
		std::string msg = NosuchSnprintf("Usage: %s [-f] -c {pipeline}\n",__argv[0]);
		MessageBoxA(NULL,msg.c_str(),"FFFF",MB_OK);
		return r;
	}
	try {
		CATCH_NULL_POINTERS;
		r = ffffMain(pipeline,fullscreen);
	} catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException in ffffMain!! - %s",e.message());
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured in ffffMain!?");
	}
	return r;
}
