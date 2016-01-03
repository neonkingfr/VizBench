#ifndef _FREEFRAMEHOST_H
#define _FREEFRAMEHOST_H

#include <string>

class FreeFrameHost {
public:
	FreeFrameHost();
	virtual ~FreeFrameHost();

	virtual int NumEffectSet() = 0;
	virtual int EnableEffect(int effectnum, bool enabled) = 0;
	virtual void ShowChoice(std::string bn, std::string text, int x, int y, int timeout) = 0;
	virtual void ShowAttract(int onoff) = 0;

private:

};

#endif
