/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "NosuchUtil.h"
#include <VizServer.h>
#include <NosuchUtil.h>
#include <NosuchJSON.h>
#include <NosuchException.h>
#include <NosuchOscManager.h>
#include <NosuchHttpServer.h>
#include <NosuchScheduler.h>
#include "UT_SharedMem.h"

#include <set>
#include <map>

using namespace std;

/* static */ VizServer* OneServer = NULL;
/* static */ int ServerCount = 0;

class VizServerClickProcessor : public NosuchClickListener {
public:
	VizServerClickProcessor(VizServer* ss) {
		_server = ss;
	}
	void AdvanceClickTo(int current_click, NosuchScheduler* sched) {
		_server->_advanceClickTo(current_click,sched);
	}
private:
	VizServer* _server;
};

bool ApiFilterMatch(ApiFilter af, std::string method) {
	std::string methprefix = method.substr(0,strlen(af.prefix));
	return (strcmp(methprefix.c_str(),af.prefix)==0);
}

class VizServerApiCallback {
public:
	VizServerApiCallback(ApiFilter af, void* cb, void* data) {
		_cb = cb;
		_data = data;
		_af = af;
	}
	void* _cb;
	void* _data;
	ApiFilter _af;
};

class VizServerCursorCallback {
public:
	VizServerCursorCallback(CursorFilter cf, void* cb, void* data) {
		_cb = cb;
		_data = data;
		_cf = cf;
	}
	void* _cb;
	void* _data;
	CursorFilter _cf;
};

class VizServerKeystrokeCallback {
public:
	VizServerKeystrokeCallback(void* cb, void* data) {
		_cb = cb;
		_data = data;
	}
	void* _cb;
	void* _data;
};

class VizServerMidiCallback {
public:
	VizServerMidiCallback(MidiFilter mf, void* cb, void* data) {
		_cb = cb;
		_data = data;
		_mf = mf;
	}
	void* _cb;
	void* _data;
	MidiFilter _mf;
};

class VizServerApiCallbackMap : public std::map<void*,VizServerApiCallback*> {
public:
	VizServerApiCallbackMap() {
	}
	void addcallback(void* handle, ApiFilter af, void* cb, void* data) {
		DEBUGPRINT1(("VizServerApiCallbackMap addcallback handle=%ld prefix=%s",(long)handle,af.prefix));
		if ( count(handle) > 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::addcallback finds existing callback for handle=%ld",(long)handle));
			return;
		}
		VizServerApiCallback* sscb = new VizServerApiCallback(af,cb,data);
		(*this)[handle] = sscb;
		DEBUGPRINT1(("VizServerApiCallbackMap inserted handle=%ld size=%d",(long)handle,size()));
	}
	void removecallback(void* handle) {
		if ( count(handle) == 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%ld",(long)handle));
			return;
		}
		erase(handle);
	}
	VizServerApiCallback* findprefix(std::string prefix) {
		std::map<void*,VizServerApiCallback*>::iterator it;
		for ( it=this->begin(); it!=this->end(); it++ ) {
			VizServerApiCallback* sscb = it->second;
			if ( sscb->_af.prefix == prefix ) {
				return sscb;
			}
		}
		return NULL;
	}
	const char* VizTags() {
		static std::string s;

		s = "VizServer";
		std::map<void*,VizServerApiCallback*>::iterator it;
		for ( it=this->begin(); it!=this->end(); it++ ) {
			VizServerApiCallback* sscb = it->second;
			std::string tag = std::string(sscb->_af.prefix);
			if ( tag != "UNTAGGED" && tag != "ffff" ) {
				s += (","+tag);
			}
		}
		return s.c_str();
	}

	void ChangeVizTag(void* handle, const char* p) {
		if ( count(handle) <= 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::ChangeVizTag didn't find existing callback for handle=%ld",(long)handle));
			return;
		}
		VizServerApiCallback* sscb = (*this)[handle];
		// Should we free prefix?  I tried freeing it in the destructor
		// of ApiFilter, and it corrupted the heap, so I'm leary.
		sscb->_af.prefix = _strdup(p);
	}
};

