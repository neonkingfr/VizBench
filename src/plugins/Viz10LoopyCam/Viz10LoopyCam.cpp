#include "Vizlet10.h"
#include "videoutil.h"
#include "looper.h"
#include "Viz10LoopyCam.h"

static CFF10PluginInfo PluginInfo ( 
	Viz10LoopyCam::CreateInstance,	// Create method
	"V598",		// Plugin unique ID
	"Viz10LoopyCam",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Viz10LoopyCam: a sample vizlet10",			// description
	"by Tim Thompson - me@timthompson.com" 		// About
);

std::string vizlet10_name() { return "Viz10LoopyCam"; }
CFF10PluginInfo& vizlet10_plugininfo() { return PluginInfo; }

// I really shouldn't have a single static instance of
// the Looper, but for some reason there's a big memory
// leak in the Looper class, and I wasn't able to find it
// so, this is the lame workaround for that so that the
// Viz10LoopyCam Vizlet can be unloaded/loaded repeatedly.
Looper* Viz10LoopyCam::m_looper = NULL;

Viz10LoopyCam::Viz10LoopyCam() : Vizlet10() {
	// DEBUGPRINT(("---- Viz10LoopyCam constructor!"));
}

Viz10LoopyCam::~Viz10LoopyCam() {
	// DEBUGPRINT(("---- Viz10LoopyCam destructor!"));
	// DO NOT delete m_looper, it's static
#if 0
	if (m_looper != NULL) {
		DEBUGPRINT(("---- DELETE Viz10LoopyCam m_looper=%ld", (long)m_looper));
		delete m_looper;
	}
#endif
}

