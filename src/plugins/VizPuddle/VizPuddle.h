#ifndef _VizPuddle_H
#define _VizPuddle_H

#include "Palette.h"
#include "FreeFrameHost.h"
#include "ResolumeHost.h"
#include "MidiVizParams.h"

class VizPuddle : public Vizlet
{
public:
	VizPuddle();
	~VizPuddle();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processAdvanceTimeTo(int milli);

	Palette* palette() { return _palette; }

private:

	class SidRangeable {
	public:
		int sid_min;
		int sid_max;
		// std::string sid_source;   // eventually
	};

	class Region : public SidRangeable {
	public:
		Region(std::string nm) {
			name = nm;
			sid_min = 0;
			sid_max = 0;
			spriteparamfile = nm + "_default";
			midiparamfile = nm + "_default";
			spriteparams = NULL;
			midiparams = NULL;
		}
		std::string name;
		SpriteVizParams* spriteparams;
		std::string spriteparamfile;
		std::time_t lastspritefileupdate;
		std::time_t lastspritefilecheck;
		MidiVizParams* midiparams;
		std::string midiparamfile;
		std::time_t lastmidifileupdate;
		std::time_t lastmidifilecheck;
	};

	class Button : public SidRangeable {
	public:
		Button(std::string nm) {
			name = nm;
			sid_min = 0;
			sid_max = 0;
			pipeline = "puddle_default";
		}
		std::string name;
		std::string pipeline;
	};

	std::string _set_sidrange(SidRangeable* r, cJSON* json, const char* id);

	std::string _set_region_spriteparams(Region* r, cJSON* json, const char* id);
	std::string _set_region_midiparams(Region* r, cJSON* json, const char* id);

	std::string _set_button_pipeline(Button* r, cJSON* json, const char* id);

	bool _loadSpriteVizParamsFile(std::string fname, Region* r);
	bool _loadMidiVizParamsFile(std::string fname, Region* r);

	void _reloadParams(Region* r);

	void _cursorSprite(VizCursor* c, SpriteVizParams* p);
	void _cursorMidi(VizCursor* c, MidiVizParams* p);

	int _pitchOf(VizCursor* c);
	int _velocityOf(VizCursor* c);
	int _channelOf(VizCursor* c);

	std::map<std::string,Region*> _region;
	std::map<std::string,Button*> _button;

	FreeFrameHost* _freeframeHost;
	Palette* _palette;
	bool _autoloadparams;
};

#endif