class VizServerCursorCallbackMap : public std::map<void*,VizServerCursorCallback*> {
public:
	VizServerCursorCallbackMap() {
	}
	void addcallback(void* handle, CursorFilter cf, void* cb, void* data) {
		if ( count(handle) > 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::addcallback finds existing callback for handle=%ld",(long)handle));
			return;
		}
		VizServerCursorCallback* sscb = new VizServerCursorCallback(cf,cb,data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if ( count(handle) == 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%ld",(long)handle));
			return;
		}
		erase(handle);
	}
};

class VizServerKeystrokeCallbackMap : public std::map<void*,VizServerKeystrokeCallback*> {
public:
	VizServerKeystrokeCallbackMap() {
	}
	void addcallback(void* handle, void* cb, void* data) {
		if ( count(handle) > 0 ) {
			DEBUGPRINT(("Hey! VizServerKeystrokeCallbackMap::addcallback finds existing callback for handle=%ld",(long)handle));
			return;
		}
		VizServerKeystrokeCallback* sscb = new VizServerKeystrokeCallback(cb,data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if ( count(handle) == 0 ) {
			DEBUGPRINT(("Hey! VizServerKeystrokeCallbackMap::removecallback didn't find existing callback for handle=%ld",(long)handle));
			return;
		}
		erase(handle);
	}
};

class VizServerMidiCallbackMap : public std::map<void*,VizServerMidiCallback*> {
public:
	VizServerMidiCallbackMap() {
	}
	void addcallback(void* handle, MidiFilter mf, void* cb, void* data) {
		if ( count(handle) > 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::addcallback finds existing callback for handle=%d",(long)handle));
			return;
		}
		VizServerMidiCallback* sscb = new VizServerMidiCallback(mf,cb,data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if ( count(handle) == 0 ) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%d",(long)handle));
			return;
		}
		erase(handle);
	}
};

//////////// VizServerJsonProcessor

class VizServerJsonProcessor : public NosuchJsonListener {

public:
	VizServerJsonProcessor() {
	}
	void AddJsonCallback(void* handle, ApiFilter af, jsoncallback_t cb, void* data) {
		_callbacks.addcallback(handle,af,(void*)cb,data);
	}
	void ChangeVizTag(void* handle, const char* p) {
		_callbacks.ChangeVizTag(handle,p);
	}
	void RemoveJsonCallback(void* handle) {
		_callbacks.removecallback(handle);
		DEBUGPRINT(("RemoveJsonCallback, _callbacks.size=%d",_callbacks.size()));
	}
	virtual std::string processJson(std::string method, cJSON* params, const char *id);
	int numCallbacks() { return _callbacks.size(); }
	const char* VizTags() { return _callbacks.VizTags(); }

private:
	VizServerApiCallbackMap _callbacks;
};

static void
methodPrefixProcess(std::string method, std::string& prefix, std::string& suffix) {
	size_t dotpos = method.find_first_of('.');
	bool hasdot = (dotpos != string::npos);
	if ( hasdot ) {
		prefix = method.substr(0,dotpos);
		suffix = method.substr(dotpos+1);
	} else {
		prefix = ""; 
		suffix = method;
	}
}

std::string
VizServerJsonProcessor::processJson(std::string fullmethod, cJSON *params, const char* id) {

	std::string prefix;
	std::string api;
	VizServer* ss = VizServer::GetServer();

	if ( fullmethod == "viztags" || fullmethod == "apitags" ) {
		const char *p = ss->VizTags();
		return jsonStringResult(std::string(p),id);
	}
#if 0
	{ static _CrtMemState s1, s2, s3;

		if ( fullmethod == "dump" ) {
			_CrtMemState s;
			_CrtMemCheckpoint( &s );
			if ( _CrtMemDifference( &s3, &s1, &s) ) {
			   _CrtMemDumpStatistics( &s3 );
			}
			_CrtDumpMemoryLeaks();
			return jsonOK(id);
		}
		if ( fullmethod == "checkpoint" ) {
			_CrtMemState s1;
			_CrtMemCheckpoint( &s1 );
			return jsonOK(id);
		}
	}
#endif

	methodPrefixProcess(fullmethod, prefix, api);

	if (prefix == "VizServer") {
		if (api == "apis") {
			return jsonStringResult("clearmidi;allnotesoff;playmidifile;set_midifile(file);set_clickspersecond(clicks)", id);
		}
		if (api == "description") {
			return jsonStringResult("Server which distributes things to Viz plugins", id);
		}
		if (api == "about") {
			return jsonStringResult("by Tim Thompson - me@timthompson.com", id);
		}
		if ( api == "clearmidi" ) {
			ss->ANO();
			ss->ScheduleClear();
			ss->ClearNotesDown();
			return jsonOK(id);
		}
		if ( api == "allnotesoff" ) {
			ss->ANO();
			ss->ClearNotesDown();
			return jsonOK(id);
		}
		if ( api == "playmidifile" ) {
			std::string filename = ss->_getMidiFile();
			std::string fpath = ManifoldPath("midifiles/"+filename);
			MidiPhrase* ph = newMidiPhraseFromFile(fpath);
			if ( ph == NULL ) {
				std::string err = NosuchSnprintf("Error reading phrase from file: %s",fpath.c_str());
				return jsonError(-32000,err,id);
			}
			ss->ScheduleMidiPhrase(ph,ss->CurrentClick(),0);
			// DO NOT free ph - scheduleMidiPhrase takes it over.
			return jsonOK(id);
		}

		// PARAMETER clickspersecond
		if ( api == "set_clickspersecond" ) {
			int nclicks = jsonNeedInt(params,"clicks",-1);
			if (nclicks > 0) {
				ss->SetClicksPerSecond(nclicks);
			}
			return jsonOK(id);
		}
		if ( api == "get_clickspersecond" ) {
			return jsonIntResult(ss->GetClicksPerSecond(), id);
		}

		// PARAMETER midifile
		if (api == "set_midifile") {
			// static so c_str() is persistent
			static std::string file;
			file = jsonNeedString(params, "file", "");
			if (file != "") {
				ss->_setMidiFile(file.c_str());
			}
			return jsonOK(id);
		}
		if (api == "get_midifile") {
			std::string file = ss->_getMidiFile();
			return jsonStringResult(file, id);
		}

		std::string err = NosuchSnprintf("VizServer - Unrecognized method '%s'",api.c_str());
		return jsonError(-32000,err,id);
	}

	VizServerApiCallback* cb = _callbacks.findprefix(prefix);
	if ( cb == NULL ) {
		std::string err = NosuchSnprintf("Unable to find Json API match for prefix=%s",prefix.c_str());
		return jsonError(-32000,err,id);
	}

	jsoncallback_t jsoncb = (jsoncallback_t)(cb->_cb);
	// NOTE: The interface in client DLLs uses char* rather than std::string
	const char *r= jsoncb(cb->_data,api.c_str(),params,id);
	std::string result;
	if ( r == NULL ) {
		DEBUGPRINT(("HEY! NULL return from json callback!?"));
		result = jsonError(-32000,"NULL return from json callback",id);
	} else if ( r[0] != '{' ) {
		DEBUGPRINT(("HEY! result from json callback doesn't start with '{' !?"));
		result = jsonError(-32000,"Result from json callback doesn't start with curly brace",id);
	} else {
		result = std::string(r);
	}
	return result;
}

//////////// VizServerOscProcessor

bool
checkAddrPattern(const char *addr, char *patt)
{
	return ( strncmp(addr,patt,strlen(patt)) == 0 );
}

class VizServerOscProcessor : public NosuchOscListener {

public:
	VizServerOscProcessor(VizServer* ss) {
		_vizserver = ss;
	}
	void processOsc(const char* source, const osc::ReceivedMessage& m) {
	    const char *types = m.TypeTags();
		const char *addr = m.AddressPattern();
		int nargs = (types==NULL?0:(int)strlen(types));

		std::string err;
		std::string prefix;
		std::string suffix;

		// DebugOscMessage("processOsc ",m);
		DEBUGPRINT1(("processOsc source=%s addr=%s",
			source==NULL?"NULL?":source,addr));

		if (checkAddrPattern(addr,"/tuio/25Dblb")) {

			std::string cmd = ArgAsString(m,0);
			if (cmd == "alive") {
				// if ( nargs > 1 )
				DEBUGPRINT2(("25Dblb alive types=%s nargs=%d\n",types,nargs));
				for (int i = 1; i < nargs; i++) {
					int sidnum = ArgAsInt32(m,i);
					_vizserver->_touchCursorSid(sidnum,source);
				}
				// _vizserver->_checkCursorUp();
			} else if (cmd == "fseq") {
				// int seq = ArgAsInt32(m,1);
			} else if (cmd == "set") {
				int sidnum = ArgAsInt32(m,1);
				double x = ArgAsFloat(m,2);
				double y = ArgAsFloat(m,3);
				double z = ArgAsFloat(m,4);
				double tuio_a = ArgAsFloat(m,5);   // Angle
				double tuio_w = ArgAsFloat(m,6);
				double tuio_h = ArgAsFloat(m,7);
				double tuio_f = ArgAsFloat(m,8);   // Area
				// y = 1.0f - y;

				_vizserver->_setCursor(sidnum, source, NosuchPos(x,y,z), tuio_f, NULL, NULL);
			}
			return;

		}

		if (checkAddrPattern(addr,"/tuio/2Dcur")) {

			std::string cmd = ArgAsString(m,0);
			if (cmd == "alive") {
				for (int i = 1; i < nargs; i++) {
					int sidnum = ArgAsInt32(m,i);
					// std::string sid = sidString(sidnum,source);
					_vizserver->_touchCursorSid(sidnum, source);
				}
				// _vizserver->_checkCursorUp();
			} else if (cmd == "fseq") {
				// int seq = ArgAsInt32(m,1);
			} else if (cmd == "set") {
				// It looks like "/tuio/2Dcur set s x y X Y m"
				int sidnum = ArgAsInt32(m,1);
				// std::string sid = sidString(sidnum,source);

				double x = ArgAsFloat(m,2);
				double y = ArgAsFloat(m,3);
				double z = -1.0;
				double tuio_f = -1.0;

				y = 1.0f - y;

				DEBUGPRINT1(("/tuio/2Dcur xy=%.3f,%.3f",x,y));

				_vizserver->_setCursor(sidnum, source, NosuchPos(x,y,z), tuio_f, NULL, NULL);
			}
			return;
		}

		if ( strncmp(addr,"/tuio/",6) == 0 ) {
			DEBUGPRINT(("Ignoring OSC message with addr=%s",addr));
			return;
		}
		
		if ( strncmp(addr,"/api",4) == 0 ) {
			if ( nargs <= 0 ) {
				return;
			}
			std::string fullmethod = ArgAsString(m,0);
			std::string jsonstr = "{}";
			if ( nargs > 1 ) {
				jsonstr = ArgAsString(m,1);
			}
			cJSON* params = cJSON_Parse(jsonstr.c_str());
			if ( ! params ) {
				NosuchErrorOutput("Unable to parse json in OSC /api: %s",jsonstr.c_str());
				return;
			}
			std::string s = _vizserver->_processJson(fullmethod,params,"12345");
			DEBUGPRINT(("/api method=%s output=%s",fullmethod.c_str(),s.c_str()));
			return;
		}
		DEBUGPRINT(("Ignoring OSC message with addr=%s",addr));
	}
private:
	VizServer* _vizserver;
};

//////////// VizServerMidiProcessor

class VizServerMidiProcessor : public NosuchMidiListener {
public:
	VizServerMidiProcessor() {
	}
	void processMidiMsg(MidiMsg* m) {
		std::map<void*,VizServerMidiCallback*>::iterator it;
		for ( it=_callbacks.begin(); it!=_callbacks.end(); it++ ) {
			VizServerMidiCallback* sscb = it->second;
			if ( m->matches(sscb->_mf) ) {
				midicallback_t cb = (midicallback_t)(sscb->_cb);
				cb(sscb->_data,m);
			}
		}
	}
	void AddMidiCallback(void* handle, MidiFilter mf, midicallback_t cb, void* data) {
		_callbacks.addcallback(handle,mf,(void*)cb,data);
	}
	void RemoveMidiCallback(void* handle) {
		_callbacks.removecallback(handle);
	}
	int numCallbacks() {
		return _callbacks.size();
	}
private:
	VizServerMidiCallbackMap _callbacks;
};

//////////// VizServerCursorProcessor

class VizServerCursorProcessor : public CursorListener {
public:
	VizServerCursorProcessor() {
	}
	void processCursor(VizCursor* c, int downdragup) {
		try {
			std::map<void*,VizServerCursorCallback*>::iterator it;
			for ( it=_callbacks.begin(); it!=_callbacks.end(); it++ ) {
				VizServerCursorCallback* sscb = it->second;
				if ( c->matches(sscb->_cf) ) {
					cursorcallback_t cb = (cursorcallback_t)(sscb->_cb);
					cb(sscb->_data,c,downdragup);
				}
			}
		} catch (NosuchException& e) {
			DEBUGPRINT(("NosuchException in processCursor: %s",e.message()));
		} catch (...) {
			// Does this really work?  Not sure
			DEBUGPRINT(("Some other kind of exception in processCursor occured!?"));
		}
	}
	void AddCursorCallback(void* handle, CursorFilter cf, cursorcallback_t cb, void* data) {
		_callbacks.addcallback(handle,cf,(void*)cb,data);
	}
	void RemoveCursorCallback(void* handle) {
		_callbacks.removecallback(handle);
	}
	int numCallbacks() { return _callbacks.size(); }
private:
	VizServerCursorCallbackMap _callbacks;
};

//////////// VizServerKeystrokeProcessor

class VizServerKeystrokeProcessor : public KeystrokeListener {
public:
	VizServerKeystrokeProcessor() {
	}
	void processKeystroke(int key, int downup) {
		try {
			std::map<void*,VizServerKeystrokeCallback*>::iterator it;
			for ( it=_callbacks.begin(); it!=_callbacks.end(); it++ ) {
				VizServerKeystrokeCallback* sscb = it->second;
				keystrokecallback_t cb = (keystrokecallback_t)(sscb->_cb);
				cb(sscb->_data,key,downup);
			}
		} catch (NosuchException& e) {
			DEBUGPRINT(("NosuchException in processCursor: %s",e.message()));
		} catch (...) {
			// Does this really work?  Not sure
			DEBUGPRINT(("Some other kind of exception in processKeystroke occured!?"));
		}
	}
	void AddKeystrokeCallback(void* handle, keystrokecallback_t cb, void* data) {
		_callbacks.addcallback(handle,(void*)cb,data);
	}
	void RemoveKeystrokeCallback(void* handle) {
		_callbacks.removecallback(handle);
	}
	int numCallbacks() { return _callbacks.size(); }
private:
	VizServerKeystrokeCallbackMap _callbacks;
};

//////////// VizServer

VizServer::VizServer() {
	_started = false;
	_frameseq = 0;
	_htmldir = "html";
	_midifile = "";
	_jsonprocessor = NULL;
	_oscprocessor = NULL;
	_midiinputprocessor = NULL;
	_midioutputprocessor = NULL;
	_cursorprocessor = NULL;
	_keystrokeprocessor = NULL;
	_daemon = NULL;
	_scheduler = NULL;
	_cursors = new std::list<VizCursor*>();
	NosuchLockInit(&_cursors_mutex,"cursors");

	_midi_input_name = NULL;
	_midi_output_name = NULL;
	_do_midimerge = false;
	_do_sharedmem = false;
	_sharedmem_outlines = NULL;
	_sharedmemname = NULL;
	_do_errorpopup = false;
	_do_ano = false;
	_maxcallbacks = 0;

	_osc_input_port = -1;
	_osc_input_host = "";
}

VizServer::~VizServer() {
	Stop();
}

VizCursor*
VizServer::_getCursor(int sidnum, std::string sidsource, bool lockit) {

	VizCursor* retc = NULL;
 
	if ( lockit ) {
		LockCursors();
	}
	for ( std::list<VizCursor*>::iterator i = _cursors->begin(); i!=_cursors->end(); i++ ) {
		VizCursor* c = *i;
		NosuchAssert(c);
		if (c->sid == sidnum && c->source == sidsource) {
			retc = c;
			break;
		}
	}
	if ( lockit ) {
		UnlockCursors();
	}
	return retc;
}

void VizServer::_checkCursorUp() {
	
	for ( std::list<VizCursor*>::iterator i = _cursors->begin(); i!=_cursors->end(); ) {
		VizCursor* c = *i;
		NosuchAssert(c);
		int dt = MilliNow() - c->last_touched;
		// This timeout used to be 4ms, but when camera input
		// is being used, the frame rate can be slow, so 20ms
		// is more appropriate.
		if (dt > 100) {
			DEBUGPRINT1(("processing and erasing cursor c sid=%d  cursors size=%d",c->sid,_cursors->size()));
			_cursorprocessor->processCursor(c,CURSOR_UP);
			i = _cursors->erase(i);
			delete c;
		} else {
			i++;
		}
	}
}

void
VizServer::AdvanceCursorTo(VizCursor* c, int tm) {
	c->advanceTo(tm);
}

void
VizServer::_setCursor(int sidnum, std::string sidsource, NosuchPos pos, double area, OutlineMem* om, MMTT_SharedMemHeader* hdr)
{
	LockCursors();
	VizCursor* c = _getCursor(sidnum,sidsource,false);
	if ( c != NULL ) {
		c->target_pos = pos;
		DEBUGPRINT1(("_setCursor c=%ld setting target_pos= %.3f %.3f",(long)c,c->target_pos.x,c->target_pos.y));
		c->pos = pos;
		c->area = area;
		c->outline = om;
		c->hdr = hdr;
		_cursorprocessor->processCursor(c,CURSOR_DRAG);
	} else {
		c = new VizCursor(this, sidnum, sidsource, pos, area,om,hdr);
		DEBUGPRINT1(("_setCursor NEW c=%ld pos= %.3f %.3f",(long)c,pos.x,pos.y));
		_cursors->push_back(c);
		_cursorprocessor->processCursor(c,CURSOR_DOWN);
	}
	c->advanceTo(MilliNow());
	c->touch(MilliNow());
	UnlockCursors();
}

void VizServer::_setCursorSid(int sidnum, const char* source, double x, double y, double z, double tuio_f, OutlineMem* om, MMTT_SharedMemHeader* hdr ) {
	_setCursor(sidnum, source, NosuchPos(x,y,z), tuio_f, om, hdr);
}

void VizServer::_processCursorsFromBuff(MMTT_SharedMemHeader* hdr) {
	buff_index b = hdr->buff_to_display;
	int ncursors = hdr->numoutlines[b];
	if ( ncursors == 0 ) {
		return;
	}

	for ( int n=0; n<ncursors; n++ ) {
		OutlineMem* om = hdr->outline(b,n);
		_setCursorSid(om->sid, "sharedmem", om->x,om->y, om->z, om->area ,om,hdr);
	}
}

void VizServer::ErrorPopup(const char* msg) {
		MessageBoxA(NULL,msg,"Palette",MB_OK);
}

int VizServer::MilliNow() {
	if ( _scheduler == NULL )
		return 0;
	return _scheduler->_MilliNow;
}

click_t VizServer::CurrentClick() {
	if ( _scheduler == NULL ) {
		DEBUGPRINT(("In VizServer::CurrentClick, _scheduler==NULL?"));
		return 0;
	} else {
		return _scheduler->CurrentClick();
	}
}

std::string
VizServer::_processJson(std::string fullmethod,cJSON* params,const char* id) {
		return _jsonprocessor->processJson(fullmethod,params,id);
}

cJSON*
VizServer::_readconfig(const char* fn) {

	std::string fname = std::string(fn);
	std::string err;

	cJSON* json = jsonReadFile(fname,err);
	if ( ! json ) {
		NosuchErrorOutput("Unable to read/parse config file (name=%s, err=%s), disabling Freeframe plugin!\n",fname.c_str(),err.c_str());
	}
	return json;
}

void
VizServer::InsertKeystroke(int key, int downup) {
	if ( _keystrokeprocessor ) {
		_keystrokeprocessor->processKeystroke(key,downup);
	}
}

int
VizServer::GetClicksPerSecond() {
	return _scheduler->ClicksPerSecond;
}
void
VizServer::SendMidiMsg(MidiMsg* msg) {
	DEBUGPRINT(("Hi from VizServer::SendMidiMsg IS NOT DOING ANYTHING?"));
}

void
VizServer::SendControllerMsg(MidiMsg* m, void* handle, bool smooth) {
	_scheduler->SendControllerMsg(m,handle,smooth);
}

void
VizServer::SendPitchBendMsg(MidiMsg* m, void* handle, bool smooth) {
	_scheduler->SendPitchBendMsg(m,handle,smooth);
}

void
VizServer::IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle) {
	_scheduler->IncomingNoteOff(clk,ch,pitch,vel,handle);
}

void
VizServer::IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle) {
	_scheduler->IncomingMidiMsg(m,clk,handle);
}

