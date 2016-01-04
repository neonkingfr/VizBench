#ifndef _PALETTE_H
#define _PALETTE_H

#include "FreeFrameHost.h"
class PyEvent;

extern char* MuseumGraphics[];
extern char* MayGraphics[];
extern char* BurnGraphics[];

class PaletteParams : public VizParams {
public:
	PaletteParams() {
		musicscale = "newage";
		tonic = 0;
		doquantize = true;
		showfps = false;
		zexponential = 2.0;
		zmultiply = 1.4f;
		switchyz = false;
		area2d = 0.03;
		depth2d = 0.03;
	}
	std::string JsonListOfValues() {
		char *names[] = {
			"musicscale",
			"tonic",
			"doquantize",
			"showfps",
			"zexponential",
			"zmultiply",
			"switchyz",
			"area2d",
			"depth2d",
			NULL
		};
		return _JsonListOfValues(names);
	}
	std::string JsonListOfParams() {
		char *names[] = {
			"musicscale",
			"tonic",
			"doquantize",
			"showfps",
			"zexponential",
			"zmultiply",
			"switchyz",
			"area2d",
			"depth2d",
			NULL
		};
		return _JsonListOfParams(names);
	}
	void Set(std::string nm, std::string val) {
		if ( nm == "musicscale" ) { musicscale = val; }
		else if ( nm == "tonic" ) { tonic = string2int(val); }
		else if ( nm == "doquantize" ) { doquantize = string2bool(val); }
		else if ( nm == "showfps" ) { showfps = string2bool(val); }
		else if ( nm == "zexponential" ) { zexponential = string2double(val); }
		else if ( nm == "zmultiply" ) { zmultiply = string2double(val); }
		else if ( nm == "switchyz" ) { switchyz = string2bool(val); }
		else if ( nm == "area2d" ) { area2d = string2double(val); }
		else if ( nm == "depth2d" ) { depth2d = string2double(val); }
		else if ( nm == "minmove" ) { /* ignore it, it's now a region param */ }
		else {
			NosuchDebug("No such parameter: %s",nm.c_str());
		}
		// To abide by the limits for each value, we rely on the code in Increment()
		Increment(nm,0.0);
	}
	// NOTE: this (the Increment method) is where value ranges get specified/enforced
	void Increment(std::string nm, double amount) {
		if ( nm == "musicscale" ) { NosuchDebug("musicscale increment needs work"); }
		else if ( nm == "tonic" ) { tonic = adjust(tonic,amount,-12,12); }
		else if ( nm == "doquantize" ) { doquantize = adjust(doquantize,amount); }
		else if ( nm == "showfps" ) { showfps = adjust(showfps,amount); }
		else if ( nm == "zexponential" ) { zexponential = adjust(zexponential,amount,-3.0,3.0); }
		else if ( nm == "zmultiply" ) { zmultiply = adjust(zmultiply,amount,0.1,11.0); }
		else if ( nm == "switchyz" ) { switchyz = adjust(switchyz,amount); }
		else if ( nm == "area2d" ) { area2d = adjust(area2d,amount,0.001,1.0); }
		else if ( nm == "depth2d" ) { depth2d = adjust(depth2d,amount,0.001,1.0); }
		else if ( nm == "minmove" ) { /* ignore it, it's now a region param */ }
		else { NosuchDebug("No such parameter: %s",nm.c_str()); }
	}
	void Toggle(std::string nm) {
		if ( nm == "musicscale" ) { NosuchDebug("musicscale can't be toggled"); }
		else if ( nm == "tonic" ) { NosuchDebug("tonic can't be toggled"); }
		else if ( nm == "doquantize" ) { doquantize = !doquantize; }
		else if ( nm == "showfps" ) { showfps = !showfps; }
		else if ( nm == "switchyz" ) { switchyz = !switchyz; }
		else { NosuchDebug("No Toggle implemented for %s",nm.c_str()); }
	}
	std::string Get(std::string nm) {
		if ( nm == "musicscale" ) { return musicscale; }
		else if ( nm == "tonic" ) { return IntString(tonic); }
		else if ( nm == "doquantize" ) { return BoolString(doquantize); }
		else if ( nm == "showfps" ) { return BoolString(showfps); }
		else if ( nm == "zexponential" ) { return DoubleString(zexponential); }
		else if ( nm == "zmultiply" ) { return DoubleString(zmultiply); }
		else if ( nm == "switchyz" ) { return BoolString(switchyz); }
		else if ( nm == "area2d" ) { return DoubleString(area2d); }
		else if ( nm == "depth2d" ) { return DoubleString(depth2d); }
		return "";
	}
	std::string musicscale;
	int tonic;		// -12 to 12
	bool doquantize;
	bool showfps;
	double zexponential;	// -3 to 3
	double zmultiply;		// 0.1 to 11.0
	bool switchyz;
	double area2d;
	double depth2d;
};

