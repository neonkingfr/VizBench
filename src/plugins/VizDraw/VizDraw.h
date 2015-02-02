#ifndef _VizDraw_H
#define _VizDraw_H

class VizDraw : public Vizlet
{

public:
	VizDraw();
	~VizDraw();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

private:
	// Put private things here.
	AllVizParams* m_params;
	std::string m_spriteparams;
};

#endif