void
VizServer::ScheduleMidiPhrase(MidiPhrase* ph, click_t clk, void* handle) {
	NosuchAssert(_scheduler);
	_scheduler->ScheduleMidiPhrase(ph,clk,handle);
}

void
VizServer::ScheduleMidiMsg(MidiMsg* m, click_t clk, void* handle) {
	NosuchAssert(_scheduler);
	_scheduler->ScheduleMidiMsg(m,clk,handle);
}

void
VizServer::ScheduleClear() {
	NosuchAssert(_scheduler);
	_scheduler->ScheduleClear();
}

void
VizServer::QueueMidiPhrase(MidiPhrase* ph, click_t clk) {
	NosuchAssert(_scheduler);
	_scheduler->QueueMidiPhrase(ph,clk);
}

void
VizServer::QueueMidiMsg(MidiMsg* m, click_t clk) {
	NosuchAssert(_scheduler);
	_scheduler->QueueMidiMsg(m,clk);
}

void
VizServer::QueueClear() {
	NosuchAssert(_scheduler);
	_scheduler->QueueClear();
}

void
VizServer::SetClicksPerSecond(int clicks) {
	NosuchAssert(_scheduler);
	_scheduler->SetClicksPerSecond(clicks);
}

void
VizServer::SetTempoFactor(float f) {
	_scheduler->SetTempoFactor(f);
}

