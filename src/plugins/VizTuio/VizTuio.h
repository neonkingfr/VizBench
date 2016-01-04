#ifndef _VizTuio_H
#define _VizTuio_H

#include "Palette.h"
#include "FreeFrameHost.h"
#include "ResolumeHost.h"

class VizTuio : public Vizlet
{
public:
	VizTuio();
	~VizTuio();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processAdvanceTimeTo(int milli);

	Palette* palette() { return _palette; }

private:

	struct paramsfile_info {
		int channel;
		SpriteVizParams* params;
		std::string paramsfname;
		std::time_t lastfileupdate;
		std::time_t lastfilecheck;
	};

	bool _loadSpriteVizParamsFile(std::string fname, paramsfile_info& spriteinfo);
	void _cursorSprite(VizCursor* c);

	struct paramsfile_info _region[4];
	FreeFrameHost* _freeframeHost;
	Palette* _palette;
};

#endif
