#include "Vizlet.h"
#include "VizDraw.h"

static CFFGLPluginInfo PluginInfo(
	VizDraw::CreateInstance,	// Create method
	"VZDR",		// Plugin unique ID
	"VizDraw",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizDraw: draw sprites with TUIO input",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
	);

std::string vizlet_name() { return "VizDraw"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizDraw::VizDraw() : Vizlet() {
	std::string path = VizParamPath("default");
	m_params = getAllVizParams(path);
}

VizDraw::~VizDraw() {
}

DWORD __stdcall VizDraw::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizDraw();
	return (*ppInstance != NULL) ? FF_SUCCESS : FF_FAIL;
}

void VizDraw::processCursor(VizCursor* c, int downdragup) {
	VizSprite* s = makeAndAddVizSprite(m_params, c->pos);
	VizSpriteOutline* so = (VizSpriteOutline*)s;
	if (so != NULL && c->outline != NULL) {
		so->setOutline(c->outline, c->hdr);
	}
	// NO OpenGL calls here
}

std::string VizDraw::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	if (meth == "apis") {
		return jsonStringResult("set_spriteparams(paramfile)", id);
	}

	// PARAMETER "spriteparams"
	if (meth == "set_spriteparams") {
		std::string file = jsonNeedString(json, "paramfile", "");
		if (file == "") {
			return jsonError(-32000, "Bad file value", id);
		}
		std::string path = VizParamPath(file);
		AllVizParams* p = getAllVizParams(path);
		if (p) {
			m_spriteparams = path;
			m_params = p;
		}
		return jsonOK(id);
	}
	if (meth == "get_spriteparams") {
		return jsonStringResult(m_spriteparams, id);
	}

	throw NosuchException("VizDraw - Unrecognized method '%s'", meth.c_str());
}

bool VizDraw::processDraw() {
	// OpenGL calls here
	DrawVizSprites();
	return true;
}