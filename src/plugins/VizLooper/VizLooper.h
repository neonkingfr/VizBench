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
	AllVizParams* m_midiparams;
	std::string m_spriteparamspath;
	bool m_autoloadparams;
	std::time_t m_lastfileupdate;
	std::time_t m_lastfilecheck;

	int m_midiin;
	int m_midiout;
};

#endif
