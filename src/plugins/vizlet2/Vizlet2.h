#ifndef _VIZLET2_H
#define _VIZLET2_H

class Vizlet2 : public Vizlet
{
public:
	Vizlet2();
	~Vizlet2();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

private:

	std::string _shape;
	int _minpitch;
	int _maxpitch;
	int _stepmilli;

#define DISPLEN 128
	std::string _s;
	char _disp[DISPLEN];
};

#endif