void
VizServer::ANO(int channel) {
	NosuchAssert(_scheduler);
	_scheduler->ANO(channel);
}

int
VizServer::NumCallbacks() {
	int nmidiin = _midiinputprocessor?_midiinputprocessor->numCallbacks():0;
	int nmidiout = _midioutputprocessor?_midioutputprocessor->numCallbacks():0;
	int njson = _jsonprocessor?_jsonprocessor->numCallbacks():0;
	int ncursor = _cursorprocessor?_cursorprocessor->numCallbacks():0;
	return nmidiin + nmidiout + njson + ncursor;
}

bool
VizServer::Start() {

	bool r = true;

	if ( _started ) {
		DEBUGPRINT1(("VizServer::Start called - things are already running "));
		return true;
	}
	DEBUGPRINT(("VizServer::Start"));

	if ( _do_errorpopup ) {
		NosuchErrorPopup = VizServer::ErrorPopup;
	} else {
		NosuchErrorPopup = NULL;
	}
	try {
		std::string configfile = "config/vizserver.json";
		std::string configpath = ManifoldPath(configfile);
		DEBUGPRINT1(("configpath = %s",configpath.c_str()));

		cJSON* json = _readconfig(configpath.c_str());
		if ( json == NULL ) {
			DEBUGPRINT(("Unable to load config?  path=%s",configpath.c_str()));
		} else {
			_processServerConfig(json);
			// NOTE: DO NOT FREE json - some of the char* values in it get saved/used later.
		}

		_scheduler = new NosuchScheduler();
		_scheduler->setPeriodicANO(_do_ano);

		_midiinputprocessor = new VizServerMidiProcessor();
		_midioutputprocessor = new VizServerMidiProcessor();
		_scheduler->SetMidiInputListener(_midiinputprocessor,_do_midimerge);
		_scheduler->SetMidiOutputListener(_midioutputprocessor);

		_cursorprocessor = new VizServerCursorProcessor();
		_keystrokeprocessor = new VizServerKeystrokeProcessor();

		_jsonprocessor = new VizServerJsonProcessor();
		_oscprocessor = new VizServerOscProcessor(this);
		_daemon = new NosuchDaemon(_osc_input_port,_osc_input_host,_oscprocessor,
						_httpport,_htmldir,_jsonprocessor);

		_started = true;

		_openSharedMemOutlines();

		if ( _midi_output_name == NULL ) {
			DEBUGPRINT(("Warning: MIDI output wasn't defined in configuration!"));
			_midi_output_name = "Midi Yoke:  1";
		}
		if ( _midi_input_name == NULL ) {
			DEBUGPRINT(("Warning: MIDI input wasn't defined in configuration!"));
			_midi_input_name = "Midi Yoke:  2";
		}
		_scheduler->StartMidi(_midi_input_name,_midi_output_name);

		_clickprocessor = new VizServerClickProcessor(this);
		_scheduler->SetClickProcessor(_clickprocessor);

	} catch (NosuchException& e) {
		DEBUGPRINT(("NosuchException: %s",e.message()));
		r = false;
	} catch (...) {
		// Does this really work?  Not sure
		DEBUGPRINT(("Some other kind of exception occured!?"));
		r = false;
	}
	return r;
}

