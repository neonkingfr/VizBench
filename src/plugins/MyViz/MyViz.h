#ifndef _MyViz_H
#define _MyViz_H

class MyViz : public Vizlet
{
public:
	MyViz();
	~MyViz();

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
