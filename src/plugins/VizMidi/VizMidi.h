#ifndef _VizMidi_H
#define _VizMidi_H

class VizMidi : public Vizlet
{
public:
	VizMidi();
	~VizMidi();

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
	std::string _spriteparams;
};

#endif