void VizServer::_advanceClickTo(int current_click, NosuchScheduler* sched) {

	// XXX - should all of these things be done on EVERY click?
	_checkSharedMem();

	if ( TryLockCursors() != 0 ) {
		return;
	}
	_checkCursorUp();

	// DEBUGPRINT(("VizServer::_advanceClickTo click=%d now=%d",
	// 	current_click,VizServer::MilliNow()));
	for ( std::list<VizCursor*>::iterator i = _cursors->begin(); i!=_cursors->end(); i++ ) {
		VizCursor* c = *i;
		NosuchAssert(c);
		c->advanceTo(MilliNow());
	}

	UnlockCursors();
}

void VizServer::_checkSharedMem() {

	_openSharedMemOutlines();

	MMTT_SharedMemHeader* mem = NULL;

	UT_SharedMem* outlines = _getSharedMemOutlines();
	if ( ! outlines ) {
		return;
	}

	outlines->lock();

	void *data = outlines->getMemory();
	if ( !data ) {
		DEBUGPRINT(("VizServer:: NULL returned from getMemory of Shared Memory!  (B)"));
		goto getout;
	}

	mem = (MMTT_SharedMemHeader*) data;
	NosuchAssert(mem);

	if ( mem->version != MMTT_SHM_VERSION_NUMBER ) {
		DEBUGPRINT(("VizServer, MMTT sharedmem is the wrong version!  Expected %d, got %d", MMTT_SHM_VERSION_NUMBER, mem->version));
		outlines->unlock();
		_closeSharedMemOutlines();
		outlines = NULL;
		mem = NULL;
		goto getout;
	}

	long dt = timeGetTime() - mem->lastUpdateTime;

	DEBUGPRINT1(("_checkSharedMem mem->seqnum=%d _frameseq=%d",mem->seqnum,_frameseq));

	if ( mem->seqnum < 0 ) {
		// If seqnum is negative, the shared memory
		// is being initialized (MMTT is probably doing
		// auto-alignment), so do nothing.
		DEBUGPRINT1(("VizServer, sharedmem is being initialized"));
		goto getout;
	}
	// Don't process the same frame twice
	if ( mem->seqnum == _frameseq ) { 
		goto getout;
	}
	_frameseq = mem->seqnum;

	if ( dt > 10000 ) {
		DEBUGPRINT(("VizServer, sharedmem isn't being updated, closing it (seqnum=%d dt=%d)",mem->seqnum,dt));
		outlines->unlock();
		_closeSharedMemOutlines();
		outlines = NULL;
		mem = NULL;
		goto getout;

	}

	mem->buff_to_display = BUFF_UNSET;
	if ( mem->buff_to_display_next == BUFF_UNSET ) {
		// Use the buffer that was displayed last time
		if ( mem->buff_displayed_last_time == BUFF_UNSET ) {
			DEBUGPRINT(("HEY!  Both buff_to_display_next and buff_displayed_last_time are UNSET??"));
			// Leave buff_to_display set to BUFF_UNSET
		} else {
			mem->buff_to_display = mem->buff_displayed_last_time;
		}
	} else {
		mem->buff_to_display = mem->buff_to_display_next;
		if ( mem->buff_displayed_last_time != BUFF_UNSET ) {
			mem->buff_inuse[mem->buff_displayed_last_time] = false;
		}
		mem->buff_displayed_last_time = mem->buff_to_display_next;
		mem->buff_to_display_next = BUFF_UNSET;
		// The buff_inuse flags are unchanged;
	}

	_processCursorsFromBuff(mem);

getout:
	if ( outlines ) {
		outlines->unlock();
	}
}

