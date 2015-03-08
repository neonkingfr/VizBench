#ifndef _VizPalette_H
#define _VizPalette_H

class PaletteHost;

class VizPalette : public Vizlet
{
public:
	VizPalette();
	~VizPalette();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);
	void processAdvanceTimeTo(int milli);

	Palette* palette();

private:
	// Put private things here.
	PaletteHost* _palettehost;

	// The index of _midiparams is 0 (for default parameters)
	// or 1-16 (for channel-specific parameters)
	AllVizParams* _midiparams[17];

	void _midiVizSprite(MidiMsg* m);
};

#endif
