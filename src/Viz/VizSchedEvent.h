#ifndef _SCHEDEVENT_H
#define _SCHEDEVENT_H

#include <list>

class MidiVizParams;
class VizSchedEvent;
class VizSchedEventList;

// typedef std::list<VizSchedEvent*> VizSchedEventList;
// typedef std::list<VizSchedEvent*>::iterator VizSchedEventIterator;

typedef int click_t;

#include "VizUtil.h"
#include "VizMidi.h"
#include "VizJSON.h"
#include "porttime.h"

class MidiMsg;
class MidiPhrase;

class VizCursorMotion {
public:
	VizCursorMotion(int downdragup, VizVector pos, float depth) {
		m_downdragup = downdragup;
		m_pos = pos;
		m_depth = depth;
	}
	int m_downdragup;
	VizVector m_pos;
	float m_depth;
};

class VizSchedEvent {
public:
	enum Type {
		MIDIMSG,
		CURSORMOTION,
		MIDIPHRASE
	};

	VizSchedEvent(MidiMsg* m, click_t c, int cursorid, bool looping=false, MidiVizParams* mp=0);
	VizSchedEvent(MidiPhrase* ph, click_t c, int cursorid, bool loopingfalse, MidiVizParams* mp=0);
	VizSchedEvent(VizCursorMotion* cm, click_t c, int cursorid, bool looping = false, MidiVizParams* mp = 0);

	virtual ~VizSchedEvent();
	std::string DebugString();

	click_t click;	// absolute, not relative
	union {
		MidiMsg* midimsg;
		MidiPhrase* midiphrase;
		VizCursorMotion* cursormotion;
	} u;

	int eventtype() { return m_eventtype; }

protected:
	int m_cursorid;	// Id of cursor that created it, if positive.
					// Negative values may have a different meaning someday,
					// when things other than cursors can originate events
	friend class VizScheduler;

private:

	int m_eventtype;
	int m_loopclicks;	// if 0, it's not looping
	double m_loopfade;	// looping fade factor, 0.0 to 1.0, where 1.0 is no fading

	void init(click_t click, int cursorid, bool looping, MidiVizParams* mp);

	VizSchedEvent* m_next;
	VizSchedEvent* m_prev;
	friend class VizSchedEventList;
};

#endif
