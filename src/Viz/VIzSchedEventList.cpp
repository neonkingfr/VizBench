#include "VizUtil.h"
#include "VizSchedEvent.h"
#include "VizSchedEventList.h"
#include "MidiVizParams.h"

VizSchedEventList::VizSchedEventList() {
	m_first = NULL;
	m_last = NULL;
}

bool VizSchedEventList::IsEmpty() {
	DEBUGPRINT1(("IsEmpty"));
	return (m_first == NULL);
}

click_t VizSchedEventList::FirstClick() {
	DEBUGPRINT1(("FirstClick"));
	VizAssert(m_first != NULL);
	return m_first->click;
}

// We assume we're locked when this is called
VizSchedEvent* VizSchedEventList::PopFirst() {
	DEBUGPRINT1(("PopFirst"));
	VizAssert(m_first != NULL);
	VizSchedEvent* e = m_first;
	m_first = e->m_next;
	if (m_first != NULL) {
		m_first->m_prev = NULL;
	}
	CheckSanity("PopFirst end");
	return e;
}

// We assume we're locked when this is called
void VizSchedEventList::DeleteAll() {
	DEBUGPRINT1(("DeleteAll"));
	VizSchedEvent* nexte;
	for (VizSchedEvent* e = m_first; e != NULL; e = nexte) {
		nexte = e->m_next;
		DEBUGPRINT1(("Deleting X VizSchedEvent e=%ld",(long)e));
		delete e;
	}
	m_first = NULL;
	m_last = NULL;
}

// We assume we're locked when this is called
void VizSchedEventList::DeleteAllId(int cursorid) {

	DEBUGPRINT1(("DeleteAllId id=%d",cursorid));
	// There can/will be multiple events with the same cursorid

	CheckSanity("DeleteAllId A");

	VizSchedEvent* nexte;
	for (VizSchedEvent* e = m_first; e != NULL; e = nexte) {
		nexte = e->m_next;
		if (e->m_cursorid != cursorid) {
			continue;  // i.e. not deleting it
		}
#if 0
		if (e->m_eventtype == VizSchedEvent::MIDIMSG ) {
			MidiMsg* m = e->u.midimsg;
			if (m->MidiType() == MIDI_NOTE_OFF) {
				DEBUGPRINT1(("DeleteAllId sees NOTEOFF and is not deleting it"));
				continue;
			}
		}
#endif
		// cursorid matches, so we want to delete e from the list
		if (e == m_last) {
			DEBUGPRINT1(("Deleting AA VizSchedEvent e=%ld",(long)e));
			if (m_last->m_prev == NULL) {
				// It's the only one in the list
				m_last = NULL;
				m_first = NULL;
			}
			else {
				m_last->m_prev->m_next = NULL;
				m_last = m_last->m_prev;
			}
			DEBUGPRINT1(("Deleting A VizSchedEvent e=%ld", (long)e));
			delete e;
			continue;
		}
		if (e == m_first) {
			if (m_first->m_next == NULL) {
				// It's the only one in the list
				m_last = NULL;
				m_first = NULL;
			}
			else {
				m_first->m_next->m_prev = NULL;
				m_first = m_first->m_next;
			}
			DEBUGPRINT1(("Deleting B VizSchedEvent e=%ld", (long)e));
			delete e;
			continue;
		}
		// At this point e is not the first or last in the list
		e->m_prev->m_next = e->m_next;
		e->m_next->m_prev = e->m_prev;
		DEBUGPRINT1(("Deleting C VizSchedEvent e=%ld", (long)e));
		delete e;
	}
	DEBUGPRINT1(("post Deleting loop for cursorid=%d", cursorid));
	CheckSanity("DeleteAllId end");
}

