#ifndef _VizPuddle_H
#define _VizPuddle_H

#include <set>

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
			sid_max = 99999;
			spriteparamfile = nm + "_default";
			midiparamfile = nm + "_default";
			spriteparams = NULL;
			midiparams = NULL;
			m_controllerval = 0;
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

		std::set<VizCursor*> m_cursors;
		int m_controllerval;
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

	Region* _findRegion(VizCursor* c);
	void _trackCursorsPerRegion(VizCursor* c, int downdragup, Region* r);
	void _cursorSprite(VizCursor* c, int downdragup, Region* r);
	void _cursorMidi(VizCursor* c, int downdragup, Region* r);

	void _queueNoteonWithNoteoffPending(VizCursor* c, MidiVizParams* mp);

	void _genArpeggiatedMidi(VizCursor* c, int downdragup, MidiVizParams* mp);
	void _genNormalMidi(VizCursor* c, int downdragup, MidiVizParams* mp);
	void _genControlMidi(VizCursor* c, int downdragup, MidiVizParams* mp);
	void _genControllerReset(VizCursor* c, MidiVizParams* mp);

	int _pitchOf(VizCursor* c, MidiVizParams* mp);
	int _velocityOf(VizCursor* c);
	int _channelOf(VizCursor* c, MidiVizParams* mp);
	click_t _durationOf(VizCursor* c);
	click_t _quantOf(VizCursor* c);
	click_t _clicksPerBeat();
	click_t _quantizeToNext(click_t tm, click_t q);

	std::map<std::string,Region*> _region;
	std::map<std::string,Button*> _button;

	bool _autoloadparams;

#define MAX_MIDI_PORTS 64
	const char* m_porthandle[MAX_MIDI_PORTS];
};

#endif