DWORD __stdcall Viz10LoopyCam::CreateInstance(CFreeFrame10Plugin **ppInstance) {
	*ppInstance = new Viz10LoopyCam();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void Viz10LoopyCam::processCursor(VizCursor* c, int downdragup) {
	DEBUGPRINT(("processCursor"));
}

#if 0
void Viz10LoopyCam::processOsc(const char *source, const osc::ReceivedMessage& m) {
	osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
	const char* s = (arg++)->AsString();
	DEBUGPRINT(("processOSC arg0 = %s",s));
}
#endif

std::string Viz10LoopyCam::processJson(std::string meth, cJSON *params, const char *id) {

	LockVizlet();
	std::string s = realProcessJson(meth, params, id);
	UnlockVizlet();
	return s;
}

std::string Viz10LoopyCam::realProcessJson(std::string meth, cJSON *params, const char *id) {

	Looper* lp = m_looper;
	if (meth == "apis") {
		return jsonStringResult("record_on;record_off;record(onoff);play(loopnum,onoff);"
			"blackout(onoff);randompositions(aspect);randomposition1(aspect);"
			"recordoverlay(onoff);morewindows;lesswindows;"
			"setwindows(numwindows);fulldisplay;quadrantdisplay;"
			"alllive(onoff);autonext(n);togglemovesmooth;movesmooth(onoff);"
			"setinterp(v);nextloop;playfactor(loopnum,factor);"
			"playfactorreset(loopnum);moveamount(amount);"
			"restart(loopnum);restartrandom(loopnum);"
			"restartsave(loopnum);restartrestore(loopnum);"
			"freeze(loopnum);unfreeze(loopnum);"
			"truncate(loopnum);xor(onoff);border(onoff);"
			"fliph(onoff);flipv(onoff);recborder(onoff);"
			"setstart(loopnum,pos);setend(loopnum,pos);setreverse(loopnum,onoff);"
			, id);
	}
	if (meth == "record") {
		lp->setRecord(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "record_on") {
		lp->setRecord(true);
		return jsonOK(id);
	}
	if (meth == "record_off") {
		lp->setRecord(false);
		return jsonOK(id);
	}
	if (meth == "play") {
		int loopnum = jsonNeedInt(params, "loopnum");
		bool onoff = jsonNeedBool(params, "onoff");
		if (lp->validLoopnum(loopnum)) {
			lp->setPlay(loopnum, onoff);
		}
		return jsonOK(id);
	}
	if (meth == "blackout") {
		lp->setBlackout(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "randompositions") {
		double aspect = jsonNeedDouble(params,"aspect", -1.0);
		lp->randompositions(aspect);
		return jsonOK(id);
	}
	if (meth == "randomposition1") {
		double aspect = jsonNeedDouble(params,"aspect", -1.0);
		lp->randomposition(aspect);
		return jsonOK(id);
	}
	if (meth == "recordoverlay") {
		lp->_setRecordOverlay(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "recordoverly_on") {
		lp->_setRecordOverlay(true);
		return jsonOK(id);
	}
	if (meth == "recordoverly_off") {
		lp->_setRecordOverlay(false);
		return jsonOK(id);
	}
	if (meth == "morewindows") {
		lp->morewindows();
		return jsonOK(id);
	}
	if (meth == "lesswindows") {
		lp->lesswindows();
		return jsonOK(id);
	}
	if (meth == "setwindows") {
		int n = jsonNeedInt(params, "numwindows");
		lp->setwindows(n);
		return jsonOK(id);
	}
	if (meth == "fulldisplay") {
		lp->_fullDisplay();
		return jsonOK(id);
	}
	if (meth == "quadrantdisplay") {
		lp->_quadrantDisplay();
		return jsonOK(id);
	}
	if (meth == "alllive") {
		lp->_allLive(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "autonext") {
		lp->_setautoNext(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "togglesmovemooth") {
		lp->togglemovesmooth();
		return jsonOK(id);
	}
	if (meth == "movesmooth") {
		lp->_setmovesmooth(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "setinterp") {
		lp->setinterp(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "nextloop") {
		lp->_nextLoop();
		return jsonOK(id);
	}
	if (meth == "playfactor") {
		int loopnum = jsonNeedInt(params, "loopnum");
		double factor = jsonNeedDouble(params, "factor", 0.5);
		lp->_setplayfactor(loopnum, factor);
		return jsonOK(id);
	}
	if (meth == "playfactorreset") {
		int loopnum = jsonNeedInt(params, "loopnum");
		lp->_resetplayfactor(loopnum);
		return jsonOK(id);
	}
	if (meth == "moveamount") {
		lp->_moveamount = jsonNeedInt(params, "value");
		return jsonOK(id);
	}
	if (meth == "restart") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_restart(loopnum);
		}
		return jsonOK(id);
	}
	if (meth == "restartrandom") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_restartrandom(loopnum);
		}
		return jsonOK(id);
	}
	if (meth == "restartsave") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_restartsave(loopnum);
		}
		return jsonOK(id);
	}
	if (meth == "restartrestore") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_restartrestore(loopnum);
		}
		return jsonOK(id);
	}
	if (meth == "freeze") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_freeze(loopnum, 1);
		}
		return jsonOK(id);
	}
	if (meth == "unfreeze") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_freeze(loopnum, 0);
		}
		return jsonOK(id);
	}
	if (meth == "truncate") {
		int loopnum = jsonNeedInt(params, "loopnum");
		if (lp->validLoopnum(loopnum)) {
			lp->_truncate(loopnum);
		}
		return jsonOK(id);
	}
	if (meth == "xor") {
		lp->_enableXOR = jsonNeedBool(params, "onoff");
		return jsonOK(id);
	}
	if (meth == "border") {
		lp->_border = jsonNeedBool(params, "onoff");
		return jsonOK(id);
	}
	if (meth == "fliph") {
		lp->_set_fliph(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "flipv") {
		lp->_set_flipv(jsonNeedBool(params, "onoff"));
		return jsonOK(id);
	}
	if (meth == "recborder") {
		lp->_recborder = jsonNeedBool(params, "onoff");
		return jsonOK(id);
	}
	if (meth == "setstart") {
		int loopnum = jsonNeedInt(params, "loopnum");
		double pos = jsonNeedDouble(params, "pos");
		if (lp->validLoopnum(loopnum)) {
			lp->_loop[loopnum].setStartPos(pos);
		}
		return jsonOK(id);
	}
	if (meth == "setend") {
		int loopnum = jsonNeedInt(params, "loopnum");
		double pos = jsonNeedDouble(params, "pos");
		if (lp->validLoopnum(loopnum)) {
			lp->_loop[loopnum].setEndPos(pos);
		}
		return jsonOK(id);
	}
	if (meth == "setreverse") {
		int loopnum = jsonNeedInt(params, "loopnum");
		bool onoff = jsonNeedBool(params, "onoff");
		if (lp->validLoopnum(loopnum)) {
			lp->_loop[loopnum].setReverse(onoff);
		}
		return jsonOK(id);
	}
	throw NosuchException("Viz10LoopyCam - Unrecognized method '%s'", meth.c_str());
}

void Viz10LoopyCam::processMidiInput(MidiMsg* m) {
}

void Viz10LoopyCam::processMidiOutput(MidiMsg* m) {
}

bool Viz10LoopyCam::processFrame24Bit() {
	if (m_looper == NULL) {
		m_looper = new Looper(FrameWidth(), FrameHeight());
		// DEBUGPRINT(("----- MALLOC new Looper = %ld", (long)m_looper));
	}
	IplImage* img = FrameImage();
	if (m_looper->_flipv) {
		if (m_looper->_fliph) {
			cvFlip(img, 0, -1);  // flip both
		} else {
			cvFlip(img, 0, 0);  // flip vert
		}
	} else if (m_looper->_fliph) {
		cvFlip(img,0,1);  // flip horiz
	}
	m_looper->processFrame24Bit(img);
	return true;
}

bool Viz10LoopyCam::processFrame32Bit() {
	return false;
}