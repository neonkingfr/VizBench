#ifndef _VIZLET1_H
#define _VIZLET1_H

class Vizlet1 : public Vizlet
{
public:
	Vizlet1();
	~Vizlet1();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processAdvanceClickTo(int clk);
};

#endif