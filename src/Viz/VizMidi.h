#ifndef NOSUCHMIDI_H
#define NOSUCHMIDI_H

#include "portmidi.h"
#include "VizScheduler.h"
#include "VizException.h"

#define MAX_MIDI_PORTS 64

#define MIDI_CLOCK      0xf8
#define MIDI_ACTIVE     0xfe
#define MIDI_STATUS_MASK 0x80
#define MIDI_SYSEX      0xf0
#define MIDI_EOX        0xf7
#define MIDI_START      0xFA
#define MIDI_STOP       0xFC
#define MIDI_CONTINUE   0xFB
#define MIDI_F9         0xF9
#define MIDI_FD         0xFD
#define MIDI_RESET      0xFF
#define MIDI_NOTE_ON    0x90
#define MIDI_NOTE_OFF   0x80
#define MIDI_CHANNEL_AT 0xD0
#define MIDI_POLY_AT    0xA0
#define MIDI_PROGRAM    0xC0
#define MIDI_CONTROL    0xB0
#define MIDI_PITCHBEND  0xE0
#define MIDI_MTC        0xF1
#define MIDI_SONGPOS    0xF2
#define MIDI_SONGSEL    0xF3
#define MIDI_TUNE       0xF6

#define MIDI_PORT_OF_GENERATED_STUFF (-2)

// Other defines that are like MIDI_* ,
// but the values are arbitrary, not status nibbles.
#define MIDI_ALL	0x100
#define MIDI_NOTE	0x101

extern int QuarterNoteClicks;
extern bool VizDebugMidiAll;
extern bool VizDebugMidiNotes;
// #define QuarterNoteClicks 96

typedef long MidiTimestamp;

#define LIMIT_0_TO_127(v) if(v<0)v=0;else if (v>127)v=127

#define NO_VALUE -1

extern char* ReadableMidiPitch(int pitch);
extern int BoundValue(int v, int minval, int maxval);
extern const char* MidiType2Name(int x);

class MidiFilter {
public:
	MidiFilter() {
		msgtype = MIDI_ALL;
		chan = 0;
	}
	MidiFilter(int mt, int ch) {
		msgtype = mt;
		chan = ch;
	}

	int msgtype;
	int chan;  // If 0, match all channels
};

class MidiMsg {
public:
	MidiMsg() {
		m_inport = -1;
		m_outport = -1;
		DEBUGPRINT1(("MidiMsg constructor!  this=%ld",(long)this));
	}
	virtual ~MidiMsg() {
		DEBUGPRINT1(("MidiMsg destructor!  this=%ld",(long)this));
	}
	virtual PmMessage PortMidiMessage() = 0;
	virtual PmMessage PortMidiMessageOff() { return 0; }
	virtual int MidiType() { return -1; }
	virtual int Channel() { return -1; }
	virtual int Pitch() { return -1; }
	virtual int Pitch(int np) { return -1; }
	virtual int Velocity() { return -1; }
	virtual int Controller() { return -1; }
	virtual int Value(int val = NO_VALUE) { return NO_VALUE; }
	int InputPort() { return m_inport; }
	int OutputPort() { return m_outport; }
	MidiMsg* SetInputPort(int n) { m_inport = n; return this; }
	MidiMsg* SetOutputPort(int n) { m_outport = n; return this; }
	const char* MidiTypeName() { return MidiType2Name(MidiType()); }
	bool isSameAs(MidiMsg* m) {
		VizAssert(m);
		switch (MidiType()) {
		case MIDI_NOTE_ON:
		case MIDI_NOTE_OFF:
			if ( MidiType() == m->MidiType()
				&& Channel() == m->Channel()
				&& Pitch() == m->Pitch()
				&& Velocity() == m->Velocity() )
				return true;
			break;
		default:
			DEBUGPRINT(("MidiMsg::isSameAs not implemented for MidiType=%d",MidiType()));
			break;
		}
		return false;
	}
	virtual MidiMsg* clone() {
		DEBUGPRINT(("Unable to clone MidiMsg of type %d, returning NULL",MidiType()));
		return NULL;
	}

	static MidiMsg* make(int msg);

