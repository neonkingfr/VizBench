#ifndef _VizTuio_H
#define _VizTuio_H

#include "Palette.h"
#include "FreeFrameHost.h"
#include "ResolumeHost.h"
#include "MidiVizParams.h"

class VizTuio : public Vizlet
{
public:
	VizTuio();
	~VizTuio();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processAdvanceTimeTo(int milli);

	Palette* palette() { return _palette; }

private:

	struct region_info {
		std::string sid;
		int sid_min;
		int sid_max;
		SpriteVizParams* spriteparams;
		std::string spriteparamfile;
		std::time_t lastspritefileupdate;
		std::time_t lastspritefilecheck;
		MidiVizParams* midiparams;
		std::string midiparamfile;
		std::time_t lastmidifileupdate;
		std::time_t lastmidifilecheck;
	};

	struct button_info {
		std::string sid;
		int sid_min;
		int sid_max;
		std::string pipeline;
	};

	std::string _set_region_spriteparams(region_info& regioninfo, cJSON* json, const char* id);
	bool _loadSpriteVizParamsFile(std::string fname, region_info& regioninfo);
	std::string _set_region_midiparams(region_info& regioninfo, cJSON* json, const char* id);
	bool _loadMidiVizParamsFile(std::string fname, region_info& regioninfo);
	std::string _set_region_sid(region_info& regioninfo, cJSON* json, const char* id);

	void _cursorSprite(VizCursor* c);

	struct region_info _region[4];
	struct button_info _button[12];
	FreeFrameHost* _freeframeHost;
	Palette* _palette;
	bool _autoloadparams;
};

#endif
