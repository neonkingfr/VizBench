#ifndef _SPACE_SERVER_H
#define _SPACE_SERVER_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VIZDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VIZDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VIZDLL_EXPORTS
#define VIZDLL_API __declspec(dllexport)
#else
#define VIZDLL_API __declspec(dllimport)
#endif

// #include "NosuchDaemon.h"
#include "NosuchJSON.h"
#include "cJSON.h"
#include "NosuchOsc.h"
#include "NosuchMidi.h"
#include "NosuchGraphics.h"
#include "mmtt_sharedmem.h"
// #include "limits.h"
typedef int click_t;

#include <list>
#include <pthread.h>

class MidiMsg;
class VizServerJsonProcessor;
class VizServerOscProcessor;
class VizServerMidiProcessor;
class VizServerCursorProcessor;
class VizServerKeystrokeProcessor;
class NosuchDaemon;
class NosuchScheduler;
class MidiPhrase;
class UT_SharedMem;
class MMTT_SharedMemHeader;
class MidiMsg;
class VizServer;
class Region;
class VizCursor;
struct OutlineMem;

typedef const char* (*jsoncallback_t)(void* data,const char *method, cJSON* json, const char* id);
typedef void (*osccallback_t)(void* data,const char* source, const osc::ReceivedMessage&);
typedef void (*midicallback_t)(void* data,MidiMsg* m);
typedef void (*cursorcallback_t)(void* data,VizCursor* c,int downdragup);
typedef void (*keystrokecallback_t)(void* data,int key,int downup);

#define CURSOR_DOWN 0
#define CURSOR_DRAG 1
#define CURSOR_UP 2

#define KEYSTROKE_DOWN 0
#define KEYSTROKE_UP 1

struct ApiFilter {
public:
	// Don't use std::string here, so memory management between
	// clients and server is clean.
	ApiFilter(const char* p) {
		prefix = _strdup(p);
	}
	ApiFilter() {
		ApiFilter("");
	}
	const char* prefix;
};

class KeystrokeListener {
public:
	virtual void processKeystroke(int key, int downup) {
		DEBUGPRINT(("HEY!  processKeystroke in KeystrokeListener called!?"));
	}
};

class CursorListener {
public:
	virtual void processCursor(VizCursor* c, int downdragup) {
		DEBUGPRINT(("HEY!  processCursor in NosuchCursorProcess called!?"));
	}
};

struct CursorFilter {
public:
	CursorFilter(int mn, int mx) {
		sidmin = mn;
		sidmax = mx;
	}
	CursorFilter() {
		sidmin = INT_MIN;
		sidmax = INT_MAX;
	}
	int sidmin;
	int sidmax;
};

class VizCursor {
public:
	// methods
	VizCursor(VizServer* ss, int sid_, std::string source_,
		NosuchPos pos_, double area_, OutlineMem* om_, MMTT_SharedMemHeader* hdr_);

	void touch(int millinow) {
		last_touched = millinow;
	}
	bool matches(CursorFilter cf) {
		return (sid >= cf.sidmin && sid <= cf.sidmax);
	}
	double depth() {
		return pos.z;
	}
	void setdepth(double d) {
		pos.z = d;
	}
	double target_depth() { return _target_depth; }
	void set_target_depth(double d) { _target_depth = d; }

	NosuchPos previous_musicpos() { return _previous_musicpos; }
	double last_depth() { return _last_depth; }
	void set_previous_musicpos(NosuchPos p) { _previous_musicpos = p; }
	void set_last_depth(double f) { _last_depth = f; }
	void advanceTo(int tm);

	double radian2degree(double r) {
		return r * 360.0 / (2.0 * (double)M_PI);
	}

	std::vector<int>& lastpitches() { return _last_pitches; }
	int lastchannel() { return _last_channel; }
	int lastclick() { return _last_click; }

	void add_last_note(int clk, MidiMsg* m) {
		// NosuchDebug(2,"ADD_LAST_NOTE clk=%d m=%s",clk,m->DebugString().c_str());
		_last_click = clk;
		_last_channel = m->Channel();
		_last_pitches.push_back(m->Pitch());
	}
	int last_pitch() {
		if ( _last_pitches.size() == 0 ) {
			return -1;
		}
		return _last_pitches.front();
	}
	void clear_last_note() {
		NosuchDebug(2,"CLEAR_LAST_NOTE!");
		_last_click = -1;
		_last_channel = -1;
		_last_pitches.clear();
	}

	// members
	NosuchPos pos;
	NosuchPos target_pos;
	VizServer* _vizserver;
	int last_touched;
	int sid;
	std::string source;
	double area;
	OutlineMem* outline;
	MMTT_SharedMemHeader* hdr;
	Region* region;

	double curr_speed;
	double curr_degrees;

private:
	double _target_depth;
	double _last_depth;
	std::vector<int> _last_pitches;
	int _last_channel;
	int _last_click;
	NosuchPos _previous_musicpos;
	int _last_tm;
	NosuchPos _last_pos;
	double _target_degrees;
	bool _g_firstdir;
	double _smooth_degrees_factor;

};

class VIZDLL_API VizServer {
public:
	static VizServer* GetServer();
	static void DeleteServer();

	// Some utilities
	int MilliNow();
	click_t CurrentClick();

	bool Start();
	void Stop();

