#include "NosuchUtil.h"
#include "NosuchLooper.h"
#include "NosuchScheduler.h"

NosuchLooper::NosuchLooper() {
	DEBUGPRINT2(("NosuchLooper constructor"));
	m_last_click = 0;
	NosuchLockInit(&_looper_mutex,"looper");
}

NosuchLooper::~NosuchLooper() {
	DEBUGPRINT2(("NosuchLooper destructor"));
}

std::string
NosuchLooper::DebugString() {
	Lock();
	std::string s;
	std::vector<NosuchLoop*>::iterator it = m_loops.begin();
	int n = 0;
	for(; it != m_loops.end(); it++) {
		NosuchLoop *lp = *it;
		NosuchAssert(lp);
		s += NosuchSnprintf("Loop %d\n%s",n++,lp->DebugString("   ").c_str());
	}
	Unlock();
	return s;
}

void
NosuchLooper::AddLoop(NosuchLoop* loop) {
	Lock();
	m_loops.push_back(loop);
	Unlock();
}

void
NosuchLooper::AdvanceClickTo(click_t click, NosuchScheduler* sched) {
	Lock();
	int nclicks = click - m_last_click;
	static int last_printed_click = 0;
	if ( click >= (last_printed_click+10*sched->m_ClicksPerSecond) ) {
		DEBUGPRINT1(("NosuchLooper::AdvanceClickTo click=%d now=%d",click,Pt_Time()));
		last_printed_click = click;
	}
	while ( nclicks-- > 0 ) {
		std::vector<NosuchLoop*>::iterator it = m_loops.begin();
		for(; it != m_loops.end(); it++) {
			NosuchLoop* lp = *it;
			NosuchAssert(lp);
			lp->AdvanceClickBy1();
		}
	}
	m_last_click = click;
	Unlock();
}

static bool
loopevent_compare (SchedEvent* first, SchedEvent* second)
{
	if ( first->click < second->click )
		return true;
	else
		return false;
}

void
NosuchLoop::SendMidiLoopOutput(MidiMsg* mm) {
#if 0
	m_vizlet->SendMidiMsg();
	// _vizlet->scheduler()->SendMidiMsg(mm,id());
	// noticeMidiOutput(mm,XXX);
#endif
}

std::string
NosuchLoop::DebugString(std::string indent) {
	Lock();
	SchedEventIterator it = m_events.begin();
	int n = 0;
	std::string s = NosuchSnprintf("Loop id=%d {\n",m_id);
	for(; it != m_events.end(); it++) {
		SchedEvent* ep = *it;
		NosuchAssert(ep);
		s += (indent+NosuchSnprintf("   Event %d = %s\n",n++,ep->DebugString().c_str()));
	}
	s += "   }";
	Unlock();
	return s;
}

int
NosuchLoop::AddLoopEvent(SchedEvent* e) {
	int nn;
	Lock();
	m_events.push_back(e);
	m_events.sort(loopevent_compare);
	nn = NumNotes();
	Unlock();
	return nn;
}

bool
isMatchingOff(SchedEvent* ev, MidiNoteOn* note) {
	MidiMsg* m = ev->u.midimsg;
	if ( m->MidiType() == MIDI_NOTE_OFF
		&& m->Channel() == note->Channel()
		&& m->Pitch() == note->Pitch() ) {
		return true;
	}
	return false;
}

