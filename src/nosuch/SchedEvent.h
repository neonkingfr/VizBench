#ifndef _SCHEDEVENT_H
#define _SCHEDEVENT_H

#include <list>

class MidiVizParams;
class SchedEvent;

typedef std::list<SchedEvent*> SchedEventList;
typedef std::list<SchedEvent*>::iterator SchedEventIterator;

typedef int click_t;

#include "NosuchUtil.h"
#include "NosuchMidi.h"
#include "NosuchJSON.h"
#include "porttime.h"
#if 0
#include "NosuchGraphics.h"
#include "MidiVizParams.h"
#include <pthread.h>
#include <map>
#include <algorithm>
#endif

class MidiMsg;
class MidiPhrase;

class NosuchCursorMotion {
public:
	NosuchCursorMotion(int downdragup, NosuchVector pos, float depth) {
		m_downdragup = downdragup;
		m_pos = pos;
		m_depth = depth;
	}
	int m_downdragup;
	NosuchVector m_pos;
	float m_depth;
};

class SchedEvent {
public:
	enum Type {
		MIDIMSG,
		CURSORMOTION,
		MIDIPHRASE
	};

	SchedEvent(MidiMsg* m, click_t c, int cursorid, bool looping=false, MidiVizParams* mp=0);
	SchedEvent(MidiPhrase* ph, click_t c, int cursorid, bool loopingfalse, MidiVizParams* mp=0);
	SchedEvent(NosuchCursorMotion* cm, click_t c, int cursorid, bool looping = false, MidiVizParams* mp = 0);

	virtual ~SchedEvent();
	std::string DebugString();

	click_t click;	// relative when in loops, absolute elsewhere
	union {
		MidiMsg* midimsg;
		MidiPhrase* midiphrase;
		NosuchCursorMotion* cursormotion;
	} u;

	int eventtype() { return m_eventtype; }

protected:
	int m_cursorid;	// Id of cursor that created it, if positive.
					// Negative values may have a different meaning someday,
					// when things other than cursors can originate events
	friend class NosuchScheduler;

private:

	int m_eventtype;
	int m_loopclicks;	// if 0, it's not looping
	double m_loopfade;	// looping fade factor, 0.0 to 1.0, where 1.0 is no fading

	void init(click_t click, int cursorid, bool looping, MidiVizParams* mp);
};

#endif
