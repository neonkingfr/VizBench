#ifndef _BEHAVIOUR_H
#define _BEHAVIOUR_H

#if 0
#include <string>
#include "NosuchGraphics.h"

class Vizlet;
class MidiMsg;
class SpaceCursor;

class Behaviour {
public:
	Behaviour() { }
	virtual void processMidi(MidiMsg* m, bool isinput) { }
	virtual void processCursor(SpaceCursor* c, int downdragup) { }
};
#endif


#endif
