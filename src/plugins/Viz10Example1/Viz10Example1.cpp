#include "Vizlet10.h"
#include "Viz10Example1.h"

static CFF10PluginInfo PluginInfo ( 
	Viz10Example1::CreateInstance,	// Create method
	"V025",		// Plugin unique ID
	"Viz10Example1",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Viz10Example1: a sample vizlet10",			// description
	"by Tim Thompson - me@timthompson.com" 		// About
);

std::string vizlet10_name() { return "Viz10Example1"; }
CFF10PluginInfo& vizlet10_plugininfo() { return PluginInfo; }

Viz10Example1::Viz10Example1() : Vizlet10() {
}

Viz10Example1::~Viz10Example1() {
}

DWORD __stdcall Viz10Example1::CreateInstance(CFreeFrame10Plugin **ppInstance) {
	*ppInstance = new Viz10Example1();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void Viz10Example1::processCursor(VizCursor* c, int downdragup) {
	// DEBUGPRINT(("Viz10Example1::processCursor! pos=%f,%f", c->pos.x, c->pos.y));
}

std::string Viz10Example1::processJson(std::string meth, cJSON *json, const char *id) {
	throw NosuchException("Viz10Example1 - Unrecognized method '%s'",meth.c_str());
}

void Viz10Example1::processMidiInput(MidiMsg* m) {
	// DEBUGPRINT(("Viz10Example1::processMidiInput"));
}

void Viz10Example1::processMidiOutput(MidiMsg* m) {
	// DEBUGPRINT(("Viz10Example1::processMidiOutput"));
}

// bool Viz10Example1::processDraw() {
// 	return true;
// }

bool Viz10Example1::processFrame24Bit() {
	cvFlip(FrameImage());
	return true;
}

bool Viz10Example1::processFrame32Bit() {
	return false;
}

#if 0
DWORD Viz10Example1::ProcessFrame(void* pFrame) {

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

DWORD Viz10Example1::ProcessFrameCopy(ProcessFrameCopyStruct* pFrameData) {
	return FF_FAIL;
}
#endif

void Viz10Example1::processDrawNote(MidiMsg* m) {
}