class Palette {

public:
	Palette(FreeFrameHost* b);
	~Palette();

	// STATIC STUFF
	static Palette* _singleton;
	static Palette* palette() { return _singleton; }
	static const double UNSET_DOUBLE;
	static const std::string UNSET_STRING;

	static const std::string configSuffix;
	static const std::string configSeparator;
	static bool initialized;
	static int lastsprite;

	static void initialize();

	static int now;   // milliseconds
	static const int idleattract = 0;

	// Checking for isSelectorDown is currently very expensive, so I've disabled it.
	// If you re-enable this, GraphicBehaviour.isSelectorDown() needs to be worked on (i.e.
	// do it in Palette, and only compute it when buttons go down/up.)
	static const bool selector_check = false;

	static int NumMuseumGraphics;
	static int CurrentMuseumGraphic;
	static int NumMayGraphics;
	static int CurrentMayGraphic;
	static int NumBurnGraphics;
	static int CurrentBurnGraphic;

	// NON-STATIC STUFF

	FreeFrameHost* freeframeHost() { return _freeframeHost; }

	void LockPalette() {
#ifndef DO_OSC_AND_HTTP_IN_MAIN_THREAD
		NosuchLock(&_palette_mutex,"palette");
#endif
	}
	void UnlockPalette() {
#ifndef DO_OSC_AND_HTTP_IN_MAIN_THREAD
		NosuchUnlock(&_palette_mutex,"palette");
#endif
	}

	void init_loops();
	Region* RegionForSid(int sidnum);
	Region* NewButtonNamed(std::string nm, int sid_low, int sid_high);
	Region* NewSurfaceNamed(std::string nm, int sid_low, int sid_high);
	void initRegionSounds();
	void setGlobalParamDouble(std::string nm, double f);
	void setGlobalParamString(std::string nm, std::string s);
	Param* getGlobalParam(std::string nm);
	Param* getRegionParam(std::string nm, int region, bool createIfMissing);
	Param* getParam(std::string nm, int region, bool createIfMissing);
	double getGlobalParamDouble(std::string nm);

	int draw();
	void process_cursors_from_buff(MMTT_SharedMemHeader* hdr);
	void process_midi_input(int midimsg);
#if 0
	void make_sprites_along_outlines(MMTT_SharedMemHeader* hdr);
#endif
	void advanceTo(int tm);
	void hitSprites();
	void getCursors(std::list<Cursor*>& cursors);

	void randConfig();
	void saveConfig(std::ofstream& out);
	std::string loadConfig(std::ifstream &f);
	std::string ParamConfigDir();
	std::string ConfigNormalizeSuffix(std::string name);
	void ConfigFiles(std::vector<std::string> &files);
	std::string ConfigPath(std::string name);
	int ConfigIndex(std::vector<std::string> &files, std::string name);
	std::string ConfigNext(std::string name, int dir);
	std::string ConfigNew(std::string name);
	std::string ConfigRand(std::string name);
	std::string ConfigLoadRand(std::string& name);

