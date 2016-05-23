#include "Vizlet.h"
#include "VizBrush.h"

static CFFGLPluginInfo PluginInfo ( 
	VizBrush::CreateInstance,	// Create method
	"V955",		// Plugin unique ID
	"VizBrush",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizBrush: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizBrush"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizBrush::VizBrush() : Vizlet() {
	DEBUGPRINT1(("VizBrush is being created and initialized! this=%ld",(long)this));
}

VizBrush::~VizBrush() {
}

DWORD __stdcall VizBrush::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizBrush();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizBrush::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	DEBUGPRINT1(("VizBrush::processCursor! downdragup=%d c=%.4f %.4f",downdragup,c->pos.x,c->pos.y));

	CursorBehaviour* cb = getCursorBehaviour();
	if ( cb->_inRegion(c) ) {
		cb->_trackCursors(c, downdragup);
		cb->_cursorSprite(c, downdragup);
		cb->_cursorMidi(c, downdragup);
	}
}

std::string VizBrush::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	DEBUGPRINT1(("VizBrush::processJson brush=%ld meth=%s",(long)this,meth.c_str()));

	CursorBehaviour* cb = getCursorBehaviour();

	if (meth == "apis") {
		std::string apis = "";
		// We don't show set_sidrange, because we don't want it to show up in any auto-generated UIs
		apis += "set_sprite(paramfile);";
		apis += "set_midi(paramfile);";
		apis += "set_looping(onoff);";
		apis += "set_autoloadparams(onoff);";
		apis += "testapi();";
		return jsonStringResult(apis, id);
	}

	if (meth == "testapi") {
		DEBUGPRINT(("VizBrush.testapi called!"));
		return jsonOK(id);
	}

	if (meth == "dump") {
		std::string dump = "[";

		dump += NosuchSnprintf("{\"method\":\"set_sprite\",\"params\":{\"paramfile\":\"%s\"}}", cb->spriteparamfile.c_str());
		dump += NosuchSnprintf(",{\"method\":\"set_midi\",\"params\":{\"paramfile\":\"%s\"}}", cb->midiparamfile.c_str());
		dump += NosuchSnprintf(",{\"method\":\"set_looping\",\"params\":{\"onoff\":\"%d\"}}", cb->m_looping);

		// Don't dump the sidrange value - it's set from FFF via API
		// dump += NosuchSnprintf(",{\"method\":\"set_sidrange\",\"params\":{\"sidrange\":\"%d-%d\"}}", cb->sid_min, cb->sid_max);

		dump += "]";
		return jsonStringResult(dump, id);
	}

	// Here we go through all the region names and look for their set/get methods
	std::string result;

	if (meth == "set_sidrange") {
		result = cb->_set_sidrange(json, id);
	}
	if (meth == "get_sidrange") {
		result = jsonStringResult(NosuchSnprintf("%d-%d", cb->sid_min, cb->sid_max), id);
	}

	if (meth == "set_sprite") {
		result = cb->_set_region_spriteparams(json, id);
	}
	if (meth == "get_sprite") {
		result = jsonStringResult(cb->spriteparamfile, id);
	}

	if (meth == "set_midi") { result = cb->_set_region_midiparams(json, id); }
	if (meth == "get_midi") { result = jsonStringResult(cb->midiparamfile, id); }

	if (meth == "set_looping") { result = cb->_set_looping(json, id); }
	if (meth == "get_looping") { result = jsonIntResult(cb->m_looping, id); }

	if (result != "") {
		const char *p = result.c_str();
		if (p[0] != '{') {
			DEBUGPRINT(("Bad result in meth=%s result=%s", meth.c_str(), p));
		}
		return result;
	}

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		cb->m_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(cb->m_autoloadparams ? 1 : 0, id);
	}

	throw NosuchException("VizBrush - Unrecognized method '%s'",meth.c_str());
}


bool VizBrush::processDraw() {
	// OpenGL calls here
	getCursorBehaviour()->_reloadParams();
	DrawVizSprites();
	return true;
}
