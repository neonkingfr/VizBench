#ifndef _Viz10Example1_H
#define _Viz10Example1_H

class Viz10Example1 : public Vizlet10
{
public:
	Viz10Example1();
	~Viz10Example1();

	static DWORD __stdcall CreateInstance(CFreeFrame10Plugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);
	bool processFrame24Bit();
	bool processFrame32Bit();

private:
	// Put private things here.
};

#endif
