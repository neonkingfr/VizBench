#include "NosuchUtil.h"
#include "SchedEvent.h"
#include "MidiVizParams.h"

SchedEvent::SchedEvent(MidiMsg* m, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = SchedEvent::MIDIMSG;
	u.midimsg = m;
	init(click, cursorid, looping, mp);
}

SchedEvent::SchedEvent(MidiPhrase* ph, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = SchedEvent::MIDIPHRASE;
	u.midiphrase = ph;
	init(click, cursorid, looping, mp);
}

SchedEvent::SchedEvent(NosuchCursorMotion* cm, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = SchedEvent::CURSORMOTION;
	u.midimsg = NULL;
	u.cursormotion = cm;
	init(click, cursorid, looping, mp);
}

void
SchedEvent::init(click_t click_, int cursorid, bool looping, MidiVizParams* mp) {
	click = click_;
	m_cursorid = cursorid;
	if (mp == NULL) {
		DEBUGPRINT(("Hey, mp==NULL in SchedEvent::init!?"));
		m_loopclicks = 0;
		m_loopfade = 0.0;
	} else {
		m_loopclicks = looping ? mp->loopclicks : 0;
		m_loopfade = mp->loopfade;
	}
}

SchedEvent::~SchedEvent() {
	switch(m_eventtype) {
	case MIDIPHRASE:
		delete u.midiphrase;
		u.midiphrase = NULL;
		break;
	case MIDIMSG:
		// Should u.midimsg be deleted here?  I believe the answer is NO.
		break;
	case CURSORMOTION:
		DEBUGPRINT(("Should u.cursormotion be deleted in SchedEvent destructor?"));
		break;
	}
}

