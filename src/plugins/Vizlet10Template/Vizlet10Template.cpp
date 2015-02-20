#include "Vizlet10.h"
#include "Vizlet10Template.h"

static CFF10PluginInfo PluginInfo ( 
	Vizlet10Template::CreateInstance,	// Create method
	"VZID",		// Plugin unique ID
	"Vizlet10Template",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Vizlet10Template: a sample vizlet10",			// description
	"by Tim Thompson - me@timthompson.com" 		// About
);

std::string vizlet10_name() { return "Vizlet10Template"; }
CFF10PluginInfo& vizlet10_plugininfo() { return PluginInfo; }

Vizlet10Template::Vizlet10Template() : Vizlet10() {
}

Vizlet10Template::~Vizlet10Template() {
}

DWORD __stdcall Vizlet10Template::CreateInstance(CFreeFrame10Plugin **ppInstance) {
	*ppInstance = new Vizlet10Template();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void Vizlet10Template::processCursor(VizCursor* c, int downdragup) {
}

std::string Vizlet10Template::processJson(std::string meth, cJSON *json, const char *id) {
	throw NosuchException("Vizlet10Template - Unrecognized method '%s'",meth.c_str());
}

void Vizlet10Template::processMidiInput(MidiMsg* m) {
}

void Vizlet10Template::processMidiOutput(MidiMsg* m) {
}

bool Vizlet10Template::processDraw() {
	return true;
}

bool Vizlet10Template::processFrame24Bit() {
	return false;
}

bool Vizlet10Template::processFrame32Bit() {
	return false;
}

#if 0
DWORD Vizlet10Template::ProcessFrame(void* pFrame) {

	switch (m_VideoInfo.BitDepth) {
	case 1:
		m_image->origin = 1;
		m_image->imageData = (char*)pFrame;
		return processFrame24Bit();
	case 2:
		m_image->origin = 1;
		m_image->imageData = (char*)pFrame;
		return processFrame32Bit();
	default:
		return FF_FAIL;
	}
}

DWORD Vizlet10Template::ProcessFrameCopy(ProcessFrameCopyStruct* pFrameData) {
	return FF_FAIL;
}
#endif

void Vizlet10Template::processDrawNote(MidiMsg* m) {
}
