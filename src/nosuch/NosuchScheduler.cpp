#include "NosuchUtil.h"
#include "NosuchScheduler.h"

#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

int SchedulerCount = 0;
int GlobalPitchOffset = 0;

int NosuchScheduler::m_ClicksPerSecond = 0;
double NosuchScheduler::m_ClicksPerMillisecond = 0;
int NosuchScheduler::m_LastTimeStampInMilliseconds;
int NosuchScheduler::m_timestamp0InMilliseconds;
click_t NosuchScheduler::m_currentclick;

void midi_callback( PtTimestamp timestamp, void *userData ) {
	NosuchScheduler* ms = (NosuchScheduler*)userData;
	NosuchAssert(ms);

	// DEBUGPRINT(("midi_callback time=%ld timestamp=%ld", timeGetTime(),timestamp));
	try {
		CATCH_NULL_POINTERS;
		ms->Callback(timestamp);
	} catch (NosuchException& e) {
		DEBUGPRINT(("NosuchException: %s",e.message()));
	}
}

static bool
schedevent_compare (SchedEvent* first, SchedEvent* second)
{
	if (first->click<second->click)
		return true;
	else
		return false;
}

void NosuchScheduler::_sortEvents(SchedEventList* sl) {
	stable_sort (sl->begin(), sl->end(),schedevent_compare);
}

void NosuchScheduler::ANO(int psi,int ch) {
	NosuchAssert(ch!=0);
	if (psi < 0) {
		// Send it on all output ports
		for (size_t pi = 0; pi < m_midi_output_stream.size(); pi++) {
			ANO(m_midi_output_stream[pi], ch);
		}
	} else {
		ANO(m_midi_output_stream[psi], ch);
	}
}

void NosuchScheduler::ANO(PmStream* ps,int ch) {
	NosuchAssert(ch!=0);
	if ( ch < 0 ) {
		// send it on all channels
		for ( int ch=1; ch<=16; ch++ ) {
			ANO(ps,ch);
		}
	} else {
		DEBUGPRINT1(("ANO on ps %ld channel %d",(long)ps,ch));
		PmMessage pm = Pm_Message((ch-1) + 0xb0, 0x7b, 0x00 );
		SendPmMessage(pm,ps);
	}
}