void NosuchLoop::AdvanceClickBy1() {
	Lock();
	m_click++;
	int clicklength = QuarterNoteClicks * m_looplength;
	if ( m_click > clicklength ) {
		m_click = 0;
	}

	SchedEventIterator it = m_events.begin();
	for(; it != m_events.end(); ) {

		SchedEvent* ev = *it;
		NosuchAssert(ev);
		if ( ev->click != m_click ) {
			it++;
			continue;
		}

		if ( ev->eventtype() == SchedEvent::CURSORMOTION ) {
			it++;
			DEBUGPRINT(("LOOP CURSORMOTION!  event=%s",ev->DebugString().c_str()));
			continue;
		}

		MidiMsg* m = ev->u.midimsg;
		bool incrementit = true;

		NosuchAssert(m);
		switch ( m->MidiType() ) {
		case MIDI_NOTE_ON: {

			// NOTE: we could apply the decay *after* we play the note,
			// IF we wanted the first iteration of a loop to have
			// the same volume as the original.

			MidiNoteOn* noteon = (MidiNoteOn*)m;
			if ( noteon->Velocity() < MIN_LOOP_VELOCITY ) {

				DEBUGPRINT1(("Note Velocity is < 5, should be removing"));
				it = m_events.erase(it);
				incrementit = false;

				// the noteon pointer is still valid, we
				// need to delete it after trying to find the
				// matching noteoff

				SchedEventIterator offit = findNoteOff(noteon,it);
				delete noteon;

				if ( offit != m_events.end() ) {
					SchedEvent* evoff = *offit;
					bool updateit = false;
					if ( offit == it ) {
						// We've found the noteoff, and it's the
						// current valule of 'it', so we need to update it.
						updateit = true;
					}
					MidiNoteOff* noteoff = (MidiNoteOff*)(evoff->u.midimsg);
					NosuchAssert(noteoff);
					delete noteoff;
					offit = m_events.erase(offit);
					if ( updateit ) {
						it = offit;
					}

					DEBUGPRINT1(("Another NEW DELETE of evoff"));
					delete evoff;
				}
				DEBUGPRINT1(("After removing from lp=%d, nnotes=%d",id(),NumNotes()));

				DEBUGPRINT1(("Another NEW DELETE of ev"));
				delete ev;

				int nnotes;
				int ncontrollers;
				NumEvents(nnotes,ncontrollers);
				if ( nnotes == 0 ) {
					DEBUGPRINT(("No more notes in loop, CLEARING IT!"));
					ClearNoLock();
					// XXX - should be forcing controller value to 0
					goto getout;
				}

			} else {
				SendMidiLoopOutput(noteon);
				// Fade the velocity
				double fade = m_loopfade;
				DEBUGPRINT1(("Fading velocity, vel=%d fade=%.4f",(int)(noteon->Velocity()),fade));
				noteon->Velocity((int)(noteon->Velocity()*fade));
			}
			break;
			}
		case MIDI_NOTE_OFF: {
			MidiNoteOff* noteoff = (MidiNoteOff*)m;
			SendMidiLoopOutput(noteoff);
			break;
			}
		case MIDI_CONTROL: {
			MidiController* ctrl = (MidiController*)m;
			SendMidiLoopOutput(ctrl);
			break;
			}
		default:
			DEBUGPRINT(("AdvanceClickBy1 can't handle miditype=%d",m->MidiType()));
			break;
		}
		if ( incrementit ) {
			it++;
		}
	}
getout:
	Unlock();
}

SchedEventIterator
NosuchLoop::findNoteOff(MidiNoteOn* noteon, SchedEventIterator& it) {
	// Look for the matching noteoff, IF it exists in the loop.
	SchedEventIterator offit = it;
	MidiNoteOff* noteoff = NULL;
	for( ; offit != m_events.end(); offit++ ) {
		if ( isMatchingOff(*offit,noteon) ) {
			return offit;
		}
	}
	// Didn't find it yet, scan from beginning
	offit = m_events.begin();
	for( ; offit != m_events.end(); offit++ ) {
		if ( isMatchingOff(*offit,noteon) ) {
			return offit;
		}
	}
	return m_events.end();
}

SchedEventIterator
NosuchLoop::oldestNoteOn() {
	SchedEventIterator it = m_events.begin();
	click_t oldest = INT_MAX;
	SchedEventIterator oldest_it;
	for( ; it != m_events.end(); it++ ) {
		SchedEvent* ev = *it;
		NosuchAssert(ev);
		if ( ev->eventtype() != SchedEvent::MIDIMSG ) {
			continue;
		}
		MidiMsg* mm = ev->u.midimsg;
		NosuchAssert(mm);
		if ( mm->MidiType() == MIDI_NOTE_ON ) {
			if ( ev->created() < oldest ) {
				oldest_it = it;
				oldest = ev->created();
			}
		}
	}
	if ( oldest < INT_MAX ) {
		return oldest_it;
	} else {
		return m_events.end();
	}
}