	bool matches(MidiFilter mf) {
		if ( Channel() > 0 ) {
			// Channel message
			if (mf.chan == 0 || mf.chan == Channel() ) {
				return true;
			}
		} else {
			// non-Channel message
		}
		return false;
	}
protected:
	MidiMsg* finishclone(MidiMsg* m) {
		m->m_inport = m_inport;
		m->m_outport = m_outport;
		return m;
	}
	friend class VizScheduler;
	friend class MidiPhrase;
	virtual std::string DebugString() = 0;
	int m_inport;   // MIDI input port
	int m_outport;  // MIDI output port
};


class ChanMsg : public MidiMsg {
public:
	ChanMsg(int ch) : MidiMsg() {
		VizAssert(ch>=1 && ch<=16);
		DEBUGPRINT2(("ChanMsg constructor"));
		m_chan = ch;
	}
	virtual ~ChanMsg() {
		DEBUGPRINT2(("ChanMsg destructor"));
	}
	virtual PmMessage PortMidiMessage() = 0;
	virtual PmMessage PortMidiMessageOff() { return 0; }
	virtual int MidiType() { return -1; }
	virtual int Pitch() { return -1; }
	virtual int Pitch(int np) { return -1; }
	virtual int Velocity() { return -1; }
	virtual int Controller() { return -1; }
	virtual int Value(int v = NO_VALUE) { return NO_VALUE; }
	int Channel() { return m_chan; }
protected:
	int m_chan;   // 1-based
	friend class VizScheduler;
	friend class MidiPhrase;
	virtual std::string DebugString() = 0;
};

class MidiNoteOff : public ChanMsg {
public:
	static MidiNoteOff* make(int ch, int p, int v) {
		MidiNoteOff* m = new MidiNoteOff(ch,p,v);
		DEBUGPRINT2(("MidiNoteOff::make m=%d",m));
		return m;
	};
	PmMessage PortMidiMessage() {
		return Pm_Message(0x80 | (m_chan-1), m_pitch, m_velocity);
	}
	int MidiType() { return MIDI_NOTE_OFF; }
	int Pitch() { return m_pitch; }
	int Pitch(int np) { m_pitch = np;  return m_pitch; }
	int Velocity() { return m_velocity; }
	MidiNoteOff* clone() {
		MidiNoteOff* newm = MidiNoteOff::make(Channel(),Pitch(),Velocity());
		finishclone(newm);
		return newm;
	};
protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return NoteString();
	}
	std::string NoteString() {
		return VizSnprintf("-p%dc%dv%d",m_pitch,m_chan,m_velocity);
	}
private:
	MidiNoteOff(int ch, int p, int v) : ChanMsg(ch) {
		m_pitch = p;
		m_velocity = v;
	};
	int m_pitch;
	int m_velocity;
};

class MidiNoteOn : public ChanMsg {
public:
	static MidiNoteOn* make(int ch, int p, int v) {
		MidiNoteOn* m = new MidiNoteOn(ch,p,v);
		DEBUGPRINT2(("MidiNoteOn::make m=%d",m));
		return m;
	}
	MidiNoteOff* makenoteoff() {
		MidiNoteOff* newm = MidiNoteOff::make(Channel(),Pitch(),Velocity());
		return newm;
	}
	MidiNoteOn* clone() {
		MidiNoteOn* newm = MidiNoteOn::make(Channel(),Pitch(),Velocity());
		finishclone(newm);
		return newm;
	};
	PmMessage PortMidiMessage() {
		return Pm_Message(0x90 | (m_chan-1), m_pitch, m_velocity);
	}
	PmMessage PortMidiMessageOff() {
		return Pm_Message(0x80 | (m_chan-1), m_pitch, 0);
	}
	int MidiType() { return MIDI_NOTE_ON; }
	int Pitch() { return m_pitch; }
	int Pitch(int np) { m_pitch = np;  return m_pitch; }
	int Velocity() { return m_velocity; }
	int Velocity(int v) { m_velocity = v; return m_velocity; }

protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return NoteString();
	}
	std::string NoteString() {
		return VizSnprintf("+p%dc%dv%d",m_pitch,m_chan,m_velocity);
	}

private:
	MidiNoteOn(int ch, int p, int v) : ChanMsg(ch) {
		DEBUGPRINT2(("MidiNoteOn constructor"));
		m_pitch = p;
		m_velocity = v;
	};
	int m_pitch;
	int m_velocity;
};