	void schedSessionEnd(int sidnum);
	MidiMsg* schedNewNoteInClicks(int sid,int ch,int qnt,int pitch);
	MidiMsg* schedNewNoteInMilliseconds(int sid,int ch,int milli,int pitch);

	void ClearAllLoops(bool loopingoff);
	void SetAllLooping(bool looping, double fade);
	void SetAllArpeggio(bool arp);
	void SetAllFullRange(bool arp);

	int CurrentEffectSet() { return _effectSet; }
	int NextEffectSet() { return (_effectSet+1)% _freeframeHost->NumEffectSet(); }
	int PrevEffectSet() {
		int neff = _freeframeHost->NumEffectSet();
		return (_effectSet - 1 + neff) % neff;
	}
	int RandomEffectSet() {
		return (rand() % _freeframeHost->NumEffectSet());
	}
	void LoadEffectSet(int eset);
	void LoadRandomEffectSet();

	std::string ConfigLoad(std::string name);
	void ConfigSave(std::string name);
	std::string ConfigLoadRandom() {
		int newgraphic;
		int ngraphics = NumMuseumGraphics;
		while ( (newgraphic=rand()%ngraphics) == CurrentMuseumGraphic ) {
			// try again
		}
		CurrentMuseumGraphic = newgraphic;
		NosuchDebug("RANDOM GRAPHIC! n=%d",CurrentMuseumGraphic);
		return ConfigLoad(MuseumGraphics[CurrentMuseumGraphic]);
	}

	int findSoundChannel(std::string sound, int regionid);

	// void initRegions();
	Region* getRegion(int region);
	void checkCursorUp(int milli);

	void buttonDown(std::string bn);
	void buttonUp(std::string bn);
	void cursorDown(Cursor* c);
	void cursorDrag(Cursor* c);
	void cursorUp(Cursor* c);

	bool isButtonDown(std::string bn) { return _buttonDown[bn]; }
	void setButtonUsed(std::string bn, bool b) { _buttonUsed[bn] = b; }
	bool isButtonUsed(std::string bn) { return _buttonUsed[bn]; }
	// bool isSelectorDown();
	bool isShiftDown();
	bool isShifted() { return _shifted; }
	void setShifted(bool b) { _shifted = b; }
	int soundBank(int sb = -1) {
		if ( sb >= 0 ) {
			_soundbank = sb;
		}
		return _soundbank;
	}
	int setRegionSound(Region* r, std::string nm);
	int setRegionSound(int rid, std::string nm);
	int setRegionSound(std::string region, std::string nm);
	void changeSoundSet(int selected);
	void UpdateSound(int r);
	Cursor* MostRecentCursorDown() {
		return _recentCursor;
	}
	void SetMostRecentCursorDown(Cursor* c);

	void addPyEvent(PyEvent* e);
	PyEvent* popPyEvent();

	std::vector<Region*> _regions;
	std::vector<Channel*> _channels;

	Region* GetRegionNamed(std::string nm);

private:

	std::list<PyEvent*> _pyevents;

	int _effectSet;
	Cursor* _recentCursor;

	Region* newRegionNamed(std::string nm);

	void clearButtonDownAndUsed() {
		std::map<std::string,bool>::iterator it = _buttonDown.begin();
		for ( ; it!=_buttonDown.end(); it++ ) {
			std::string nm = it->first;
			_buttonDown[nm] = false;
			_buttonUsed[nm] = false;
		}
	}

	std::map<std::string,bool> _buttonDown;
	// bool _buttonDown[32];
	std::map<std::string,bool> _buttonUsed;
	// bool _buttonUsed[32];
	bool _shifted;

	int _soundbank;  // 0 through 7

	FreeFrameHost* _freeframeHost;
	pthread_mutex_t _palette_mutex;

	std::string currentConfig;
	int frames;
	int frames_last;
};

#endif