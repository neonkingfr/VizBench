#ifndef _VizMidi4_H
#define _VizMidi4_H

class VizMidi4 : public Vizlet
{
public:
	VizMidi4();
	virtual ~VizMidi4();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

private:
	// Put private things here.

	bool _loadParamsFile(int n, std::string file);
	void _midiVizSprite(MidiMsg* m);
	std::string _set_params(int n, cJSON* json, const char* id);

	struct sprite_info {
		int channel;			// 0 means unused, otherwise 1-16
		AllVizParams* params;
		std::string paramsfile;
		std::string paramspath;
		std::time_t lastfileupdate;
		std::time_t lastfilecheck;
	} m_sprite[4];

	bool m_autoloadparams;
};

#endif
