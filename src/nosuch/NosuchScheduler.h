#ifndef _NOSUCHSCHEDULER_H
#define _NOSUCHSCHEDULER_H

typedef int click_t;

#include "NosuchUtil.h"
#include "NosuchMidi.h"
#include "porttime.h"
#include "portmidi.h"
#include "pmutil.h"
#include "NosuchGraphics.h"
#include <pthread.h>
#include <list>
#include <map>
#include <algorithm>

#define OUT_QUEUE_SIZE 1024

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
extern bool LoopCursors;

class NosuchScheduler;
class MidiMsg;
class MidiPhrase;

class NosuchClickListener {
public:
	virtual void AdvanceClickTo(int current_click, NosuchScheduler* sched) = 0;
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

class NosuchCursorMotion {
public:
	NosuchCursorMotion(int downdragup, NosuchVector pos, float depth) {
		_downdragup = downdragup;
		_pos = pos;
		_depth = depth;
	}
	int _downdragup;
	NosuchVector _pos;
	float _depth;
};

class SchedEvent {
public:
	enum Type {
		MIDIMSG,
		CURSORMOTION,
		MIDIPHRASE
	};
	SchedEvent(MidiMsg* m, click_t c, void* h = 0) {
		_eventtype = SchedEvent::MIDIMSG;
		u.midimsg = m;
		click = c;
		handle = h;
		_created = GlobalClick;
	}
	SchedEvent(MidiPhrase* ph, click_t c, void* h = 0) {
		_eventtype = SchedEvent::MIDIPHRASE;
		u.midiphrase = ph;
		click = c;
		handle = h;
		_created = GlobalClick;
	}
	SchedEvent(NosuchCursorMotion* cm,click_t c, void* h = 0) {
		_eventtype = SchedEvent::CURSORMOTION;
		u.midimsg = NULL;
		u.cursormotion = cm;
		click = c;
		handle = h;
		_created = GlobalClick;
	}
	~SchedEvent();
	std::string DebugString();

	click_t click;	// relative when in loops, absolute elsewhere
	union {
		MidiMsg* midimsg;
		MidiPhrase* midiphrase;
		NosuchCursorMotion* cursormotion;
	} u;

	int eventtype() { return _eventtype; }
	click_t created() { return _created; }

protected:
	void* handle;	// handle to thing that created it
	friend class NosuchScheduler;

private:

	int _eventtype;
	click_t _created;	// absolute
};

typedef std::list<SchedEvent*> SchedEventList;
typedef std::list<SchedEvent*>::iterator SchedEventIterator;

class NosuchScheduler {
public:
	NosuchScheduler() {
		DEBUGPRINT1(("NosuchScheduler CONSTRUCTED!!, count=%d",SchedulerCount++));
		m_running = false;
		clicknow = 0;
		SetMilliNow(0);

#ifdef NOWPLAYING
		_nowplaying_note.clear();
		// _nowplaying_controller.clear();
#endif

		SetClicksPerSecond(192);

		m_clicks_per_clock = 4;
		NosuchLockInit(&_scheduled_mutex,"scheduled");
		NosuchLockInit(&_notesdown_mutex,"notesdown");
		_midioutput_client = NULL;

		_midi_input_stream = std::vector<PmStream*>();
		_midi_input_name = std::vector<std::string>();

		_midi_output_stream = std::vector<PmStream*>();
		_midi_output_name = std::vector<std::string>();

		_midiinput_client = NULL;
		_midiinput_merge = false;
		_midioutput_client = NULL;

		_click_client = NULL;
		_periodic_ANO = false;
		_queue = new SchedEventList();
	}
	~NosuchScheduler() {
		DEBUGPRINT(("NosuchScheduler destructor!"));
		Stop();
	}

	const char* MidiInputName(size_t n)  {
		if (n>=_midi_input_name.size()) {
			return NULL;
		}
		return _midi_input_name[n].c_str();
	}

	const char* MidiOutputName(size_t n)  {
		if (n>=_midi_output_name.size()) {
			return NULL;
		}
		return _midi_output_name[n].c_str();
	}

	static void SetClicksPerSecond(int clkpersec);
	static void SetTempoFactor(float f);

	void SetClickProcessor(NosuchClickListener* client) {
		_click_client = client;
	}
	void SetMidiInputListener(NosuchMidiListener* client, bool merge) {
		_midiinput_client = client;
		_midiinput_merge = merge;
	}
	void SetMidiOutputListener(NosuchMidiListener* client) {
		_midioutput_client = client;
	}
	bool StartMidi(std::string midi_input, std::string midi_output);
	void Stop();
	void AdvanceTimeAndDoEvents(PtTimestamp timestamp);
	void Callback(PtTimestamp timestamp);
	std::string DebugString();

