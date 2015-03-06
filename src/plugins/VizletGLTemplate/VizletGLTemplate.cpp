#include "Vizlet.h"
#include "VizletGLTemplate.h"

static CFFGLPluginInfo PluginInfo ( 
	VizletGLTemplate::CreateInstance,	// Create method
	"VZID",		// Plugin unique ID
	"VizletGLTemplate",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizletGLTemplate: a sample vizlet",			// description
	"by Tim Thompson - me@timthompson.com" 		// About
);

std::string vizlet_name() { return "VizletGLTemplate"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizletGLTemplate::VizletGLTemplate() : Vizlet() {
}

VizletGLTemplate::~VizletGLTemplate() {
}

DWORD __stdcall VizletGLTemplate::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizletGLTemplate();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizletGLTemplate::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string VizletGLTemplate::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("VizletGLTemplate - Unrecognized method '%s'",meth.c_str());
}

void VizletGLTemplate::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizletGLTemplate::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizletGLTemplate::processDraw() {
	// OpenGL calls here
	return true;
}

void VizletGLTemplate::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}