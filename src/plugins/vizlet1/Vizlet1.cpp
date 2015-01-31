#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "Vizlet1.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	Vizlet1::CreateInstance,	// Create method
	"NSV1",		// Plugin unique ID
	"Vizlet1",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Vizlet1: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "Vizlet1"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
// void vizlet_setdll(std::string dll) { }

Vizlet1::Vizlet1() : Vizlet() {
}

Vizlet1::~Vizlet1() {
	DEBUGPRINT1(("Destructor for Vizlet1"));
}

DWORD __stdcall Vizlet1::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new Vizlet1();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void Vizlet1::processAdvanceClickTo(int clk) {
	DEBUGPRINT(("processAdvanceClickTo clk=%d",clk));
}

void Vizlet1::processCursor(VizCursor* c, int downdragup) {

	switch(downdragup) {
	case CURSOR_DOWN:
		makeAndAddVizSprite(defaultParams(), c->pos);
		break;
	case CURSOR_DRAG:
		makeAndAddVizSprite(defaultParams(), c->pos);
		break;
	case CURSOR_UP:
		break;
	}
}

std::string Vizlet1::processJson(std::string meth, cJSON *json, const char *id) {

	if ( meth == "ping" ) {
		return jsonOK(id);
	}
	if ( meth == "newsprite" ) {
		double x = ((double)rand())/RAND_MAX;
		double y = ((double)rand())/RAND_MAX;
		double z = 0.5;
		makeAndAddVizSprite(defaultParams(),NosuchPos(x,y,z));
		return jsonOK(id);
	}

	throw NosuchException("Vizlet1 - Unrecognized method '%s'",meth.c_str());
}

void Vizlet1::processMidiInput(MidiMsg* m) {
	defaultMidiVizSprite(m);
}

void Vizlet1::processMidiOutput(MidiMsg* m) {
	defaultMidiVizSprite(m);
}

bool Vizlet1::processDraw() {
	DrawVizSprites();
	return true;
}