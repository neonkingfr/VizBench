#include "VizUtil.h"
#include "VizScheduler.h"

#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

int SchedulerCount = 0;
int GlobalPitchOffset = 0;

int VizScheduler::m_ClicksPerSecond = 0;
double VizScheduler::m_ClicksPerMillisecond = 0;
int VizScheduler::m_LastTimeStampInMilliseconds;
int VizScheduler::m_timestamp0InMilliseconds;
click_t VizScheduler::m_currentclick;

void midi_callback( PtTimestamp timestamp, void *userData ) {
	VizScheduler* ms = (VizScheduler*)userData;
	VizAssert(ms);

	// DEBUGPRINT(("midi_callback time=%ld timestamp=%ld", timeGetTime(),timestamp));
	try {
		CATCH_NULL_POINTERS;
		ms->Callback(timestamp);
	} catch (VizException& e) {
		DEBUGPRINT(("VizException: %s",e.message()));
	}
}

VizScheduler::VizScheduler() {
	DEBUGPRINT1(("VizScheduler CONSTRUCTED!!, count=%d", SchedulerCount++));
	m_running = false;
	m_currentclick = 0;

#ifdef NOWPLAYING
	m_nowplaying_note.clear();
	// m_nowplaying_controller.clear();
#endif

	SetClicksPerSecond(192);

	VizLockInit(&m_scheduled_mutex, "scheduled");
	VizLockInit(&m_notesdown_mutex, "notesdown");
	VizLockInit(&m_queue_mutex, "queue");
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
	m_queue = new VizSchedEventList();
	m_scheduled = new VizSchedEventList();
}

VizScheduler::~VizScheduler() {
	Stop();
}

static bool
VizSchedEvent_compare (VizSchedEvent* first, VizSchedEvent* second)
{
	if (first->click<second->click)
		return true;
	else
		return false;
}

#ifdef OLDSCHEDULER
void VizScheduler::_sortEvents(VizSchedEventList* sl) {
	stable_sort (sl->begin(), sl->end(),VizSchedEvent_compare);
}
#endif

void VizScheduler::ANO(int psi,int ch) {
	VizAssert(ch!=0);
	if (psi < 0) {
		// Send it on all output ports
		for (size_t pi = 0; pi < m_midi_output_stream.size(); pi++) {
			ANO(m_midi_output_stream[pi], ch);
		}
	} else {
		ANO(m_midi_output_stream[psi], ch);
	}
}

void VizScheduler::ANO(PmStream* ps,int ch) {
	VizAssert(ch!=0);
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

void VizScheduler::ScheduleMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizSchedEvent* e = new VizSchedEvent(m,clk,cursorid, looping, mp);
	if ( ! ScheduleAddEvent(e) ) {
		DEBUGPRINT(("ScheduleMidiMsg is DELETING VizSchedEvent! e=%ld",(long)e));
		delete e;
	}
}

void VizScheduler::ScheduleMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizSchedEvent* e = new VizSchedEvent(ph,clk,cursorid, looping, mp);
	if ( ! ScheduleAddEvent(e) ) {
		DEBUGPRINT(("ScheduleMidiPhrase is DELETING VizSchedEvent! e=%ld",(long)e));
		delete e;
	}
}

void VizScheduler::ScheduleClear(int cursorid, bool lockit) {
	if (lockit) {
		LockScheduled();
	}
	if (cursorid == SCHEDID_ALL) {
		m_scheduled->DeleteAll();
	}
	else {
		m_scheduled->DeleteAllId(cursorid);
	}
	if (lockit) {
		UnlockScheduled();
	}
}

bool
VizScheduler::ScheduleAddEvent(VizSchedEvent* e, bool lockit) {
	// DEBUGPRINT(("ScheduleAddEvent clicknow=%d clk=%d handle=%ld %s",
	//	m_currentclick,e->click,(long)(e->handle),e->DebugString().c_str()));
	if ( lockit ) {
		LockScheduled();
	}

	m_scheduled->InsertEvent(e);

#ifdef OLDSCHEDULER
	// XXX - profiling shows this to be a hotspot
	VizSchedEventList* sl = m_scheduled;
	sl->push_back(e);
	// XXX - especially here.  Need to replace VizSchedEventList with
	// XXX - something that keeps track of the end of the list and
	// XXX - adds it there quickly if it can, rather than blindly
	// XXX - adding it and sorting.
	_sortEvents(sl);
#endif

	if ( lockit ) {
		UnlockScheduled();
	}
	return true;
}

