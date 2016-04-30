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

#include "drawtext.h"

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

	// Remove shell expansion, because I want things to be the same between the
	// Vizbench and Vizlets repositories.  If anything, a general environment
	// variable subsitution mechanism should be put here.

	int camera_index = jsonNeedInt(config, "camera", -1);  // -1 for no camera, 0+ for camera

	int window_monitor = jsonNeedInt(config, "window_monitor", 800);
	int window_width = jsonNeedInt(config, "window_width", 800);
	int window_height = jsonNeedInt(config, "window_height", 600);
	int window_x = jsonNeedInt(config, "window_x", 0);
	int window_y = jsonNeedInt(config, "window_y", 0);

	int preview_monitor = jsonNeedInt(config, "preview_monitor", 800);
	int preview_width = jsonNeedInt(config, "preview_width", 800);
	int preview_height = jsonNeedInt(config, "preview_height", 600);
	int preview_x = jsonNeedInt(config, "preview_x", 0);
	int preview_y = jsonNeedInt(config, "preview_y", 0);

	FfffOutputPrefix = jsonNeedString(config, "outputprefix", "");
	FfffOutputFPS = jsonNeedInt(config, "outputfps", 30);

	// The command-line pipeset value presides.
	if (pipeset == "") {
		// Then we look at FFFF.json for a pipeset value
		pipeset = jsonNeedString(config, "pipeset", "");
		if (pipeset == "") {
			// Last is the pipeset value in the saved state
			pipeset = F->m_state->pipeset();
		}
	}
	if (pipeset == "") {
		pipeset = "default";
	}

	if (!F->StartStuff()) {
		DEBUGPRINT(("StartStuff failed!?"));
		exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		DEBUGPRINT(("glfwInit failed!?"));
		exit(EXIT_FAILURE);
	}

	GLFWmonitor* monitor;
	int nmonitors;
	GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);

	for (int n = 0; n < nmonitors; n++) {
		const char* nm = glfwGetMonitorName(monitors[n]);
		const GLFWvidmode* mode = glfwGetVideoMode(monitors[n]);
		DEBUGPRINT(("GLFW Monitor %d - name=%s  width=%d height=%d refresh=%d",n,nm?nm:"NULL?",mode->width,mode->height,mode->refreshRate));
	}

	if (window_monitor >= nmonitors) {
		DEBUGPRINT(("window_monitor value is too big (%d), there are only %d monitors!", window_monitor, nmonitors));
		window_monitor = 0;
	}

	if (fullscreen) {
		if (nmonitors == 0) {
			DEBUGPRINT(("No monitors!?"));
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		monitor = monitors[nmonitors - 1]; // XXX - this is probably wrong
		// monitor = glfwGetPrimaryMonitor();
	}
	else {
		monitor = NULL;
	}

	glfwWindowHint(GLFW_DECORATED, GL_FALSE);

	F->window = glfwCreateWindow(window_width, window_height, "FFFF", monitor, NULL);
	if (F->window == NULL) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetWindowPos(F->window, window_x, window_y);
	glfwSetWindowSize(F->window, window_width, window_height);
	glfwShowWindow(F->window);

	// MAKE PREVIEW WINDOW, be sure to share resources with main window
	F->preview = glfwCreateWindow(preview_width, preview_height, "Preview", monitor, F->window);
	if (F->preview == NULL) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetWindowPos(F->preview, preview_x, preview_y);
	glfwSetWindowSize(F->preview, preview_width, preview_height);
	glfwShowWindow(F->preview);
	// END MAKE PREVIEW WINDOW

	glfwMakeContextCurrent(F->window);

	glfwSetKeyCallback(F->window, key_callback);

	if ( ! F->InitGlExtensions() ) {
		DEBUGPRINT(("InitGlExtensions failed!?"));
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	const char *fontname = "c:\\windows\\fonts\\poorich.ttf";
	int fontsize = 48;
	struct dtx_font *font = dtx_open_font(fontname, fontsize);
	if (font == NULL) {
		DEBUGPRINT(("Unable to load font '%s'!?",fontname));
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	F->loadAllPluginDefs(ff10path, ffglpath, window_width, window_height);

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
		F->loadPipeset(pipeset);
	}
	catch (NosuchException& e) {
		NosuchErrorOutput("NosuchException while loading Pipeset!! - %s", e.message());
	}
	catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		NosuchErrorOutput("Some other kind of exception occured while loading Pipeline!?");
	}

#ifdef DUMPOBJECTS
	_CrtMemState s0;
	_CrtMemCheckpoint(&s0);
#endif

	if ( ! F->spoutInit(window_width,window_height) ) {
		DEBUGPRINT(("spoutInit failed!?"));
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// int count = 0;
	while (!glfwWindowShouldClose(F->window))
	{
		F->checkAndExecuteJSON();

		if (F->hidden == false) {
			int width, height;

			// MAIN WINDOW
			glfwMakeContextCurrent(F->window);
			glfwGetFramebufferSize(F->window, &width, &height);

			for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
				if (F->isPipelineEnabled(pipenum)) {
					F->doCameraAndFF10Pipeline(pipenum, use_camera, mapTexture.Handle);
					F->doPipeline(pipenum, width, height);
				}
			}
			glClearColor(0, 0, 0, 0);
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
				if (!F->isPipelineEnabled(pipenum)) {
					continue;
				}
				FFGLPipeline& pipeline = F->Pipeline(pipenum);

				pipeline.paintTexture();
			}

			// Draw a rectangle just to show that we're alive.
			glColor4f(0.0, 1.0, 0.0, 1.0);
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
			glScalef(0.002, 0.004, 1.0);
			dtx_use_font(font, fontsize);
			dtx_string("Space Puddle");
			glPopMatrix();

			F->sendSpout(width, height);

			glfwSwapBuffers(F->window);

			///////////////////////////////////////
			// PREVIEW WINDOW
			glfwMakeContextCurrent(F->preview);
			glPushMatrix();

			glClearColor(0, 0.0, 0, 0);
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			for (int pipenum = 0; pipenum < NPIPELINES; pipenum++) {
				if (!F->isPipelineEnabled(pipenum)) {
					continue;
				}
				FFGLPipeline& pipeline = F->Pipeline(pipenum);

				pipeline.paintTexture();
			}

			// Draw a rectangle just to show that we're alive.
			glColor4f(1.0, 0.0, 0.0, 1.0);
			glLineWidth((GLfloat)2.0f);
			glBegin(GL_LINE_LOOP);
			glVertex3f(0.1f, 0.1f, 0.0f);	// Top Left
			glVertex3f(0.1f, 0.9f, 0.0f);	// Top Right
			glVertex3f(0.9f, 0.9f, 0.0f);	// Bottom Right
			glVertex3f(0.9f, 0.1f, 0.0f);	// Bottom Left
			glEnd();

			glPopMatrix();
			glfwSwapBuffers(F->preview);
			// END PREVIEW WINDOW
			///////////////////////////////////////
		}

		glfwPollEvents();

		F->CheckFPS();
		F->CheckAutoload();
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
