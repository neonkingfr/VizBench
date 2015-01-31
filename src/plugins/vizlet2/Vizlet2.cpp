#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"
#include <sys/stat.h>

#include "Vizlet.h"
#include "Vizlet2.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	Vizlet2::CreateInstance,	// Create method
	"NSV2",		// Plugin unique ID
	"Vizlet2",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Vizlet2: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "Vizlet2"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
// void vizlet_setdll(std::string dll) { }

Vizlet2::Vizlet2() : Vizlet() {
	_stepmilli = 125;
}

Vizlet2::~Vizlet2() {
}

DWORD __stdcall Vizlet2::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new Vizlet2();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

std::string Vizlet2::processJson(std::string meth, cJSON *json, const char *id) {
	throw NosuchException("Vizlet2 - Unrecognized method '%s'",meth.c_str());
}

void Vizlet2::processMidiInput(MidiMsg* m) {
}

void Vizlet2::processMidiOutput(MidiMsg* m) {
}

bool Vizlet2::processDraw() {
	DrawVizSprites();
	DrawNotesDown();
	return true;
}

void
Vizlet2::processDrawNote(MidiMsg* m) {

	glColor4f(1.0,0.0,0.0,1.0);

	GLfloat w = 1.8f / 128.0;	// width of note
	GLfloat w2 = w / 2.0f;
	GLfloat h = 0.1f;			// height of note

	GLfloat x = (GLfloat)(-0.9f + w * m->Pitch());

	int ch = m->Channel();
	NosuchColor clr = channelColor(ch);
	glColor4f(clr.r()/255.0f,clr.g()/255.0f,clr.b()/255.0f,1.0f);

	GLfloat y = -1.0f + ch * 0.1f;

	// draw a tall rectangle representing the note
	glBegin(GL_LINE_LOOP);
	glVertex3f(x-w2, y, 0.0f);	// Top Left
	glVertex3f(x-w2, y+h, 0.0f);	// Top Right
	glVertex3f(x+w2, y+h, 0.0f);	// Bottom Right
	glVertex3f(x+w2, y, 0.0f);	// Bottom Left
	glEnd();
}