	const char *VizTags();
	void ChangeVizTag(void* handle, const char* newtag);
	void AdvanceCursorTo(VizCursor* c, int tm);

	void AddJsonCallback(void* handle, ApiFilter af, jsoncallback_t cb, void* data);
	void AddMidiInputCallback(void* handle, MidiFilter mf, midicallback_t cb, void* data);
	void AddMidiOutputCallback(void* handle, MidiFilter mf, midicallback_t cb, void* data);
	void AddCursorCallback(void* handle, CursorFilter cf, cursorcallback_t cb, void* data);
	void AddKeystrokeCallback(void* handle, keystrokecallback_t cb, void* data);

	void RemoveJsonCallback(void* handle);
	void RemoveMidiInputCallback(void* handle);
	void RemoveMidiOutputCallback(void* handle);
	void RemoveCursorCallback(void* handle);
	void RemoveKeystrokeCallback(void* handle);

	void SendMidiMsg(MidiMsg* msg);

	void QueueMidiMsg(MidiMsg* m, click_t clk);
	void QueueMidiPhrase(MidiPhrase* ph, click_t clk);
	void QueueClear();

	void SetClicksPerSecond(int clicks);
	int GetClicksPerSecond();
	void ANO(int ch = -1);
	void SetTempoFactor(float f);
	void SetGlobalPitchOffset(int offset) {
		GlobalPitchOffset = offset;
	}

	void IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle);
	void IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle);
	void SendControllerMsg(MidiMsg* m, void* handle, bool smooth);
	void SendPitchBendMsg(MidiMsg* m, void* handle, bool smooth);

	void InsertKeystroke(int key, int downup);

	bool Started() { return _started; }
	int NumCallbacks();
	int MaxCallbacks() { return _maxcallbacks; }

	void LockNotesDown() {
		_scheduler->LockNotesDown();
	}
	std::list<MidiMsg*>& NotesDown() {
		return _scheduler->NotesDown();
	}
	void ClearNotesDown() {
		return _scheduler->ClearNotesDown();
	}
	void UnlockNotesDown() {
		_scheduler->UnlockNotesDown();
	}

	int TryLockCursors() {
		return NosuchTryLock(&_cursors_mutex,"cursors");
	}
	void LockCursors() {
		NosuchLock(&_cursors_mutex,"cursors");
	}
	void UnlockCursors() {
		NosuchUnlock(&_cursors_mutex,"cursors");
	}

	int FrameSeq() { return _frameseq; }

private:
	
	friend class VizServerOscProcessor;	// so it can access _touchCursorSid, _checkCursorUp, _setCursor
	friend class VizServerClickProcessor;	// so it can access _advanceClickTo
	friend class VizServerJsonProcessor;

	VizServer();
	virtual ~VizServer();

	void _setMidiFile(const char* file) { _midifile = file; }
	const char* _getMidiFile() { return _midifile; }

	void _processServerConfig(cJSON* json);
	cJSON* _readconfig(const char* fname);
	void _setMaxCallbacks();

	std::string _processJson(std::string fullmethod,cJSON* params,const char* id);
	void _advanceClickTo(int current_click, NosuchScheduler* sched);
	void _checkSharedMem();

	void _openSharedMemOutlines();
	void _closeSharedMemOutlines();
	UT_SharedMem* _getSharedMemOutlines() { return _sharedmem_outlines; }

	void _checkCursorUp();
	VizCursor* _getCursor(int sidnum, std::string sidsource, bool lockit);
	void _touchCursorSid(int sidnum, std::string sidsource);
	void _setCursor(int sidnum, std::string sidsource, NosuchPos pos, double area, OutlineMem* om, MMTT_SharedMemHeader* hdr);
	void _setCursorSid(int sidnum, const char* source, double x, double y, double z, double tuio_f, OutlineMem* om, MMTT_SharedMemHeader* hdr );
	void _processCursorsFromBuff(MMTT_SharedMemHeader* hdr);

	void ScheduleMidiMsg(MidiMsg* m, click_t clk, void* handle);
	void ScheduleMidiPhrase(MidiPhrase* ph, click_t clk, void* handle);
	void ScheduleClear();

	static void ErrorPopup(const char* msg);

	bool _started;

	UT_SharedMem* _sharedmem_outlines;
	long _sharedmem_last_attempt;

	// These things are pulled from the config file
	const char* _midi_input_name;
	const char* _midi_output_name;
	bool _do_midimerge;
	bool _do_sharedmem;
	const char *_sharedmemname;
	bool _do_errorpopup;
	bool _do_ano;
	const char* _midifile;

	int _osc_input_port;
	const char * _osc_input_host;

	int _maxcallbacks;
	int _frameseq;

	pthread_mutex_t _cursors_mutex;
	std::list<VizCursor*>* _cursors;

	int _httpport;
	const char* _htmldir;
	NosuchDaemon* _daemon;

	// Processors are Listeners that broadcast things to Callbacks
	VizServerJsonProcessor* _jsonprocessor;
	VizServerOscProcessor* _oscprocessor;
	VizServerMidiProcessor* _midiinputprocessor;
	VizServerMidiProcessor* _midioutputprocessor;
	VizServerCursorProcessor* _cursorprocessor;
	VizServerKeystrokeProcessor* _keystrokeprocessor;
	NosuchScheduler* _scheduler;
	NosuchClickListener* _clickprocessor;

};

#endif
