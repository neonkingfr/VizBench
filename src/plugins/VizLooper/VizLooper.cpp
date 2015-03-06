#include "Vizlet.h"
#include "VizLooper.h"

static CFFGLPluginInfo PluginInfo ( 
	VizLooper::CreateInstance,	// Create method
	"VZLP",		// Plugin unique ID
	"VizLooper",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizLooper: looping notes",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizLooper"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizLooper::VizLooper() : Vizlet() {

	m_midiin = MidiInputNumberOf("09. Internal MIDI");
	m_midiout = MidiOutputNumberOf("03. Internal MIDI");

#if 0
	m_midiparams = defaultParams();
	m_midiparams->shape.set("line");

	double fadetime = 2.0;
	m_midiparams->speedinitial.set(0.05);

	m_midiparams->sizeinitial.set(0.1);
	m_midiparams->sizefinal.set(0.05);
	m_midiparams->sizetime.set(fadetime);

	m_midiparams->lifetime.set(fadetime);

	m_midiparams->thickness.set(3.0);
	// m_midiparams->rotauto.set(true);
	m_midiparams->rotangspeed.set(30.0);

	m_midiparams->alphainitial.set(0.8);
	m_midiparams->alphafinal.set(0.0);
	m_midiparams->alphatime.set(fadetime);

	m_midiparams->gravity.set(false);
	m_midiparams->mass.set(0.01);
#endif
	m_spriteparamspath = VizParamPath("default");
	m_midiparams = getAllVizParams(m_spriteparamspath);
	m_lastfileupdate = 0;
	m_lastfilecheck = 0;
}

VizLooper::~VizLooper() {
}

DWORD __stdcall VizLooper::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizLooper();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizLooper::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string VizLooper::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	if (meth == "apis") {
		return jsonStringResult("set_spriteparams(paramfile);set_autoloadparams(onoff)", id);
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
			m_spriteparamspath = path;
			m_midiparams = p;
			return jsonOK(id);
		}
		else {
			return jsonError(-32000, "Bad params file?", id);
		}
	}
	if (meth == "get_spriteparams") {
		return jsonStringResult(VizPath2ConfigName(m_spriteparamspath), id);
	}

	// PARAMETER "autoloadparams"
	if (meth == "set_autoloadparams") {
		m_autoloadparams = jsonNeedBool(json, "onoff", false);
		return jsonOK(id);
	}
	if (meth == "get_autoloadparams") {
		return jsonIntResult(m_autoloadparams?1:0, id);
	}

	throw NosuchException("VizLooper - Unrecognized method '%s'", meth.c_str());
}

void VizLooper::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
	if (m->InputPort() != m_midiin) {
		return;
	}

	MidiMsg* m1 = m->clone();
	m1->SetOutputPort(m_midiout);
	QueueMidiMsg(m1, SchedulerCurrentClick());

	MidiMsg* m2 = m->clone();
	m2->SetOutputPort(m_midiout);
	QueueMidiMsg(m2,SchedulerCurrentClick()+192);
}

void VizLooper::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	_midiVizSprite(m);
}

bool VizLooper::processDraw() {
	// OpenGL calls here
	if (m_autoloadparams) {
		AllVizParams* p = checkAndLoadIfModifiedSince(m_spriteparamspath,
								m_lastfilecheck, m_lastfileupdate);
		if (p) {
			m_midiparams = p;
		}
	}
	DrawVizSprites();
	return true;
}

void VizLooper::_midiVizSprite(MidiMsg* m) {

	int minpitch = 0;
	int maxpitch = 127;

	if (m->MidiType() == MIDI_NOTE_ON) {
		if (m->Velocity() == 0) {
			DEBUGPRINT(("Vizlet1 sees noteon with 0 velocity, ignoring"));
			return;
		}
		// control color with channel
		NosuchColor clr = channelColor(m->Channel());
		double hue = clr.hue();
		m_midiparams->hueinitial.set(hue);
		m_midiparams->huefinal.set(hue);
		m_midiparams->huefillinitial.set(hue);
		m_midiparams->huefillfinal.set(hue);

		NosuchPos pos;
		float movedir = 0.0;
		bool randposdir = false;
		if (randposdir) {
			pos.x = pos.y = 0.5;
			pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);
			movedir = (float)(rand() % 360);
		}
		else {
			switch (m->Channel() % 2) {
			case 0:
				pos.y = (m->Pitch() / 127.0);
				pos.x = 0.4;
				movedir = 90.0;
				break;
			case 1:
				pos.y = (m->Pitch() / 127.0);
				pos.x = 0.6;
				movedir = 270.0;
				break;
			}
			pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);
		}

		m_midiparams->movedir.set(movedir);

		makeAndAddVizSprite(m_midiparams, pos);

		// pos.x += 0.2;
		// makeAndAddVizSprite(m_midiparams, pos);
	}
}