void VizScheduler::QueueMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizSchedEvent* e = new VizSchedEvent(m,clk,cursorid,looping,mp);
	if ( ! QueueAddEvent(e) ) {
		DEBUGPRINT(("QueueMidiMsg is DELETING VizSchedEvent! e=%ld",(long)e));
		delete e;
	}
}

void VizScheduler::QueueMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizSchedEvent* e = new VizSchedEvent(ph,clk,cursorid,looping,mp);
	if ( ! QueueAddEvent(e) ) {
		DEBUGPRINT(("QueueMidiPhrase is DELETING VizSchedEvent! e=%ld",(long)e));
		delete e;
	}
}

void VizScheduler::QueueClear() {
	LockQueue();
	m_queue->DeleteAll();
	UnlockQueue();
}

void VizScheduler::QueueRemoveBefore(int cursorid, click_t clk) {
	m_scheduled->DeleteAllBefore(cursorid,clk);
	m_queue->DeleteAllBefore(cursorid,clk);
	DEBUGPRINT1(("VizScheduler::QueueRemoveBefore clk=%d sid=%d  afterward NumQueued=%d  NumScheduled=%d",
		clk,cursorid,NumQueuedEventsOfSid(cursorid),NumScheduledEventsOfSid(cursorid) ));
}

int
VizScheduler::NumQueuedEventsOfSid(int sid) {
	return m_queue->NumEventsOfSid(sid);
}

int
VizScheduler::NumScheduledEventsOfSid(int sid) {
	return m_scheduled->NumEventsOfSid(sid);
}

bool
VizScheduler::QueueAddEvent(VizSchedEvent* e) {
	static int count = 0;
	LockQueue();

	m_queue->InsertEvent(e);

	DEBUGPRINT1(("QueueAddEvent, queue size is now %d  cursorsid=%d  numofsid=%d",
		m_queue->NumEvents(),e->m_cursorid,m_queue->NumEventsOfSid(e->m_cursorid)));

	count++;
	DEBUGPRINT1(("QueueAddEvent, count=%d queue is now: %s ", count, m_queue->DebugString().c_str()));
	if (count == 3) {
		DEBUGPRINT1(("CHECK HERE"));
	}

#ifdef OLDSCHEDULER
	// XXX - profiling shows this to be a hotspot
	// XXX - need to rework m_queue - most additions will be
	// XXX - at the end of the list, which can be done quickly
	// XXX - without sorting.
	m_queue->push_back(e);
	_sortEvents(m_queue);
#endif

	UnlockQueue();
	return true;
}

