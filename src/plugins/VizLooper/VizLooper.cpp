#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "VizLooper.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	VizLooper::CreateInstance,	// Create method
	"V137",		// Plugin unique ID
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
void vizlet_setdll(std::string dll) { }

VizLooper::VizLooper() : Vizlet() {

	_midiin = MidiInputNumberOf("09. Internal MIDI");
	_midiout = MidiOutputNumberOf("03. Internal MIDI");

#if 0
	_midiparams = defaultParams();
	_midiparams->shape.set("line");

	double fadetime = 2.0;
	_midiparams->speedinitial.set(0.05);

	_midiparams->sizeinitial.set(0.1);
	_midiparams->sizefinal.set(0.05);
	_midiparams->sizetime.set(fadetime);

	_midiparams->lifetime.set(fadetime);

	_midiparams->thickness.set(3.0);
	// _midiparams->rotauto.set(true);
	_midiparams->rotangspeed.set(30.0);

	_midiparams->alphainitial.set(0.8);
	_midiparams->alphafinal.set(0.0);
	_midiparams->alphatime.set(fadetime);

	_midiparams->gravity.set(false);
	_midiparams->mass.set(0.01);
#endif

	_spriteparams = "default";
	_midiparams = getAllVizParams(_spriteparams);
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
		return jsonStringResult("set_spriteparams(file)", id);
	}

	// PARAMETER "spriteparams"
	if (meth == "set_spriteparams") {
		std::string file = jsonNeedString(json, "file", "");
		if (file == "") {
			return jsonError(-32000, "Bad file value", id);
		}
		AllVizParams* p = getAllVizParams(file);
		if (p) {
			_spriteparams = file;
			_midiparams = getAllVizParams(_spriteparams);
			return jsonOK(id);
		}
		else {
			return jsonError(-32000, "Bad params file?", id);
		}
	}
	if (meth == "get_spriteparams") {
		return jsonStringResult(_spriteparams, id);
	}

	throw NosuchException("VizLooper - Unrecognized method '%s'", meth.c_str());
}

void VizLooper::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
	if (m->InputPort() != _midiin) {
		return;
	}

	MidiMsg* m1 = m->clone();
	m1->SetOutputPort(_midiout);
	QueueMidiMsg(m1, CurrentClick());

	MidiMsg* m2 = m->clone();
	m2->SetOutputPort(_midiout);
	QueueMidiMsg(m2,CurrentClick()+192);
}

void VizLooper::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	_midiVizSprite(m);
}

bool VizLooper::processDraw() {
	// OpenGL calls here
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
		_midiparams->hueinitial.set(hue);
		_midiparams->huefinal.set(hue);
		_midiparams->huefillinitial.set(hue);
		_midiparams->huefillfinal.set(hue);

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

		_midiparams->movedir.set(movedir);

		makeAndAddVizSprite(_midiparams, pos);

		// pos.x += 0.2;
		// makeAndAddVizSprite(_midiparams, pos);
	}
}