class MidiController : public ChanMsg {
public:
	static MidiController* make(int ch, int ctrl, int v) {
		MidiController* m = new MidiController(ch,ctrl,v);
		return m;
	};
	PmMessage PortMidiMessage() {
		return Pm_Message(0xb0 | (m_chan-1), m_controller, m_value);
	}
	int MidiType() { return MIDI_CONTROL; }
	int Controller() { return m_controller; }
	int Value(int v = NO_VALUE) {
		if ( v >= 0 ) {
			VizAssert(v <= 127);
			m_value = v;
		}
		return m_value;
	}
	MidiController* clone() {
		MidiController* newm = MidiController::make(Channel(),Controller(),Value());
		finishclone(newm);
		return newm;
	};
protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return VizSnprintf("Controller ch=%d ct=%d v=%d",m_chan,m_controller,m_value);
	}
private:
	MidiController(int ch, int ctrl, int v) : ChanMsg(ch) {
		m_controller = ctrl;
		m_value = v;
	};
	int m_controller;
	int m_value;
};

class MidiChanPressure : public ChanMsg {
public:
	static MidiChanPressure* make(int ch, int v) {
		MidiChanPressure* m = new MidiChanPressure(ch,v);
		return m;
	};
	PmMessage PortMidiMessage() {
		return Pm_Message(0xb0 | (m_chan-1), m_value, 0);
	}
	int MidiType() { return MIDI_CHANNEL_AT; }
	int Value(int v = NO_VALUE) {
		if ( v >= 0 ) {
			VizAssert(v <= 127);
			m_value = v;
		}
		return m_value;
	}
	MidiChanPressure* clone() {
		MidiChanPressure* newm = MidiChanPressure::make(Channel(),Value());
		finishclone(newm);
		return newm;
	};

protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return VizSnprintf("ChanPressure ch=%d v=%d",m_chan,m_value);
	}

private:
	MidiChanPressure(int ch, int v) : ChanMsg(ch) {
		m_value = v;
	};
	int m_value;
};

class MidiPressure : public ChanMsg {
public:
	static MidiPressure* make(int ch, int pitch, int v) {
		MidiPressure* m = new MidiPressure(ch,pitch,v);
		return m;
	};
	PmMessage PortMidiMessage() {
		return Pm_Message(0xb0 | (m_chan-1), m_pitch, m_value);
	}
	int MidiType() { return MIDI_POLY_AT; }
	int Pitch() { return m_pitch; }
	int Pitch(int np) { m_pitch = np;  return m_pitch; }
	int Value(int v = NO_VALUE) {
		if ( v >= 0 ) {
			VizAssert(v <= 127);
			m_value = v;
		}
		return m_value;
	}
	MidiPressure* clone() {
		MidiPressure* newm = MidiPressure::make(Channel(),Pitch(),Value());
		finishclone(newm);
		return newm;
	};
protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return VizSnprintf("Pressure ch=%d pitch=%d v=%d",m_chan,m_pitch,m_value);
	}
private:
	MidiPressure(int ch, int p, int v) : ChanMsg(ch) {
		m_pitch = p;
		m_value = v;
	};
	int m_pitch;
	int m_value;
};

class MidiProgramChange : public ChanMsg {
public:
	static MidiProgramChange* make(int ch, int v) {
		MidiProgramChange* m = new MidiProgramChange(ch,v);
		DEBUGPRINT2(("MidiProgramChange::make m=%d",m));
		return m;
	};
	PmMessage PortMidiMessage() {
		// Both channel and value going out are 0-based
		return Pm_Message(0xc0 | (m_chan-1), m_value-1, 0);
	}
	int MidiType() { return MIDI_PROGRAM; }
	int Value(int v = NO_VALUE) {
		if ( v > 0 ) {
			VizAssert(v<=128);  // program change value is 1-based
			m_value = v;
		}
		return m_value;
	}
	MidiProgramChange* clone() {
		MidiProgramChange* newm = MidiProgramChange::make(Channel(),Value());
		finishclone(newm);
		return newm;
	};
protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return VizSnprintf("ProgramChange ch=%d v=%d",m_chan,m_value);
	}
private:
	MidiProgramChange(int ch, int v) : ChanMsg(ch) {
		VizAssert(v>0);  // program change value is 1-based
		m_value = v;
	};
	int m_value;   // 1-based
};

