#ifndef _VizText_H
#define _VizText_H

class VizText : public Vizlet
{
public:
	VizText();
	~VizText();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(SpaceCursor* c, int downdragup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

private:
	// Put private things here.
};

#endif
