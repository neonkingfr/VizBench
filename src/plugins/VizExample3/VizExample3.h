#ifndef _VizExample3_H
#define _VizExample3_H

class VizExample3 : public Vizlet
{

public:
	VizExample3();
	virtual ~VizExample3();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();

private:
	// Put private things here.
	SpriteVizParams* m_params;
	std::string m_spriteparams;
};

#endif
