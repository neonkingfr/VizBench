#ifndef _PALETTE_H
#define _PALETTE_H

#include <map>

class Param;
class VizCursor;

class PaletteHost;

class Palette {

public:
	Palette(PaletteHost* b);
	~Palette();

	// THESE SHOULD NOT BE STATIC!!
	static int lastsprite;
	static int now;   // milliseconds
	static const int idleattract = 0;


	static const double UNSET_DOUBLE;
	static const std::string UNSET_STRING;

	static const std::string configSuffix;
	static const std::string configSeparator;

	// NON-STATIC STUFF

	PaletteHost* paletteHost() { return _paletteHost; }
	NosuchScheduler* scheduler() { return _paletteHost->scheduler(); }

	void LockPalette() {
		NosuchLock(&_palette_mutex,"palette");
	}
	void UnlockPalette() {
		NosuchUnlock(&_palette_mutex,"palette");
	}

	// void init_loops();
	Region* RegionForSid(int sidnum);
	Region* NewButtonNamed(std::string nm, int sid_low, int sid_high,std::string patchname);
	Region* NewRegionNamed(std::string nm, int sid_low, int sid_high);
	Patch* NewPatchNamed(std::string nm);
	Patch* GetPatchNamed(std::string nm);
	void initRegionSounds();
	void applyPatch(std::string nm);

	int draw();
	void process_cursors_from_buff(MMTT_SharedMemHeader* hdr);
	void process_midi_input(int midimsg);
	void make_sprites_along_outlines(MMTT_SharedMemHeader* hdr);
	void advanceTo(int tm);
	void hitPaletteSprites();

	void randConfig();
	std::string ParamConfigDir();
	std::string ConfigNormalizeSuffix(std::string name);

	std::string ConfigPath(std::string name);

	MidiMsg* schedNewNoteInClicks(int sid,int ch,int qnt,int pitch);
	MidiMsg* schedNewNoteInMilliseconds(int sid,int ch,int milli,int pitch);

	void SetAllArpeggio(bool arp);
	void SetAllFullRange(bool arp);

	void LoadEffectSet(std::string effects) {
		_paletteHost->LoadEffectSet(effects);
	}

	std::string ConfigLoad(std::string name);

	int findSoundChannel(std::string sound, int regionid);

	// void initRegions();
	Region* getRegion(int region);

	void processCursor(VizCursor* c, int downdraguup);

	void buttonDown(std::string bn);
	void buttonUp(std::string bn);

	bool isButtonDown(std::string bn) { return _buttonDown[bn]; }
	void setButtonUsed(std::string bn, bool b) { _buttonUsed[bn] = b; }
	bool isButtonUsed(std::string bn) { return _buttonUsed[bn]; }
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
	VizCursor* MostRecentVizCursorDown() {
		return _recentVizCursor;
	}
	void SetMostRecentVizCursorDown(VizCursor* c);

	std::vector<Region*> _regions;
	std::vector<Patch*> _patches;
	std::vector<Channel*> _channels;

	Region* GetRegionNamed(std::string nm);
	Vizlet* vizlet() { return _paletteHost->vizlet(); }

	int tonic;
	std::string musicscale;

private:

	VizCursor* _recentVizCursor;

	std::string _loadConfig(std::ifstream &f);
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

	PaletteHost* _paletteHost;
	pthread_mutex_t _palette_mutex;

	std::string currentConfig;
	int frames;
	int frames_last;
};

#endif