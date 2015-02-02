#ifndef _NOSUCH_LOOP_EVENT
#define _NOSUCH_LOOP_EVENT

#include <list>
#include "NosuchScheduler.h"

#define DEFAULT_LOOPFADE 0.7f
#define MIN_LOOP_VELOCITY 5

class SchedEvent;

typedef std::list<SchedEvent*> SchedEventList;

class Vizlet;

class NosuchLoop {
public:
	NosuchLoop(Vizlet* mf, int id, int looplength, double loopfade) {
		m_vizlet = mf;
		m_click = 0;
		m_looplength = looplength;
		m_loopfade = loopfade;
		m_id = id;
		NosuchLockInit(&_loop_mutex,"loop");
	};
	~NosuchLoop() {
		DEBUGPRINT1(("NosuchLoop DESTRUCTOR!"));
	}
	void Clear();
	void AdvanceClickBy1();
	int AddLoopEvent(SchedEvent* e);
	std::string DebugString(std::string indent = "");
	void SendMidiLoopOutput(MidiMsg* mm);
	int id() { return m_id; }
	click_t click() { return m_click; }
	int NumNotes();
	void NumEvents(int& nnotes, int& ncontrollers);
	void removeOldestNoteOn();

private:
	SchedEventList m_events;
	click_t m_click; // relative within loop
	int m_id;
	Vizlet* m_vizlet;
	int m_looplength;
	double m_loopfade;

	pthread_mutex_t _loop_mutex;
	void Lock() { NosuchLock(&_loop_mutex,"loop"); }
	void Unlock() { NosuchUnlock(&_loop_mutex,"loop"); }

	SchedEventIterator findNoteOff(MidiNoteOn* noteon, SchedEventIterator& it);
	SchedEventIterator oldestNoteOn();
	void removeNote(SchedEventIterator it);
	void ClearNoLock();
};

class NosuchLooper : public NosuchClickListener {
public:
	NosuchLooper(/*Vizlet* b*/);
	~NosuchLooper();
	void AdvanceClickTo(click_t click, NosuchScheduler* s);
	std::string DebugString();
	void AddLoop(NosuchLoop* loop);

private:
	std::vector<NosuchLoop*> m_loops;
	int m_last_click;
	Vizlet* _vizlet;
	pthread_mutex_t _looper_mutex;

	void Lock() { NosuchLock(&_looper_mutex,"looper"); }
	void Unlock() { NosuchUnlock(&_looper_mutex,"looper"); }

};

#endif