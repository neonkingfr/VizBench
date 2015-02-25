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
			"blackout(onoff);randompositions(aspect);randomposition1(aspect)"
			"recordoverlay(onoff);morewindows;lesswindows;"
			"setwindows(numwindows);fulldisplay;quadrantdisplay;"
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
#if 0
	if (meth == "alllive") {
		int onoff = getAsInt32(nargs, types, arg, 1);
		_allLive(onoff);
		// } else if ( s == "sizefactorall" ) {
		// 	float x = m.getArgAsFloat(0);
		// 	for ( int n=0; n<MAX_LOOPS; n++ ) {
		// 		_sizeFactor[n] = x;
		// 		_changePosSize(n,_pos[n],cvSize((int)(_sz[n].width*_sizeFactor[n]),int(_sz[n].height*_sizeFactor[n])));
		// }
	}
	if (meth == "autonext") {
		int v = getAsInt32(nargs, types, arg);
		_setautoNext(v);
	}
	if (meth == "togglesmooth") {
		togglesmooth();
	}
	if (meth == "setsmooth") {
		int v = getAsInt32(nargs, types, arg);
		setsmooth(v);
	}
	if (meth == "setinterp") {
		int v = getAsInt32(nargs, types, arg);
		setinterp(v);
	}
	if (meth == "nextloop") {
		_nextLoop();
	}
	if (meth == "playfactor") {
		int loopnum = getAsInt32(nargs, types, arg);
		float x = getAsFloat(nargs, types, arg, 0.5);
		_setplayfactor(loopnum, x);
	}
	if (meth == "playfactorreset") {
		int loopnum = getAsInt32(nargs, types, arg);
		_resetplayfactor(loopnum);
	}
	if (meth == "moveamount") {
		_moveamount = getAsInt32(nargs, types, arg);
	}
	if (meth == "restart") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_restart(loopnum);
		}
	}
	if (meth == "restartrandom") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_restartrandom(loopnum);
		}
	}
	if (meth == "restartsave") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_restartsave(loopnum);
		}
	}
	if (meth == "restartrestore") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_restartrestore(loopnum);
		}
	}
	if (meth == "freeze") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_freeze(loopnum, 1);
		}
	}
	if (meth == "unfreeze") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_freeze(loopnum, 0);
		}
	}
	if (meth == "truncate") {
		int loopnum = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_truncate(loopnum);
		}
	}
	if (meth == "xor") {
		int onoff = getAsInt32(nargs, types, arg);
		_enableXOR = (onoff ? 1 : 0);
	}
	if (meth == "border") {
		int onoff = getAsInt32(nargs, types, arg);
		_border = (onoff ? 1 : 0);
	}
	if (meth == "fliph") {
		int onoff = getAsInt32(nargs, types, arg);
		NS_debug("IGNORING FLIPH onoff=%d\n", onoff);
		// set_fliph(onoff!=0);
		// post2flip->setparam("Horizontal",(float)onoff);
	}
	if (meth == "flipv") {
		int onoff = getAsInt32(nargs, types, arg);
		NS_debug("IGNORING FLIPV onoff=%d\n", onoff);
		// _set_flipv(onoff!=0);
		// post2flip->setparam("Vertical",(float)onoff);
	}
	if (meth == "recborder") {
		int onoff = getAsInt32(nargs, types, arg);
		_recborder = (onoff ? 1 : 0);
	}
	if (meth == "dotrail") {
		int onoff = getAsInt32(nargs, types, arg);
		_trail = (onoff ? 1 : 0);
		_set_trail();
	}
	if (meth == "trail") {
		float x = getAsFloat(nargs, types, arg);
		_trailamount = x;
		_set_trail();
	}
	if (meth == "setstart") {
		int loopnum = getAsInt32(nargs, types, arg);
		float x = getAsFloat(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_loop[loopnum].setStartPos(x);
		}
	}
	if (meth == "setend") {
		int loopnum = getAsInt32(nargs, types, arg);
		float x = getAsFloat(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_loop[loopnum].setEndPos(x);
		}
	}
	if (meth == "setreverse") {
		int loopnum = getAsInt32(nargs, types, arg);
		int onoff = getAsInt32(nargs, types, arg);
		if (_validLoopnum(loopnum)) {
			_loop[loopnum].setReverse(onoff);
		}
	}
#endif
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