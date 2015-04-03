#include "Vizlet.h"
#include "VizMidi4.h"

static CFFGLPluginInfo PluginInfo ( 
	VizMidi4::CreateInstance,	// Create method
	"VZMD",		// Plugin unique ID
	"VizMidi4",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizMidi4: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizMidi4"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizMidi4::VizMidi4() : Vizlet() {

	for (int n = 0; n < 4; n++) {
		m_sprite[n].channel = n + 1;
		_loadParamsFile(n, NosuchSnprintf("midi4_%c","ABCD"[n]));
	}
}

VizMidi4::~VizMidi4() {
	DEBUGPRINT1(("VizMidi4 DESTRUCTOR!!"));
}

DWORD __stdcall VizMidi4::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizMidi4();
	DEBUGPRINT1(("CreateInstance of VizMidi4 = %ld", (long)(*ppInstance)));
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizMidi4::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

bool VizMidi4::_loadParamsFile(int n, std::string file) {
	m_sprite[n].lastfilecheck = time(0);
	m_sprite[n].lastfileupdate = time(0);
	std::string path = VizParamPath(file);
	AllVizParams* p = getAllVizParams(path);
	if (!p) {
		m_sprite[n].paramsfile = "";
		m_sprite[n].paramspath = "";
		m_sprite[n].params = NULL;
		return false;
	}
	m_sprite[n].paramsfile = file;
	m_sprite[n].paramspath = path;
	m_sprite[n].params = p;
	return true;
}

std::string
VizMidi4::_set_params(int n, cJSON* json, const char* id)
{
	std::string file = jsonNeedString(json, "paramfile", "");
	if (file == "") {
		return jsonError(-32000, "Bad file value", id);
	}
	if (_loadParamsFile(n, file)) {
		return jsonOK(id);
	}
	else {
		return jsonError(-32000, "Unable to load spriteparams file", id);
	}
}

std::string VizMidi4::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult(
			"set_sprite_A(paramfile);"
			"set_sprite_B(paramfile);"
			"set_sprite_C(paramfile);"
			"set_sprite_D(paramfile);"
			"set_channel_A(channel);"
			"set_channel_B(channel);"
			"set_channel_C(channel);"
			"set_channel_D(channel);"
			"set_autoloadparams(onoff);"
			, id);
	}

	if (meth == "set_channel_A") { m_sprite[0].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_A") { return jsonIntResult(m_sprite[0].channel,id); }
	if (meth == "set_channel_B") { m_sprite[1].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_B") { return jsonIntResult(m_sprite[1].channel,id); }
	if (meth == "set_channel_C") { m_sprite[2].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_C") { return jsonIntResult(m_sprite[2].channel,id); }
	if (meth == "set_channel_D") { m_sprite[3].channel = jsonNeedInt(json, "channel"); return jsonOK(id); }
	if (meth == "get_channel_D") { return jsonIntResult(m_sprite[3].channel,id); }

	if (meth == "set_sprite_A") { return _set_params(0, json,id); }
	if (meth == "get_sprite_A") { return jsonStringResult(m_sprite[0].paramsfile, id); }
	if (meth == "set_sprite_B") { return _set_params(1, json,id); }
	if (meth == "get_sprite_B") { return jsonStringResult(m_sprite[1].paramsfile, id); }
	if (meth == "set_sprite_C") { return _set_params(2, json,id); }
	if (meth == "get_sprite_C") { return jsonStringResult(m_sprite[2].paramsfile, id); }
	if (meth == "set_sprite_D") { return _set_params(3, json,id); }
	if (meth == "get_sprite_D") { return jsonStringResult(m_sprite[3].paramsfile, id); }

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		m_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(m_autoloadparams?1:0, id);
	}

	throw NosuchException("VizMidi4 - Unrecognized method '%s'",meth.c_str());
}

void VizMidi4::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
	_midiVizSprite(m);
}

void VizMidi4::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	_midiVizSprite(m);
}

bool VizMidi4::processDraw() {
	// OpenGL calls here	

#if 0
	glColor4f(1.0,0.0,0.0,0.5);
	glLineWidth((GLfloat)2.0f);
	// Draw a rectangle just to show that we're alive.
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.2f, 0.2f, 0.0f);	// Top Left
	glVertex3f(0.2f, 0.8f, 0.0f);	// Top Right
	glVertex3f(0.8f, 0.8f, 0.0f);	// Bottom Right
	glVertex3f(0.8f, 0.2f, 0.0f);	// Bottom Left
	glEnd();
#endif

	// XXX SHOULDN'T REALLY DO THIS SO OFTEN...
	if (m_autoloadparams) {
		static int cnt = 0;
		int n = cnt++ % 4;	// cycle between the 4 
		AllVizParams* p = checkAndLoadIfModifiedSince(m_sprite[n].paramspath,
								m_sprite[n].lastfilecheck, m_sprite[n].lastfileupdate);
		if (p) {
			m_sprite[n].params = p;
		}
	}

	DrawVizSprites();
	return true;
}

void VizMidi4::_midiVizSprite(MidiMsg* m) {

	int minpitch = 0;
	int maxpitch = 127;

	if ( m->MidiType() == MIDI_NOTE_ON ) {
		if ( m->Velocity() == 0 ) {
			DEBUGPRINT(("Vizlet1 sees noteon with 0 velocity, ignoring"));
			return;
		}
		NosuchPos pos;

		int n = (m->Channel()-1) % 4;
		std::string place = m_sprite[n].params->placement;

		if (place == "nowhere") {
			return;
		}
		else if (place == "random") {
			pos.x = (rand() % 1000) / 1000.0;
			pos.y = (rand() % 1000) / 1000.0;
		}
		else if (place == "bottom") {
			pos.x = (m->Pitch() / 127.0);
			pos.y = 0.2;
		}
		else if (place == "left") {
			pos.x = 0.2;
			pos.y = (m->Pitch() / 127.0);
		}
		else if (place == "right") {
			pos.x = 0.8;
			pos.y = (m->Pitch() / 127.0);
		}
		else if (place == "top") {
			pos.x = (m->Pitch() / 127.0);
			pos.y = 0.8;
		}
		else if (place == "center") {
			pos.x = 0.5;
			pos.y = 0.5;
		}
		else {
			DEBUGPRINT(("Invalid placement value: %s",place.c_str()));
			pos.x = 0.5;
			pos.y = 0.5;
		}
		pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);

		makeAndAddVizSprite(m_sprite[n].params, pos);
	}
}