void VizScheduler::_maintainNotesDown(MidiMsg* m) {

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

void VizScheduler::Callback(PtTimestamp timestamp) {

	if ( m_running == false ) {
		return;
	}

	// m_MilliNow = timestamp;
	AdvanceTimeAndDoEvents(timestamp);

	static int lastcallback = 0;
	lastcallback = timestamp;

	static int lastdump = 0;
	// VizDebug messages aren't flushed to the log right away, to avoid
	// screwed up timing continuously.  I.e. we only screw up the timing every 5 seconds
	if ( VizDebugAutoFlush==false && (timestamp - lastdump) > 5000 ) {
		lastdump = timestamp;
		VizDebugDumpLog();
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

bool VizScheduler::StartMidi(cJSON* config) {

	if (m_running)
		return true;

	cJSON* midi_inputs = jsonGetArray(config, "midiinputs");
	cJSON* midi_outputs = jsonGetArray(config, "midioutputs");
	cJSON* midi_merges = jsonGetArray(config, "midimerges");

	m_timestamp0InMilliseconds = 0;

	int resolution = 5;   // normally 1, maybe 5?
	DEBUGPRINT(("Calling Pt_Start with resolution=%d", resolution));
	Pt_Start(resolution, midi_callback, (void *)this);

	// std::vector<std::string> inputs = VizSplitOnString(midi_input, ";");
	// std::vector<std::string> outputs = VizSplitOnString(midi_output, ";");
	// std::vector<std::string> merges = VizSplitOnString(midi_merge, ";");

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
		throw VizException("midimerges isn't the same size as midiinputs!?");
	}

	for (size_t i = 0; i < ninputs; i++) {
		cJSON *j = cJSON_GetArrayItem(midi_inputs,i);
		VizAssert(j->type == cJSON_String);
		m_midi_input_stream[i] = _openMidiInput(j->valuestring);
		m_midi_input_name[i] = j->valuestring;
	}
	for (size_t i = 0; i < noutputs; i++) {
		cJSON *j = cJSON_GetArrayItem(midi_outputs,i);
		VizAssert(j->type == cJSON_String);
		m_midi_output_stream[i] = _openMidiOutput(j->valuestring);
		m_midi_output_name[i] = j->valuestring;
	}
	for (size_t i = 0; i < nmerges; i++) {
		cJSON *j = cJSON_GetArrayItem(midi_merges,i);
		VizAssert(j->type == cJSON_String);
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
			throw VizException("Didn't find midimerge value '%s' in midioutputs!", m_midi_merge_name[i].c_str());
		}
		m_midi_merge_outport[i] = outport;
	}

	DEBUGPRINT(("MIDI started: %d inputs, %d outputs",ninputs,noutputs));

	m_running = true;
	return true;
}

PmStream*
VizScheduler::_openMidiOutput(std::string midi_output) {

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
VizScheduler::_openMidiInput(std::string midi_input) {

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
VizSchedEventList* VizScheduler::ScheduleOf(const char* handle) {
	std::map<const char*,VizSchedEventList*>::iterator it = m_scheduled.find(handle);
	if ( it == m_scheduled.end() ) {
		DEBUGPRINT1(("CREATING NEW VizSchedEventList SCHEDULE for handle = %ld",(long)handle));
		m_scheduled[handle] = new VizSchedEventList();
		return m_scheduled[handle];
	} else {
		return it->second;
	}
}
#endif

void VizScheduler::Stop() {
	if ( m_running == TRUE ) {
		DEBUGPRINT(("VizScheduler is being stopped"));
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
		DEBUGPRINT(("VizScheduler is stopped"));
	}
}

void VizScheduler::AdvanceTimeAndDoEvents(PtTimestamp timestamp) {

	m_LastTimeStampInMilliseconds = timestamp;
	VizAssert(m_running==true);

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
		DEBUGPRINT1(("VizScheduler::Callback timestamp=%d - TryLockScheduled failed",timestamp));
		return;
	}

	int nevents = 0;

	while ( ! m_scheduled->IsEmpty() ) {

		click_t firstclick = m_scheduled->FirstClick();
		if ( firstclick > m_currentclick ) {
			break;		// VizSchedEventList is sorted by time
		}

		nevents++;

		// This happens occasionally, at least 1 click diff
		click_t delta = m_currentclick - firstclick;
		if ( delta > 4 ) {
			DEBUGPRINT1(("Hmmm, clicknow (%d) is a lot more than ev->click (%d) !?",m_currentclick,firstclick));
		}

		// We assume ev is still valid, right?  (since the VizSchedEventList is
		// a list of pointers)
		VizSchedEvent* e = m_scheduled->PopFirst();

		static int count = 0;
		count++;
		DEBUGPRINT1(("POPPED EVENT FROM SCHEDULED count=%d, ABOUT TO DO IT e=%s",count,e->DebugString().c_str()));
		m_scheduled->CheckSanity("AdvanceTime A");
		VizAssert(e != NULL);
		if (count == 3) {
			DEBUGPRINT1(("TRACE NOW"));
		}
		DoEventAndDelete(e);
		m_scheduled->CheckSanity("AdvanceTime B");
		DEBUGPRINT1(("AFTER POPPED EVENT IS DONE count=%d, queue: %s",count,m_queue->DebugString().c_str()));
		DEBUGPRINT1(("AFTER POPPED EVENT IS DONE count=%d, scheduled: %s",count,m_scheduled->DebugString().c_str()));
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
VizScheduler::_addQueueToScheduled() {
	// Add _queue things to _scheduled
	int nqueue = 0;
	static int count = 0;

	LockQueue();
	m_scheduled->CheckSanity("addQueueToScheduled A");
	while ( ! m_queue->IsEmpty() ) {
		VizSchedEvent* ev = m_queue->PopFirst();
		nqueue++;
		// We assume ev is still valid, right?  (since the VizSchedEventList is
		// a list of pointers)
		count++;
		DEBUGPRINT1(("ADDING QUEUE to SCHEDULED ev=%s count=%d",ev->DebugString().c_str(),count));
		if (ev->m_eventtype == VizSchedEvent::MIDIMSG && ev->u.midimsg->MidiType() == MIDI_NOTE_OFF ) {
			DEBUGPRINT1(("TRACE FROM HERE"));
		}
		ScheduleAddEvent(ev, false);  // we're already locked, so lockit==false
		DEBUGPRINT1(("====== AFTER ADDING QUEUE TO SCHEDULED: m_scheduled = %s", m_scheduled->DebugString().c_str()));
		DEBUGPRINT1(("====== AFTER ADDING QUEUE TO SCHEDULED: m-queue = %s", m_queue->DebugString().c_str()));
	}
	m_scheduled->CheckSanity("addQueueTo B");
	if (!m_scheduled->IsEmpty()){
		DEBUGPRINT1(("====== after addqueuetoscheduled, scheduled is: %s", m_scheduled->DebugString().c_str()));
		DEBUGPRINT1(("====== after addqueuetoscheduled, queue is: %s", m_queue->DebugString().c_str()));
	}
	UnlockQueue();
}
	
// SendPmMessage IS BEING PHASED OUT - ONLY ANO STILL USES IT
void VizScheduler::SendPmMessage(PmMessage pm, PmStream* ps) {

	PmEvent ev[1];
	ev[0].timestamp = TIME_PROC(TIME_INFO);
	ev[0].message = pm;
	DEBUGPRINT(("Obsolete SendPmMessage called!?"));
	if ( ps ) {
		Pm_Write(ps,ev,1);
	} else {
		DEBUGPRINT1(("SendPmMessage: No MIDI output device?"));
	}
	if ( VizDebugMidiAll ) {
		DEBUGPRINT(("MIDI OUTPUT PM bytes=%02x %02x %02x",
			Pm_MessageStatus(pm),
			Pm_MessageData1(pm),
			Pm_MessageData2(pm)));
	}
}

// The sid (session-id) indicates who sent it.  It can either be a
// TUIO session id, a Loop ID, or -1.
// The reason we pass in MidiMsg* is so we can use it for the Notification call.
void VizScheduler::SendMidiMsg(MidiMsg* msg, int cursorid) {
	MidiMsg* origmsg = msg;
	MidiMsg* mm = msg;
	MidiMsg* newmm = NULL;

	PtTimestamp tm = TIME_PROC(TIME_INFO);

	DEBUGPRINT1(("SendMidiMsg tm=%ld",tm));

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

	if ( VizDebugMidiAll ) {
		DEBUGPRINT(("MIDI OUTPUT MM bytes=%02x %02x %02x",
			Pm_MessageStatus(pm),
			Pm_MessageData1(pm),
			Pm_MessageData2(pm)));
	} else if (isnote && VizDebugMidiNotes) {
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
VizScheduler::_handleLoopEvent(VizSchedEvent* e)
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
			DEBUGPRINT1(("Loop finished for cursorid=%d  numevents is now %d",e->m_cursorid,m_scheduled->NumEvents()));
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

// We assume that the VizSchedEvent here has already been removed from the
// Schedule list.  So, after processing it, we may (if looping is turned on) merely
// re-queue the event (using QueueAddEvent) for being added back to the Scheduler list,
// rather than deleting it.  From the caller's point of view, though, the VizSchedEvent
// has been deleted.
void VizScheduler::DoEventAndDelete(VizSchedEvent* e) {

	MidiMsg* delete_midimsg = NULL;
	VizSchedEvent* delete_event = e;

	// We assume the schedule is locked

	if (e->eventtype() == VizSchedEvent::MIDIMSG ) {
		MidiMsg* m = e->u.midimsg;
		VizAssert(m);

		if (e->m_loopclicks > 0) {
			// Re-use the same VizSchedEvent and MidiMsg, and queue it up for
			// eventually being added to the Scheduled list
			delete_event = NULL;
			e->click += e->m_loopclicks;
			bool continueloop = _handleLoopEvent(e);
			if (continueloop) {
				DoMidiMsgEvent(m, e->m_cursorid);
			}
		}
		else {
			DoMidiMsgEvent(m,e->m_cursorid);
			delete_midimsg = m;
		}
	} else if (e->eventtype() == VizSchedEvent::MIDIPHRASE ) {
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
				VizSchedEvent* e2 = new VizSchedEvent(m, nextclick, e->m_cursorid);
				bool continueloop = _handleLoopEvent(e2);
				if (continueloop) {
					DoMidiMsgEvent(m, e->m_cursorid);
				}
			}
			else {
				DoMidiMsgEvent(m, e->m_cursorid);
				delete_midimsg = m;
			}
			// XXX - put this back!!   delete pu;
			ph->first = nextpu;
			if ( nextpu != NULL ) {
				// Reschedule the event, using the same MidiPhrase
				delete_event = NULL;
				click_t nextclick = nextpu->click;
				e->click = e->click + (nextclick - prevclick);
				QueueAddEvent(e);
			}
		}
	} else {
		DEBUGPRINT(("Hey, DoEvent can't handle event type %d!?",e->eventtype()));
	}
	if ( delete_event ) {
		delete delete_event;
	}
	if ( delete_midimsg ) {
		delete delete_midimsg;
	}
}

// NOTE: this routine takes ownership of the MidiMsg pointed to by m,
// so the caller shouldn't delete it or even try to access it afterward.
void
VizScheduler::DoMidiMsgEvent(MidiMsg* m, int cursorid)
{
	VizAssert(m);
	SendMidiMsg(m,cursorid);
	return;

#ifdef NOWPLAYING
	if ( m_nowplaying_note.find(sidnum) != _nowplaying_note.end() ) {
		DEBUGPRINT2(("DoEvent, found m_nowplaying_note for sid=%d",sidnum));
		MidiMsg* nowplaying = m_nowplaying_note[sidnum];
		VizAssert(nowplaying);
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
		VizAssert(nowon);
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
VizSchedEvent::DebugString() {
	std::string s;
	switch (m_eventtype) {
	case VizSchedEvent::CURSORMOTION:
		VizAssert(u.cursormotion != NULL);
		s = VizSnprintf("VizSchedEvent CursorMotion downdragup=%d pos=%.4f,%.4f depth=%.4f",u.cursormotion->m_downdragup,u.cursormotion->m_pos.x,u.cursormotion->m_pos.y,u.cursormotion->m_depth);
		break;
	case VizSchedEvent::MIDIMSG:
		VizAssert(u.midimsg != NULL);
		// DEBUGPRINT(("DebugString u.midimsg=%ld", (long)u.midimsg));
		s = VizSnprintf("VizSchedEvent MidiMsg midimsg=%ld  type=%s",(long)u.midimsg, u.midimsg->MidiTypeName());
		break;
	case VizSchedEvent::MIDIPHRASE:
		VizAssert(u.midiphrase != NULL);
		VizAssert(u.midiphrase->first);
		VizAssert(u.midiphrase->first->msg);
		s = VizSnprintf("VizSchedEvent MidiPhrase (first msgtype=%s",u.midiphrase->first->msg->MidiTypeName());
		break;
	default:
		s = "Unknown eventtype !?";
		break;
	}
	return VizSnprintf("Ev click=%d %s",click,s.c_str());
}

void VizScheduler::SetClicksPerSecond(int clkpersec) {
	DEBUGPRINT1(("Setting ClicksPerSecond to %d",clkpersec));
	m_ClicksPerSecond = clkpersec;
	m_ClicksPerMillisecond = m_ClicksPerSecond / 1000.0;
	int timesofar = m_LastTimeStampInMilliseconds - m_timestamp0InMilliseconds;
	int clickssofar = (int)(0.5 + timesofar * m_ClicksPerMillisecond);
	m_currentclick = clickssofar;
}

click_t VizScheduler::ClicksPerSecond() {
	return int(m_ClicksPerMillisecond * 1000.0 + 0.5);
}

// A "beat" is a quarter note, typically
click_t VizScheduler::ClicksPerBeat() {
	int bpm = 120;
	return ClicksPerSecond() * 60 / bpm;
}

void VizScheduler::SetTempoFactor(float f) {
	QuarterNoteClicks = (int)(96 * f);
	DEBUGPRINT(("Setting QuarterNoteClicks to %d",QuarterNoteClicks));
}
