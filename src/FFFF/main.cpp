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


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "glstuff.h"

#include "resource.h"

#include "FFFF.h"
#include "NosuchJSON.h"
#include "NosuchMidi.h"

#include "stdint.h"
#include "FFGLPlugin.h"
#include "FF10Plugin.h"
#include "ffffutil.h"
#include "Timer.h"
#include "XGetopt.h"

FFFF* F;

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

int ffffMain(std::string pipeset, bool fullscreen)
{
	NosuchDebugInit();
	NosuchDebugSetLogDirFile(VizPath("log"), "ffff.debug");
	DEBUGPRINT(("FFFF is starting."));

	if (NosuchNetworkInit()) {
		DEBUGPRINT(("Unable to initialize networking?"));
		exit(EXIT_FAILURE);
	}

	glfwSetTime(0.0);

	std::string err;
	std::string fname = VizConfigPath("FFFF.json");
	cJSON* config = jsonReadFile(fname, err);
	if (!config) {
		DEBUGPRINT(("Hey!  Error in reading JSON from %s!  err=%s", fname.c_str(), err.c_str()));
		exit(EXIT_FAILURE);
	}

	F = new FFFF(config);

	jsonSetDebugConfig(config);

	// Allow the config to override the default paths for these
	std::string ff10path = jsonNeedString(config, "ff10path", "ff10plugins");
	std::string ffglpath = jsonNeedString(config, "ffglpath", "ffglplugins");

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		DEBUGPRINT(("glfwInit failed!?"));
		exit(EXIT_FAILURE);
	}

	try {
		CATCH_NULL_POINTERS;

		F->StartStuff();

		F->CreateWindows();

		F->InitCamera();

		F->loadAllPluginDefs(ff10path, ffglpath);

		F->loadPipeset(pipeset);

		F->spoutInit();
	}
	catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException while initializing!! - %s", e.message());
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured while loading Pipeline!?");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

#ifdef DUMPOBJECTS
	_CrtMemState s0;
	_CrtMemCheckpoint(&s0);
#endif

	// int count = 0;
	while (!glfwWindowShouldClose(F->window))
	{
		F->checkAndExecuteJSON();

		if (F->hidden == false) {

			F->drawWindowPipelines();
			F->drawWindowFinish();

			F->drawPrefixPipelines();
			F->drawPrefixFinish();
		}

		glfwPollEvents();

		F->CheckFPS();
		F->CheckAutoload();
	}

	for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
		F->clearPipeline(pipenum);
	}

	F->StopStuff();

#ifdef _DEBUG
#ifdef DUMPOBJECTS
	_CrtMemDumpAllObjectsSince( &s0 );
#endif
#endif

	// F->clearPipeline();

    glfwDestroyWindow(F->window);
    glfwDestroyWindow(F->preview);

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

	std::string pipeset = "";

	for (unsigned int n = 0; n < args.size(); n++) {
		std::string arg = args[n];
		if (arg.size() > 1 && arg[0] == '-') {
			switch (arg[1]) {
			case 'f':
				fullscreen = true;
				break;
			case 'p':
				if ((n + 1) < args.size()) {
					pipeset = args[n + 1];
					n++;
				}
				break;
			}
		}
	}

	try {
		CATCH_NULL_POINTERS;
		r = ffffMain(pipeset,fullscreen);
	} catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException in ffffMain!! - %s",e.message());
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured in ffffMain!?");
	}
	return r;
}
