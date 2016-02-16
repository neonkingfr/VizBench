#include "NosuchUtil.h"
#include "NosuchException.h"
#include "NosuchScheduler.h"
#include "NosuchJson.h"

SchedEvent::SchedEvent(MidiMsg* m, click_t c, const char* h, click_t loopclicks) {
	m_eventtype = SchedEvent::MIDIMSG;
	u.midimsg = m;
	init(c, h, loopclicks);
}

SchedEvent::SchedEvent(MidiPhrase* ph, click_t c, const char* h, click_t loopclicks) {
	m_eventtype = SchedEvent::MIDIPHRASE;
	u.midiphrase = ph;
	init(c, h, loopclicks);
}

SchedEvent::SchedEvent(NosuchCursorMotion* cm,click_t c, const char* h, click_t loopclicks) {
	m_eventtype = SchedEvent::CURSORMOTION;
	u.midimsg = NULL;
	u.cursormotion = cm;
	init(c, h, loopclicks);
}

void
SchedEvent::init(click_t c, const char* h, click_t loopclicks) {
	click = c;
	m_handle = h;
	m_loopclicks = loopclicks;
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