// We assume we're locked when this is called
void VizSchedEventList::DeleteAllBefore(int cursorid, click_t beforeclk) {

	DEBUGPRINT1(("DeleteAllId id=%d",cursorid));
	// There can/will be multiple events with the same cursorid

	CheckSanity("DeleteAllId A");

	VizSchedEvent* nexte;
	for (VizSchedEvent* e = m_first; e != NULL; e = nexte) {
		nexte = e->m_next;
		if (e->m_cursorid != cursorid) {
			continue;  // i.e. not deleting it
		}
		if (e->click > beforeclk) {
			DEBUGPRINT1(("click > beforeclk, not deleting it"));
			continue;  // i.e. not deleting it
		}
		if (e->click <= beforeclk) {
			DEBUGPRINT1(("click <= beforeclk, deleting it"));
		}
		if (e->m_eventtype == VizSchedEvent::MIDIMSG ) {
			MidiMsg* m = e->u.midimsg;
			if (m->MidiType() == MIDI_NOTE_OFF) {
				DEBUGPRINT1(("DeleteAllBefore sees NOTEOFF and is not deleting it"));
				continue;
			}
		}
		// cursorid matches, so we want to delete e from the list
		if (e == m_last) {
			DEBUGPRINT1(("Deleting AA VizSchedEvent e=%ld",(long)e));
			if (m_last->m_prev == NULL) {
				// It's the only one in the list
				m_last = NULL;
				m_first = NULL;
			}
			else {
				m_last->m_prev->m_next = NULL;
				m_last = m_last->m_prev;
			}
			DEBUGPRINT1(("Deleting A VizSchedEvent e=%ld", (long)e));
			delete e;
			continue;
		}
		if (e == m_first) {
			if (m_first->m_next == NULL) {
				// It's the only one in the list
				m_last = NULL;
				m_first = NULL;
			}
			else {
				m_first->m_next->m_prev = NULL;
				m_first = m_first->m_next;
			}
			DEBUGPRINT1(("Deleting B VizSchedEvent e=%ld", (long)e));
			delete e;
			continue;
		}
		// At this point e is not the first or last in the list
		e->m_prev->m_next = e->m_next;
		e->m_next->m_prev = e->m_prev;
		DEBUGPRINT1(("Deleting C VizSchedEvent e=%ld", (long)e));
		delete e;
	}
	DEBUGPRINT1(("post Deleting loop for cursorid=%d", cursorid));
	CheckSanity("DeleteAllId end");
}

void VizSchedEventList::CheckSanity(const char* tag) {
	DEBUGPRINT1(("CheckSanity tag=%s", tag));
	for (VizSchedEvent* t = m_first; t != NULL; t = t->m_next) {
		if (t == m_first) {
			if (t->m_prev != NULL) {
				DEBUGPRINT(("UNEXPECTED A"));
			}
			if (t->m_next == NULL && t != m_last) {
				DEBUGPRINT(("UNEXPECTED B"));
			}
		}
		if (t == m_last) {
			if (t->m_next != NULL) {
				DEBUGPRINT(("UNEXPECTED C"));
			}
			if (t->m_prev == NULL && t != m_first) {
				DEBUGPRINT(("UNEXPECTED D"));
			}
		}
		if (t != m_first && t != m_last) {
			if (t->m_next == NULL) {
				DEBUGPRINT(("UNEXPECTED E"));
			}
			if (t->m_prev == NULL) {
				DEBUGPRINT(("UNEXPECTED F"));
			}
		}
	}
}

