#include "NosuchUtil.h"
#include "NosuchException.h"
#include "NosuchScheduler.h"
#include "NosuchLooper.h"

#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

#define QUIT_MSG 1000

int SchedulerCount = 0;

click_t GlobalClick = -1;
int GlobalPitchOffset = 0;
bool NoMultiNotes = true;
bool LoopCursors = true;

int NosuchScheduler::m_timestamp = 0;
int NosuchScheduler::m_ClicksPerSecond = 0;
double NosuchScheduler::m_ClicksPerMillisecond = 0;
int NosuchScheduler::m_LastTimeStamp;
int NosuchScheduler::m_timestamp0;
click_t NosuchScheduler::m_currentclick;

void midi_callback( PtTimestamp timestamp, void *userData ) {
	NosuchScheduler* ms = (NosuchScheduler*)userData;
	NosuchAssert(ms);

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
		SendPmMessage(pm,ps,NULL);
	}
}

void NosuchScheduler::IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle) {
	DEBUGPRINT1(("NosuchScheduler::IncomingNoteOff clk=%d ch=%d pitch=%d sid=%d",clk,ch,pitch,handle));
	ScheduleMidiMsg(MidiNoteOff::make(ch,pitch,vel),clk,handle);
}

void NosuchScheduler::IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle) {
	SchedEvent* e = new SchedEvent(m,clk,handle);
	if ( ! ScheduleAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::ScheduleMidiMsg(MidiMsg* m, click_t clk, void* handle) {
	SchedEvent* e = new SchedEvent(m,clk,handle);
	if ( ! ScheduleAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::ScheduleMidiPhrase(MidiPhrase* ph, click_t clk, void* handle) {
	SchedEvent* e = new SchedEvent(ph,clk,handle);
	if ( ! ScheduleAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::ScheduleClear() {
	LockScheduled();
	std::map<void*,SchedEventList*>::iterator it = m_scheduled.begin();
	for (; it != m_scheduled.end(); it++ ) {
		SchedEventList* sl = it->second;
		sl->clear();
	}
	UnlockScheduled();
}

bool
NosuchScheduler::ScheduleAddEvent(SchedEvent* e, bool lockit) {
	// DEBUGPRINT(("ScheduleAddEvent clicknow=%d clk=%d handle=%ld %s",
	// 	clicknow,e->click,(long)(e->handle),e->DebugString().c_str()));
	if ( lockit ) {
		LockScheduled();
	}

	SchedEventList* sl = ScheduleOf(e->handle);
	sl->push_back(e);
	_sortEvents(sl);

	if ( lockit ) {
		UnlockScheduled();
	}
	return true;
}

void NosuchScheduler::QueueMidiMsg(MidiMsg* m, click_t clk) {
	SchedEvent* e = new SchedEvent(m,clk);
	if ( ! QueueAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::QueueMidiPhrase(MidiPhrase* ph, click_t clk) {
	SchedEvent* e = new SchedEvent(ph,clk);
	if ( ! QueueAddEvent(e) ) {
		delete e;
	}
}

void NosuchScheduler::QueueClear() {
	m_queue->clear();
}

bool
NosuchScheduler::QueueAddEvent(SchedEvent* e) {
	m_queue->push_back(e);
	_sortEvents(m_queue);
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
									QueueMidiMsg(cloned, m_currentclick);
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

bool NosuchScheduler::StartMidi(std::string midi_input, std::string midi_output, std::string midi_merge) {
	if (m_running)
		return true;

	m_timestamp0 = 0;
	Pt_Start(1, midi_callback, (void *)this);   // maybe should be 5?

	std::vector<std::string> inputs = NosuchSplitOnString(midi_input, ";");
	std::vector<std::string> outputs = NosuchSplitOnString(midi_output, ";");
	std::vector<std::string> merges = NosuchSplitOnString(midi_merge, ";");

	m_midi_input_stream.resize(inputs.size());
	m_midi_input_name.resize(inputs.size());

	m_midi_output_stream.resize(outputs.size());
	m_midi_output_name.resize(outputs.size());

	m_midi_merge_outport.resize(merges.size());
	m_midi_merge_name.resize(merges.size());

	for (size_t i = 0; i < inputs.size(); i++) {
		m_midi_input_stream[i] = _openMidiInput(inputs[i]);
		m_midi_input_name[i] = inputs[i];
	}
	for (size_t i = 0; i < outputs.size(); i++) {
		m_midi_output_stream[i] = _openMidiOutput(outputs[i]);
		m_midi_output_name[i] = outputs[i];
	}
	for (size_t i = 0; i < merges.size(); i++) {
		m_midi_merge_name[i] = merges[i];
		// Find the output port in the m_midi_output_* list
		int outport = -1;
		for (size_t k = 0; k < outputs.size(); k++) {
			if (merges[i] == m_midi_output_name[k]) {
				outport = k;
				break;
			}
		}
		m_midi_merge_outport[i] = outport;
		if (outport < 0) {
			DEBUGPRINT(("Didn't find midimerge value '%s' in midioutputs!", merges[i]));
		}
	}

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
	DEBUGPRINT(("Found MIDI output:%s", found_output.c_str()));

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
	DEBUGPRINT(("Found MIDI input: %s", found_input.c_str()));
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

SchedEventList* NosuchScheduler::ScheduleOf(void* handle) {
	std::map<void*,SchedEventList*>::iterator it = m_scheduled.find(handle);
	if ( it == m_scheduled.end() ) {
		DEBUGPRINT1(("CREATING NEW SchedEventList SCHEDULE for handle = %ld",(long)handle));
		m_scheduled[handle] = new SchedEventList();
		return m_scheduled[handle];
	} else {
		return it->second;
	}
}

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

	m_timestamp = timestamp;
	m_LastTimeStamp = timestamp;
	NosuchAssert(m_running==true);

	int timesofar = timestamp - m_timestamp0;
	int clickssofar = (int)(0.5 + timesofar * m_ClicksPerMillisecond);

	if ( clickssofar <= m_currentclick ) {
		return;
	}
	m_currentclick = clickssofar;
	if ( m_click_client ) {
		m_click_client->AdvanceClickTo(m_currentclick,this);
	}
	GlobalClick = m_currentclick;

	// We don't want to collect a whole bunch of blocked callbacks,
	// so if we can't get the lock, we just give up.
	int err = TryLockScheduled();
	if ( err != 0 ) {
		DEBUGPRINT1(("NosuchScheduler::Callback timestamp=%d - TryLockScheduled failed",timestamp));
		return;
	}

	int nevents = 0;

	std::map<void*,SchedEventList*>::iterator itsid = m_scheduled.begin();

	for (; itsid != m_scheduled.end(); itsid++ ) {

		void* handle = itsid->first;

		SchedEventList* sl = itsid->second;
		NosuchAssert(sl);
		while ( sl->size() > 0 ) {
			SchedEvent* ev = sl->front();
			nevents++;
			int clk = ev->click;
			if ( clk > m_currentclick ) {
				break;		// SchedEventList is sorted by time
			}
			// This happens occasionally, at least 1 click diff
			click_t delta = m_currentclick - ev->click;
			if ( delta > 4 ) {
				DEBUGPRINT1(("Hmmm, clicknow (%d) is a lot more than ev->click (%d) !?",clicknow,ev->click));
			}
			sl->pop_front();
			// We assume ev is still valid, right?  (since the SchedEventList is
			// a list of pointers)

			DoEventAndDelete(ev,handle);
		}
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

	SchedEventList* sl = m_queue;
	while (sl->size() > 0) {
		SchedEvent* ev = sl->front();
		nqueue++;
		sl->pop_front();
		// We assume ev is still valid, right?  (since the SchedEventList is
		// a list of pointers)
		ScheduleAddEvent(ev, false);  // we're already locked, so lockit==false
	}
}
	
// SendPmMessage IS BEING PHASED OUT - ONLY ANO STILL USES IT
void NosuchScheduler::SendPmMessage(PmMessage pm, PmStream* ps, void* handle) {

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
void NosuchScheduler::SendMidiMsg(MidiMsg* msg, void* handle) {
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
		if ( (m_LastTimeStamp-lastwarning) > 1000 ) {
			DEBUGPRINT(("SendMidiMsg: no MIDI output device for outputport=%d?", outputport));
			lastwarning = m_LastTimeStamp;
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

void NosuchScheduler::DoEventAndDelete(SchedEvent* e, void* handle) {
	bool deleteit = true;
	if (e->eventtype() == SchedEvent::MIDIMSG ) {
		MidiMsg* m = e->u.midimsg;
		NosuchAssert(m);
		DoMidiMsgEvent(m,handle);  // deletes m
	} else if (e->eventtype() == SchedEvent::MIDIPHRASE ) {
		MidiPhrase* ph = e->u.midiphrase;
		MidiPhraseUnit* pu = ph->first;
		if ( pu ) {
			MidiPhraseUnit* nextpu = pu->next;
			click_t prevclick = pu->click;
			DoMidiMsgEvent(pu->msg,handle);  // deletes msg
			// XXX - put this back!!   delete pu;
			ph->first = nextpu;
			if ( nextpu != NULL ) {
				// Reschedule the event, using the same MidiPhrase
				deleteit = false;
				click_t nextclick = nextpu->click;
				e->click = e->click + (nextclick - prevclick);
				ScheduleAddEvent(e,false);  // we're already locked, so lockit==false
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
NosuchScheduler::DoMidiMsgEvent(MidiMsg* m, void* handle)
{
	NosuchAssert(m);
	SendMidiMsg(m,handle);
	delete m;
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

// SendControllerMsg takes ownership of MidiMsg pointed-to by m.
void
NosuchScheduler::SendControllerMsg(MidiMsg* m, void* handle, bool smooth)
{
	int mc = m->Controller();
	SendMidiMsg(m,handle);
	delete m;

#ifdef NOWPLAYING
	MidiMsg* nowplaying = NULL;
	std::map<int,std::map<int,MidiMsg*>>::iterator it = m_nowplaying_controller.find(sidnum);
	if ( it != m_nowplaying_controller.end() ) {
		DEBUGPRINT1(("SendControllerMsg, found m_nowplaying_controller for sid=%d",sidnum));

		std::map<int,MidiMsg*>& ctrlmap = it->second;
		// look for the controller we're going to put out

		std::map<int,MidiMsg*>::iterator it2 = ctrlmap.find(mc);
		if ( it2 != ctrlmap.end() ) {

			nowplaying = it2->second;
			NosuchAssert(nowplaying);
			NosuchAssert(nowplaying->MidiType() == MIDI_CONTROL);
			NosuchAssert(nowplaying != m );

			ctrlmap.erase(mc);

			// int currctrl = nowplaying->Controller();
			// NosuchAssert(currctrl==1);  // currently, we only handle controller 1 (modulation)

			if ( smooth ) {
				int currval = nowplaying->Value();
				int newval = (currval + m->Value())/2;
				DEBUGPRINT1(("SendControllerMsg, smoothing controller value=%d  new value=%d  avg val=%d\n",currval,m->Value(),newval));
				m->Value(newval);
			} else {
				DEBUGPRINT1(("SendControllerMsg, NO smoothing controller value=%d",m->Value()));
			}
		}
	}
	if ( nowplaying )
		delete nowplaying;
	SendMidiMsg(m,sidnum);
	m_nowplaying_controller[sidnum][mc] = m;
#endif
}

// SendPitchBendMsg takes ownership of MidiMsg pointed-to by m.
void
NosuchScheduler::SendPitchBendMsg(MidiMsg* m, void* handle, bool smooth)
{
	SendMidiMsg(m,handle);
	delete m;
	return;

#ifdef NOWPLAYING
	MidiMsg* nowplaying = NULL;
	std::map<int,MidiMsg*>::iterator it = m_nowplaying_pitchbend.find(sidnum);
	if ( it != m_nowplaying_pitchbend.end() ) {
		DEBUGPRINT1(("SendPitchBendMsg, found m_nowplaying_pitchbend for sid=%d",sidnum));

		nowplaying = it->second;
		NosuchAssert(nowplaying);
		NosuchAssert(nowplaying->MidiType() == MIDI_PITCHBEND);
		NosuchAssert(nowplaying != m );

		m_nowplaying_pitchbend.erase(sidnum);

		if ( smooth ) {
			int currval = nowplaying->Value();
			int newval = (currval + m->Value())/2;
			DEBUGPRINT1(("SendPitchBendMsg, smoothing pitchbend value=%d  new value=%d  avg val=%d\n",currval,m->Value(),newval));
			m->Value(newval);
		} else {
			DEBUGPRINT1(("SendPitchBendMsg, NO smoothing pitchbend value=%d",m->Value()));
		}
	}
	if ( nowplaying )
		delete nowplaying;
	SendMidiMsg(m,sidnum);
	m_nowplaying_pitchbend[sidnum] = m;
#endif
}

std::string
NosuchScheduler::DebugString() {

	std::string s;
	s = "NosuchScheduler (\n";

	std::map<void*,SchedEventList*>::iterator itsid = m_scheduled.begin();
	for (; itsid != m_scheduled.end(); itsid++ ) {
		void* handle = itsid->first;
		SchedEventList* sl = itsid->second;
		NosuchAssert(sl);
		SchedEventIterator it = sl->begin();
		for ( ; it != sl->end(); it++ ) {
			SchedEvent* ep = *it;
			NosuchAssert(ep);
			s += NosuchSnprintf("      Event %s\n",ep->DebugString().c_str());
		}
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
	int timesofar = m_LastTimeStamp - m_timestamp0;
	int clickssofar = (int)(0.5 + timesofar * m_ClicksPerMillisecond);
	m_currentclick = clickssofar;
}

void NosuchScheduler::SetTempoFactor(float f) {
	QuarterNoteClicks = (int)(96 * f);
	DEBUGPRINT(("Setting QuarterNoteClicks to %d",QuarterNoteClicks));
}
