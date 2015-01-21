#ifndef _VizLooper_H
#define _VizLooper_H

#include <ctime>

class VizLooper : public Vizlet
{
public:
	VizLooper();
	~VizLooper();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

private:
	// Put private things here.

	void _midiVizSprite(MidiMsg* m);
	AllVizParams* _midiparams;
	std::string _spriteparamspath;
	bool _autoloadparams;
	std::time_t _lastfileupdate;
	std::time_t _lastfilecheck;

	int _midiin;
	int _midiout;
};

#endif
