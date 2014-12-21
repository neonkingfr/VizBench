#ifndef _VizDraw_H
#define _VizDraw_H

class VizDraw : public Vizlet
{

public:
	VizDraw();
	~VizDraw();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	void processKeystroke(int key, int downup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

private:
	// Put private things here.
	AllVizParams* _params;
};

#endif
