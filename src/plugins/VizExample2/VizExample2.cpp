//
// This Vizlet example shows how to create Sprites, using 3 different methods.
// All of the methods operate simultaneously.  Sprites are created:
//
// 1) When cursor events are received by processCursor().
// 2) When MIDI events are received by processMidiOutput or processMidiInput
// 3) When the "newsprite" API is received by processJson
//

#include "Vizlet.h"

class VizExample2 : public Vizlet
{
public:
	VizExample2();
	virtual ~VizExample2();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

private:
	AllVizParams* m_midiparams;
	AllVizParams* m_cursorparams;
	AllVizParams* m_apiparams;
};

static CFFGLPluginInfo PluginInfo ( 
	VizExample2::CreateInstance,	// Create method
	"VZX2",		// Plugin unique ID
	"VizExample2",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizExample2: an example vizlet",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizExample2"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizExample2::VizExample2() : Vizlet() {

	// Set up different parameters for each type of Sprite
	m_cursorparams = new AllVizParams();
	m_cursorparams->shape.set("circle");

	m_midiparams = new AllVizParams();
	m_midiparams->shape.set("square");

	m_apiparams = new AllVizParams();
	m_apiparams->shape.set("triangle");
}

VizExample2::~VizExample2() {
}

DWORD __stdcall VizExample2::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizExample2();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizExample2::processCursor(VizCursor* c, int downdragup) {

	// Create Sprites whenever and wherever a cursor is moved.
	// Cursor input for multiple cursors comes in via TUIO over OSC.
	switch(downdragup) {
	case CURSOR_DOWN:
		makeAndAddVizSprite(m_cursorparams, c->pos);
		break;
	case CURSOR_DRAG:
		makeAndAddVizSprite(m_cursorparams, c->pos);
		break;
	case CURSOR_UP:
		break;
	}
}

std::string VizExample2::processJson(std::string meth, cJSON *json, const char *id) {

	// All Vizlets should have an "apis" method that lists the
	// APIs that the Vizlet understands.  In this case, there is just
	// one api - newsprite - which creates and randomly places a new Sprite.
	if (meth == "apis") {
		return jsonStringResult("newsprite",id);
	}

	if ( meth == "newsprite" ) {
		// Position the Sprite randomly
		double x = 0.1 + 0.8*((double)rand())/RAND_MAX;   // 0.1-0.9
		double y = 0.1 + 0.8*((double)rand())/RAND_MAX;   // 0.1-0.9
		double z = 0.5;
		makeAndAddVizSprite(m_apiparams,NosuchPos(x,y,z));
		return jsonOK(id);
	}

	throw NosuchException("VizExample2 - Unrecognized method '%s'",meth.c_str());
}

void VizExample2::processMidiInput(MidiMsg* m) {

	// Ignore everything except MIDI_NOTE_ON
	if ( m->MidiType() != MIDI_NOTE_ON || m->Velocity() == 0 ) {
			return;
	}

	// The sprite position is based on the MIDI pitch and velocity
	NosuchPos pos;
	pos.x = 0.5f;
	pos.y = (m->Pitch()) / float(127);
	pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);

	// Sprite color is controlled by the MIDI channel
	NosuchColor clr = channelColor(m->Channel());
	double hue = clr.hue();

	if ((rand() % 2) == 0) {
		m_midiparams->movedir.set(90.0);
	} else {
		m_midiparams->movedir.set(270.0);
	}
	m_midiparams->speedinitial.set(0.2);
	m_midiparams->hueinitial.set(hue);
	m_midiparams->huefinal.set(hue);
	m_midiparams->huefillinitial.set(hue);
	m_midiparams->huefillfinal.set(hue);

	makeAndAddVizSprite(m_midiparams, pos);
}

void VizExample2::processMidiOutput(MidiMsg* m) {
	processMidiInput(m);
}

bool VizExample2::processDraw() {

	glColor4f(0.0,1.0,0.0,1.0);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.2f, 0.2f, 0.0);	// Bottom Left
	glVertex3f(0.2f, 0.8f, 0.0);	// Top Left
	glVertex3f(0.8f, 0.8f, 0.0);	// Top Right
	glVertex3f(0.8f, 0.2f, 0.0);	// Bottom Right
	glEnd();

	// Draw all active sprites
	DrawVizSprites();
	return true;
}