void
VizServer::Stop() {
	DEBUGPRINT(("VizServer::Stop called"));
	if ( ! _started ) {
		return;
	}
	_started = false;
	if ( _scheduler ) {
		ANO();
		ScheduleClear();
		ClearNotesDown();
		_scheduler->Stop();
		delete _scheduler;
		_scheduler = NULL;
	}
	//	_scheduler->StopMidi(_midi_input_name,_midi_output_name);
	if ( _daemon ) { delete _daemon; _daemon = NULL; }
	if ( _midiinputprocessor ) { delete _midiinputprocessor; _midiinputprocessor = NULL; }
	if ( _midioutputprocessor ) { delete _midioutputprocessor; _midioutputprocessor = NULL; }
	if ( _jsonprocessor ) { delete _jsonprocessor; _jsonprocessor = NULL; }
	if ( _oscprocessor ) { delete _oscprocessor; _oscprocessor = NULL; }
	if ( _cursorprocessor ) { delete _cursorprocessor; _cursorprocessor = NULL; }
	if ( _clickprocessor ) { delete _clickprocessor; _clickprocessor = NULL; }

	_closeSharedMemOutlines();
}

void
VizServer::_openSharedMemOutlines()
{
	if ( ! _do_sharedmem || _sharedmem_outlines != NULL ) {
		return;
	}
	if ( ! Pt_Started() ) {
		return;
	}
	long now = Pt_Time();
	// Only check once in a while
	if ( (now - _sharedmem_last_attempt) < 5000 ) {
		return;
	}
	_sharedmem_last_attempt = now;
	_sharedmem_outlines = new UT_SharedMem(_sharedmemname);
	UT_SharedMemError err = _sharedmem_outlines->getErrorState();
	if ( err != UT_SHM_ERR_NONE ) {
		static long last_warning = 0;
		if ( last_warning==0 || (now-last_warning) > 60000 ) {
			last_warning = now;
			DEBUGPRINT(("Unable to open shared memory with name='%s' ?  Is MMTT running?  err=%d",_sharedmemname,err));
		}
		_closeSharedMemOutlines();
	} else {
		DEBUGPRINT(("Successfully opened shared memory, name=%s",_sharedmemname));
	}

}

