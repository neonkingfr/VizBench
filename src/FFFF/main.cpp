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
#include "ffutil.h"
#include "FFGLPlugin.h"
#include "Timer.h"

#include "Python.h"

FFFF* F;

int CV_interp = CV_INTER_NN;  // or CV_INTER_LINEAR

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

int ffffMain(std::string config)
{
	NosuchDebugInit();
   	NosuchDebugSetLogDirFile(ManifoldLogDir(),"ffff.debug");
	DEBUGPRINT(("FFFF is starting."));

	if ( NosuchNetworkInit() ) {
		DEBUGPRINT(("Unable to initialize networking?"));
		return false;
	}  

    glfwSetTime(0.0);

	std::string err;
	std::string fname = ManifoldPath("config/FFFF.json");
	cJSON* j = jsonReadFile(fname,err);
	if ( !j ) {
		DEBUGPRINT(("Hey!  Error in reading JSON from %s!  err=%s",fname.c_str(),err.c_str()));
		return false;
	}

	jsonSetDebugConfig(j);

	// int httpport = jsonNeedInt(j,"httpport");
	std::string initialconfig = jsonNeedString(j,"initialconfig");
	if (config != "" ) {
		if (initialconfig != "") {
			DEBUGPRINT(("command-line config (%s) is overriding initialconfig (%s) in config file", config.c_str(),initialconfig.c_str()));
		}
		initialconfig = config;
	}

	// Allow the config to override the default paths for these
	std::string ffpath = jsonNeedString(j,"ffpath",ManifoldPath("ffplugins"));
	std::string ffglpath = jsonNeedString(j,"ffglpath",ManifoldPath("ffglplugins"));

	std::string toreplace = "$VIZBENCH";
	std::string manifold = ManifoldPath("");
	int pos;
	while ( (pos=ffglpath.find(toreplace)) != ffglpath.npos) {
		ffglpath.replace(pos, toreplace.length(), manifold);
	}

	int camera_index = jsonNeedInt(j,"camera",-1);  // -1 for no camera, 0+ for camera

	F = new FFFF();
	F->SetShowFPS(jsonNeedInt(j,"showfps",0)?true:false);

	F->StartStuff();

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

	GLFWmonitor* monitor;
	bool fullscreen = false;
	if ( fullscreen ) {
		monitor = glfwGetPrimaryMonitor();
	} else {
		monitor = NULL;
	}

	int window_width = 800; // 640;
	int window_height = 600; // 480;
	int ffgl_width = window_width;
	int ffgl_height = window_height;

	int window_x = jsonNeedInt(j,"window_x",100);
	int window_y = jsonNeedInt(j,"window_y",100);

    F->window = glfwCreateWindow(window_width, window_height, "FFFF", monitor, NULL);
    if ( F->window == NULL ) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(F->window);

    glfwSetKeyCallback(F->window, key_callback);

	F->loadPluginDefs(ffpath,ffglpath,ffgl_width,ffgl_height);

	bool use_camera = FALSE;
	if ( camera_index < 0 ) {
		DEBUGPRINT(("Camera is disabled (camera < 0)"));
	} else if ( ! F->initCamera(camera_index) ) {
			DEBUGPRINT(("Camera is disabled (init failed)"));
	} else {
		use_camera = TRUE;
	}

	try {
		CATCH_NULL_POINTERS;
		F->loadPipeline(initialconfig);
	} catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException while loading Pipeline!! - %s",e.message());
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured while loading Pipeline!?");
	}

#ifdef _DEBUG
#ifdef DUMPOBJECTS
	_CrtMemState s0;
	_CrtMemCheckpoint(&s0);
#endif
#endif

	glfwSetWindowPos(F->window, window_x, window_y);
	glfwSetWindowSize(F->window, window_width, window_height);
	glfwIconifyWindow(F->window);
	glfwShowWindow(F->window);

	// int count = 0;
    while (!glfwWindowShouldClose(F->window))
    {
		F->checkAndExecuteJSON();

		if ( F->hidden == false ) {
	        int width, height;
	        glfwGetFramebufferSize(F->window, &width, &height);

			F->doOneFrame(use_camera,width,height);

	        glfwSwapBuffers(F->window);
		}

        glfwPollEvents();

		F->CheckFPS();
    }

	F->clearPipeline();

	F->StopStuff();

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
	
	std::string config = "";
	if (__argc > 1) {
		config = __argv[1];
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
