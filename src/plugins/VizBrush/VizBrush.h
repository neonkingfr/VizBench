#ifndef _VizBrush_H
#define _VizBrush_H

class VizBrush : public Vizlet
{
public:
	VizBrush();
	~VizBrush();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

};

#endif
