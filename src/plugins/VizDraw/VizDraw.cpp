#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "VizDraw.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo(
	VizDraw::CreateInstance,	// Create method
	"V510",		// Plugin unique ID
	"VizDraw",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizDraw: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
	);

std::string vizlet_name() { return "VizDraw"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizDraw::VizDraw() : Vizlet() {
	_params = defaultParams();
}

VizDraw::~VizDraw() {
}

DWORD __stdcall VizDraw::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizDraw();
	return (*ppInstance != NULL) ? FF_SUCCESS : FF_FAIL;
}

void VizDraw::processKeystroke(int key, int downup) {
	DEBUGPRINT(("VizDraw::processKeystroke!!  key=%d downup=%d", key, downup));
}

void VizDraw::processCursor(VizCursor* c, int downdragup) {
#if 0
	_params->shape.set("square");
	_params->filled.set(false);
	_params->sizeinitial.set(1.0);
	_params->sizefinal.set(0.1);
	_params->sizetime.set(0.5);
	_params->alphainitial.set(1.0);
	_params->alphafinal.set(0.0);
	_params->alphatime.set(0.5);
#endif
	VizSprite* s = makeAndAddVizSprite(_params, c->pos);
	VizSpriteOutline* so = (VizSpriteOutline*)s;
	if (so != NULL && c->outline != NULL) {
		so->setOutline(c->outline, c->hdr);
	}
	// NO OpenGL calls here
}

std::string VizDraw::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	if (meth == "apis") {
		return jsonStringResult("set_spriteparams(file)", id);
	}

	// PARAMETER "spriteparams"
	if (meth == "set_spriteparams") {
		std::string file = jsonNeedString(json, "file", "");
		if (file == "") {
			return jsonError(-32000, "Bad file value", id);
		}
		std::string path = VizParamPath(file);
		AllVizParams* p = getAllVizParams(path);
		if (p) {
			_spriteparams = path;
			_params = p;
		}
		return jsonOK(id);
	}
	if (meth == "get_spriteparams") {
		return jsonStringResult(_spriteparams, id);
	}

	throw NosuchException("VizDraw - Unrecognized method '%s'", meth.c_str());
}

void VizDraw::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizDraw::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool VizDraw::processDraw() {
	// OpenGL calls here
	DrawVizSprites();
	return true;
}

void VizDraw::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}