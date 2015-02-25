#ifndef _Viz10LoopyCam_H
#define _Viz10LoopyCam_H

class Viz10LoopyCam : public Vizlet10
{
public:
	Viz10LoopyCam();
	~Viz10LoopyCam();

	static DWORD __stdcall CreateInstance(CFreeFrame10Plugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processFrame24Bit();
	bool processFrame32Bit();

private:
	std::string realProcessJson(std::string meth, cJSON *jsonparams, const char *id);
	// Put private things here.
	Looper* m_looper;
};

#endif