class MidiPitchBend : public ChanMsg {
public:
	static MidiPitchBend* make(int ch, int v) {
		// The v coming in is expected to be 0 to 16383, inclusive
		MidiPitchBend* m = new MidiPitchBend(ch,v);
		return m;
	};
	PmMessage PortMidiMessage() {

// The two bytes of the pitch bend message form a 14 bit number, 0 to 16383.
// The value 8192 (sent, LSB first, as 0x00 0x40), is centered, or "no pitch bend."
// The value 0 (0x00 0x00) means, "bend as low as possible,"
// and, similarly, 16383 (0x7F 0x7F) is to "bend as high as possible."

		VizAssert(m_value >= 0 && m_value <= 16383);
		return Pm_Message(0xe0 | (m_chan-1), m_value & 0x7f, (m_value>>7) & 0x7f);
	}
	int MidiType() { return MIDI_PITCHBEND; }
	int Value(int v = NO_VALUE) {
		if ( v >= 0 ) {
			VizAssert(v >= 0 && v <= 16383);
			m_value = v;
		}
		return m_value;
	}
	MidiPitchBend* clone() {
		MidiPitchBend* newm = MidiPitchBend::make(Channel(),Value());
		finishclone(newm);
		return newm;
	};
protected:
	friend class VizScheduler;
	friend class MidiPhrase;
	std::string DebugString() {
		return VizSnprintf("PitchBend ch=%d v=%d",m_chan,m_value);
	}
private:
	MidiPitchBend(int ch, int v) : ChanMsg(ch) {
		VizAssert(v >= 0 && v <= 16383);
		Value(v);
	};
	int m_value;   // from 0 to 16383
};

class MidiPhraseUnit {
public:
	MidiPhraseUnit(MidiMsg* m, click_t c) {
		click = c;
		msg = m;
		next = NULL;
	}
	virtual ~MidiPhraseUnit() {
		delete msg;
	}
	click_t click;  // relative to start of phrase
	MidiMsg* msg;
	MidiPhraseUnit* next;
};

class MidiPhrase {
public:
	MidiPhrase() {
		first = NULL;
		last = NULL;
	}
	virtual ~MidiPhrase() {
		MidiPhraseUnit* nextpu;
		for ( MidiPhraseUnit* pu=first; pu!=NULL; pu=nextpu ) {
			nextpu = pu->next;
			delete pu;
		}
		first = NULL;
		last = NULL;
	}
	void SetInputPort(int port) {
		for (MidiPhraseUnit* pu = first; pu != NULL; pu = pu->next) {
			pu->msg->SetInputPort(port);
		}
	}
	void SetOutputPort(int port) {
		for (MidiPhraseUnit* pu = first; pu != NULL; pu = pu->next) {
			pu->msg->SetOutputPort(port);
		}
	}
	void insert(MidiMsg* msg, click_t click);  // takes ownership

	friend class VizScheduler;
	std::string DebugString() {
		std::string s = VizSnprintf("MidiPhrase(");
		std::string sep = "";
		for ( MidiPhraseUnit* pu=first; pu!=NULL; pu=pu->next ) {
			s += VizSnprintf("%s%st%d",sep.c_str(),pu->msg->DebugString().c_str(),pu->click);
			sep = ",";
		}
		s += ")";
		return s;
	}

#ifdef TOO_EXPENSIVE
	std::string SummaryString() {
		int tot[17] = {0};
		for ( MidiPhraseUnit* pu=first; pu!=NULL; pu=pu->next ) {
			int ch = pu->msg->Channel();
			if ( ch == 0 ) {
				DEBUGPRINT(("Hey!  Channel() returned 0!?"));
			} else if ( ch < 0 ) {
				tot[0]++;
			} else {
				tot[ch]++;
			}
		}
		std::string s = VizSnprintf("MidiPhrase(");
		std::string sep = "";
		for ( int ch=1; ch<17; ch++ ) {
			if ( tot[ch] > 0 ) {
				s += VizSnprintf("%s%d in ch#%d",sep.c_str(),tot[ch],ch);
				sep = ", ";
			}
		}
		if ( tot[0] > 0 ) {
			s += VizSnprintf("%s%d non-channel",sep.c_str(),tot[0]);
		}
		s += ")";
		return s;
	}
#endif

	MidiPhraseUnit* first;
	MidiPhraseUnit* last;
	// eventually put phrase length here
};

MidiPhrase* newMidiPhraseFromFile(std::string filename);

#endif