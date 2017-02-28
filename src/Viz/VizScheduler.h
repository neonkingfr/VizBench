#ifndef _NOSUCHSCHEDULER_H
#define _NOSUCHSCHEDULER_H

typedef int click_t;

#include "VizUtil.h"
#include "VizMidi.h"
#include "VizJSON.h"
#include "porttime.h"
#include "VizGraphics.h"
#include "VizSchedEvent.h"
#include "VizSchedEventList.h"
#include <pthread.h>
#include <list>
#include <map>
#include <algorithm>

#define OUT_QUEUE_SIZE 1024

// Session IDs (TUIO cursor sessions) and Loop IDs need to be distinct,
// since these IDs are attached to scheduled events so we can know
// who was responsible for creating the events.

extern int SchedulerCount;
extern int GlobalPitchOffset;

class VizScheduler;
class MidiMsg;
class MidiPhrase;

class ClickListener {
public:
	virtual void processAdvanceClickTo(int click) = 0;
};

class VizMidiListener {
public:
	virtual void processMidiMsg(MidiMsg* mm) = 0;
};

class VizMidiInputListener {
public:
	virtual void processMidiInput(MidiMsg* mm) = 0;
};

class VizMidiOutputListener {
public:
	virtual void processMidiOutput(MidiMsg* mm) = 0;
};

#define SCHEDID_ALL 0

class VizScheduler {
public:
	VizScheduler();
	virtual ~VizScheduler();

	const char* MidiInputName(size_t n)  {
		if (n>=m_midi_input_name.size()) {
			return NULL;
		}
		return m_midi_input_name[n].c_str();
	}

	const char* MidiOutputName(size_t n)  {
		if (n>=m_midi_output_name.size()) {
			return NULL;
		}
		return m_midi_output_name[n].c_str();
	}

	const char* MidiMergeName(size_t n)  {
		if (n>=m_midi_merge_name.size()) {
			return NULL;
		}
		return m_midi_merge_name[n].c_str();
	}

	static click_t ClicksPerBeat();
	static click_t ClicksPerSecond();
	static void SetClicksPerSecond(click_t clkpersec);
	static void SetTempoFactor(float f);

	void SetClickProcessor(ClickListener* client) {
		m_click_client = client;
	}
	void SetMidiInputListener(VizMidiListener* client) {
		m_midiinput_client = client;
	}
	void SetMidiOutputListener(VizMidiListener* client) {
		m_midioutput_client = client;
	}
	bool StartMidi(cJSON *config);
	void Stop();
	void AdvanceTimeAndDoEvents(PtTimestamp timestamp);
	void Callback(PtTimestamp timestamp);
	std::string DebugString();

	// SchedEventList* ScheduleOf(const char* handle);
	void ScheduleMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp);
	void ScheduleMidiPhrase(MidiPhrase* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp);
	void ScheduleClear(int cursorid, bool lockit=true);
	bool ScheduleAddEvent(VizSchedEvent* e, bool lockit=true);  // returns false if not added, i.e. means caller should free it

	void QueueMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping=false, MidiVizParams* mp = 0);
	void QueueMidiPhrase(MidiPhrase* m, click_t clk, int cursorid, bool looping=false, MidiVizParams* mp = 0);
	void QueueClear();
	bool QueueAddEvent(VizSchedEvent* e);  // returns false if not added, i.e. means caller should free it

	int NumQueuedEventsOfSid(int sid);
	int NumScheduledEventsOfSid(int sid);

	void QueueRemoveBefore(int cursorid, click_t clk);

	void SendPmMessage(PmMessage pm, PmStream* ps);
	void SendMidiMsg(MidiMsg* mm, int cursorid);
	void ANO(int psi = -1, int ch = -1);
	void ANO(PmStream* ps, int ch = -1);
	void setPeriodicANO(bool b) { m_periodic_ANO = b; }

	// void SendControllerMsg(MidiMsg* m, const char* handle, bool smooth);  // gives ownership of m away
	// void SendPitchBendMsg(MidiMsg* m, const char* handle, bool smooth);  // gives ownership of m away

	static int m_ClicksPerSecond;
	static double m_ClicksPerMillisecond;
	static int m_LastTimeStampInMilliseconds;
	static int m_timestamp0InMilliseconds;
	static click_t m_currentclick;

	click_t CurrentClick() { return m_currentclick; }

	std::list<MidiMsg*>& NotesDown() {
		return m_notesdown;
	}

	void ClearNotesDown() {
		LockNotesDown();
		m_notesdown.clear();
		UnlockNotesDown();
	}

	void LockNotesDown() { VizLock(&m_notesdown_mutex,"notesdown"); }
	void UnlockNotesDown() { VizUnlock(&m_notesdown_mutex,"notesdown"); }

	bool m_queue_locked;
	bool m_scheduled_locked;

	void LockQueue() {
		// DEBUGPRINT(("LockQueue pthread=%ld", (long)pthread_self().p));
		VizLock(&m_queue_mutex,"queue");
		m_queue_locked = true;
	}
	void UnlockQueue() {
		// DEBUGPRINT(("UnLockQueue pthread=%ld", (long)pthread_self().p));
		VizUnlock(&m_queue_mutex,"queue");
		m_queue_locked = false;
	}

