#ifndef _Vizlet10Template_H
#define _Vizlet10Template_H

class Vizlet10Template : public Vizlet10
{
public:
	Vizlet10Template();
	~Vizlet10Template();

	static DWORD __stdcall CreateInstance(CFreeFrame10Plugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processFrame24Bit();
	bool processFrame32Bit();

private:
	// Put private things here.
};

#endif
