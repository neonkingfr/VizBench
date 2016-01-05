#include "Vizlet.h"
#include "VizTuio.h"

static CFFGLPluginInfo PluginInfo ( 
	VizTuio::CreateInstance,	// Create method
	"V018",		// Plugin unique ID
	"VizTuio",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizTuio: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizTuio"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizTuio::VizTuio() : Vizlet() {
	_freeframeHost = new ResolumeHost();
	_palette = new Palette(_freeframeHost);

	// for (int n = 0; n < 4; n++) {
	// 	_loadSpriteVizParamsFile(NosuchSnprintf("patch_1_%c_on", "ABCD"[n]), _region[n]);
	// }
	_autoloadparams = true;
}

VizTuio::~VizTuio() {
}

DWORD __stdcall VizTuio::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizTuio();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizTuio::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	DEBUGPRINT1(("VizTuio::processCursor! downdragup=%d c=%.4f %.4f",downdragup,c->pos.x,c->pos.y));
	// palette()->processCursor(c,downdragup);
	if (downdragup == CURSOR_DOWN || downdragup == CURSOR_DRAG) {
		_cursorSprite(c);
	}
}

std::string VizTuio::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult(

			"set_region_A_sid(sid);set_region_A_sprite(paramfile);set_region_A_midi(paramfile);"
			"set_region_B_sid(sid);set_region_B_sprite(paramfile);set_region_B_midi(paramfile);"
			"set_region_C_sid(sid);set_region_C_sprite(paramfile);set_region_C_midi(paramfile);"
			"set_region_D_sid(sid);set_region_D_sprite(paramfile);set_region_D_midi(paramfile);"

			"set_button_A_sid(sid);set_button_A_pipeline(pipeline);"
			"set_button_B_sid(sid);set_button_B_pipeline(pipeline);"
			"set_button_C_sid(sid);set_button_C_pipeline(pipeline);"
			"set_button_D_sid(sid);set_button_D_pipeline(pipeline);"
			"set_button_E_sid(sid);set_button_E_pipeline(pipeline);"
			"set_button_F_sid(sid);set_button_F_pipeline(pipeline);"
			"set_button_G_sid(sid);set_button_G_pipeline(pipeline);"
			"set_button_H_sid(sid);set_button_H_pipeline(pipeline);"
			"set_button_I_sid(sid);set_button_I_pipeline(pipeline);"
			"set_button_J_sid(sid);set_button_J_pipeline(pipeline);"
			"set_button_K_sid(sid);set_button_K_pipeline(pipeline);"
			"set_button_L_sid(sid);set_button_L_pipeline(pipeline);"

			"set_autoloadparams(onoff);"
			"testapi();"

			, id);
	}
	if (meth == "testapi") {
		DEBUGPRINT(("VizTuio.testapi called!"));
		return jsonOK(id);
	}
	if (meth == "dump") {
		std::string dump =
			"["
			+ NosuchSnprintf("{\"method\":\"set_region_A_sid\",\"params\":{\"sid\":\"%s\"}}", _region[0].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_B_sid\",\"params\":{\"sid\":\"%s\"}}", _region[1].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_C_sid\",\"params\":{\"sid\":\"%s\"}}", _region[2].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_D_sid\",\"params\":{\"sid\":\"%s\"}}", _region[3].sid.c_str())

			+ NosuchSnprintf("{\"method\":\"set_region_A_sprite\",\"params\":{\"paramfile\":\"%s\"}}", _region[0].spriteparamfile.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_B_sprite\",\"params\":{\"paramfile\":\"%s\"}}", _region[1].spriteparamfile.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_C_sprite\",\"params\":{\"paramfile\":\"%s\"}}", _region[2].spriteparamfile.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_D_sprite\",\"params\":{\"paramfile\":\"%s\"}}", _region[3].spriteparamfile.c_str())

			+ NosuchSnprintf("{\"method\":\"set_region_A_midi\",\"params\":{\"paramfile\":\"%s\"}}", _region[0].midiparamfile.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_B_midi\",\"params\":{\"paramfile\":\"%s\"}}", _region[1].midiparamfile.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_C_midi\",\"params\":{\"paramfile\":\"%s\"}}", _region[2].midiparamfile.c_str())
			+ NosuchSnprintf("{\"method\":\"set_region_D_midi\",\"params\":{\"paramfile\":\"%s\"}}", _region[3].midiparamfile.c_str())

			+ NosuchSnprintf("{\"method\":\"set_button_A_sid\",\"params\":{\"sid\":\"%s\"}}", _button[0].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_B_sid\",\"params\":{\"sid\":\"%s\"}}", _button[1].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_C_sid\",\"params\":{\"sid\":\"%s\"}}", _button[2].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_D_sid\",\"params\":{\"sid\":\"%s\"}}", _button[3].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_E_sid\",\"params\":{\"sid\":\"%s\"}}", _button[4].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_F_sid\",\"params\":{\"sid\":\"%s\"}}", _button[5].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_G_sid\",\"params\":{\"sid\":\"%s\"}}", _button[6].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_H_sid\",\"params\":{\"sid\":\"%s\"}}", _button[7].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_I_sid\",\"params\":{\"sid\":\"%s\"}}", _button[8].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_J_sid\",\"params\":{\"sid\":\"%s\"}}", _button[9].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_K_sid\",\"params\":{\"sid\":\"%s\"}}", _button[10].sid.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_L_sid\",\"params\":{\"sid\":\"%s\"}}", _button[11].sid.c_str())

			+ NosuchSnprintf("{\"method\":\"set_button_A_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[0].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_B_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[1].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_C_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[2].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_D_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[3].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_E_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[4].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_F_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[5].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_G_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[6].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_H_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[7].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_I_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[8].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_J_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[9].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_K_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[10].pipeline.c_str())
			+ NosuchSnprintf("{\"method\":\"set_button_L_pipeline\",\"params\":{\"pipeline\":\"%s\"}}", _button[11].pipeline.c_str())

			+ "]"
			;
		return jsonStringResult(dump, id);
	}
	if (meth == "restore") {
		std::string dump = jsonNeedString(json, "dump");
		ExecuteDump(dump);
		return jsonOK(id);
	}

	if (meth == "set_region_A_sid") { return _set_region_sid(_region[0], json, id); }
	if (meth == "get_region_A_sid") { return jsonStringResult(_region[0].sid, id); }
	if (meth == "set_region_B_sid") { return _set_region_sid(_region[1], json, id); }
	if (meth == "get_region_B_sid") { return jsonStringResult(_region[1].sid, id); }
	if (meth == "set_region_C_sid") { return _set_region_sid(_region[2], json, id); }
	if (meth == "get_region_C_sid") { return jsonStringResult(_region[2].sid, id); }
	if (meth == "set_region_D_sid") { return _set_region_sid(_region[3], json, id); }
	if (meth == "get_region_D_sid") { return jsonStringResult(_region[3].sid, id); }

	if (meth == "set_region_A_sprite") { return _set_region_spriteparams(_region[0], json, id); }
	if (meth == "get_region_A_sprite") { return jsonStringResult(_region[0].spriteparamfile, id); }
	if (meth == "set_region_B_sprite") { return _set_region_spriteparams(_region[1], json, id); }
	if (meth == "get_region_B_sprite") { return jsonStringResult(_region[1].spriteparamfile, id); }
	if (meth == "set_region_C_sprite") { return _set_region_spriteparams(_region[2], json, id); }
	if (meth == "get_region_C_sprite") { return jsonStringResult(_region[2].spriteparamfile, id); }
	if (meth == "set_region_D_sprite") { return _set_region_spriteparams(_region[3], json, id); }
	if (meth == "get_region_D_sprite") { return jsonStringResult(_region[3].spriteparamfile, id); }

	if (meth == "set_region_A_midi") { return _set_region_midiparams(_region[0], json, id); }
	if (meth == "get_region_A_midi") { return jsonStringResult(_region[0].midiparamfile, id); }
	if (meth == "set_region_B_midi") { return _set_region_midiparams(_region[1], json, id); }
	if (meth == "get_region_B_midi") { return jsonStringResult(_region[1].midiparamfile, id); }
	if (meth == "set_region_C_midi") { return _set_region_midiparams(_region[2], json, id); }
	if (meth == "get_region_C_midi") { return jsonStringResult(_region[2].midiparamfile, id); }
	if (meth == "set_region_D_midi") { return _set_region_midiparams(_region[3], json, id); }
	if (meth == "get_region_D_midi") { return jsonStringResult(_region[3].midiparamfile, id); }

	if (meth == "set_region_A_midi") { return _set_region_midiparams(_region[0], json, id); }
	if (meth == "get_region_A_midi") { return jsonStringResult(_region[0].midiparamfile, id); }

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(_autoloadparams ? 1 : 0, id);
	}

	throw NosuchException("VizTuio - Unrecognized method '%s'",meth.c_str());
}