private:

	PmStream* _openMidiInput(std::string input);
	PmStream* _openMidiOutput(std::string output);

	void _maintainNotesDown(MidiMsg* m);
	std::list<MidiMsg*> m_notesdown;

	int TryLockScheduled() {
		DEBUGPRINT1(("TryLockScheduled"));
		int i = VizTryLock(&m_scheduled_mutex,"sched");
		if (i == 0) {
			m_scheduled_locked = true;
		}
		return i;
	}
	void LockScheduled() {
		DEBUGPRINT1(("LockScheduled"));
		VizLock(&m_scheduled_mutex,"sched");
		m_scheduled_locked = true;
	}
	void UnlockScheduled() {
		DEBUGPRINT1(("UnLockScheduled"));
		VizUnlock(&m_scheduled_mutex,"sched");
		m_scheduled_locked = false;
	}

	void _addQueueToScheduled();
	bool _handleLoopEvent(VizSchedEvent* e);

	void _sortEvents(VizSchedEventList* sl);
	void DoEventAndDelete(VizSchedEvent* e);
	void DoMidiMsgEvent(MidiMsg* m, int cursorid);
	bool m_running;

	VizSchedEventList* m_scheduled;
	VizSchedEventList* m_queue;  // Things to be added to _scheduled

#ifdef NOWPLAYING
	// This is a mapping of session id (either TUIO session id or Looping id)
	// to whatever MIDI message (i.e. notes) are currently active (and need a
	// noteoff if we change).
	std::map<int,MidiMsg*> m_nowplaying_note;

	// This is a mapping of session id (either TUIO session id or Looping id)
	// to whatever the last MIDI controllers were.  The
	// map inside the map is a mapping of controller numbers
	// to the messages.
	std::map<int,std::map<int,MidiMsg*>> m_nowplaying_controller;

	// This is a mapping of session id (either TUIO session id or Looping id)
	// to whatever the last pitchbend was.
	std::map<int,MidiMsg*> m_nowplaying_pitchbend;
#endif

	std::vector<PmStream *> m_midi_input_stream;
	std::vector<std::string> m_midi_input_name;

	std::vector<PmStream *> m_midi_output_stream;
	std::vector<std::string> m_midi_output_name;

	std::vector<int> m_midi_merge_outport;
	std::vector<std::string> m_midi_merge_name;

	ClickListener* m_click_client;
	VizMidiListener*	m_midioutput_client;
	VizMidiListener*	m_midiinput_client;

	pthread_mutex_t m_scheduled_mutex;
	pthread_mutex_t m_notesdown_mutex;
	pthread_mutex_t m_queue_mutex;
	bool m_periodic_ANO;
};

#endif