void
VizServer::_closeSharedMemOutlines()
{
	if ( _sharedmem_outlines ) {
		delete _sharedmem_outlines;
		// DON'T set _do_sharedmem to false, we want
		// to keep trying to connect to the sharedmem
		// every once in a while.
		_sharedmem_outlines = NULL;
	}
}

void
VizServer::_processServerConfig(cJSON* json) {
	cJSON *j;


	if ( (j=jsonGetNumber(json,"periodicANO")) != NULL ) {
		_do_ano = (j->valueint != 0);
	}
	if ( (j=jsonGetNumber(json,"sharedmem")) != NULL ) {
		_do_sharedmem = (j->valueint != 0);
	}
	if ( (j=jsonGetString(json,"sharedmemname")) != NULL ) {
		_sharedmemname = j->valuestring;
	}
	if ( (j=jsonGetString(json,"midiinput")) != NULL ) {
		_midi_input_name = j->valuestring;
	}
	if ( (j=jsonGetNumber(json,"midimerge")) != NULL ) {
		_do_midimerge = (j->valueint != 0);
	}
	if ( (j=jsonGetString(json,"midioutput")) != NULL ) {
		_midi_output_name = j->valuestring;
	}
	if ( (j=jsonGetNumber(json,"errorpopup")) != NULL ) {
		_do_errorpopup = (j->valueint != 0);
	}
	if ( (j=jsonGetNumber(json,"tuio")) != NULL ) {
		DEBUGPRINT(("tuio value in palette.json no longer used.  Remove tuioport value to disable tuio"));
	}
	if ( (j=jsonGetNumber(json,"tuioport")) != NULL ) {
		_osc_input_port = j->valueint;
	}
	if ( (j=jsonGetString(json,"tuiohost")) != NULL ) {
		_osc_input_host = j->valuestring;
	}
	if ( (j=jsonGetNumber(json,"httpport")) != NULL ) {
		_httpport = j->valueint;
	}
	if ((j = jsonGetString(json, "htmldir")) != NULL) {
		// If the _htmldir isn't a fullpath, add $VIZBENCH
		char *manifold;
		if (_htmldir[0] != '/' && _htmldir[1] != ':' && (manifold=getenv("VIZBENCH")) != NULL ) {
			std::string hd = NosuchSnprintf("%s\\%s", manifold, _htmldir);
			_htmldir = _strdup(hd.c_str());
		}
		else {
			_htmldir = j->valuestring;
		}
	}
	if ( (j=jsonGetNumber(json,"debugtoconsole")) != NULL ) {
		NosuchDebugToConsole = j->valueint?TRUE:FALSE;
	}
	if ( (j=jsonGetNumber(json,"debugmidinotes")) != NULL ) {
		NosuchDebugMidiNotes = j->valueint?TRUE:FALSE;
	}
	if ( (j=jsonGetNumber(json,"debugmidiall")) != NULL ) {
		NosuchDebugMidiAll = j->valueint?TRUE:FALSE;
	}
	if ( (j=jsonGetNumber(json,"debugtolog")) != NULL ) {
		bool b = j->valueint?TRUE:FALSE;
		// If we're turning debugtolog off, put a final
		// message out so we know that!
		if ( NosuchDebugToLog && !b ) {
			DEBUGPRINT(("ALERT: NosuchDebugToLog is being set to false!"));
		}
		NosuchDebugToLog = b;
	}
	if ( (j=jsonGetNumber(json,"debugautoflush")) != NULL ) {
		NosuchDebugAutoFlush = j->valueint?TRUE:FALSE;
	}
	if ( (j=jsonGetString(json,"debuglogfile")) != NULL ) {
		NosuchDebugSetLogDirFile(NosuchDebugLogDir,std::string(j->valuestring));
	}
}

VizServer*
VizServer::GetServer() {
	if ( OneServer == NULL ) {
		OneServer = new VizServer();
		DEBUGPRINT1(("NEW VizServer!  Setting OneServer"));
	}
	return OneServer;
}

void
VizServer::DeleteServer() {
	if (OneServer) {
		delete OneServer;
	}
	OneServer = NULL;
}

void
VizServer::_setMaxCallbacks() {
	int ncb = NumCallbacks();
	if ( ncb > _maxcallbacks ) {
		_maxcallbacks = ncb;
	}
}

const char *
VizServer::VizTags() {
	static std::string s;
	s = _jsonprocessor->VizTags();
	return s.c_str();
}

void
VizServer::ChangeVizTag(void* handle, const char* p) {
	_jsonprocessor->ChangeVizTag(handle,p);
}

void
VizServer::AddJsonCallback(void* handle, ApiFilter af, jsoncallback_t callback, void* data) {
	_jsonprocessor->AddJsonCallback(handle,af,callback,data);
	_setMaxCallbacks();
}

void
VizServer::AddMidiInputCallback(void* handle, MidiFilter mf, midicallback_t callback, void* data) {
	_midiinputprocessor->AddMidiCallback(handle,mf,callback,data);
	_setMaxCallbacks();
}

void
VizServer::AddMidiOutputCallback(void* handle, MidiFilter mf, midicallback_t callback, void* data) {
	_midioutputprocessor->AddMidiCallback(handle,mf,callback,data);
	_setMaxCallbacks();
}

void
VizServer::AddCursorCallback(void* handle, CursorFilter cf, cursorcallback_t callback, void* data) {
	_cursorprocessor->AddCursorCallback(handle,cf,callback,data);
	_setMaxCallbacks();
}