bool VizTuio::processDraw() {
	// OpenGL calls here
	DrawVizSprites();
	return true;
}

void VizTuio::processAdvanceTimeTo(int milli) {
	// DO NOT PUT DEBUGPRINT HERE!
	// DEBUGPRINT(("VizTuio::processAdvanceTimeTo milli=%d",milli));
	// CheckVizCursorUp(milli);
	palette()->AdvanceTo(milli);
}

void VizTuio::_cursorSprite(VizCursor* c) {

	DEBUGPRINT(("cursorSprite! xy=", c->pos.x, c->pos.y, c->pos.z));

#if 0
	int minpitch = 0;
	int maxpitch = 127;

	if ( m->MidiType() == MIDI_NOTE_ON ) {
		if ( m->Velocity() == 0 ) {
			DEBUGPRINT(("Vizlet1 sees noteon with 0 velocity, ignoring"));
			return;
		}
		// control color with channel
		NosuchColor clr = channelColor(m->Channel());
		double hue = clr.hue();
		int ch = m->Channel();  // returns 1-16
		_midiparams[ch]->hueinitial.set(hue);
		_midiparams[ch]->huefinal.set(hue);
		_midiparams[ch]->huefillinitial.set(hue);
		_midiparams[ch]->huefillfinal.set(hue);

		NosuchPos pos;
		float movedir = 0.0;
		bool randposdir = false;
		if ( randposdir ) {
			pos.x = pos.y = 0.5;
			pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);
			movedir = (float)(rand() % 360);
		} else {
			switch ( m->Channel() % 2 ) {
			case 0:
				pos.x = (m->Pitch() / 127.0);
				pos.y = 0.2;
				movedir = 0.0;
				break;
			case 1:
				pos.x = (m->Pitch() / 127.0);
				pos.y = 0.8;
				movedir = 180.0;
				break;
			}
			pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);
		}

		_midiparams[ch]->movedir.set(movedir);

		makeAndAddVizSprite(_midiparams[ch], pos);

		// pos.x += 0.2;
		// makeAndAddVizSprite(_midiparams, pos);
	}