	// int NumberScheduled(click_t minclicks, click_t maxclicks, std::string sid);
	bool AnythingPlayingAt(click_t clk, void* handle);
	void IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle);
	void IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle);

	SchedEventList* ScheduleOf(void* handle);
	void ScheduleMidiMsg(MidiMsg* m, click_t clk, void* handle);
	void ScheduleMidiPhrase(MidiPhrase* m, click_t clk, void* handle);
	void ScheduleClear();
	bool ScheduleAddEvent(SchedEvent* e, bool lockit=true);  // returns false if not added, i.e. means caller should free it

	void QueueMidiMsg(MidiMsg* m, click_t clk);
	void QueueMidiPhrase(MidiPhrase* m, click_t clk);
	void QueueClear();
	bool QueueAddEvent(SchedEvent* e);  // returns false if not added, i.e. means caller should free it

	void SendPmMessage(PmMessage pm, PmStream* ps, void* handle);
	void SendMidiMsg(MidiMsg* mm, void* handle);
	void SendControllerMsg(MidiMsg* m, void* handle, bool smooth);  // gives ownership of m away
	void SendPitchBendMsg(MidiMsg* m, void* handle, bool smooth);  // gives ownership of m away
	void ANO(int psi = -1, int ch = -1);
	void ANO(PmStream* ps, int ch = -1);
	void setPeriodicANO(bool b) { _periodic_ANO = b; }

	static int _MilliNow;
	static int ClicksPerSecond;
	static double ClicksPerMillisecond;
	static int LastTimeStamp;
	static int millitime0;
	static click_t clicknow;

	void SetMilliNow(int tm) {
		_MilliNow = tm;
	}
	click_t CurrentClick() { return clicknow; }

	// PmStream *midi_input(int n) { return _midi_input[n]; }

	std::list<MidiMsg*>& NotesDown() {
		return _notesdown;
	}

	void ClearNotesDown() {
		LockNotesDown();
		_notesdown.clear();
		UnlockNotesDown();
	}

	void LockNotesDown() { NosuchLock(&_notesdown_mutex,"notesdown"); }
	void UnlockNotesDown() { NosuchUnlock(&_notesdown_mutex,"notesdown"); }

private:

	PmStream* _openMidiInput(std::string input);
	PmStream* _openMidiOutput(std::string output);

	void _maintainNotesDown(MidiMsg* m);
	std::list<MidiMsg*> _notesdown;

	int TryLockScheduled() { return NosuchTryLock(&_scheduled_mutex,"sched"); }
	void LockScheduled() { NosuchLock(&_scheduled_mutex,"sched"); }
	void UnlockScheduled() { NosuchUnlock(&_scheduled_mutex,"sched"); }

	void _addQueueToScheduled();

	void _sortEvents(SchedEventList* sl);
	void DoEventAndDelete(SchedEvent* e, void* handle);
	void DoMidiMsgEvent(MidiMsg* m, void* handle);
	bool m_running;
	int m_clicks_per_clock;

	// Per-handle scheduled events
	std::map<void*,SchedEventList*> _scheduled;
	SchedEventList* _queue;  // Things to be added to _scheduled

#ifdef NOWPLAYING
	// This is a mapping of session id (either TUIO session id or Looping id)
	// to whatever MIDI message (i.e. notes) are currently active (and need a
	// noteoff if we change).
	std::map<int,MidiMsg*> _nowplaying_note;

	// This is a mapping of session id (either TUIO session id or Looping id)
	// to whatever the last MIDI controllers were.  The
	// map inside the map is a mapping of controller numbers
	// to the messages.
	std::map<int,std::map<int,MidiMsg*>> _nowplaying_controller;

	// This is a mapping of session id (either TUIO session id or Looping id)
	// to whatever the last pitchbend was.
	std::map<int,MidiMsg*> _nowplaying_pitchbend;
#endif

	std::vector<PmStream *> _midi_input_stream;
	std::vector<std::string> _midi_input_name;

	std::vector<PmStream *> _midi_output_stream;
	std::vector<std::string> _midi_output_name;

	NosuchClickListener* _click_client;
	NosuchMidiListener*	_midioutput_client;
	NosuchMidiListener*	_midiinput_client;
	bool _midiinput_merge;
	pthread_mutex_t _scheduled_mutex;
	pthread_mutex_t _notesdown_mutex;
	bool _periodic_ANO;
};

#endif
