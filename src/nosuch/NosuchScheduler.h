#ifndef _NOSUCHSCHEDULER_H
#define _NOSUCHSCHEDULER_H

typedef int click_t;

#include "NosuchUtil.h"
#include "NosuchMidi.h"
#include "NosuchJSON.h"
#include "porttime.h"
#include "NosuchGraphics.h"
#include "SchedEvent.h"
#include <pthread.h>
#include <list>
#include <map>
#include <algorithm>

#define OUT_QUEUE_SIZE 1024

#define HANDLE_DEFAULT ((const char*)0)

// Session IDs (TUIO cursor sessions) and Loop IDs need to be distinct,
// since these IDs are attached to scheduled events so we can know
// who was responsible for creating the events.

// Loop IDs are session IDs that are >= LOOPID_BASE.
// Each region has one loop.
#define LOOPID_BASE 1000000
#define LOOPID_OF_REGION(id) (LOOPID_BASE+id)

extern int SchedulerCount;
extern click_t GlobalClick;
extern int GlobalPitchOffset;
extern bool NoMultiNotes;

class NosuchScheduler;
class MidiMsg;
class MidiPhrase;

class ClickListener {
public:
	virtual void processAdvanceClickTo(int click) = 0;
};

class NosuchMidiListener {
public:
	virtual void processMidiMsg(MidiMsg* mm) = 0;
};

class NosuchMidiInputListener {
public:
	virtual void processMidiInput(MidiMsg* mm) = 0;
};

class NosuchMidiOutputListener {
public:
	virtual void processMidiOutput(MidiMsg* mm) = 0;
};

class NosuchScheduler {
public:
	NosuchScheduler() {
		DEBUGPRINT1(("NosuchScheduler CONSTRUCTED!!, count=%d",SchedulerCount++));
		m_running = false;
		m_currentclick = 0;

#ifdef NOWPLAYING
		m_nowplaying_note.clear();
		// m_nowplaying_controller.clear();
#endif

		SetClicksPerSecond(192);

		NosuchLockInit(&m_scheduled_mutex,"scheduled");
		NosuchLockInit(&m_notesdown_mutex,"notesdown");
		NosuchLockInit(&m_queue_mutex,"queue");
		m_midioutput_client = NULL;

		m_midi_input_stream = std::vector<PmStream*>();
		m_midi_input_name = std::vector<std::string>();

		m_midi_output_stream = std::vector<PmStream*>();
		m_midi_output_name = std::vector<std::string>();

		m_midi_merge_outport = std::vector<int>();
		m_midi_merge_name = std::vector<std::string>();

		m_midiinput_client = NULL;
		m_midioutput_client = NULL;

		m_click_client = NULL;
		m_periodic_ANO = false;
		m_queue = new SchedEventList();
	}
	virtual ~NosuchScheduler() {
		Stop();
	}

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
	void SetMidiInputListener(NosuchMidiListener* client) {
		m_midiinput_client = client;
	}
	void SetMidiOutputListener(NosuchMidiListener* client) {
		m_midioutput_client = client;
	}
	bool StartMidi(cJSON *config);
	void Stop();
	void AdvanceTimeAndDoEvents(PtTimestamp timestamp);
	void Callback(PtTimestamp timestamp);
	std::string DebugString();

	// int NumberScheduled(click_t minclicks, click_t maxclicks, std::string sid);
	// void IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle);
	// void IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle);

	SchedEventList* ScheduleOf(const char* handle);
	void ScheduleMidiMsg(MidiMsg* m, click_t clk, const char* handle);
	void ScheduleMidiPhrase(MidiPhrase* m, click_t clk, const char* handle);
	void ScheduleClear(const char* handle = 0);
	bool ScheduleAddEvent(SchedEvent* e, bool lockit=true);  // returns false if not added, i.e. means caller should free it

	void QueueMidiMsg(MidiMsg* m, click_t clk, const char* handle, click_t loopclicks = 0);
	void QueueMidiPhrase(MidiPhrase* m, click_t clk, const char* handle, click_t loopclicks = 0);
	void QueueClear();
	bool QueueAddEvent(SchedEvent* e);  // returns false if not added, i.e. means caller should free it

	void SendPmMessage(PmMessage pm, PmStream* ps, const char* handle);
	void SendMidiMsg(MidiMsg* mm, const char* handle);
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

	void LockNotesDown() { NosuchLock(&m_notesdown_mutex,"notesdown"); }
	void UnlockNotesDown() { NosuchUnlock(&m_notesdown_mutex,"notesdown"); }

	void LockQueue() { NosuchLock(&m_queue_mutex,"queue"); }
	void UnlockQueue() { NosuchUnlock(&m_queue_mutex,"queue"); }

private:

	PmStream* _openMidiInput(std::string input);
	PmStream* _openMidiOutput(std::string output);

	void _maintainNotesDown(MidiMsg* m);
	std::list<MidiMsg*> m_notesdown;

	int TryLockScheduled() { return NosuchTryLock(&m_scheduled_mutex,"sched"); }
	void LockScheduled() { NosuchLock(&m_scheduled_mutex,"sched"); }
	void UnlockScheduled() { NosuchUnlock(&m_scheduled_mutex,"sched"); }

	void _addQueueToScheduled();

	void _sortEvents(SchedEventList* sl);
	void DoEventAndDelete(SchedEvent* e, const char* handle);
	void DoMidiMsgEvent(MidiMsg* m, const char* handle);
	bool m_running;

	// Per-handle scheduled events
	std::map<const char*,SchedEventList*> m_scheduled;
	SchedEventList* m_queue;  // Things to be added to _scheduled

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
	NosuchMidiListener*	m_midioutput_client;
	NosuchMidiListener*	m_midiinput_client;

	pthread_mutex_t m_scheduled_mutex;
	pthread_mutex_t m_notesdown_mutex;
	pthread_mutex_t m_queue_mutex;
	bool m_periodic_ANO;
};

#endif
