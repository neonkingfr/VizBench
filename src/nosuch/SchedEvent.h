#ifndef _SCHEDEVENT_H
#define _SCHEDEVENT_H

typedef int click_t;

#include "NosuchUtil.h"
#include "NosuchMidi.h"
#include "NosuchJSON.h"
#include "porttime.h"
#include "NosuchGraphics.h"
#include <pthread.h>
#include <list>
#include <map>
#include <algorithm>

#define OUT_QUEUE_SIZE 1024

#define HANDLE_DEFAULT ((const char*)0)

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

	SchedEvent(MidiMsg* m, click_t c, const char* h, click_t loopclicks = 0);
	SchedEvent(MidiPhrase* ph, click_t c, const char* h, click_t loopclicks = 0);
	SchedEvent(NosuchCursorMotion* cm, click_t c, const char* h, click_t loopclicks = 0);

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
	const char* m_handle;	// handle to thing that created it
	friend class NosuchScheduler;

private:

	int m_eventtype;
	int m_loopclicks;	// if 0, it's not looping

	void init(click_t click, const char* h, click_t loopclicks);
};

typedef std::list<SchedEvent*> SchedEventList;
typedef std::list<SchedEvent*>::iterator SchedEventIterator;

#endif