void
NosuchLoop::removeNote(SchedEventIterator it) {
	// We assume we've been given an iterator that points to a noteon
	SchedEvent* ev = *it;
	NosuchAssert(ev);
	if ( ev->eventtype() != SchedEvent::MIDIMSG ) {
		throw NosuchException("removeNote got event that's not MIDI");
	}
	if ( ev->u.midimsg->MidiType() != MIDI_NOTE_ON ) {
		throw NosuchException("removeNote didn't get a noteon");
	}
	MidiNoteOn* noteon = (MidiNoteOn*)ev->u.midimsg;
	NosuchAssert(noteon);

	it = m_events.erase(it);
	SchedEventIterator offit = findNoteOff(noteon,it);
	delete noteon;
}

void
NosuchLoop::removeOldestNoteOn() {
	Lock();
	SchedEventIterator it = oldestNoteOn();
	if ( it == m_events.end() ) {
		DEBUGPRINT(("Hmmm, removeOldestNoteOn didn't find an oldest note!?"));
		Unlock();
		return;
	}
	MidiNoteOn* noteon = (MidiNoteOn*)((*it)->u.midimsg);
	NosuchAssert(noteon);
	it = m_events.erase(it);
	SchedEventIterator offit = findNoteOff(noteon,it);
	delete noteon;
	if ( offit == m_events.end() ) {
		DEBUGPRINT(("Hmmm, removeOldestNoteOn didn't find noteoff for oldest note!?"));
	} else {
		SchedEvent* ev = *offit;
		MidiNoteOff* noteoff = (MidiNoteOff*)(ev->u.midimsg);
		NosuchAssert(noteoff);
		m_events.erase(offit);
		delete noteoff;

		DEBUGPRINT1(("NEW DELETE CODE IN removeOldestNoteOn"));
		delete ev;
	}
	Unlock();
}

int
NosuchLoop::NumNotes() {
	int nnotes;
	int ncontrollers;
	NumEvents(nnotes,ncontrollers);
	return nnotes;
}

void
NosuchLoop::NumEvents(int& nnotes, int& ncontrollers) {
	SchedEventIterator it = m_events.begin();
	nnotes = 0;
	ncontrollers = 0;
	for(; it != m_events.end(); it++ ) {
		SchedEvent* ev = *it;
		NosuchAssert(ev);
		if ( ev->eventtype() != SchedEvent::MIDIMSG ) {
			continue;
		}
		MidiMsg* m = ev->u.midimsg;
		NosuchAssert(m);
		switch (m->MidiType()) {
		case MIDI_NOTE_ON:
			nnotes++;
			break;
		case MIDI_CONTROL:
			ncontrollers++;
			break;
		}
	}
	return;
}

void
NosuchLoop::Clear() {
	Lock();
	ClearNoLock();
	Unlock();
}

void
NosuchLoop::ClearNoLock() {
	while ( true ) {
		SchedEventIterator it = m_events.begin();
		if ( it == m_events.end() ) {
			break;
		}
		SchedEvent* ev = *it;
		NosuchAssert(ev);
		switch (ev->eventtype()) {
		case SchedEvent::MIDIMSG: {
			MidiMsg* m = ev->u.midimsg;
			if ( m->MidiType() == MIDI_NOTE_OFF ) {
				MidiNoteOff* noteoff = (MidiNoteOff*)m;
				SendMidiLoopOutput(noteoff);
			}
			NosuchAssert(m);
			delete m;
			break;
			}
		case SchedEvent::CURSORMOTION: {
			NosuchCursorMotion* cm = ev->u.cursormotion;
			DEBUGPRINT(("Clearing Cursormotion event!"));
			delete cm;
			break;
			}
		}
		m_events.erase(it);
		delete ev;
	}
}