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

Viz10LoopyCam::Viz10LoopyCam() : Vizlet10() {
	m_looper = NULL;
}

Viz10LoopyCam::~Viz10LoopyCam() {
}

DWORD __stdcall Viz10LoopyCam::CreateInstance(CFreeFrame10Plugin **ppInstance) {
	*ppInstance = new Viz10LoopyCam();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void Viz10LoopyCam::processCursor(VizCursor* c, int downdragup) {
}

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
			"alllive(onoff);autonext(n);togglesmooth;setsmooth(v);"
			"setinterp(v);nextloop;playfactor(loopnum,factor);"
			"playfactorreset(loopnum);moveamount(amount);"
			"restart(loopnum);restartrandom(loopnum);"
			"restartsave(loopnum);restartrestore(loopnum);"
			"freeze(loopnum);unfreeze(loopnum);"
			"truncate(loopnum);xor(onoff);border(onoff);"
			"fliph(onoff);flipv(onoff);recborder(onoff);dotrail(onoff);"
			"trail(amount);setstart(loopnum,pos);"
			"setend(loopnum,pos);setreverse(loopnum,onoff);"
			, id);
	}
	if (meth == "record") {
		bool onoff = (jsonNeedInt(params,"onoff") != 0);
		lp->setRecord(onoff);
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
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		if (lp->validLoopnum(loopnum)) {
			lp->setPlay(loopnum, onoff);
		}
		return jsonOK(id);
	}
	if (meth == "blackout") {
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		lp->setBlackout(onoff);
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
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		lp->_setRecordOverlay(onoff);
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
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		lp->_allLive(onoff);
		return jsonOK(id);
	}
	if (meth == "autonext") {
		int v = jsonNeedInt(params, "n");
		lp->_setautoNext(v);
		return jsonOK(id);
	}
	if (meth == "togglesmooth") {
		lp->togglesmooth();
		return jsonOK(id);
	}
	if (meth == "setsmooth") {
		int v = jsonNeedInt(params, "v");
		lp->setsmooth(v);
		return jsonOK(id);
	}
	if (meth == "setinterp") {
		int v = jsonNeedInt(params, "v");
		lp->setinterp(v);
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
		lp->_moveamount = jsonNeedInt(params, "amount");
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
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		lp->_enableXOR = (onoff ? 1 : 0);
		return jsonOK(id);
	}
	if (meth == "border") {
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		lp->_border = (onoff ? 1 : 0);
		return jsonOK(id);
	}
	if (meth == "fliph") {
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		DEBUGPRINT(("IGNORING FLIPH onoff=%d\n", onoff));
		// set_fliph(onoff!=0);
		// post2flip->setparam("Horizontal",(float)onoff);
		return jsonOK(id);
	}
	if (meth == "flipv") {
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		DEBUGPRINT(("IGNORING FLIPV onoff=%d\n", onoff));
		// _set_flipv(onoff!=0);
		// post2flip->setparam("Vertical",(float)onoff);
		return jsonOK(id);
	}
	if (meth == "recborder") {
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
		lp->_recborder = (onoff ? 1 : 0);
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
		bool onoff = (jsonNeedInt(params, "onoff") != 0);
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
	}
	IplImage* img = FrameImage();
	cvFlip(img);
	m_looper->processFrame24Bit(img);
	return true;
}

bool Viz10LoopyCam::processFrame32Bit() {
	return false;
}