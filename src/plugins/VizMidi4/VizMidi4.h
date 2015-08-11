#ifndef _VizMidi4_H
#define _VizMidi4_H

class sprite_info {
public:
	sprite_info(int c, int p) : channel(c), pitch(p) {
	}
	int channel;
	int pitch;
};

class VizMidi4 : public Vizlet
{
public:
	VizMidi4();
	virtual ~VizMidi4();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processAdvanceClickTo(int click);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

private:
	// Put private things here.

	struct paramsfile_info {
		int channel;			// 0 means unused, otherwise 1-16
		AllVizParams* params;
		std::string paramsfile;
		std::string paramspath;
		std::time_t lastfileupdate;
		std::time_t lastfilecheck;
	};

	bool _loadParamsFile(std::string file, VizMidi4::paramsfile_info& spriteinfo);
	VizSprite* _midiVizNoteOnSprite(MidiMsg* m);
	std::string _set_params_on(int n, cJSON* json, const char* id);
	std::string _set_params_off(int n, cJSON* json, const char* id);
	std::string _set_params(paramsfile_info& spriteinfo, cJSON* json, const char* id);
	void _reload_params(paramsfile_info& si);

	struct paramsfile_info m_sprite_on[4];
	struct paramsfile_info m_sprite_off[4];

	bool m_autoloadparams;
};

#endif