// We assume we're locked when this is called
void VizSchedEventList::InsertEvent(VizSchedEvent* e) {

	DEBUGPRINT1(("InsertEvent 1 this=%ld e=%ld m_first=%ld", (long)this, (long)e, (long)m_first));

	CheckSanity("InsertEvent start");

	// Currently, the sorting of the list is based on the click value,
	// and events with equal click value are sorted (implicitly, by
	// the way they're inserted below) in the order they are inserted.

	// Special case things to avoid expensive insertion sort
	if (m_first == NULL) {
		e->m_next = NULL;
		e->m_prev = NULL;
		m_first = e;
		m_last = e;
		DEBUGPRINT1(("InsertEvent 2"));
		CheckSanity("InsertEvent 2");
		return;
	}
	VizAssert(m_first != NULL && m_last != NULL);

	DEBUGPRINT1(("InsertEvent 3   e->click=%ld   m_first=%ld m_first->click=%ld", e->click, (long)m_first, m_first->click));
	if (e->click < m_first->click) {
		e->m_next = m_first;
		e->m_prev = NULL;
		m_first->m_prev = e;
		m_first = e;
		// m_last stays the same
		DEBUGPRINT1(("InsertEvent 4"));
		CheckSanity("InsertEvent 4");
		return;
	}
	DEBUGPRINT1(("InsertEvent 5"));
	if (e->click >= m_last->click) {
		e->m_next = NULL;
		e->m_prev = m_last;
		// m_first stays the same
		m_last->m_next = e;
		m_last = e;
		DEBUGPRINT1(("InsertEvent 6"));
		CheckSanity("InsertEvent 6");
		return;
	}
	DEBUGPRINT1(("BEFORE INSERTION SORT list = %s", DebugString().c_str()));
	// Insertion sort - at this point we know (because of code above)
	// that there's at least 2 events in the list, and that the
	// new event is NOT going to be at the beginning or end of the list.
	DEBUGPRINT1(("InsertEvent 7  m_first=%ld  m_first->click=%ld", (long)m_first, m_first->click));
	for (VizSchedEvent* t = m_first; t != NULL; t = t->m_next) {
		DEBUGPRINT1(("InsertEvent 9   e->click=%ld   t->m_next=%ld  t->m_next->click=%ld", e->click, (long)(t->m_next), t->m_next->click));
		DEBUGPRINT1(("InsertEvent 9b  m_first=%ld  m_first->click=%ld", (long)m_first, m_first->click));
		if (e->click < t->click) {
			DEBUGPRINT1(("InsertEvent 10 t=%ld", (long)t));
			DEBUGPRINT1(("InsertEvent 11 t->m_prev=%ld", (long)(t->m_prev)));
			if (t->m_prev == NULL) {
				DEBUGPRINT1(("Hey, t->m_prev == NULL!?"));
			}
			t->m_prev->m_next = e;
			DEBUGPRINT1(("InsertEvent 12 t=%ld", (long)t));
			e->m_prev = t->m_prev;
			DEBUGPRINT1(("InsertEvent 13 e=%ld", (long)(e)));
			e->m_next = t;
			DEBUGPRINT1(("InsertEvent 14 t=%ld", (long)t));
			t->m_prev = e;
			DEBUGPRINT1(("InsertEvent 15"));
			DEBUGPRINT1(("END OF INSERTION SORT list = %s", DebugString().c_str()));
			return;
		}
	}
	DEBUGPRINT(("Hey! Shouldn't get here either!  VizSchedEvent not added!"));
	CheckSanity("InsertEvent end");
	return;
}

int VizSchedEventList::NumEvents() {
	DEBUGPRINT1(("NumEvents"));
	CheckSanity("NumEvents");
	int n = 0;
	for (VizSchedEvent* e = m_first; e != NULL; e = e->m_next) {
		n++;
	}
	return n;
}

int VizSchedEventList::NumEventsOfSid(int sid) {
	DEBUGPRINT1(("NumEvents"));
	CheckSanity("NumEvents");
	int n = 0;
	for (VizSchedEvent* e = m_first; e != NULL; e = e->m_next) {
		if (e->m_cursorid == sid) {
			n++;
		}
	}
	return n;
}

std::string VizSchedEventList::DebugString() {
	std::string s;
	s = VizSnprintf("VizSchedEventList %ld (\n", (long)this);;
	for (VizSchedEvent* e = m_first; e != NULL; e = e->m_next) {
		s += VizSnprintf("      Event %s\n", e->DebugString().c_str());
	}
	s += ")\n";
	return s;
}