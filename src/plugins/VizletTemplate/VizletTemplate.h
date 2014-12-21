#ifndef _VizletTemplate_H
#define _VizletTemplate_H

class VizletTemplate : public Vizlet
{
public:
	VizletTemplate();
	~VizletTemplate();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

private:
	// Put private things here.
};

#endif
