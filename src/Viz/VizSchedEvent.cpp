#include "VizUtil.h"
#include "VizSchedEvent.h"
#include "MidiVizParams.h"

VizSchedEvent::VizSchedEvent(MidiMsg* m, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = VizSchedEvent::MIDIMSG;
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
		DEBUGPRINT1(("VizSchedEvent constructor A this=%ld cursorid=%d click=%d miditype=%s",(long)this,cursorid,click,m->MidiTypeName()));
	}
}

VizSchedEvent::VizSchedEvent(MidiPhrase* ph, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = VizSchedEvent::MIDIPHRASE;
	u.midiphrase = ph;
	init(click, cursorid, looping, mp);
	DEBUGPRINT1(("VizSchedEvent B this=%ld cursorid=%d miditype=%s",(long)this,cursorid,m->MidiTypeName()));
	DEBUGPRINT(("Is MidiPhrase actually used?"));
}

VizSchedEvent::VizSchedEvent(VizCursorMotion* cm, click_t click, int cursorid, bool looping, MidiVizParams* mp) {
	m_eventtype = VizSchedEvent::CURSORMOTION;
	u.midimsg = NULL;
	u.cursormotion = cm;
	init(click, cursorid, looping, mp);
	DEBUGPRINT1(("VizSchedEvent C this=%ld cursorid=%d miditype=%s",(long)this,cursorid,m->MidiTypeName()));
}

void
VizSchedEvent::init(click_t click_, int cursorid, bool looping, MidiVizParams* mp) {
	m_next = NULL;
	m_prev = NULL;
	click = click_;
	m_cursorid = cursorid;
	if (mp == NULL) {
		DEBUGPRINT(("Hey, mp==NULL in VizSchedEvent::init!?"));
		m_loopclicks = 0;
		m_loopfade = 0.0;
	} else {
		m_loopclicks = looping ? mp->loopclicks : 0;
		m_loopfade = mp->loopfade;
	}
}

VizSchedEvent::~VizSchedEvent() {
	switch(m_eventtype) {
	case MIDIPHRASE:
		DEBUGPRINT1(("VizSchedEvent DELETING midiphrase this=%ld click=%d midimsg=%ld", (long)this, click));
		delete u.midiphrase;
		u.midiphrase = NULL;
		break;
	case MIDIMSG:
		// Should u.midimsg be deleted here?  I believe the answer is NO.
	{
		MidiMsg* m = u.midimsg;
		DEBUGPRINT1(("VizSchedEvent DELETING midimsg this=%ld click=%d midimsg=%ld type=%s", (long)this, click, (long)m, m->MidiTypeName()));
	}
		break;
	case CURSORMOTION:
		DEBUGPRINT(("Should u.cursormotion be deleted in VizSchedEvent destructor?"));
		break;
	}
	DEBUGPRINT1(("end of VizSchedEvent DELETING this=%ld",(long)this));
}

