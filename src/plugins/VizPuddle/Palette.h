#ifndef _PALETTE_H
#define _PALETTE_H

#include <string>

class FreeFrameHost;

class Palette {
public:
	Palette(FreeFrameHost* ffhost);
	void AdvanceTo(int milli);
	void ConfigLoad(std::string pname);

private:
	FreeFrameHost* _freeframeHost;
};

#endif
