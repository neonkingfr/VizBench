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

	for (int n = 0; n < 4; n++) {
		_region[n].channel = n + 1;
		_loadSpriteVizParamsFile(NosuchSnprintf("patch_1_%c_on", "ABCD"[n]), _region[n]);
	}

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
	if ( meth == "load" ) {
		std::string pname = jsonNeedString(json,"patch");
		// XXX - not so sure about this...
		palette()->ConfigLoad(pname);
		return jsonOK(id);
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

bool VizTuio::_loadSpriteVizParamsFile(std::string fname, VizTuio::paramsfile_info& spriteinfo) {
	spriteinfo.lastfilecheck = time(0);
	spriteinfo.lastfileupdate = time(0);
	SpriteVizParams* p = getSpriteVizParams(fname);
	if (!p) {
		spriteinfo.paramsfname = "";
		spriteinfo.params = NULL;
		return false;
	}
	spriteinfo.paramsfname = fname;
	spriteinfo.params = p;
	return true;
}