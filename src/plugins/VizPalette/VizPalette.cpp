#include "Vizlet.h"
#include "PaletteAll.h"
#include "VizPalette.h"

static CFFGLPluginInfo PluginInfo ( 
	VizPalette::CreateInstance,	// Create method
	"V018",		// Plugin unique ID
	"VizPalette",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizPalette: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizPalette"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizPalette::VizPalette() : Vizlet() {
	_palettehost = new PaletteHost(this);

	for ( int ch=0; ch<16; ch++ ) {
		// _midiparams[ch] = new AllVizParams(true);
		_midiparams[ch] = getAllVizParams("oozingcolor2_1");
	}
}

VizPalette::~VizPalette() {
}

DWORD __stdcall VizPalette::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizPalette();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

Palette* VizPalette::palette() {
	return _palettehost->palette();
}

void VizPalette::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
	DEBUGPRINT1(("VizPalette::processCursor! downdragup=%d c=%.4f %.4f",downdragup,c->pos.x,c->pos.y));
	palette()->processCursor(c,downdragup);
}

std::string VizPalette::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	if ( meth == "load" ) {
		std::string pname = jsonNeedString(json,"patch");
		palette()->applyPatch(pname);
		return jsonOK(id);
	}
	throw NosuchException("VizPalette - Unrecognized method '%s'",meth.c_str());
}

void VizPalette::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
	DEBUGPRINT(("processMidiInput"));
	_midiVizSprite(m);
	switch(m->MidiType()) {
	case MIDI_NOTE_ON:
		{
			MidiNoteOn* n = (MidiNoteOn*)m;
			NosuchAssert(n);
			DEBUGPRINT(("NOTE_ON pitch=%d",n->Pitch()));
		}
		break;
	case MIDI_NOTE_OFF:
		{
			MidiNoteOff* n = (MidiNoteOff*)m;
			NosuchAssert(n);
			DEBUGPRINT(("NOTE_OFF pitch=%d",n->Pitch()));
		}
		break;
	}
}

void VizPalette::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
	DEBUGPRINT(("processMidiOutput"));
}

bool VizPalette::processDraw() {
	// OpenGL calls here
	_palettehost->hostProcessDraw();
	DrawVizSprites();
	return true;
}

void VizPalette::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}

void VizPalette::processAdvanceTimeTo(int milli) {
	// DO NOT PUT DEBUGPRINT HERE!
	// DEBUGPRINT(("VizPalette::processAdvanceTimeTo milli=%d",milli));
	// CheckVizCursorUp(milli);
	palette()->advanceTo(milli);
}

void VizPalette::_midiVizSprite(MidiMsg* m) {

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
}