#endif

}

std::string
VizTuio::_set_region_spriteparams(region_info& regioninfo, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadSpriteVizParamsFile(file, regioninfo)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

std::string
VizTuio::_set_region_sid(region_info& regioninfo, cJSON* json, const char* id)
{
	std::string sid = jsonNeedString(json, "sid", "");
	if (sid == "") {
		return jsonError(-32000, "Bad sid value", id);
	}
	int sidmin;
	int sidmax;
	int n = sscanf(sid.c_str(), "%d-%d",&sidmin,&sidmax);
	if (n == 1) {
		regioninfo.sid_min = sidmin;
		regioninfo.sid_max = sidmin;
	}
	else if (n == 2) {
		regioninfo.sid_min = sidmin;
		regioninfo.sid_max = sidmax;
	}
	else {
		return jsonError(-32000, "Unable to interpret sid value: %s", sid.c_str());
	}
	return jsonOK(id);
}

bool VizTuio::_loadSpriteVizParamsFile(std::string fname, VizTuio::region_info& regioninfo) {
	regioninfo.lastspritefilecheck = time(0);
	regioninfo.lastspritefileupdate = time(0);
	SpriteVizParams* p = getSpriteVizParams(fname);
	if (!p) {
		regioninfo.spriteparamfile = "";
		regioninfo.spriteparams = NULL;
		return false;
	}
	regioninfo.spriteparamfile = fname;
	regioninfo.spriteparams = p;
	return true;
}

std::string
VizTuio::_set_region_midiparams(region_info& regioninfo, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadMidiVizParamsFile(file, regioninfo)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

bool VizTuio::_loadMidiVizParamsFile(std::string fname, VizTuio::region_info& regioninfo) {
	regioninfo.lastmidifilecheck = time(0);
	regioninfo.lastmidifileupdate = time(0);
	MidiVizParams* p = getMidiVizParams(fname);
	if (!p) {
		regioninfo.midiparamfile = "";
		regioninfo.midiparams = NULL;
		return false;
	}
	regioninfo.midiparamfile = fname;
	regioninfo.midiparams = p;
	return true;
}