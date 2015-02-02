//
// This Vizlet example just draws a square.
//
#include "Vizlet.h"

class VizExample2 : public Vizlet
{
public:
	VizExample2();
	~VizExample2();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
};

static CFFGLPluginInfo PluginInfo ( 
	VizExample2::CreateInstance,	// Create method
	"VZX2",		// Plugin unique ID
	"VizExample2",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizExample2: an example vizlet",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizExample2"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizExample2::VizExample2() : Vizlet() {
}

VizExample2::~VizExample2() {
	DEBUGPRINT1(("Destructor for VizExample2"));
}

DWORD __stdcall VizExample2::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizExample2();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizExample2::processCursor(VizCursor* c, int downdragup) {

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

std::string VizExample2::processJson(std::string meth, cJSON *json, const char *id) {

	if (meth == "apis") {
		return jsonStringResult("newsprite",id);
	}

	if ( meth == "newsprite" ) {
		double x = ((double)rand())/RAND_MAX;
		double y = ((double)rand())/RAND_MAX;
		double z = 0.5;
		makeAndAddVizSprite(defaultParams(),NosuchPos(x,y,z));
		return jsonOK(id);
	}

	throw NosuchException("VizExample2 - Unrecognized method '%s'",meth.c_str());
}

void VizExample2::processMidiInput(MidiMsg* m) {
	defaultMidiVizSprite(m);
}

void VizExample2::processMidiOutput(MidiMsg* m) {
	defaultMidiVizSprite(m);
}

bool VizExample2::processDraw() {
	DrawVizSprites();
	return true;
}