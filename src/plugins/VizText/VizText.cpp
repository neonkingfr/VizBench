#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include <gl/glut.h>
#include <FTGL/ftgl.h>

#include "Vizlet.h"
#include "VizText.h"
#include "NosuchOsc.h"

// #include "PaletteSprite.h"
#include "SpaceServer.h"

static CFFGLPluginInfo PluginInfo ( 
	VizText::CreateInstance,	// Create method
	"V767",		// Plugin unique ID
	"VizText",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizText: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizText"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }
void vizlet_setdll(std::string dll) { }

VizText::VizText() : Vizlet() {
}

VizText::~VizText() {
}

DWORD __stdcall VizText::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizText();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void VizText::processCursor(SpaceCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string VizText::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("VizText - Unrecognized method '%s'",meth.c_str());
}

void VizText::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void VizText::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

static bool dofont = true;
static FTFont *font = NULL;

bool VizText::processDraw() {
	// OpenGL calls here
	if ( dofont ) {
		if ( font == NULL ) {
			font = new FTPolygonFont("c:/windows/font/arial.ttf");
			if ( font->Error() ) {
				DEBUGPRINT(("Unable to load font!?"));
				font = NULL;
				dofont = false;
			} else {
				font->FaceSize(80);
				font->CharMap(ft_encoding_unicode);
			}
		}
	}

	glColor4f(1.0,0.0,0.0,1.0);

	// draw a tall rectangle representing the note
	glBegin(GL_LINE_LOOP);
	glVertex3f(-0.8f, -0.8f, 0.0f);	// Top Left
	glVertex3f(-0.8f, 0.8f, 0.0f);	// Top Right
	glVertex3f(0.8f, 0.8f, 0.0f);	// Bottom Right
	glVertex3f(0.8f, -0.8f, 0.0f);	// Bottom Left
	glEnd();

	// font->Render("Hello");

	return true;
}

void VizText::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}