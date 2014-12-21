#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "VizletTemplate.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	VizletTemplate::CreateInstance,	// Create method
	"VZID",		// Plugin unique ID
	"VizletTemplate",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizletTemplate: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizletTemplate"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizletTemplate::VizletTemplate() : Vizlet() {
}

VizletTemplate::~VizletTemplate() {
}

DWORD __stdcall VizletTemplate::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizletTemplate();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizletTemplate::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string VizletTemplate::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("VizletTemplate - Unrecognized method '%s'",meth.c_str());
}

void VizletTemplate::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizletTemplate::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizletTemplate::processDraw() {
	// OpenGL calls here
	return true;
}

void VizletTemplate::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}