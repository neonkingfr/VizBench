#ifndef _CURSORBEHAVIOUR_H
#define _CURSORBEHAVIOUR_H

class VizCursor;
class Vizlet;
class SpriteVizParams;
class MidiVizParams;

class CursorBehaviour {
public:
	CursorBehaviour(Vizlet* vizlet);
	void _trackCursors(VizCursor* c, int downdragup);
	bool _inRegion(VizCursor* c);
	void _reloadParams();
	std::string _set_region_spriteparams(cJSON* json, const char* id);
	std::string _set_looping(cJSON* json, const char* id);
	std::string _set_sidrange(cJSON* json, const char* id);
	bool _loadSpriteVizParamsFile(std::string fname);
	std::string _set_region_midiparams(cJSON* json, const char* id);
	bool _loadMidiVizParamsFile(std::string fname);
	click_t _loopClicks();
	void _cursorMidi(VizCursor* c, int downdragup);
	void _queueRegionMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid);
	int _channelOf(VizCursor* c, MidiVizParams* mp);
	int _interpolate(double f, int minval, int maxval);
	int _pitchOf(VizCursor* c, MidiVizParams* mp);
	int _velocityOf(VizCursor* c);
	click_t _durationOf(VizCursor* c);
	click_t _quantOf(VizCursor* c);
	click_t _quantizeToNext(click_t tm, click_t q);
	void _queueRegionMidiMsg(MidiMsg* m, click_t clk, int cursorid);
	void _genArpeggiatedMidi(VizCursor* c, int downdragup);
	void _genControlMidi(VizCursor* c, int downdragup);
	void _genControllerReset(VizCursor* c);
	int _outputPort(MidiVizParams* mp);
	void _queueNoteonWithNoteoffPending(VizCursor* c);
	void _finishNote(VizCursor* c, click_t noteoff_click, int outport);
	void _genNormalMidi(VizCursor* c, int downdragup);
	void _cursorSprite(VizCursor* c, int downdragup);
	click_t SchedulerCurrentClick();

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
	bool m_looping;
	Vizlet* m_vizlet;
	bool m_autoloadparams;

	std::set<const VizCursor*> m_cursors;
	int m_controllerval;  // current controller value (saved for smoothing)
};

#endif
