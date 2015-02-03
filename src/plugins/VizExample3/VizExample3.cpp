#include "Vizlet.h"
#include "VizExample3.h"

static CFFGLPluginInfo PluginInfo(
	VizExample3::CreateInstance,	// Create method
	"VZDR",		// Plugin unique ID
	"VizExample3",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizExample3: draw sprites with TUIO input",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
	);

std::string vizlet_name() { return "VizExample3"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizExample3::VizExample3() : Vizlet() {
	m_params = getAllVizParams(VizParamPath("default"));
}

VizExample3::~VizExample3() {
}

DWORD __stdcall VizExample3::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizExample3();
	return (*ppInstance != NULL) ? FF_SUCCESS : FF_FAIL;
}

void VizExample3::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	makeAndAddVizSprite(m_params, c->pos);
}

std::string VizExample3::processJson(std::string meth, cJSON *json, const char *id) {
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

	throw NosuchException("VizExample3 - Unrecognized method '%s'", meth.c_str());
}

bool VizExample3::processDraw() {
	// OpenGL calls here
	DrawVizSprites();
	return true;
}