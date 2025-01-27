#include "VizUtil.h"
#include "VizException.h"
#include "VizMidi.h"
#include "midifile.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

static char *ReadableMidiPitches[128];
static char *ReadableCanonic[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static bool ReadableNotesInitialized = false;

bool VizDebugMidiAll = false;
bool VizDebugMidiNotes = false;

int QuarterNoteClicks = 96;

char* ReadableMidiPitch(int p) {
	if ( ! ReadableNotesInitialized ) {
		for ( int n=0; n<128; n++ ) {
			int canonic = n % 12;
			int octave = (n / 12) - 2;
			std::string s = VizSnprintf("%s%d",ReadableCanonic[canonic],octave);
			ReadableMidiPitches[n] = _strdup(s.c_str());
		}
		ReadableNotesInitialized = true;
	}
	return ReadableMidiPitches[p];
}

// Should this be inline?
int
BoundValue(int v, int minval, int maxval) {
	if (v < minval) {
		v = minval;
	}
	else if (v > maxval) {
		v = maxval;
	}
	return v;
}

MidiMsg* MidiMsg::make(int msg) {
	int status = Pm_MessageStatus(msg);
	int command = status & 0xf0;
	int chan = 1 + (status & 0x0f);
	int byte1 = Pm_MessageData1(msg);
	int byte2 = Pm_MessageData2(msg);
	switch (command) {
	case MIDI_NOTE_ON:
		return MidiNoteOn::make(chan,byte1,byte2);
	case MIDI_NOTE_OFF:
		return MidiNoteOff::make(chan,byte1,byte2);
	case MIDI_CONTROL:
		return MidiController::make(chan,byte1,byte2);
	case MIDI_PROGRAM:
		return MidiProgramChange::make(chan,byte1);
	case MIDI_CHANNEL_AT:
		return MidiChanPressure::make(chan, byte1);
	case MIDI_POLY_AT:
		return MidiPressure::make(chan, byte1, byte2);
	case MIDI_PITCHBEND: {
		int v = byte1 | (byte2<<7);		// 7 bits from lsb, 7 bits from msb
		return MidiPitchBend::make(chan,v);
		}
	default:
		DEBUGPRINT(("Unhandled command in MidiMsg::make!  cmd=0x%02x",command));
		return NULL;
	}
}

static MidiPhrase* MidiFilePhrase = NULL;
static std::ifstream Mf_input_stream;

int my_Mf_insert(MidiMsg* m) {
	MidiFilePhrase->insert(m,Mf_currtime);
	return 0;
}

int my_Mf_noteon(int ch,int p, int v) {
	DEBUGPRINT1(("my_Mf_noteon was called, p=%d v=%d",p,v));
	ch++; // In MidiMsg, channels are 1-16
	if ( v == 0 ) {
		return my_Mf_insert(MidiNoteOff::make(ch,p,v));
	} else {
		return my_Mf_insert(MidiNoteOn::make(ch,p,v));
	}
}

int my_Mf_noteoff(int ch, int p, int v) {
	DEBUGPRINT1(("my_Mf_noteoff was called, p=%d v=%d",p,v));
	ch++; // In MidiMsg, channels are 1-16
	return my_Mf_insert(MidiNoteOff::make(ch,p,v));
}

int my_Mf_pressure(int ch, int p, int v) {
	DEBUGPRINT1(("my_Mf_pressure was called, p=%d v=%d",p,v));
	ch++; // In MidiMsg, channels are 1-16
	return my_Mf_insert(MidiPressure::make(ch,p,v));
}

int my_Mf_controller(int ch, int p, int v) {
	DEBUGPRINT1(("my_Mf_controller was called, p=%d v=%d",p,v));
	ch++; // In MidiMsg, channels are 1-16
	return my_Mf_insert(MidiController::make(ch,p,v));
}

int my_Mf_pitchbend(int ch, int lsb, int msb) {
	ch++; // In MidiMsg, channels are 1-16
	int v = lsb | (msb<<7);		// 7 bits from lsb, 7 bits from msb
	DEBUGPRINT1(("my_Mf_pitchbend was called, v=%d",v));
	return my_Mf_insert(MidiPitchBend::make(ch,v));
}

int my_Mf_program(int ch, int p) {
	ch++; // In MidiMsg, channels are 1-16
	p++;  // In MidiProgramChange, program values are 1-128
	DEBUGPRINT1(("my_Mf_program was called, p=%d",p));
	return my_Mf_insert(MidiProgramChange::make(ch,p));
}

int my_Mf_chanpressure(int ch, int p) {
	ch++; // In MidiMsg, channels are 1-16
	DEBUGPRINT1(("my_Mf_chanpressure was called, p=%d",p));
	return my_Mf_insert(MidiChanPressure::make(ch,p));
}

int my_Mf_sysex(int msgleng,char* p) {
	DEBUGPRINT1(("my_Mf_sysex was called, msgleng=%d",msgleng));
	return 0;
}

int my_Mf_getc() {
	int c = Mf_input_stream.get();
	if ( Mf_input_stream.eof() ) {
		return EOF;
	} else {
		return c;
	}
}

MidiPhrase*
newMidiPhraseFromFile(std::string fname) {

	// XXX - should really lock something here, since it uses globals
	Mf_noteon = my_Mf_noteon;
	Mf_noteoff = my_Mf_noteoff;
	Mf_getc = my_Mf_getc;
	Mf_pressure = my_Mf_pressure;
	Mf_controller = my_Mf_controller;
	Mf_pitchbend = my_Mf_pitchbend;
	Mf_program = my_Mf_program;
	Mf_chanpressure = my_Mf_chanpressure;
	Mf_sysex = my_Mf_sysex;

	std::string err = "";

	Mf_input_stream.open(fname.c_str(),std::ios::binary);
	if ( ! Mf_input_stream.good() ) {
		return NULL;
	}
	MidiFilePhrase = new MidiPhrase();
	err = mfread();
	Mf_input_stream.close();
#if 0
	DEBUGPRINT(("MidiPhraseFromFile = %s",
	 	MidiFilePhrase->DebugString().c_str()));
	DEBUGPRINT(("MidiPhraseFromFile = %s",
		MidiFilePhrase->SummaryString().c_str()));
#endif
	if ( err == "" ) {
		return MidiFilePhrase;	// caller is responsible for deleting
	} else {
		DEBUGPRINT(("Error in reading midifile: %s",err.c_str()));
		delete MidiFilePhrase;
		MidiFilePhrase = NULL;
		return NULL;
	}
}

void MidiPhrase::insert(MidiMsg* msg, click_t click) {  // takes ownership
	MidiPhraseUnit* pu = new MidiPhraseUnit(msg,click);
	if ( first == NULL ) {
		first = last = pu;
	} else if ( click >= last->click ) {
		// quick append to end of phrase
		last->next = pu;
		last = pu;
	} else {
		MidiPhraseUnit* pos = first;
		MidiPhraseUnit* prev = NULL;
		while ( pos != NULL && click >= pos->click ) {
			prev = pos;
			pos = pos->next;
		}
		if ( pos == NULL ) {
			// It's at the end
			last->next = pu;
			last = pu;
		} else if ( prev == NULL ) {
			// It's at the beginning
			pu->next = first;
			first = pu;
		} else {
			pu->next = pos;
			prev->next = pu;
		}
	}
}

const char* MidiType2Name(int x) {
	if ( x == MIDI_CLOCK) { return "CLOCK"; }
	if ( x == MIDI_ACTIVE) { return "ACTIVE"; }
	if ( x == MIDI_SYSEX) { return "SYSEX"; }
	if ( x == MIDI_EOX) { return "EOX"; }
	if ( x == MIDI_START) { return "START"; }
	if ( x == MIDI_STOP) { return "STOP"; }
	if ( x == MIDI_CONTINUE) { return "CONTINUE"; }
	if ( x == MIDI_F9) { return "F9"; }
	if ( x == MIDI_FD) { return "FD"; }
	if ( x == MIDI_RESET) { return "RESET"; }
	if ( x == MIDI_NOTE_ON) { return "NOTE_ON"; }
	if ( x == MIDI_NOTE_OFF) { return "NOTE_OFF"; }
	if ( x == MIDI_CHANNEL_AT) { return "CHANNEL_AT"; }
	if ( x == MIDI_POLY_AT) { return "POLY_AT"; }
	if ( x == MIDI_PROGRAM) { return "PROGRAM"; }
	if ( x == MIDI_CONTROL) { return "CONTROL"; }
	if ( x == MIDI_PITCHBEND) { return "PITCHBEND"; }
	if ( x == MIDI_MTC) { return "MTC"; }
	if ( x == MIDI_SONGPOS) { return "SONGPOS"; }
	if ( x == MIDI_SONGSEL) { return "SONGSEL"; }
	if ( x == MIDI_TUNE) { return "TUNE"; }
	return "UNKNOWN";
}

