#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "NosuchMidi.h"
#include "ffutil.h"

#include "Vizlet.h"
#include "VizMidi.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	VizMidi::CreateInstance,	// Create method
	"V498",		// Plugin unique ID
	"VizMidi",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizMidi: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizMidi"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizMidi::VizMidi() : Vizlet() {
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

	_midiparams->gravity.set(true);
	_midiparams->mass.set(0.01);

	_spriteparams = "";

}

VizMidi::~VizMidi() {
}

DWORD __stdcall VizMidi::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizMidi();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizMidi::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string VizMidi::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here

	if (meth == "apis") {
		return jsonStringResult("set_shape(shape);set_spriteparams(file)",id);
	}

	// PARAMETER "shape"
	if (meth == "set_shape") {
		std::string val = jsonNeedString(json, "shape", "");
		if (val == "") {
			return jsonError(-32000, "Bad shape value", id);
		}
		_midiparams->shape.set(val);
		return jsonOK(id);
	}
	if (meth == "get_shape") {
		return jsonStringResult(_midiparams->shape.get(), id);
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
			_midiparams = p;
		}
		return jsonOK(id);
	}
	if (meth == "get_spriteparams") {
		return jsonStringResult(_spriteparams, id);
	}

	throw NosuchException("VizMidi - Unrecognized method '%s'",meth.c_str());
}

void VizMidi::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizMidi::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	_midiVizSprite(m);
}

bool VizMidi::processDraw() {
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

	DrawVizSprites();
	return true;
}

void VizMidi::_midiVizSprite(MidiMsg* m) {

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
		_midiparams->hueinitial.set(hue);
		_midiparams->huefinal.set(hue);
		_midiparams->huefillinitial.set(hue);
		_midiparams->huefillfinal.set(hue);

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