void NosuchScheduler::ScheduleMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	SchedEvent* e = new SchedEvent(m,clk,cursorid, looping, mp);
	if ( ! ScheduleAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::ScheduleMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	SchedEvent* e = new SchedEvent(ph,clk,cursorid, looping, mp);
	if ( ! ScheduleAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::ScheduleClear(int cursorid, bool lockit) {
	if (lockit) {
		LockScheduled();
	}
	if (cursorid == SCHEDID_ALL) {
		m_scheduled->clear();
	}
	else {
		SchedEventIterator it = m_scheduled->begin();
		for (; it != m_scheduled->end(); ) {
			SchedEvent* ep = *it;
			NosuchAssert(ep);
			if (ep->m_cursorid == cursorid) {
				delete ep;
				it = m_scheduled->erase(it);
			}
			else {
				it++;
			}
		}
	}
	if (lockit) {
		UnlockScheduled();
	}
}

bool
NosuchScheduler::ScheduleAddEvent(SchedEvent* e, bool lockit) {
	// DEBUGPRINT(("ScheduleAddEvent clicknow=%d clk=%d handle=%ld %s",
	//	m_currentclick,e->click,(long)(e->handle),e->DebugString().c_str()));
	if ( lockit ) {
		LockScheduled();
	}

	// XXX - profiling shows this to be a hotspot
	SchedEventList* sl = m_scheduled;
	sl->push_back(e);
	// XXX - especially here.  Need to replace SchedEventList with
	// XXX - something that keeps track of the end of the list and
	// XXX - adds it there quickly if it can, rather than blindly
	// XXX - adding it and sorting.
	_sortEvents(sl);

	if ( lockit ) {
		UnlockScheduled();
	}
	return true;
}

void NosuchScheduler::QueueMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	SchedEvent* e = new SchedEvent(m,clk,cursorid,looping,mp);
	if ( ! QueueAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::QueueMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	SchedEvent* e = new SchedEvent(ph,clk,cursorid,looping,mp);
	if ( ! QueueAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::QueueClear() {
	LockQueue();
	m_queue->clear();
	UnlockQueue();
}

bool
NosuchScheduler::QueueAddEvent(SchedEvent* e) {
	LockQueue();
	// XXX - profiling shows this to be a hotspot
	// XXX - need to rework m_queue - most additions will be
	// XXX - at the end of the list, which can be done quickly
	// XXX - without sorting.
	m_queue->push_back(e);
	_sortEvents(m_queue);
	UnlockQueue();
	return true;
}

void NosuchScheduler::_maintainNotesDown(MidiMsg* m) {

	LockNotesDown();

	// These things work (i.e. return -1) even if m isn't a note
	int chan = m->Channel();
	int pitch = m->Pitch();
	int velocity = m->Velocity();

	bool isnoteon = (m->MidiType()==MIDI_NOTE_ON);
	bool isnoteoff = (m->MidiType()==MIDI_NOTE_OFF);
	if ( isnoteon && m->Velocity() == 0 ) {
		isnoteoff = true;
		isnoteon = false;
	}

	if ( isnoteon ) {
		DEBUGPRINT1(("Adding pitch=%d to m_notesdown",m->Pitch()));
		m_notesdown.push_back(m->clone());
	} else if ( isnoteoff ) {
		// Find note in current list
		bool found = false;
	    for ( std::list<MidiMsg*>::const_iterator ci = m_notesdown.begin(); ci != m_notesdown.end(); ) {
			MidiMsg* m2 = *ci;
			if ( chan == m2->Channel() && pitch == m2->Pitch() ) {
				m_notesdown.erase(ci++);
				DEBUGPRINT1(("Removing pitch=%d from m_notesdown",pitch));
				found = true;
				// We could break, but continuing on
				// allows us to remove multiple instances
				// of a note (which "shouldn't happen").
			} else {
				ci++;
			}
		}
	}

	UnlockNotesDown();
}

void NosuchScheduler::Callback(PtTimestamp timestamp) {

	if ( m_running == false ) {
		return;
	}

	// m_MilliNow = timestamp;
	AdvanceTimeAndDoEvents(timestamp);

	static int lastcallback = 0;
	lastcallback = timestamp;

	static int lastdump = 0;
	// NosuchDebug messages aren't flushed to the log right away, to avoid
	// screwed up timing continuously.  I.e. we only screw up the timing every 5 seconds
	if ( NosuchDebugAutoFlush==false && (timestamp - lastdump) > 5000 ) {
		lastdump = timestamp;
		NosuchDebugDumpLog();
	}

	static int lasttime = 0;

	PmError result;
	for (size_t i = 0; i < m_midi_input_stream.size(); i++ ) {
		PmStream* ps = m_midi_input_stream[i];
		if (ps == NULL) {
			continue;
		}
		do {
			result = Pm_Poll(ps);
			if (result) {
				PmEvent buffer;
		        int rslt = Pm_Read(ps, &buffer, 1);
				if ( rslt == 1 ) {
					int msg = buffer.message;
					lasttime = timestamp;
					if ( m_midiinput_client ) {
						MidiMsg* mm = MidiMsg::make(msg);
						if ( mm == NULL ) {
							DEBUGPRINT(("Hey, MidiMsg::make returned NULL!?"));
						} else {
							mm->SetInputPort(i);
							_maintainNotesDown(mm);
							// NOTE: ownership of the MidiMsg memory is given up here.
							// It is the responsibility of the client to delete it.
							MidiMsg* cloned = mm->clone();
							m_midiinput_client->processMidiMsg(mm);
							// send to all midimerge outputs
							for (size_t j = 0; j < m_midi_merge_outport.size(); j++ ) {
								int outport = m_midi_merge_outport[j];
								if (outport>=0) {
									// DEBUGPRINT(("Sending to midi_merge j=%d outport=%d", j,outport));
									cloned->SetOutputPort(outport);
#define CURSORID_FOR_MIDIMERGE (-2)
									QueueMidiMsg(cloned, m_currentclick, CURSORID_FOR_MIDIMERGE);
								}
							}
						}
					}
				} else if (rslt == pmBufferOverflow) {
					DEBUGPRINT(("Input Buffer Overflow!?\n"));
				} else {
					DEBUGPRINT(("Unexpected Pm_Read rslt=%d\n",rslt));
				}
			}
		} while (result);
	}

	return;
}

static char easytolower(char in){
  if(in<='Z' && in>='A')
    return in-('Z'-'z');
  return in;
} 

static std::string lowercase(std::string s) {
	std::string lc = s;
	std::transform(lc.begin(), lc.end(), lc.begin(), easytolower);
	return lc;
}

int findDevice(std::string nm, bool findinput, std::string& found_name)
{
	nm = lowercase(nm);
	int cnt = Pm_CountDevices();
	DEBUGPRINT2(("findDevice findinput=%d looking for (%s)",findinput,nm.c_str()));
	for ( int n=0; n<cnt; n++ ) {
		const PmDeviceInfo* info = Pm_GetDeviceInfo(n);
		DEBUGPRINT2(("Looking at MIDI device: i=%d o=%d name=(%s)",info->input,info->output,info->name));
		if ( findinput == true && info->input == false )
			continue;
		if ( findinput == false && info->output == false )
			continue;

		std::string lowername = lowercase(std::string(info->name));
		DEBUGPRINT1(("info->name=%s  lowername=%s  nm=%s",info->name,lowername.c_str(),nm.c_str()));
		if ( lowername.find(nm) != lowername.npos ) {
			found_name = std::string(info->name);
			return n;
		}
	}
	return -1;
}

int findOutputDevice(std::string nm, std::string& found_name)
{
	return findDevice(nm,0,found_name);
}

int findInputDevice(std::string nm, std::string& found_name)
{
	return findDevice(nm,1,found_name);
}

bool NosuchScheduler::StartMidi(cJSON* config) {

	if (m_running)
		return true;

	cJSON* midi_inputs = jsonGetArray(config, "midiinputs");
	cJSON* midi_outputs = jsonGetArray(config, "midioutputs");
	cJSON* midi_merges = jsonGetArray(config, "midimerges");

	m_timestamp0InMilliseconds = 0;

	int resolution = 5;   // normally 1, maybe 5?
	DEBUGPRINT(("Calling Pt_Start with resolution=%d", resolution));
	Pt_Start(resolution, midi_callback, (void *)this);

	// std::vector<std::string> inputs = NosuchSplitOnString(midi_input, ";");
	// std::vector<std::string> outputs = NosuchSplitOnString(midi_output, ";");
	// std::vector<std::string> merges = NosuchSplitOnString(midi_merge, ";");

	size_t ninputs = cJSON_GetArraySize(midi_inputs);
	m_midi_input_stream.resize(ninputs);
	m_midi_input_name.resize(ninputs);

	size_t noutputs = cJSON_GetArraySize(midi_outputs);
	m_midi_output_stream.resize(noutputs);
	m_midi_output_name.resize(noutputs);

	size_t nmerges = cJSON_GetArraySize(midi_merges);
	m_midi_merge_outport.resize(nmerges);
	m_midi_merge_name.resize(nmerges);

	if (nmerges > 0 && nmerges != ninputs) {
		throw NosuchException("midimerges isn't the same size as midiinputs!?");
	}

	for (size_t i = 0; i < ninputs; i++) {
		cJSON *j = cJSON_GetArrayItem(midi_inputs,i);
		NosuchAssert(j->type == cJSON_String);
		m_midi_input_stream[i] = _openMidiInput(j->valuestring);
		m_midi_input_name[i] = j->valuestring;
	}
	for (size_t i = 0; i < noutputs; i++) {
		cJSON *j = cJSON_GetArrayItem(midi_outputs,i);
		NosuchAssert(j->type == cJSON_String);
		m_midi_output_stream[i] = _openMidiOutput(j->valuestring);
		m_midi_output_name[i] = j->valuestring;
	}
	for (size_t i = 0; i < nmerges; i++) {
		cJSON *j = cJSON_GetArrayItem(midi_merges,i);
		NosuchAssert(j->type == cJSON_String);
		m_midi_merge_name[i] = j->valuestring;
		// Find the output port in the m_midi_output_* list
		int outport = -1;
		for (size_t k = 0; k < noutputs; k++) {
			if (m_midi_merge_name[i] == m_midi_output_name[k]) {
				outport = k;
				break;
			}
		}
		if (outport < 0) {
			throw NosuchException("Didn't find midimerge value '%s' in midioutputs!", m_midi_merge_name[i].c_str());
		}
		m_midi_merge_outport[i] = outport;
	}

	DEBUGPRINT(("MIDI started: %d inputs, %d outputs",ninputs,noutputs));

	m_running = true;
	return true;
}

PmStream*
NosuchScheduler::_openMidiOutput(std::string midi_output) {

	int outputId = -1;
	int inputId = -1;

	std::string found_output;
	int id = findOutputDevice(midi_output, found_output);
	if (id < 0) {
		DEBUGPRINT(("Unable to open MIDI output: %s", midi_output.c_str()));
		return NULL;
	}
	DEBUGPRINT1(("Found MIDI output:%s", found_output.c_str()));

	PmStream* pm_out;
	/* use zero latency because we want output to be immediate */
	PmError e = Pm_OpenOutput(&pm_out, 
					id, 
					NULL /* driver info */,
					OUT_QUEUE_SIZE,
					NULL, /* timeproc */
					NULL /* time info */,
					0 /* Latency */);

	if ( e != pmNoError ) {
		DEBUGPRINT(("Error when opening MIDI Output : %d\n",e));
		pm_out = NULL;
	}
	return pm_out;
}

PmStream*
NosuchScheduler::_openMidiInput(std::string midi_input) {

	std::string found_input;
	int id = findInputDevice(midi_input, found_input);
	if (id < 0) {
		DEBUGPRINT(("Unable to open MIDI input: %s", midi_input.c_str()));
		return NULL;
	}
	DEBUGPRINT1(("Found MIDI input: %s", found_input.c_str()));
	PmStream* pm_in;
	PmError e = Pm_OpenInput(&pm_in, 
						id, 
						NULL /* driver info */,
						0 /* use default input size */,
						NULL,
						NULL /* time info */);

	if ( e != pmNoError ) {
		DEBUGPRINT(("Error when opening MIDI Input : %d\n",e));
		pm_in = NULL;
	}
	return pm_in;
}

#if 0
SchedEventList* NosuchScheduler::ScheduleOf(const char* handle) {
	std::map<const char*,SchedEventList*>::iterator it = m_scheduled.find(handle);
	if ( it == m_scheduled.end() ) {
		DEBUGPRINT1(("CREATING NEW SchedEventList SCHEDULE for handle = %ld",(long)handle));
		m_scheduled[handle] = new SchedEventList();
		return m_scheduled[handle];
	} else {
		return it->second;
	}
}
#endif

void NosuchScheduler::Stop() {
	if ( m_running == TRUE ) {
		DEBUGPRINT(("NosuchScheduler is being stopped"));
		Pt_Stop();
		for (size_t i = 0; i<m_midi_input_stream.size(); i++ ) {
			if (m_midi_input_stream[i]) {
				Pm_Close(m_midi_input_stream[i]);
				m_midi_input_stream[i] = NULL;
				m_midi_input_name[i] = "";
			}
		}
		for (size_t i = 0; i<m_midi_output_stream.size(); i++ ) {
			if (m_midi_output_stream[i]) {
				Pm_Close(m_midi_output_stream[i]);
				m_midi_output_stream[i] = NULL;
				m_midi_output_name[i] = "";
			}
		}
		for (size_t i = 0; i<m_midi_merge_outport.size(); i++ ) {
			m_midi_merge_outport[i] = -1;
			m_midi_merge_name[i] = "";
		}
		Pm_Terminate();
		m_running = false;
		DEBUGPRINT(("NosuchScheduler is stopped"));
	}
}

void NosuchScheduler::AdvanceTimeAndDoEvents(PtTimestamp timestamp) {

	m_LastTimeStampInMilliseconds = timestamp;
	NosuchAssert(m_running==true);

	int timesofar = timestamp - m_timestamp0InMilliseconds;
	int clickssofar = (int)(0.5 + timesofar * m_ClicksPerMillisecond);

	if ( clickssofar <= m_currentclick ) {
		return;
	}
	m_currentclick = clickssofar;
	if ( m_click_client ) {
		m_click_client->processAdvanceClickTo(m_currentclick);
	}

	// We don't want to collect a whole bunch of blocked callbacks,
	// so if we can't get the lock, we just give up.
	int err = TryLockScheduled();
	if ( err != 0 ) {
		DEBUGPRINT1(("NosuchScheduler::Callback timestamp=%d - TryLockScheduled failed",timestamp));
		return;
	}

	int nevents = 0;

	while ( m_scheduled->size() > 0 ) {
		SchedEvent* ev = m_scheduled->front();
		nevents++;
		int clk = ev->click;
		if ( clk > m_currentclick ) {
			break;		// SchedEventList is sorted by time
		}
		// This happens occasionally, at least 1 click diff
		click_t delta = m_currentclick - ev->click;
		if ( delta > 4 ) {
			DEBUGPRINT1(("Hmmm, clicknow (%d) is a lot more than ev->click (%d) !?",m_currentclick,ev->click));
		}
		m_scheduled->pop_front();
		// We assume ev is still valid, right?  (since the SchedEventList is
		// a list of pointers)

		DoEventAndDelete(ev);
	}
	
	_addQueueToScheduled();

	UnlockScheduled();

	if ( m_periodic_ANO ) {
		// We send an ANO every once in a while, if there's been no events,
		// to deal with stuck notes.
		static int last_ano_or_event = 0;
		if ( nevents == 0 ) {
			if ( (timestamp - last_ano_or_event) > 3000 ) {
				ANO(-1);
				last_ano_or_event = timestamp;
			}
		} else {
				last_ano_or_event = timestamp;
		}
	}
}

void
NosuchScheduler::_addQueueToScheduled() {
	// Add _queue things to _scheduled
	int nqueue = 0;

	LockQueue();
	// XXX - profiling shows this to be a hotspot
	SchedEventList* sl = m_queue;
	while (sl->size() > 0) {
		SchedEvent* ev = sl->front();
		nqueue++;
		sl->pop_front();
		// We assume ev is still valid, right?  (since the SchedEventList is
		// a list of pointers)
		ScheduleAddEvent(ev, false);  // we're already locked, so lockit==false
	}
	UnlockQueue();
}
	
// SendPmMessage IS BEING PHASED OUT - ONLY ANO STILL USES IT
void NosuchScheduler::SendPmMessage(PmMessage pm, PmStream* ps) {

	PmEvent ev[1];
	ev[0].timestamp = TIME_PROC(TIME_INFO);
	ev[0].message = pm;
	if ( ps ) {
		Pm_Write(ps,ev,1);
	} else {
		DEBUGPRINT1(("SendPmMessage: No MIDI output device?"));
	}
	if ( NosuchDebugMidiAll ) {
		DEBUGPRINT(("MIDI OUTPUT PM bytes=%02x %02x %02x",
			Pm_MessageStatus(pm),
			Pm_MessageData1(pm),
			Pm_MessageData2(pm)));
	}
}

// The sid (session-id) indicates who sent it.  It can either be a
// TUIO session id, a Loop ID, or -1.
// The reason we pass in MidiMsg* is so we can use it for the Notification call.
void NosuchScheduler::SendMidiMsg(MidiMsg* msg, int cursorid) {
	MidiMsg* origmsg = msg;
	MidiMsg* mm = msg;
	MidiMsg* newmm = NULL;

	int outputport = msg->OutputPort();

	bool isnoteon = (mm->MidiType() == MIDI_NOTE_ON);
	bool isnoteoff = (mm->MidiType() == MIDI_NOTE_OFF);
	bool isnote = isnoteon || isnoteoff;

	if (GlobalPitchOffset != 0 && mm->Channel() != 10 && isnote) {
		// XXX - need to do this without new/delete
		newmm = mm->clone();
		newmm->Pitch(newmm->Pitch() + GlobalPitchOffset);
	}
	if (newmm != NULL) {
		mm = newmm;
	}

	// XXX - need to have map of channels that don't do pitch offset
	_maintainNotesDown(mm);

	PtTimestamp tm = TIME_PROC(TIME_INFO);
	PmMessage pm = mm->PortMidiMessage();
	PmEvent ev[1];
	ev[0].timestamp = tm;
	ev[0].message = pm;

	if (outputport < 0 && m_midi_output_stream.size() > 0) {
		DEBUGPRINT1(("SendMidiMsg: unspecified MIDI outputport, using default"));
		outputport = 0;
	}

	PmStream* ps = (outputport < 0) ? NULL : m_midi_output_stream[outputport];
	if (ps) {
		Pm_Write(ps, ev, 1);
	} else {
		static int lastwarning = -99999;
		// Only put out warnings every second
		if ( (m_LastTimeStampInMilliseconds - lastwarning) > 1000 ) {
			DEBUGPRINT(("SendMidiMsg: no MIDI output device for outputport=%d?", outputport));
			lastwarning = m_LastTimeStampInMilliseconds;
		}
	}

	if ( NosuchDebugMidiAll ) {
		DEBUGPRINT(("MIDI OUTPUT MM bytes=%02x %02x %02x",
			Pm_MessageStatus(pm),
			Pm_MessageData1(pm),
			Pm_MessageData2(pm)));
	} else if (isnote && NosuchDebugMidiNotes) {
		int pitch = mm->Pitch();
		DEBUGPRINT(("MIDI OUTPUT %s ch=%d v=%d pitch=%s/p%d",
			isnoteon?"NoteOn":"NoteOff",
			mm->Channel(),mm->Velocity(),ReadableMidiPitch(pitch),pitch));
	}

	if ( m_midioutput_client ) {
		m_midioutput_client->processMidiMsg(mm);
	}

	if ( newmm != NULL ) {
		delete newmm;
	}

}

// Returns true if the looped event should still continue
bool
NosuchScheduler::_handleLoopEvent(SchedEvent* e)
{
	// We assume the schedule is locked.

	MidiMsg* m = e->u.midimsg;
	int mt = m->MidiType();
	bool addit = true;
	if (mt == MIDI_NOTE_ON) {
		MidiNoteOn* noteon = (MidiNoteOn*)m;
		int oldv = noteon->Velocity();
		int newv = (int)(oldv * e->m_loopfade + 0.5);
#define MIN_VELOCITY 5
		if (newv < MIN_VELOCITY) {
			addit = false;
			ScheduleClear(e->m_cursorid, false);  // we assume it's already locked
			DEBUGPRINT(("Loop finished for cursorid=%d  sched.size=%d",e->m_cursorid,m_scheduled->size()));
			delete m;
		}
		else {
			noteon->Velocity(newv);
		}
	}
	if (addit) {
		QueueAddEvent(e);
	}
	return addit;
}

// We assume that the SchedEvent here has already been removed from the
// Schedule list.  So, after processing it, we may (if looping is turned on) merely
// re-queue the event (using QueueAddEvent) for being added back to the Scheduler list,
// rather than deleting it.  From the caller's point of view, though, the SchedEvent
// has been deleted.
void NosuchScheduler::DoEventAndDelete(SchedEvent* e) {

	// We assume the schedule is locked

	bool deleteit = true;
	if (e->eventtype() == SchedEvent::MIDIMSG ) {
		MidiMsg* m = e->u.midimsg;
		NosuchAssert(m);

		if (e->m_loopclicks > 0) {
			// Re-use the same SchedEvent and MidiMsg, and queue it up for
			// eventually being added to the Scheduled list
			deleteit = false;
			e->click += e->m_loopclicks;
			bool continueloop = _handleLoopEvent(e);
			if (continueloop) {
				DoMidiMsgEvent(m, e->m_cursorid);
			}
		}
		else {
			DoMidiMsgEvent(m,e->m_cursorid);
			delete m;
		}
	} else if (e->eventtype() == SchedEvent::MIDIPHRASE ) {
		MidiPhrase* ph = e->u.midiphrase;
		MidiPhraseUnit* pu = ph->first;
		if ( pu ) {
			MidiPhraseUnit* nextpu = pu->next;
			click_t prevclick = pu->click;
			MidiMsg* m = pu->msg;
			// Looped MIDIPHRASE events are essentially converted to
			// individual MIDIMSG events, here.  That may not be desirable, someday.
			if (e->m_loopclicks > 0) {
				click_t nextclick = e->click + e->m_loopclicks;
				SchedEvent* e2 = new SchedEvent(m, nextclick, e->m_cursorid);
				bool continueloop = _handleLoopEvent(e2);
				if (continueloop) {
					DoMidiMsgEvent(m, e->m_cursorid);
				}
			}
			else {
				DoMidiMsgEvent(m, e->m_cursorid);
				delete m;
			}
			// XXX - put this back!!   delete pu;
			ph->first = nextpu;
			if ( nextpu != NULL ) {
				// Reschedule the event, using the same MidiPhrase
				deleteit = false;
				click_t nextclick = nextpu->click;
				e->click = e->click + (nextclick - prevclick);
				QueueAddEvent(e);
			}
		}
	} else {
		DEBUGPRINT(("Hey, DoEvent can't handle event type %d!?",e->eventtype()));
	}
	if ( deleteit ) {
		delete e;
	}
}

// NOTE: this routine takes ownership of the MidiMsg pointed to by m,
// so the caller shouldn't delete it or even try to access it afterward.
void
NosuchScheduler::DoMidiMsgEvent(MidiMsg* m, int cursorid)
{
	NosuchAssert(m);
	SendMidiMsg(m,cursorid);
	return;

#ifdef NOWPLAYING
	if ( m_nowplaying_note.find(sidnum) != _nowplaying_note.end() ) {
		DEBUGPRINT2(("DoEvent, found m_nowplaying_note for sid=%d",sidnum));
		MidiMsg* nowplaying = m_nowplaying_note[sidnum];
		NosuchAssert(nowplaying);
		if ( nowplaying == m ) {
			DEBUGPRINT(("Hey, DoEvent called with m==nowplaying?"));
		}

		// If the event we're doing is a noteoff, and nowplaying is
		// the same channel/pitch, then we just play the event and
		// get rid of m_nowplaying_note
		if ( mt == MIDI_NOTE_OFF ) {
			SendMidiMsg(m,sidnum);
			if ( m->Channel() == nowplaying->Channel() && m->Pitch() == nowplaying->Pitch() ) {
				m_nowplaying_note.erase(sidnum);
				delete nowplaying;
			}
			delete m;
			return;
		}

		// Controller messages here
		if ( mt != MIDI_NOTE_ON ) {
			SendMidiMsg(m,sidnum);
			delete m;
			return;
		}

		// We want a NoteOff equivalent of nowplaying (which is a NoteOn)
		MidiNoteOn* nowon = (MidiNoteOn*)nowplaying;
		NosuchAssert(nowon);
		MidiNoteOff* nowoff = nowon->makenoteoff();
		SendMidiMsg(nowoff,sidnum);
		delete nowoff;

		m_nowplaying_note.erase(sidnum);
		delete nowplaying;
	} else {
		DEBUGPRINT1(("DoEvent, DID NOT FIND m_nowplaying_note for sid=%d",sidnum));
	}

	SendMidiMsg(m,sidnum);

	if ( m->MidiType() == MIDI_NOTE_ON ) {
		m_nowplaying_note[sidnum] = m;
	} else {
		delete m;
	}
#endif
}

std::string
NosuchScheduler::DebugString() {

	std::string s;
	s = "NosuchScheduler (\n";

	SchedEventIterator it = m_scheduled->begin();
	for ( ; it != m_scheduled->end(); it++ ) {
		SchedEvent* ep = *it;
		NosuchAssert(ep);
		s += NosuchSnprintf("      Event %s\n",ep->DebugString().c_str());
	}
	s += "   }";
	return s;
}

std::string
SchedEvent::DebugString() {
	std::string s;
	switch (m_eventtype) {
	case SchedEvent::CURSORMOTION:
		NosuchAssert(u.cursormotion != NULL);
		s = NosuchSnprintf("SchedEvent CursorMotion downdragup=%d pos=%.4f,%.4f depth=%.4f",u.cursormotion->m_downdragup,u.cursormotion->m_pos.x,u.cursormotion->m_pos.y,u.cursormotion->m_depth);
		break;
	case SchedEvent::MIDIMSG:
		NosuchAssert(u.midimsg != NULL);
		DEBUGPRINT(("DebugString u.midimsg=%ld", (long)u.midimsg));
		s = NosuchSnprintf("SchedEvent MidiMsg type=%d",u.midimsg->MidiType());
		break;
	case SchedEvent::MIDIPHRASE:
		NosuchAssert(u.midiphrase != NULL);
		NosuchAssert(u.midiphrase->first);
		NosuchAssert(u.midiphrase->first->msg);
		s = NosuchSnprintf("SchedEvent MidiPhrase (first msgtype=%d",u.midiphrase->first->msg->MidiType());
		break;
	default:
		s = "Unknown eventtype !?";
		break;
	}
	return NosuchSnprintf("Ev click=%d %s",click,s.c_str());
}

void NosuchScheduler::SetClicksPerSecond(int clkpersec) {
	DEBUGPRINT1(("Setting ClicksPerSecond to %d",clkpersec));
	m_ClicksPerSecond = clkpersec;
	m_ClicksPerMillisecond = m_ClicksPerSecond / 1000.0;
	int timesofar = m_LastTimeStampInMilliseconds - m_timestamp0InMilliseconds;
	int clickssofar = (int)(0.5 + timesofar * m_ClicksPerMillisecond);
	m_currentclick = clickssofar;
}

click_t NosuchScheduler::ClicksPerSecond() {
	return int(m_ClicksPerMillisecond * 1000.0 + 0.5);
}

// A "beat" is a quarter note, typically
click_t NosuchScheduler::ClicksPerBeat() {
	int bpm = 120;
	return ClicksPerSecond() * 60 / bpm;
}

void NosuchScheduler::SetTempoFactor(float f) {
	QuarterNoteClicks = (int)(96 * f);
	DEBUGPRINT(("Setting QuarterNoteClicks to %d",QuarterNoteClicks));
}
