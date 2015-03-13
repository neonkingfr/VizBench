#ifndef _VizletGLTemplate_H
#define _VizletGLTemplate_H

class VizletGLTemplate : public Vizlet
{
public:
	VizletGLTemplate();
	virtual ~VizletGLTemplate();

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