void
VizServer::AddKeystrokeCallback(void* handle, keystrokecallback_t callback, void* data) {
	_keystrokeprocessor->AddKeystrokeCallback(handle,callback,data);
	_setMaxCallbacks();
}

void
VizServer::RemoveJsonCallback(void* handle) {
	if ( _jsonprocessor ) {
		_jsonprocessor->RemoveJsonCallback(handle);
	}
}

void
VizServer::RemoveMidiInputCallback(void* handle) {
	if ( _midiinputprocessor ) {
		_midiinputprocessor->RemoveMidiCallback(handle);
	}
}

void
VizServer::RemoveMidiOutputCallback(void* handle) {
	if ( _midioutputprocessor ) {
		_midioutputprocessor->RemoveMidiCallback(handle);
	}
}

void
VizServer::RemoveCursorCallback(void* handle) {
	if ( _cursorprocessor ) {
		_cursorprocessor->RemoveCursorCallback(handle);
	}
}	

void
VizServer::_touchCursorSid(int sid, std::string source) {
	VizCursor* c = _getCursor(sid,source,true);
	if ( c ) {
		c->touch(MilliNow());
	}
}

VizCursor::VizCursor(VizServer* ss, int sid_, std::string source_,
		NosuchPos pos_, double area_, OutlineMem* om_, MMTT_SharedMemHeader* hdr_) {

	pos = pos_;
	target_pos = pos_;
	_vizserver = ss;
	last_touched = 0;
	sid = sid_;
	source = source_;
	area = area_;
	outline = om_;
	hdr = hdr_;
	region = NULL;
	curr_speed = 0;
	curr_degrees = 0;

	_target_depth = 0;
	_last_depth = 0;
	_last_channel = 0;
	_last_click = 0;
	_last_tm = 0;
	_target_degrees = 0;
	_g_firstdir = true;
	_smooth_degrees_factor = 0.2;

	touch(ss->MilliNow());
}

double
normalize_degrees(double d) {
	if ( d < 0.0f ) {
		d += 360.0f;
	} else if ( d > 360.0f ) {
		d -= 360.0f;
	}
	return d;
}

void
VizCursor::advanceTo(int tm) {

	int dt = tm - _last_tm;
	if ( dt <= 0 ) {
		return;
	}
	DEBUGPRINT1(("    Cursor %ld advance start pos= %.3f %.3f   target= %.3f %.3f",(long)this,pos.x,pos.y,target_pos.x,target_pos.y));

	// If _pos and _g_smoothedpos are the same (x and y, not z), then there's nothing to smooth
	if ( pos.x == target_pos.x && pos.y == target_pos.y ) {
		DEBUGPRINT1(("VizCursor::advanceTo, current and target are the same"));
		return;
	}

	NosuchPos dpos = target_pos.sub(pos);
	double raw_distance = dpos.mag();
	if ( raw_distance > 1.0f ) {
		DEBUGPRINT(("VizCursor::advanceTo, raw_distance>1.0 !?"));
		return;
	}
	if ( raw_distance == 0.0f ) {
		DEBUGPRINT(("VizCursor::advanceTo, raw_distance=0.0 !?"));
		return;
	}

	double this_speed = 1000.0f * raw_distance / dt;
	double speed_limit = 60.0f;
	if ( this_speed > speed_limit ) {
		DEBUGPRINT(("Speed LIMIT (%f) EXCEEDED, throttled to %f",this_speed,speed_limit));
		this_speed = speed_limit;
	}

	dpos = dpos.normalize();

	double sfactor = 0.02f;
	// speed it up a bit when the distance gets larger
	if ( raw_distance > 0.4f ) {
		sfactor = 0.1f;
	} else if ( raw_distance > 0.2f ) {
		sfactor = 0.05f;
	}
	this_speed = this_speed * sfactor;

	double dspeed = this_speed - curr_speed;
	double smooth_speed_factor = 0.1f;
	curr_speed = curr_speed + dspeed * smooth_speed_factor;
	dpos = dpos.mult(raw_distance * curr_speed);
	_last_pos = pos;
	pos = pos.add(dpos);
	if ( pos.x > 1.0f ) {
		DEBUGPRINT(("VizCursor::advanceTo, x>1.0 !?"));
	}
	if ( pos.y > 1.0f ) {
		DEBUGPRINT(("VizCursor::advanceTo, y>1.0 !?"));
	}

	DEBUGPRINT1(("   Cursor advance end pos= %.3f %.3f",pos.x,pos.y));

	NosuchPos finaldpos = pos.sub(_last_pos);
	double final_distance = finaldpos.mag();

	/////////////// smooth the depth
	double depthsmoothfactor = 0.3f;
	double smoothdepth = depth() + ((target_depth()-depth())*depthsmoothfactor);

	setdepth(smoothdepth);

	/////////////// smooth the degrees
	double tooshort = 0.01f; // 0.05f;
	if (raw_distance < tooshort) {
		// NosuchDebug("   raw_distance=%.3f too small %s\n",
		// 	raw_distance,DebugString().c_str());
	} else {
		NosuchPos dp = pos.sub(_last_pos);
		double heading = dp.heading();
		_target_degrees = radian2degree(heading);
		_target_degrees += 90.0;
		_target_degrees = normalize_degrees(_target_degrees);

		if (_g_firstdir) {
			curr_degrees = _target_degrees;
			_g_firstdir = false;
		} else {
			double dd1 = _target_degrees - curr_degrees;
			double dd;
			if ( dd1 > 0.0f ) {
				if ( dd1 > 180.0f ) {
					dd = -(360.0f - dd1);
				}
				else {
					dd = dd1;
				}
			} else {
				if ( dd1 < -180.0f ) {
					dd = dd1 + 360.0f;
				}
				else {
					dd = dd1;
				}
			}
			curr_degrees = curr_degrees + (dd*_smooth_degrees_factor);

			curr_degrees = normalize_degrees(curr_degrees);
		}
	}
	_last_tm = tm;
}
