#include "NosuchUtil.h"
#include "SchedEvent.h"
#include "MidiVizParams.h"

SchedEvent::SchedEvent(MidiMsg* m, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = SchedEvent::MIDIMSG;
	u.midimsg = m;
	init(click, cursorid, looping, mp);

	// Don't loop controllers if loopctrl is not on
	if (m->MidiType() == MIDI_CONTROL && mp->loopctrl == 0 ) {
		m_loopclicks = 0;
	}
	else {
		static int count = 0;
		count++;
		DEBUGPRINT1(("Looping clicks=%d count=%d",m_loopclicks,count));
	}
	if (m->MidiType() != MIDI_CONTROL) {
		DEBUGPRINT1(("SchedEvent constructor A this=%ld cursorid=%d click=%d miditype=%s",(long)this,cursorid,click,m->MidiTypeName()));
	}
}

SchedEvent::SchedEvent(MidiPhrase* ph, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = SchedEvent::MIDIPHRASE;
	u.midiphrase = ph;
	init(click, cursorid, looping, mp);
	DEBUGPRINT1(("SchedEvent B this=%ld cursorid=%d miditype=%s",(long)this,cursorid,m->MidiTypeName()));
	DEBUGPRINT(("Is MidiPhrase actually used?"));
}

SchedEvent::SchedEvent(NosuchCursorMotion* cm, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = SchedEvent::CURSORMOTION;
	u.midimsg = NULL;
	u.cursormotion = cm;
	init(click, cursorid, looping, mp);
	DEBUGPRINT1(("SchedEvent C this=%ld cursorid=%d miditype=%s",(long)this,cursorid,m->MidiTypeName()));
}

void
SchedEvent::init(click_t click_, int cursorid, bool looping, MidiVizParams* mp) {
	m_next = NULL;
	m_prev = NULL;
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
		DEBUGPRINT1(("SchedEvent DELETING midiphrase this=%ld click=%d midimsg=%ld", (long)this, click));
		delete u.midiphrase;
		u.midiphrase = NULL;
		break;
	case MIDIMSG:
		// Should u.midimsg be deleted here?  I believe the answer is NO.
	{
		MidiMsg* m = u.midimsg;
		DEBUGPRINT1(("SchedEvent DELETING midimsg this=%ld click=%d midimsg=%ld type=%s", (long)this, click, (long)m, m->MidiTypeName()));
	}
		break;
	case CURSORMOTION:
		DEBUGPRINT(("Should u.cursormotion be deleted in SchedEvent destructor?"));
		break;
	}
	DEBUGPRINT1(("end of SchedEvent DELETING this=%ld",(long)this));
}

