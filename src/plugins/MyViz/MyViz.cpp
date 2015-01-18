#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "MyViz.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	MyViz::CreateInstance,	// Create method
	"V067",		// Plugin unique ID
	"MyViz",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"MyViz: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "MyViz"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

MyViz::MyViz() : Vizlet() {
}

MyViz::~MyViz() {
}

DWORD __stdcall MyViz::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new MyViz();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void MyViz::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string MyViz::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("MyViz - Unrecognized method '%s'",meth.c_str());
}

void MyViz::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void MyViz::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool MyViz::processDraw() {
	// OpenGL calls here
	return true;
}

void MyViz::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}