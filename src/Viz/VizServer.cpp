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
#include <SpriteVizParams.h>
#include <NosuchUtil.h>
#include <NosuchJSON.h>
#include <NosuchException.h>
#include <NosuchOscManager.h>
#include <NosuchHttpServer.h>
#include <NosuchScheduler.h>
#include "UT_SharedMem.h"

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call) {

	case DLL_PROCESS_ATTACH:
		{
			char path[MAX_PATH];
			GetModuleFileNameA((HMODULE)hModule, path, MAX_PATH);
			std::string dllpath = std::string(path);

			// We want to take off the final filename AND the directory.
			// This assumes that the DLL is in either a bin or ffglplugins
			// subdirectory of the main Vizpath
			size_t pos = dllpath.find_last_of("/\\");
			if ( pos != dllpath.npos && pos > 0 ) {
				std::string parent = dllpath.substr(0,pos);
				pos = dllpath.substr(0,pos-1).find_last_of("/\\");
				if ( pos != parent.npos && pos > 0) {
					SetVizPath(parent.substr(0,pos));
				}
			}
		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#include <set>
#include <map>

using namespace std;

/* static */ VizServer* OneServer = NULL;
/* static */ int ServerCount = 0;

class VizServerApiCallback {
public:
	VizServerApiCallback(const char* apiprefix, void* cb, void* data) {
		m_cb = cb;
		m_data = data;
		m_apiprefix = _strdup(apiprefix);
		for (char* s = m_apiprefix; *s; s++) {
			*s = tolower(*s);
		}
	}
	virtual ~VizServerApiCallback() {
		free((void*)m_apiprefix);
	}
	void* m_cb;
	void* m_data;
	char* m_apiprefix;
};

class VizServerCursorCallback {
public:
	VizServerCursorCallback(CursorFilter cf, void* cb, void* data) {
		m_cb = cb;
		m_data = data;
		m_cf = cf;
	}
	void* m_cb;
	void* m_data;
	CursorFilter m_cf;
};

class VizServerKeystrokeCallback {
public:
	VizServerKeystrokeCallback(void* cb, void* data) {
		m_cb = cb;
		m_data = data;
	}
	void* m_cb;
	void* m_data;
};

class VizServerClickCallback {
public:
	VizServerClickCallback(void* cb, void* data) {
		m_cb = cb;
		m_data = data;
	}
	void* m_cb;
	void* m_data;
};

class VizServerMidiCallback {
public:
	VizServerMidiCallback(MidiFilter mf, void* cb, void* data) {
		m_cb = cb;
		m_data = data;
		m_mf = mf;
	}
	void* m_cb;
	void* m_data;
	MidiFilter m_mf;
};

class VizServerApiCallbackMap : public std::map < void*, VizServerApiCallback* > {
public:
	VizServerApiCallbackMap() {
	}
	void addcallback(void* handle, const char* apiprefix, void* cb, void* data) {
		DEBUGPRINT1(("VizServerApiCallbackMap addcallback handle=%ld prefix=%s", (long)handle, apiprefix));
		if (count(handle) > 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::addcallback finds existing callback for handle=%ld", (long)handle));
			return;
		}
		VizServerApiCallback* sscb = new VizServerApiCallback(apiprefix, cb, data);
		(*this)[handle] = sscb;
		DEBUGPRINT1(("VizServerApiCallbackMap inserted handle=%ld size=%d", (long)handle, size()));
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		erase(handle);
	}
	VizServerApiCallback* findprefix(std::string prefix) {
		std::map<void*, VizServerApiCallback*>::iterator it;
		// All comparisons here are case-insensitive
		prefix = NosuchToLower(prefix);
		for (it = this->begin(); it != this->end(); it++) {
			VizServerApiCallback* sscb = it->second;
			if (NosuchToLower(std::string(sscb->m_apiprefix)) == prefix) {
				return sscb;
			}
		}
		return NULL;
	}
	const char* VizTags() {
		static std::string s;

		s = "VizServer";
		std::map<void*, VizServerApiCallback*>::iterator it;
		for (it = this->begin(); it != this->end(); it++) {
			VizServerApiCallback* sscb = it->second;
			std::string tag = std::string(sscb->m_apiprefix);
			// if (tag != "UNTAGGED" && tag != "ffff") {
			if (tag != "UNTAGGED" ) {
				s += ("," + tag);
			}
		}
		return s.c_str();
	}

	void ChangeVizTag(void* handle, const char* p) {
		if (count(handle) <= 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::ChangeVizTag didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		VizServerApiCallback* sscb = (*this)[handle];
		// Should we free prefix?  I tried freeing it in the destructor
		// of ApiFilter, and it corrupted the heap, so I'm leary.
		sscb->m_apiprefix = _strdup(p);
	}
};

class VizServerCursorCallbackMap : public std::map < void*, VizServerCursorCallback* > {
public:
	VizServerCursorCallbackMap() {
	}
	void addcallback(void* handle, CursorFilter cf, void* cb, void* data) {
		if (count(handle) > 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::addcallback finds existing callback for handle=%ld", (long)handle));
			return;
		}
		VizServerCursorCallback* sscb = new VizServerCursorCallback(cf, cb, data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		erase(handle);
	}
};

class VizServerKeystrokeCallbackMap : public std::map < void*, VizServerKeystrokeCallback* > {
public:
	VizServerKeystrokeCallbackMap() {
	}
	void addcallback(void* handle, void* cb, void* data) {
		if (count(handle) > 0) {
			DEBUGPRINT(("Hey! VizServerKeystrokeCallbackMap::addcallback finds existing callback for handle=%ld", (long)handle));
			return;
		}
		VizServerKeystrokeCallback* sscb = new VizServerKeystrokeCallback(cb, data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServerKeystrokeCallbackMap::removecallback didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		erase(handle);
	}
};

class VizServerClickCallbackMap : public std::map < void*, VizServerClickCallback* > {
public:
	VizServerClickCallbackMap() {
	}
	void addcallback(void* handle, void* cb, void* data) {
		if (count(handle) > 0) {
			DEBUGPRINT(("Hey! VizServerClickCallbackMap::addcallback finds existing callback for handle=%ld", (long)handle));
			return;
		}
		VizServerClickCallback* sscb = new VizServerClickCallback(cb, data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServerClickCallbackMap::removecallback didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		erase(handle);
	}
};

class VizServerMidiCallbackMap : public std::map < void*, VizServerMidiCallback* > {
public:
	VizServerMidiCallbackMap() {
	}
	void addcallback(void* handle, MidiFilter mf, void* cb, void* data) {
		if (count(handle) > 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::addcallback finds existing callback for handle=%d", (long)handle));
			return;
		}
		VizServerMidiCallback* sscb = new VizServerMidiCallback(mf, cb, data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%d", (long)handle));
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
	void AddJsonCallback(void* handle, const char* apiprefix, jsoncallback_t cb, void* data) {
		const char* existing = m_callbacks.VizTags();
		DEBUGPRINT(("AddJsonCallback handle=%ld  data=%ld existing=%s", (long)handle, (long)data, existing));
		m_callbacks.addcallback(handle, apiprefix, (void*)cb, data);
	}
	void ChangeVizTag(void* handle, const char* p) {
		m_callbacks.ChangeVizTag(handle, p);
	}
	void RemoveJsonCallback(void* handle) {
		m_callbacks.removecallback(handle);
		DEBUGPRINT1(("RemoveJsonCallback, m_callbacks.size=%d", m_callbacks.size()));
	}
	virtual std::string processJson(std::string method, cJSON* params, const char *id);
	int numCallbacks() { return m_callbacks.size(); }
	const char* VizTags() { return m_callbacks.VizTags(); }
	bool HasApiPrefix(std::string nm) {
		return (m_callbacks.findprefix(nm) != NULL);
	}

private:
	VizServerApiCallbackMap m_callbacks;
};

static void
methodPrefixProcess(std::string method, std::string& prefix, std::string& suffix) {
	size_t dotpos = method.find_first_of('.');
	bool hasdot = (dotpos != string::npos);
	if (hasdot) {
		prefix = method.substr(0, dotpos);
		suffix = method.substr(dotpos + 1);
	}
	else {
		prefix = "";
		suffix = method;
	}
	prefix = NosuchToLower(prefix);
}

std::string
VizServerJsonProcessor::processJson(std::string fullmethod, cJSON *params, const char* id) {

	std::string prefix;
	std::string api;
	VizServer* ss = VizServer::GetServer();

	if (ss->m_debugApi) {
		char *p = cJSON_PrintUnformatted(params);
		DEBUGPRINT(("VizServer API meth=%s params=%s", fullmethod.c_str(),p));
		cJSON_free(p);
	}
	static SpriteVizParams* allparams = NULL;
	if (allparams == NULL) {
		allparams = new SpriteVizParams();
	}

	if (fullmethod == "viztags" || fullmethod == "apitags") {  // apitags is deprecated, retained for compatibility
		const char *p = ss->VizTags();
		return jsonStringResult(std::string(p), id);
	}
#if 0
	{ static _CrtMemState s1, s2, s3;

	if ( fullmethod == "memdump" ) {
		_CrtMemState s;
		_CrtMemCheckpoint( &s );
		if ( _CrtMemDifference( &s3, &s1, &s) ) {
			_CrtMemDumpStatistics( &s3 );
		}
		_CrtDumpMemoryLeaks();
		return jsonOK(id);
	}
	if ( fullmethod == "memcheckpoint" ) {
		_CrtMemState s1;
		_CrtMemCheckpoint( &s1 );
		return jsonOK(id);
	}
	}
#endif

	methodPrefixProcess(fullmethod, prefix, api);

	if (prefix == "vizserver") {
		if (api == "apis") {
			return jsonStringResult("clearmidi;allnotesoff;play_midifile;set_midifile(file);set_clickspersecond(clicks);set_midioutput(index);set_debugapi(onoff);toggledebug", id);
		}
		if (api == "description") {
			return jsonStringResult("Server which distributes things to Viz plugins", id);
		}
		if (api == "about") {
			return jsonStringResult("by Tim Thompson - me@timthompson.com", id);
		}
		if (api == "clearmidi") {
			ss->ANO();
			ss->_scheduleClear();
			ss->ClearNotesDown();
			return jsonOK(id);
		}
		if (api == "allnotesoff") {
			ss->ANO();
			ss->ClearNotesDown();
			return jsonOK(id);
		}
		if (api == "param_vals" || api == "spriteparam_vals") {
			std::string s = allparams->JsonListOfValues();
			return jsonStringResult(s, id);
		}
		if (api == "param_stringvals" || api == "spriteparam_stringvals") {
			std::string type = jsonNeedString(params, "type", "");
			if (type == ""){
				return jsonError(-32000, "No type parameter specified on spriteparam_stringvals?", id);
			}
			std::string s = allparams->JsonListOfStringValues(type);
			return jsonStringResult(s, id);
		}
		if (api == "param_list" || api == "spriteparam_list") {
			std::string s = allparams->JsonListOfParams();
			return jsonStringResult(s, id);
		}
		if (api == "param_readfile" || api == "spriteparam_readfile") {
			std::string file = jsonNeedString(params, "paramfile", "");
			if (file != "") {
				std::string fpath = SpriteVizParamsPath(file);

				std::string err;
				cJSON* json = jsonReadFile(fpath, err);
				if (!json){
					throw NosuchException("Unable to read json from paramfile=%s err=%s", fpath.c_str(), err.c_str());
				}
				// std::string r = cJSON_escapestring(cJSON_PrintUnformatted(json));
				std::string r = cJSON_PrintUnformatted(json);
				return jsonStringResult(r, id);
			}
			else {
				return jsonError(-32000, "No file parameter specified on spriteparam_readfile?", id);
			}
		}
		if (api == "param_writefile" || api == "spriteparam_writefile") {
			std::string file = jsonNeedString(params, "paramfile", "");
			if (file == "") {
				return jsonError(-32000, "No file parameter specified on spriteparam_writefile?", id);
			}
			cJSON* j = jsonNeedJSON(params, "contents");
			std::string fpath = SpriteVizParamsPath(file);
			std::string err;
			if (jsonWriteFile(fpath, j, err)) {
				return jsonOK(id);
			}
			else {
				std::string msg = NosuchSnprintf("Unable to write file %s, err=%s",
					fpath.c_str(), err.c_str());
				return jsonError(-32000, msg.c_str(), id);
			}
		}
		if (api == "play_midifile") {
			std::string filename = ss->_getMidiFile();
			if (filename == "") {
				std::string err = NosuchSnprintf("play_midifile called with no midifile set, use set_midifile first");
				return jsonError(-32000, err, id);
			}
			std::string fpath = VizPath("midifiles\\" + filename);
			MidiPhrase* ph = newMidiPhraseFromFile(fpath);
			if (ph == NULL) {
				std::string err = NosuchSnprintf("Error reading phrase from file: %s", fpath.c_str());
				return jsonError(-32000, err, id);
			}
			ss->_scheduleMidiPhrase(ph, ss->SchedulerCurrentClick(), 0);
			// DO NOT free ph - scheduleMidiPhrase takes it over.
			return jsonOK(id);
		}

		// PARAMETER clickspersecond
		if (api == "set_clickspersecond") {
			int nclicks = jsonNeedInt(params, "clicks", -1);
			if (nclicks > 0) {
				ss->SetClicksPerSecond(nclicks);
			}
			return jsonOK(id);
		}
		if (api == "get_clickspersecond") {
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
			else {
				return jsonError(-32000, "No file parameter specified on set_midifile?", id);
			}
			return jsonOK(id);
		}
		if (api == "get_midifile") {
			return jsonStringResult(ss->_getMidiFile(), id);
		}

		// PARAMETER midioutput
		if (api == "set_midioutput") {
			int i = jsonNeedInt(params, "index", -1);
			if (i >= 0) {
				ss->_setMidiOutput(i);
			}
			return jsonOK(id);
		}
		if (api == "get_midioutput") {
			return jsonIntResult(ss->_getMidiOutput(), id);
		}

		if (api == "set_debugapi" ) {
			ss->m_debugApi = jsonNeedBool(params, "onoff", true);
			return jsonOK(id);
		}
		if (api == "get_debugapi" ) {
			return jsonIntResult(ss->m_debugApi,id);
		}
		if (api == "toggledebug" ) {
			ss->m_debugApi = !ss->m_debugApi;
			return jsonOK(id);
		}

		std::string err = NosuchSnprintf("VizServer - Unrecognized method '%s'", api.c_str());
		return jsonError(-32000, err, id);
	}

	VizServerApiCallback* cb = m_callbacks.findprefix(prefix);
	if (cb == NULL) {
		std::string err = NosuchSnprintf("Unable to find Json API match for prefix=%s", prefix.c_str());
		return jsonError(-32000, err, id);
	}

	jsoncallback_t jsoncb = (jsoncallback_t)(cb->m_cb);
	// NOTE: The interface in client DLLs uses char* rather than std::string
	const char *r = jsoncb(cb->m_data, api.c_str(), params, id);
	std::string result;
	if (r == NULL) {
		DEBUGPRINT(("HEY! NULL return from json callback!?"));
		result = jsonError(-32000, "NULL return from json callback", id);
	}
	else if (r[0] != '{') {
		DEBUGPRINT(("HEY! result from json callback doesn't start with '{' !?"));
		result = jsonError(-32000, "Result from json callback doesn't start with curly brace", id);
	}
	else {
		result = std::string(r);
	}
	return result;
}

//////////// VizServerOscProcessor

bool
checkAddrPattern(const char *addr, char *patt)
{
	return (strncmp(addr, patt, strlen(patt)) == 0);
}

class VizServerOscProcessor : public NosuchOscListener {

public:
	VizServerOscProcessor() {
		m_vizserver = NULL;
	}
	void setVizServer(VizServer* ss) {
		m_vizserver = ss;
	}
	void processOsc(const char* source, const osc::ReceivedMessage& m) {
		if ( m_vizserver == NULL ) {
			throw NosuchException("VizServerOscProcessor::processOsc called before VizServer is set!?");
			return;
		}
		const char *types = m.TypeTags();
		const char *addr = m.AddressPattern();
		int nargs = (types == NULL ? 0 : (int)strlen(types));

		std::string err;
		std::string prefix;
		std::string suffix;

		// DebugOscMessage("processOsc ",m);
		DEBUGPRINT1(("processOsc source=%s addr=%s",
			source == NULL ? "NULL?" : source, addr));

		if (checkAddrPattern(addr, "/tuio/25Dblb")) {

			std::string cmd = ArgAsString(m, 0);
			if (cmd == "alive") {
				// if ( nargs > 1 )
				DEBUGPRINT2(("25Dblb alive types=%s nargs=%d\n", types, nargs));
				for (int i = 1; i < nargs; i++) {
					int sidnum = ArgAsInt32(m, i);
					m_vizserver->_touchCursorSid(sidnum, source);
				}
				// m_vizserver->_checkCursorUp();
			}
			else if (cmd == "fseq") {
				// int seq = ArgAsInt32(m,1);
			}
			else if (cmd == "set") {
				int sidnum = ArgAsInt32(m, 1);
				double x = ArgAsFloat(m, 2);
				double y = ArgAsFloat(m, 3);
				double z = ArgAsFloat(m, 4);
				// double tuio_a = ArgAsFloat(m, 5);   // Angle
				// double tuio_w = ArgAsFloat(m, 6);
				// double tuio_h = ArgAsFloat(m, 7);
				double tuio_f = ArgAsFloat(m, 8);   // Area
				// y = 1.0f - y;

				m_vizserver->_setCursor(sidnum, source, NosuchPos(x, y, z), tuio_f, NULL, NULL);
			}
			return;

		}

		if (checkAddrPattern(addr, "/tuio/2Dcur")) {

			std::string cmd = ArgAsString(m, 0);
			if (cmd == "alive") {
				for (int i = 1; i < nargs; i++) {
					int sidnum = ArgAsInt32(m, i);
					// std::string sid = sidString(sidnum,source);
					m_vizserver->_touchCursorSid(sidnum, source);
				}
				// m_vizserver->_checkCursorUp();
			}
			else if (cmd == "fseq") {
				// int seq = ArgAsInt32(m,1);
			}
			else if (cmd == "set") {
				// It looks like "/tuio/2Dcur set s x y X Y m"
				int sidnum = ArgAsInt32(m, 1);
				// std::string sid = sidString(sidnum,source);

				double x = ArgAsFloat(m, 2);
				double y = ArgAsFloat(m, 3);
				double z = CURSOR_Z_UNSET;
				double tuio_f = CURSOR_AREA_UNSET;

				y = 1.0f - y;

				DEBUGPRINT1(("/tuio/2Dcur xy=%.3f,%.3f", x, y));

				m_vizserver->_setCursor(sidnum, source, NosuchPos(x, y, z), tuio_f, NULL, NULL);
			}
			return;
		}

		if (strncmp(addr, "/tuio/", 6) == 0) {
			DEBUGPRINT(("Ignoring OSC message with addr=%s", addr));
			return;
		}

		if (strncmp(addr, "/api", 4) == 0) {
			if (nargs <= 0) {
				return;
			}
			std::string fullmethod = ArgAsString(m, 0);
			std::string jsonstr = "{}";
			if (nargs > 1) {
				jsonstr = ArgAsString(m, 1);
			}
			cJSON* params = cJSON_Parse(jsonstr.c_str());
			if (!params) {
				NosuchErrorOutput("Unable to parse json in OSC /api: %s", jsonstr.c_str());
				return;
			}
			std::string s = m_vizserver->_processJson(fullmethod, params, "12345");
			return;
		}
		DEBUGPRINT(("Ignoring OSC message with addr=%s", addr));
	}
private:
	VizServer* m_vizserver;
};

//////////// VizServerMidiProcessor

class VizServerMidiProcessor : public NosuchMidiListener {
public:
	VizServerMidiProcessor() {
	}
	void processMidiMsg(MidiMsg* m) {
		std::map<void*, VizServerMidiCallback*>::iterator it;
		for (it = m_callbacks.begin(); it != m_callbacks.end(); it++) {
			VizServerMidiCallback* sscb = it->second;
			if (m->matches(sscb->m_mf)) {
				midicallback_t cb = (midicallback_t)(sscb->m_cb);
				cb(sscb->m_data, m);
			}
		}
	}
	void AddMidiCallback(void* handle, MidiFilter mf, midicallback_t cb, void* data) {
		m_callbacks.addcallback(handle, mf, (void*)cb, data);
	}
	void RemoveMidiCallback(void* handle) {
		m_callbacks.removecallback(handle);
	}
	int numCallbacks() {
		return m_callbacks.size();
	}
private:
	VizServerMidiCallbackMap m_callbacks;
};

//////////// VizServerCursorProcessor

class VizServerCursorProcessor : public CursorListener {
public:
	VizServerCursorProcessor() {
	}
	void processCursor(VizCursor* c, int downdragup) {
		try {
			std::map<void*, VizServerCursorCallback*>::iterator it;
			for (it = m_callbacks.begin(); it != m_callbacks.end(); it++) {
				VizServerCursorCallback* sscb = it->second;
				if (c->matches(sscb->m_cf)) {
					cursorcallback_t cb = (cursorcallback_t)(sscb->m_cb);
					cb(sscb->m_data, c, downdragup);
				}
			}
		}
		catch (NosuchException& e) {
			DEBUGPRINT(("NosuchException in processCursor: %s", e.message()));
		}
		catch (...) {
			// Does this really work?  Not sure
			DEBUGPRINT(("Some other kind of exception in processCursor occured!?"));
		}
	}
	void AddCursorCallback(void* handle, CursorFilter cf, cursorcallback_t cb, void* data) {
		m_callbacks.addcallback(handle, cf, (void*)cb, data);
	}
	void RemoveCursorCallback(void* handle) {
		m_callbacks.removecallback(handle);
	}
	int numCallbacks() { return m_callbacks.size(); }
private:
	VizServerCursorCallbackMap m_callbacks;
};

//////////// VizServerKeystrokeProcessor

class VizServerKeystrokeProcessor : public KeystrokeListener {
public:
	VizServerKeystrokeProcessor() {
	}
	void processKeystroke(int key, int downup) {
		try {
			std::map<void*, VizServerKeystrokeCallback*>::iterator it;
			for (it = m_callbacks.begin(); it != m_callbacks.end(); it++) {
				VizServerKeystrokeCallback* sscb = it->second;
				keystrokecallback_t cb = (keystrokecallback_t)(sscb->m_cb);
				cb(sscb->m_data, key, downup);
			}
		}
		catch (NosuchException& e) {
			DEBUGPRINT(("NosuchException in processKeystroke: %s", e.message()));
		}
		catch (...) {
			// Does this really work?  Not sure
			DEBUGPRINT(("Some other kind of exception in processKeystroke occured!?"));
		}
	}
	void AddKeystrokeCallback(void* handle, keystrokecallback_t cb, void* data) {
		m_callbacks.addcallback(handle, (void*)cb, data);
	}
	void RemoveKeystrokeCallback(void* handle) {
		m_callbacks.removecallback(handle);
	}
	int numCallbacks() { return m_callbacks.size(); }
private:
	VizServerKeystrokeCallbackMap m_callbacks;
};

//////////// VizServerClickProcessor

class VizServerClickProcessor : public ClickListener {
public:
	VizServerClickProcessor(VizServer* ss) {
		m_server = ss;
	}
	void processAdvanceClickTo(int click) {

		static int lastclick = -1;
		// Print warning if we get too much behind
		if (click > (lastclick + 4)) {
			DEBUGPRINT(("warning: advanced click by %d", click - lastclick));
		}
		lastclick = click;
		m_server->_advanceClickTo(click);

		try {
			std::map<void*, VizServerClickCallback*>::iterator it;
			for (it = m_callbacks.begin(); it != m_callbacks.end(); it++) {
				VizServerClickCallback* sscb = it->second;
				clickcallback_t cb = (clickcallback_t)(sscb->m_cb);
				cb(sscb->m_data, click);
			}
		}
		catch (NosuchException& e) {
			DEBUGPRINT(("NosuchException in processAdvanceClickTo: %s", e.message()));
		}
		catch (...) {
			// Does this really work?  Not sure
			DEBUGPRINT(("Some other kind of exception in processAdvanceClickTo occured!?"));
		}
	}
	void AddClickCallback(void* handle, clickcallback_t cb, void* data) {
		m_callbacks.addcallback(handle, (void*)cb, data);
	}
	void RemoveClickCallback(void* handle) {
		m_callbacks.removecallback(handle);
	}
	int numCallbacks() { return m_callbacks.size(); }
private:
	VizServerClickCallbackMap m_callbacks;
	VizServer* m_server;
};

//////////// VizServer

VizServer::VizServer() {

	VizParams::Initialize();

	m_started = false;
	m_debugApi = false;
	m_mmtt_seqnum = 0;
	m_htmlpath = _strdup(VizPath("html").c_str());
	m_midifile = "jsbach.mid";
	m_midioutput = -1;

	// m_jsonprocessor = NULL;
	// m_oscprocessor = NULL;
	// m_midiinputprocessor = NULL;
	// m_midioutputprocessor = NULL;
	// m_cursorprocessor = NULL;
	// m_keystrokeprocessor = NULL;
	// m_scheduler = NULL;

	m_jsonprocessor = new VizServerJsonProcessor();
	m_oscprocessor = new VizServerOscProcessor();
	m_midiinputprocessor = new VizServerMidiProcessor();
	m_midioutputprocessor = new VizServerMidiProcessor();
	m_cursorprocessor = new VizServerCursorProcessor();
	m_keystrokeprocessor = new VizServerKeystrokeProcessor();
	m_scheduler = new NosuchScheduler();

	m_daemon = NULL;
	m_cursors = new std::list<VizCursor*>();
	NosuchLockInit(&_cursors_mutex, "cursors");

	m_midi_input_list = "";
	m_midi_output_list = "";
	m_midi_merge_list = "";
	m_do_sharedmem = false;
	m_sharedmem_outlines = NULL;
	m_sharedmemname = "mmtt_outlines";
	m_do_errorpopup = false;
	m_do_ano = false;
	m_maxcallbacks = 0;

	m_osc_input_port = -1;
	m_osc_input_host = "";

	NosuchTimeInit();
}

VizServer::~VizServer() {
	DEBUGPRINT(("VizServer destructor called"));
	Stop();
}

VizCursor*
VizServer::_getCursor(int sidnum, std::string sidsource, bool lockit) {

	VizCursor* retc = NULL;

	if (lockit) {
		LockCursors();
	}
	for (std::list<VizCursor*>::iterator i = m_cursors->begin(); i != m_cursors->end(); i++) {
		VizCursor* c = *i;
		NosuchAssert(c);
		if (c->sid == sidnum && c->source == sidsource) {
			retc = c;
			break;
		}
	}
	if (lockit) {
		UnlockCursors();
	}
	return retc;
}

void VizServer::_checkCursorUp() {

	for (std::list<VizCursor*>::iterator i = m_cursors->begin(); i != m_cursors->end();) {
		VizCursor* c = *i;
		NosuchAssert(c);
		double dt = GetTime() - c->last_touched;
		// This timeout used to be 4ms, but when camera input
		// is being used, the frame rate can be slow, so 20ms
		// is more appropriate.
		if (dt > 100) {
			DEBUGPRINT1(("processing and erasing cursor c sid=%d  cursors size=%d", c->sid, m_cursors->size()));
			m_cursorprocessor->processCursor(c, CURSOR_UP);
			i = m_cursors->erase(i);
			delete c;
		}
		else {
			i++;
		}
	}
}

void
VizServer::AdvanceCursorTo(VizCursor* c, double tm) {
	c->advanceTo(tm);
}

void
VizServer::_setCursor(int sidnum, std::string sidsource, NosuchPos pos, double area, OutlineMem* om, MMTT_SharedMemHeader* hdr)
{
	LockCursors();
	VizCursor* c = _getCursor(sidnum, sidsource, false);
	if (c != NULL) {
		c->target_pos = pos;
		DEBUGPRINT1(("_setCursor c=%ld setting target_pos= %.3f %.3f", (long)c, c->target_pos.x, c->target_pos.y));
		c->pos = pos;
		c->area = area;
		c->outline = om;
		c->hdr = hdr;
		m_cursorprocessor->processCursor(c, CURSOR_DRAG);
	}
	else {
		c = new VizCursor(this, sidnum, sidsource, pos, area, om, hdr);
		DEBUGPRINT1(("_setCursor NEW c=%ld pos= %.3f %.3f", (long)c, pos.x, pos.y));
		m_cursors->push_back(c);
		m_cursorprocessor->processCursor(c, CURSOR_DOWN);
	}
	c->advanceTo(GetTime());
	c->touch(GetTime());
	UnlockCursors();
}

void VizServer::_setCursorSid(int sidnum, const char* source, double x, double y, double z, double tuio_f, OutlineMem* om, MMTT_SharedMemHeader* hdr) {
	_setCursor(sidnum, source, NosuchPos(x, y, z), tuio_f, om, hdr);
}

void VizServer::_processCursorsFromBuff(MMTT_SharedMemHeader* hdr) {
	buff_index b = hdr->buff_to_display;
	int ncursors = hdr->numoutlines[b];
	if (ncursors == 0) {
		return;
	}

	for (int n = 0; n < ncursors; n++) {
		OutlineMem* om = hdr->outline(b, n);
		_setCursorSid(om->sid, "sharedmem", om->x, om->y, om->z, om->area, om, hdr);
	}
}

void VizServer::_errorPopup(const char* msg) {
	MessageBoxA(NULL, msg, "Palette", MB_OK);
}

void VizServer::SetTime(double tm) {
	DEBUGPRINT(("VizServer:SetTime called, we're ignoring it!?   tm=%f", tm));
}

double VizServer::GetTime() {
	return NosuchTimeElapsed();
}

int VizServer::SchedulerTimestamp() {
	if (m_scheduler == NULL)
		return 0;
	return m_scheduler->m_timestamp;
}

click_t VizServer::SchedulerCurrentClick() {
	if (m_scheduler == NULL) {
		DEBUGPRINT(("In VizServer::CurrentClick, _scheduler==NULL?"));
		return 0;
	}
	else {
		return m_scheduler->CurrentClick();
	}
}

click_t VizServer::SchedulerClicksPerSecond() {
	return m_scheduler->ClicksPerSecond();
}

const char*
VizServer::ProcessJson(const char* fullmethod, cJSON* params, const char* id) {
	static std::string s;  // because we're returning its c_str()
	s = m_jsonprocessor->processJson(fullmethod, params, id);
	return s.c_str();
}

std::string
VizServer::_processJson(std::string fullmethod, cJSON* params, const char* id) {
	return m_jsonprocessor->processJson(fullmethod, params, id);
}

cJSON*
VizServer::_readconfig(const char* fn) {

	std::string fname = std::string(fn);
	std::string err;

	cJSON* json = jsonReadFile(fname, err);
	if (!json) {
		NosuchErrorOutput("Unable to read/parse config file (name=%s, err=%s), disabling Freeframe plugin!\n", fname.c_str(), err.c_str());
	}
	return json;
}

void
VizServer::InsertKeystroke(int key, int downup) {
	if (m_keystrokeprocessor) {
		m_keystrokeprocessor->processKeystroke(key, downup);
	}
}

int
VizServer::GetClicksPerSecond() {
	return m_scheduler->m_ClicksPerSecond;
}
void
VizServer::SendMidiMsg(MidiMsg* msg) {
	DEBUGPRINT(("Hi from VizServer::SendMidiMsg IS NOT DOING ANYTHING?"));
}

void
VizServer::SendControllerMsg(MidiMsg* m, void* handle, bool smooth) {
	m_scheduler->SendControllerMsg(m, handle, smooth);
}

void
VizServer::SendPitchBendMsg(MidiMsg* m, void* handle, bool smooth) {
	m_scheduler->SendPitchBendMsg(m, handle, smooth);
}

void
VizServer::IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle) {
	m_scheduler->IncomingNoteOff(clk, ch, pitch, vel, handle);
}

void
VizServer::IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle) {
	m_scheduler->IncomingMidiMsg(m, clk, handle);
}

void
VizServer::_scheduleMidiPhrase(MidiPhrase* ph, click_t clk, void* handle) {
	NosuchAssert(m_scheduler);
	m_scheduler->ScheduleMidiPhrase(ph, clk, handle);
}

void
VizServer::_scheduleMidiMsg(MidiMsg* m, click_t clk, void* handle) {
	NosuchAssert(m_scheduler);
	m_scheduler->ScheduleMidiMsg(m, clk, handle);
}

void
VizServer::_scheduleClear() {
	NosuchAssert(m_scheduler);
	m_scheduler->ScheduleClear();
}

void
VizServer::QueueMidiPhrase(MidiPhrase* ph, click_t clk) {
	NosuchAssert(m_scheduler);
	m_scheduler->QueueMidiPhrase(ph, clk);
}

void
VizServer::QueueMidiMsg(MidiMsg* m, click_t clk) {
	NosuchAssert(m_scheduler);
	m_scheduler->QueueMidiMsg(m, clk);
}

void
VizServer::QueueClear() {
	NosuchAssert(m_scheduler);
	m_scheduler->QueueClear();
}

void
VizServer::SetClicksPerSecond(int clicks) {
	NosuchAssert(m_scheduler);
	m_scheduler->SetClicksPerSecond(clicks);
}

void
VizServer::SetTempoFactor(float f) {
	m_scheduler->SetTempoFactor(f);
}

void
VizServer::ANO(int channel) {
	NosuchAssert(m_scheduler);
	m_scheduler->ANO(channel);
}

int
VizServer::NumCallbacks() {
	int nmidiin = m_midiinputprocessor ? m_midiinputprocessor->numCallbacks() : 0;
	int nmidiout = m_midioutputprocessor ? m_midioutputprocessor->numCallbacks() : 0;
	int njson = m_jsonprocessor ? m_jsonprocessor->numCallbacks() : 0;
	int ncursor = m_cursorprocessor ? m_cursorprocessor->numCallbacks() : 0;
	return nmidiin + nmidiout + njson + ncursor;
}

bool
VizServer::Start() {

	bool r = true;

	if (m_started) {
		DEBUGPRINT1(("VizServer::Start called - things are already running "));
		return true;
	}
	DEBUGPRINT(("VizServer::Start"));

	if (m_do_errorpopup) {
		NosuchErrorPopup = VizServer::_errorPopup;
	}
	else {
		NosuchErrorPopup = NULL;
	}
	try {
		std::string configpath = VizConfigPath("vizserver.json");
		DEBUGPRINT1(("configpath = %s", configpath.c_str()));

		cJSON* json = _readconfig(configpath.c_str());
		if (json == NULL) {
			DEBUGPRINT(("Unable to load config?  path=%s", configpath.c_str()));
		}
		else {
			_processServerConfig(json);
			// NOTE: DO NOT FREE json - some of the char* values in it get saved/used later.
		}

		m_scheduler->setPeriodicANO(m_do_ano);
		m_scheduler->SetMidiInputListener(m_midiinputprocessor);
		m_scheduler->SetMidiOutputListener(m_midioutputprocessor);

		m_oscprocessor->setVizServer(this);

		m_daemon = new NosuchDaemon(m_osc_input_port, m_osc_input_host, m_oscprocessor,
			m_httpport, m_htmlpath, m_jsonprocessor);

		m_started = true;

		_openSharedMemOutlines();

		if (m_midi_output_list == NULL) {
			DEBUGPRINT(("Warning: MIDI output wasn't defined in configuration!"));
			m_midi_output_list = "loopMIDI Port 1";
		}
		if (m_midi_input_list == NULL) {
			DEBUGPRINT(("Warning: MIDI input wasn't defined in configuration!"));
			m_midi_input_list = "loopMIDI Port";
		}
		m_clickprocessor = new VizServerClickProcessor(this);
		m_scheduler->SetClickProcessor(m_clickprocessor);

		m_scheduler->StartMidi(m_midi_input_list, m_midi_output_list, m_midi_merge_list);
	}
	catch (NosuchException& e) {
		DEBUGPRINT(("NosuchException: %s", e.message()));
		r = false;
	}
	catch (...) {
		// Does this really work?  Not sure
		DEBUGPRINT(("Some other kind of exception occured!?"));
		r = false;
	}
	return r;
}

void VizServer::_advanceClickTo(int current_click) {

	// XXX - should all of these things be done on EVERY click?
	_checkSharedMem();

	if (TryLockCursors() != 0) {
		return;
	}
	_checkCursorUp();

	for (std::list<VizCursor*>::iterator i = m_cursors->begin(); i != m_cursors->end(); i++) {
		VizCursor* c = *i;
		NosuchAssert(c);
		c->advanceTo(GetTime());
	}

	UnlockCursors();
}

void VizServer::_checkSharedMem() {

	_openSharedMemOutlines();

	MMTT_SharedMemHeader* mem = NULL;

	UT_SharedMem* outlines = _getSharedMemOutlines();
	if (!outlines) {
		return;
	}

	outlines->lock();

	void *data = outlines->getMemory();
	if (!data) {
		DEBUGPRINT(("VizServer:: NULL returned from getMemory of Shared Memory!  (B)"));
		goto getout;
	}

	mem = (MMTT_SharedMemHeader*)data;
	NosuchAssert(mem);

	if (mem->version != MMTT_SHM_VERSION_NUMBER) {
		DEBUGPRINT(("VizServer, MMTT sharedmem is the wrong version!  Expected %d, got %d", MMTT_SHM_VERSION_NUMBER, mem->version));
		outlines->unlock();
		_closeSharedMemOutlines();
		outlines = NULL;
		mem = NULL;
		goto getout;
	}

	// MMTT uses timeGetTime() values, so don't change this to use seconds
	long dt_milli = timeGetTime() - mem->lastUpdateTime;

	if (mem->seqnum < 0) {
		// If seqnum is negative, the shared memory
		// is being initialized (MMTT is probably doing
		// auto-alignment), so do nothing.
		DEBUGPRINT1(("VizServer, sharedmem is being initialized"));
		goto getout;
	}
	// Don't process the same frame twice
	if (mem->seqnum == m_mmtt_seqnum) {
		goto getout;
	}
	m_mmtt_seqnum = mem->seqnum;

	if (dt_milli > 10000) {
		DEBUGPRINT(("VizServer, sharedmem isn't being updated, closing it (seqnum=%d dt=%d)", mem->seqnum, dt_milli));
		outlines->unlock();
		_closeSharedMemOutlines();
		outlines = NULL;
		mem = NULL;
		goto getout;

	}

	mem->buff_to_display = BUFF_UNSET;
	if (mem->buff_to_display_next == BUFF_UNSET) {
		// Use the buffer that was displayed last time
		if (mem->buff_displayed_last_time == BUFF_UNSET) {
			DEBUGPRINT(("HEY!  Both buff_to_display_next and buff_displayed_last_time are UNSET??"));
			// Leave buff_to_display set to BUFF_UNSET
		}
		else {
			mem->buff_to_display = mem->buff_displayed_last_time;
		}
	}
	else {
		mem->buff_to_display = mem->buff_to_display_next;
		if (mem->buff_displayed_last_time != BUFF_UNSET) {
			mem->buff_inuse[mem->buff_displayed_last_time] = false;
		}
		mem->buff_displayed_last_time = mem->buff_to_display_next;
		mem->buff_to_display_next = BUFF_UNSET;
		// The buff_inuse flags are unchanged;
	}

	_processCursorsFromBuff(mem);

getout:
	if (outlines) {
		outlines->unlock();
	}
}

void
VizServer::Stop() {
	if (!m_started) {
		return;
	}
	m_started = false;
	DEBUGPRINT(("VizServer::Stop is shutting things down"));
	if (m_scheduler) {
		ANO();
		_scheduleClear();
		ClearNotesDown();
		m_scheduler->Stop();
		delete m_scheduler;
		m_scheduler = NULL;
	}
	//	_scheduler->StopMidi(_midi_input_name,_midi_output_name);
	if (m_daemon) { delete m_daemon; m_daemon = NULL; }
	if (m_midiinputprocessor) { delete m_midiinputprocessor; m_midiinputprocessor = NULL; }
	if (m_midioutputprocessor) { delete m_midioutputprocessor; m_midioutputprocessor = NULL; }
	if (m_jsonprocessor) { delete m_jsonprocessor; m_jsonprocessor = NULL; }
	if (m_oscprocessor) { delete m_oscprocessor; m_oscprocessor = NULL; }
	if (m_cursorprocessor) { delete m_cursorprocessor; m_cursorprocessor = NULL; }
	if (m_clickprocessor) { delete m_clickprocessor; m_clickprocessor = NULL; }

	_closeSharedMemOutlines();
}

void
VizServer::_openSharedMemOutlines()
{
	if (!m_do_sharedmem || m_sharedmem_outlines != NULL) {
		return;
	}
	if (!Pt_Started()) {
		return;
	}
	long now = Pt_Time();
	// Only check once in a while
	if ((now - m_sharedmem_last_attempt) < 5000) {
		return;
	}
	m_sharedmem_last_attempt = now;
	m_sharedmem_outlines = new UT_SharedMem(m_sharedmemname);
	UT_SharedMemError err = m_sharedmem_outlines->getErrorState();
	if (err != UT_SHM_ERR_NONE) {
		static long last_warning = 0;
		if (last_warning == 0 || (now - last_warning) > 60000) {
			last_warning = now;
			DEBUGPRINT(("Unable to open shared memory with name='%s' ?  Is MMTT running?  err=%d", m_sharedmemname, err));
		}
		_closeSharedMemOutlines();
	}
	else {
		DEBUGPRINT(("Successfully opened shared memory, name=%s", m_sharedmemname));
	}

}

void
VizServer::_closeSharedMemOutlines()
{
	if (m_sharedmem_outlines) {
		delete m_sharedmem_outlines;
		// DON'T set _do_sharedmem to false, we want
		// to keep trying to connect to the sharedmem
		// every once in a while.
		m_sharedmem_outlines = NULL;
	}
}

void
VizServer::_processServerConfig(cJSON* json) {
	cJSON *j;

	if ((j = jsonGetNumber(json, "periodicANO")) != NULL) {
		m_do_ano = (j->valueint != 0);
	}
	if ((j = jsonGetNumber(json, "sharedmem")) != NULL) {
		m_do_sharedmem = (j->valueint != 0);
	}
	if ((j = jsonGetString(json, "sharedmemname")) != NULL) {
		m_sharedmemname = j->valuestring;
	}
	if ((j = jsonGetString(json, "midiinput")) != NULL) {
		m_midi_input_list = j->valuestring;
	}
	if ((j = jsonGetString(json, "midimerge")) != NULL) {
		m_midi_merge_list = j->valuestring;
	}
	if ((j = jsonGetString(json, "midioutput")) != NULL) {
		m_midi_output_list = j->valuestring;
	}
	if ((j = jsonGetNumber(json, "errorpopup")) != NULL) {
		m_do_errorpopup = (j->valueint != 0);
	}
	if ((j = jsonGetNumber(json, "tuio")) != NULL) {
		DEBUGPRINT(("tuio value in palette.json no longer used.  Remove tuioport value to disable tuio"));
	}
	if ((j = jsonGetNumber(json, "tuioport")) != NULL) {
		m_osc_input_port = j->valueint;
	}
	if ((j = jsonGetString(json, "tuiohost")) != NULL) {
		m_osc_input_host = j->valuestring;
	}
	if ((j = jsonGetNumber(json, "httpport")) != NULL) {
		m_httpport = j->valueint;
	}
	if ((j = jsonGetString(json, "htmldir")) != NULL) {
		// There's undoubtedly a better way to check for a full path...
		char* jstr = j->valuestring;
		if (jstr[0] != '/' && jstr[0] != '\\' && jstr[1] != ':' ) {
			// It's not a full path
			m_htmlpath = _strdup(VizPath(jstr).c_str());
		}
		else {
			m_htmlpath = _strdup(j->valuestring);
		}
	}
	if ((j = jsonGetNumber(json, "debugtoconsole")) != NULL) {
		NosuchDebugToConsole = j->valueint ? TRUE : FALSE;
	}
	if ((j = jsonGetNumber(json, "debugmidinotes")) != NULL) {
		NosuchDebugMidiNotes = j->valueint ? TRUE : FALSE;
	}
	if ((j = jsonGetNumber(json, "debugmidiall")) != NULL) {
		NosuchDebugMidiAll = j->valueint ? TRUE : FALSE;
	}
	if ((j = jsonGetNumber(json, "debugtolog")) != NULL) {
		bool b = j->valueint ? TRUE : FALSE;
		// If we're turning debugtolog off, put a final
		// message out so we know that!
		if (NosuchDebugToLog && !b) {
			DEBUGPRINT(("ALERT: NosuchDebugToLog is being set to false!"));
		}
		NosuchDebugToLog = b;
	}
	if ((j = jsonGetNumber(json, "debugautoflush")) != NULL) {
		NosuchDebugAutoFlush = j->valueint ? TRUE : FALSE;
	}
}

VizServer*
VizServer::GetServer() {
	if (OneServer == NULL) {
		OneServer = new VizServer();
		DEBUGPRINT(("NEW VizServer!  Setting OneServer"));
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
	if (ncb > m_maxcallbacks) {
		m_maxcallbacks = ncb;
	}
}

bool
VizServer::IsVizlet(const char* iname) {
	return m_jsonprocessor->HasApiPrefix(iname);
}

const char *
VizServer::VizTags() {
	static std::string s;
	s = m_jsonprocessor->VizTags();
	return s.c_str();
}

void
VizServer::ChangeVizTag(void* handle, const char* p) {
	m_jsonprocessor->ChangeVizTag(handle, p);
}

void
VizServer::AddJsonCallback(void* handle, const char* apiprefix, jsoncallback_t callback, void* data) {
	m_jsonprocessor->AddJsonCallback(handle, apiprefix, callback, data);
	_setMaxCallbacks();
}

void
VizServer::AddMidiInputCallback(void* handle, MidiFilter mf, midicallback_t callback, void* data) {
	m_midiinputprocessor->AddMidiCallback(handle, mf, callback, data);
	_setMaxCallbacks();
}

void
VizServer::AddMidiOutputCallback(void* handle, MidiFilter mf, midicallback_t callback, void* data) {
	m_midioutputprocessor->AddMidiCallback(handle, mf, callback, data);
	_setMaxCallbacks();
}

void
VizServer::AddCursorCallback(void* handle, CursorFilter cf, cursorcallback_t callback, void* data) {
	m_cursorprocessor->AddCursorCallback(handle, cf, callback, data);
	_setMaxCallbacks();
}

void
VizServer::AddKeystrokeCallback(void* handle, keystrokecallback_t callback, void* data) {
	m_keystrokeprocessor->AddKeystrokeCallback(handle, callback, data);
	_setMaxCallbacks();
}

void
VizServer::AddClickCallback(void* handle, clickcallback_t callback, void* data) {
	m_clickprocessor->AddClickCallback(handle, callback, data);
	_setMaxCallbacks();
}

void
VizServer::RemoveJsonCallback(void* handle) {
	if (m_jsonprocessor) {
		m_jsonprocessor->RemoveJsonCallback(handle);
	}
}

void
VizServer::RemoveMidiInputCallback(void* handle) {
	if (m_midiinputprocessor) {
		m_midiinputprocessor->RemoveMidiCallback(handle);
	}
}

void
VizServer::RemoveMidiOutputCallback(void* handle) {
	if (m_midioutputprocessor) {
		m_midioutputprocessor->RemoveMidiCallback(handle);
	}
}

void
VizServer::RemoveCursorCallback(void* handle) {
	if (m_cursorprocessor) {
		m_cursorprocessor->RemoveCursorCallback(handle);
	}
}

void
VizServer::RemoveKeystrokeCallback(void* handle) {
	if (m_keystrokeprocessor) {
		m_keystrokeprocessor->RemoveKeystrokeCallback(handle);
	}
}

void
VizServer::RemoveClickCallback(void* handle) {
	if (m_clickprocessor) {
		m_clickprocessor->RemoveClickCallback(handle);
	}
}

void
VizServer::_touchCursorSid(int sid, std::string source) {
	VizCursor* c = _getCursor(sid, source, true);
	if (c) {
		c->touch(GetTime());
	}
}

VizCursor::VizCursor(VizServer* ss, int sid_, std::string source_,
	NosuchPos pos_, double area_, OutlineMem* om_, MMTT_SharedMemHeader* hdr_) {

	pos = pos_;
	target_pos = pos_;
	m_vizserver = ss;
	last_touched = 0;
	sid = sid_;
	source = source_;
	area = area_;
	outline = om_;
	hdr = hdr_;
	region = NULL;
	curr_speed = 0;
	curr_degrees = 0;

	m_target_depth = 0;
	m_last_depth = 0;
	m_last_channel = 0;
	m_last_click = 0;
	m_last_tm = 0;
	m_target_degrees = 0;
	m_g_firstdir = true;
	m_smooth_degrees_factor = 0.2;

	touch(ss->GetTime());
}

double
normalize_degrees(double d) {
	if (d < 0.0f) {
		d += 360.0f;
	}
	else if (d > 360.0f) {
		d -= 360.0f;
	}
	return d;
}

void
VizCursor::advanceTo(double tm) {

	double dt = tm - m_last_tm;
	if (dt <= 0) {
		return;
	}
	DEBUGPRINT1(("    Cursor %ld advance start pos= %.3f %.3f   target= %.3f %.3f", (long)this, pos.x, pos.y, target_pos.x, target_pos.y));

	// If _pos and _g_smoothedpos are the same (x and y, not z), then there's nothing to smooth
	if (pos.x == target_pos.x && pos.y == target_pos.y) {
		DEBUGPRINT1(("VizCursor::advanceTo, current and target are the same"));
		return;
	}

	NosuchPos dpos = target_pos.sub(pos);
	double raw_distance = dpos.mag();
	if (raw_distance > 1.0f) {
		DEBUGPRINT(("VizCursor::advanceTo, raw_distance>1.0 !?"));
		return;
	}
	if (raw_distance == 0.0f) {
		DEBUGPRINT(("VizCursor::advanceTo, raw_distance=0.0 !?"));
		return;
	}

	double this_speed = raw_distance / dt;
	double speed_limit = 60.0f;
	if (this_speed > speed_limit) {
		DEBUGPRINT(("Speed LIMIT (%f) EXCEEDED, throttled to %f", this_speed, speed_limit));
		this_speed = speed_limit;
	}

	dpos = dpos.normalize();

	double sfactor = 0.02f;
	// speed it up a bit when the distance gets larger
	if (raw_distance > 0.4f) {
		sfactor = 0.1f;
	}
	else if (raw_distance > 0.2f) {
		sfactor = 0.05f;
	}
	this_speed = this_speed * sfactor;

	double dspeed = this_speed - curr_speed;
	double smooth_speed_factor = 0.1f;
	curr_speed = curr_speed + dspeed * smooth_speed_factor;
	dpos = dpos.mult(raw_distance * curr_speed);
	m_last_pos = pos;
	pos = pos.add(dpos);
	if (pos.x > 1.0f) {
		DEBUGPRINT(("VizCursor::advanceTo, x>1.0 !?"));
	}
	if (pos.y > 1.0f) {
		DEBUGPRINT(("VizCursor::advanceTo, y>1.0 !?"));
	}

	DEBUGPRINT1(("   Cursor advance end pos= %.3f %.3f", pos.x, pos.y));

	NosuchPos finaldpos = pos.sub(m_last_pos);
	double final_distance = finaldpos.mag();

	/////////////// smooth the depth
	double depthsmoothfactor = 0.3f;
	double smoothdepth = depth() + ((target_depth() - depth())*depthsmoothfactor);

	setdepth(smoothdepth);

	/////////////// smooth the degrees
	double tooshort = 0.01f; // 0.05f;
	if (raw_distance < tooshort) {
		// NosuchDebug("   raw_distance=%.3f too small %s\n",
		// 	raw_distance,DebugString().c_str());
	}
	else {
		NosuchPos dp = pos.sub(m_last_pos);
		double heading = dp.heading();
		m_target_degrees = radian2degree(heading);
		m_target_degrees += 90.0;
		m_target_degrees = normalize_degrees(m_target_degrees);

		if (m_g_firstdir) {
			curr_degrees = m_target_degrees;
			m_g_firstdir = false;
		}
		else {
			double dd1 = m_target_degrees - curr_degrees;
			double dd;
			if (dd1 > 0.0f) {
				if (dd1 > 180.0f) {
					dd = -(360.0f - dd1);
				}
				else {
					dd = dd1;
				}
			}
			else {
				if (dd1 < -180.0f) {
					dd = dd1 + 360.0f;
				}
				else {
					dd = dd1;
				}
			}
			curr_degrees = curr_degrees + (dd*m_smooth_degrees_factor);

			curr_degrees = normalize_degrees(curr_degrees);
		}
	}
	m_last_tm = tm;
}
