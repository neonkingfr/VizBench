//
// This minimal Vizlet example just draws a square.
//
#include "Vizlet.h"

class VizExample1 : public Vizlet
{
public:
	VizExample1();
	~VizExample1();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);
	bool processDraw();
};

static CFFGLPluginInfo PluginInfo ( 
	VizExample1::CreateInstance,	// Create method
	"VZX1",		// Plugin unique ID
	"VizExample1",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"VizExample1: an example vizlet",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string vizlet_name() { return "VizExample1"; }
CFFGLPluginInfo& vizlet_plugininfo() { return PluginInfo; }

VizExample1::VizExample1() : Vizlet() {
}

VizExample1::~VizExample1() {
}

DWORD __stdcall VizExample1::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new VizExample1();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

bool VizExample1::processDraw() {

	glColor4f(1.0,0.0,0.0,1.0);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.1f, 0.1f, 0.0);	// Bottom Left
	glVertex3f(0.1f, 0.9f, 0.0);	// Top Left
	glVertex3f(0.9f, 0.9f, 0.0);	// Top Right
	glVertex3f(0.9f, 0.1f, 0.0);	// Bottom Right
	glEnd();
	return true;
}