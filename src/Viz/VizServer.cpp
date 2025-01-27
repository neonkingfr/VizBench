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

#include "VizUtil.h"
#include <VizServer.h>
#include <VizParams.h>
#include <MidiVizParams.h>
#include <SpriteVizParams.h>
#include <VizUtil.h>
#include <VizJSON.h>
#include <VizException.h>
#include <VizOscManager.h>
#include <VizDaemon.h>
#include <VizScheduler.h>
#include "UT_SharedMem.h"
#include "FFFF.h"

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
	VizServerApiCallback(const char* viztag, void* cb, void* data) {
		m_cb = cb;
		m_data = data;
		m_viztag = NULL;
		setViztag(viztag);
		DEBUGPRINT1(("NEW VIZSERVER API CALLBACK"));
	}
	virtual ~VizServerApiCallback() {
		DEBUGPRINT1(("DELETING VIZSERVER API CALLBACK"));
		free((void*)m_viztag);
	}
	void setViztag(const char* viztag) {
		if (m_viztag != NULL) {
			DEBUGPRINT1(("Freeing m_viztag = %s",m_viztag));
			// There used to be a crash here, usually due to
			// to some cross VizServer API confusion/bug with std::string memory
			free((void*)m_viztag);
		}
		m_viztag = _strdup(viztag);
		for (char* s = m_viztag; *s; s++) {
			*s = tolower(*s);
		}
	}
	void* m_cb;
	void* m_data;
	char* m_viztag;
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
	void ClearCallbacks() {
		DEBUGPRINT(("VIZSERVER API CALLBACK IS BEING CLEARED!"));
		this->clear();
	}
	void ClearCallbacksWithPrefix(std::string prefix) {
		DEBUGPRINT1(("ClearCallbacksWithPrefix prefix=%s", prefix.c_str()));
		VizServerApiCallback* cb;
		while ((cb = findprefixcallback(prefix)) != NULL) {
			removecallback(cb->m_cb);
		}
		// this->clear();
	}
	void addcallback(void* handle, const char* apiprefix, void* cb, void* data) {
		DEBUGPRINT1(("VizServerApiCallbackMap addcallback handle=%ld prefix=%s", (long)handle, apiprefix));
		if (count(handle) > 0) {
			if (strcmp((*this)[handle]->m_viztag, apiprefix) != 0) {
				DEBUGPRINT(("Hey! VizServerApiCallbackMap::addcallback finds existing callback for handle=%ld with different prefix=%s", (long)handle, apiprefix));
			}
			else {
				// Adding the same callback with the same handle and apiprefix
				DEBUGPRINT(("Hey! VizServerApiCallbackMap::addcallback finds existing callback with identical handle=%ld prefix=%s", (long)handle, apiprefix));
			}
			return;
		}
		// Special case - if apiprefix is "", we're adding an API callback for a Vizlet (or something)
		// that hasn't set its viztag explicitly, yet.
		if (std::string(apiprefix) != "") {
			void* findhandle = NULL;
			VizServerApiCallback* findcb = findprefixcallback(apiprefix, &findhandle);
			if (findcb != NULL) {
				if (findhandle != handle) {
					DEBUGPRINT(("Hey!? VizServerApiCallbackMap::addcallback finds existing apiprefix for handle=%ld prefix=%s", (long)handle, apiprefix));
				}
				else {
					DEBUGPRINT(("Hey! VizServerApiCallbackMap alread has apiprefix for handle=%ld prefix=%s", (long)handle, apiprefix));
				}
				return;
			}
		}
		VizServerApiCallback* sscb = new VizServerApiCallback(apiprefix, cb, data);
		(*this)[handle] = sscb;
		DEBUGPRINT1(("VizServerApiCallbackMap inserted handle=%ld size=%d", (long)handle, size()));
	}
	void removecallback(void* handle) {
		DEBUGPRINT1(("API CALLBACKMAP removecallback handle=%ld",(long)handle));
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::removecallback didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		erase(handle);
	}
	VizServerApiCallback* findprefixcallback(std::string prefix, void** phandle = NULL) {
		std::map<void*, VizServerApiCallback*>::iterator it;
		// All comparisons here are case-insensitive
		prefix = VizToLower(prefix);
		for (it = this->begin(); it != this->end(); it++) {
			VizServerApiCallback* sscb = it->second;
			std::string s = VizToLower(std::string(sscb->m_viztag));
			if (VizBeginsWith(s,prefix)) {
				if (phandle) {
					*phandle = it->first;
				}
				return sscb;
			}
		}
		return NULL;
	}
	VizServerApiCallback* findviztagcallback(std::string viztag, void** phandle = NULL) {
		std::map<void*, VizServerApiCallback*>::iterator it;
		// All comparisons here are case-insensitive
		viztag = VizToLower(viztag);
		for (it = this->begin(); it != this->end(); it++) {
			VizServerApiCallback* sscb = it->second;
			if (VizToLower(std::string(sscb->m_viztag)) == viztag) {
				if (phandle) {
					*phandle = it->first;
				}
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
			std::string tag = std::string(sscb->m_viztag);
			// if (tag != "UNTAGGED" && tag != "ffff") {
			if (tag != "UNTAGGED" ) {
				s += ("," + tag);
			}
		}
		return s.c_str();
	}

	void ChangeVizTag(void* handle, const char* viztag) {
		if (count(handle) <= 0) {
			DEBUGPRINT(("Hey! VizServercallbackMap::ChangeVizTag didn't find existing callback for handle=%ld", (long)handle));
			return;
		}
		DEBUGPRINT1(("ChangeVizTag handle=%ld  viztag=%s",(long)handle,viztag));
		VizServerApiCallback* sscb = (*this)[handle];
		// Should we free prefix?  I tried freeing it in the destructor
		// of ApiFilter, and it corrupted the heap, so I'm leary.
		sscb->setViztag(viztag);
	}
};

class VizServerCursorCallbackMap : public std::map < void*, VizServerCursorCallback* > {
public:
	VizServerCursorCallbackMap() {
	}
	void addcallback(void* handle, CursorFilter cf, void* cb, void* data) {
		if (count(handle) > 0) {
			DEBUGPRINT(("Hey! VizServerCursorCallbackMap::addcallback finds existing callback for handle=%ld", (long)handle));
			return;
		}
		VizServerCursorCallback* sscb = new VizServerCursorCallback(cf, cb, data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServerCursorCallbackMap::removecallback didn't find existing callback for handle=%ld", (long)handle));
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
			DEBUGPRINT(("Hey! VizServerMidicallbackMap::addcallback finds existing callback for handle=%d", (long)handle));
			return;
		}
		VizServerMidiCallback* sscb = new VizServerMidiCallback(mf, cb, data);
		(*this)[handle] = sscb;
	}
	void removecallback(void* handle) {
		if (count(handle) == 0) {
			DEBUGPRINT(("Hey! VizServerMidicallbackMap::removecallback didn't find existing callback for handle=%d", (long)handle));
			return;
		}
		erase(handle);
	}
};

//////////// VizServerJsonProcessor

class VizServerJsonProcessor : public VizJsonListener {

public:
	VizServerJsonProcessor() {
		VizLockInit(&_mutex, "vizserverjsonprocessor");
	}
	void LockJsonProcessor() {
		VizLock(&_mutex,"jsonprocessor");
	}
	void UnlockJsonProcessor() {
		VizUnlock(&_mutex,"jsonprocessor");
	}
	void ClearCallbacks() {
		m_callbacks.ClearCallbacks();
	}
	void ClearCallbacksWithPrefix(std::string prefix) {
		m_callbacks.ClearCallbacksWithPrefix(prefix);
	}
	void AddJsonCallback(void* handle, const char* apiprefix, jsoncallback_t cb, void* data) {
		DEBUGPRINT1(("AddJsonCallback apiprefix=%s",apiprefix));
		m_callbacks.addcallback(handle, apiprefix, (void*)cb, data);
	}
	void ChangeVizTag(void* handle, const char* p) {
		m_callbacks.ChangeVizTag(handle, p);
	}
	void RemoveJsonCallback(void* handle) {
		m_callbacks.removecallback(handle);
	}
	virtual std::string processJson(std::string method, cJSON* params, const char *id);
	virtual std::string processJsonReal(std::string method, cJSON* params, const char *id);
	int numCallbacks() { return m_callbacks.size(); }
	const char* VizTags() { return m_callbacks.VizTags(); }
	bool HasApiPrefix(std::string nm) {
		return (m_callbacks.findprefixcallback(nm) != NULL);
	}

private:
	VizServerApiCallbackMap m_callbacks;
	pthread_mutex_t _mutex;
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
	prefix = VizToLower(prefix);
}

static std::string
ApiSpecificVizParamsPath(std::string api, std::string file) {
	// NOTE: depends on the API naming convention of *param_writefile
	if (api.substr(0, 6) == "sprite") {
		return SpriteVizParamsPath(file);
	}
	else {
		return MidiVizParamsPath(file);
	}
}

std::string
VizServerJsonProcessor::processJson(std::string fullmethod, cJSON *params, const char* id) {
	// LockJsonProcessor();
	VizServer* ss = VizServer::GetServer();
	if (ss->m_debugApi) {
		char *p = cJSON_PrintUnformatted(params);
		DEBUGPRINT(("VizServerJsonProcessor API request: %s %s", fullmethod.c_str(),p));
		cJSON_free(p);
	}
	std::string r = processJsonReal(fullmethod, params, id);
	if (ss->m_debugApi) {
		DEBUGPRINT(("VizServerJsonProcessor API response: %s", r.c_str()));
	}
	// UnlockJsonProcessor();
	return r;
}

std::string
VizServerJsonProcessor::processJsonReal(std::string fullmethod, cJSON *params, const char* id) {

	std::string prefix;
	std::string api;
	VizServer* ss = VizServer::GetServer();

	// XXX - the server has only a single static instance of the various params,
	//       so multiple clients working simultaneously will probably get very confused
	static SpriteVizParams* spriteparams = NULL;
	if (spriteparams == NULL) {
		spriteparams = new SpriteVizParams();
	}

	static MidiVizParams* midiparams = NULL;
	if (midiparams == NULL) {
		midiparams = new MidiVizParams();
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
			return jsonStringResult(
				"clearmidi;"
				"allnotesoff;"
				"play_midifile;"
				"set_midifile(file);"
				"set_clickspersecond(clicks);"
				"set_midioutput(index);"
				"set_debugmidinotes(onoff);"
				"set_debugmidiall(onoff);"
				"set_debugapi(onoff);"
				"set_debughttp(onoff);"
				"set_showfps(onoff);"
				"toggledebug", id);
		}
		if (api == "description") {
			return jsonStringResult("Server which distributes things to Viz plugins", id);
		}
		if (api == "about") {
			return jsonStringResult("by Tim Thompson - me@timthompson.com", id);
		}
		if (api == "clearmidi") {
			ss->ANO();
			ss->_scheduleClearAll();
			ss->ClearNotesDown();
			return jsonOK(id);
		}
		if (api == "allnotesoff") {
			ss->ANO();
			ss->ClearNotesDown();
			return jsonOK(id);
		}
		if (api == "spriteparam_vals") {
			std::string s = spriteparams->JsonListOfValues();
			return jsonStringResult(s, id);
		}
		if (api == "midiparam_vals") {
			std::string s = midiparams->JsonListOfValues();
			return jsonStringResult(s, id);
		}
		if (api == "spriteparam_stringvals") {
			std::string type = jsonNeedString(params, "type", "");
			if (type == ""){
				return jsonError(-32000, "No type parameter specified on spriteparam_stringvals?", id);
			}
			std::string s = spriteparams->JsonListOfStringValues(type);
			return jsonStringResult(s, id);
		}
		if (api == "midiparam_stringvals") {
			std::string type = jsonNeedString(params, "type", "");
			if (type == ""){
				return jsonError(-32000, "No type parameter specified on midiparam_stringvals?", id);
			}
			std::string s = midiparams->JsonListOfStringValues(type);
			return jsonStringResult(s, id);
		}
		if (api == "spriteparam_list") {
			std::string s = spriteparams->JsonListOfParams();
			return jsonStringResult(s, id);
		}
		if (api == "midiparam_list") {
			std::string s = midiparams->JsonListOfParams();
			return jsonStringResult(s, id);
		}
		if (api == "spriteparam_readfile" || api == "midiparam_readfile") {
			std::string file = jsonNeedString(params, "paramfile", "");
			if (file == "") {
				return jsonError(-32000, "No paramfile parameter specified on *param_readfile?", id);
			}
			std::string fpath = ApiSpecificVizParamsPath(api,file);
			bool exists = VizFileExists(fpath);
			if (!exists) {
				// If it's a file that doesn't exist,
				// make a copy of the current one
				throw VizException("File doesn't exist - fpath=%s", fpath.c_str());
			}
			std::string err;
			cJSON* json = jsonReadFile(fpath, err);
			if (!json){
				throw VizException("Unable to read json from fpath=%s err=%s", fpath.c_str(), err.c_str());
			}
			// std::string r = cJSON_escapestring(cJSON_PrintUnformatted(json));
			std::string r = cJSON_PrintUnformatted(json);
			return jsonStringResult(r, id);
		}
		if (api == "spriteparam_writefile" || api == "midiparam_writefile") {
			std::string file = jsonNeedString(params, "paramfile", "");
			if (file == "") {
				return jsonError(-32000, "No paramfile parameter specified on *param_writefile?", id);
			}
			cJSON* j = jsonNeedJSON(params, "contents");
			std::string fpath = ApiSpecificVizParamsPath(api,file);
			std::string err;
			if (jsonWriteFile(fpath, j, err)) {
				return jsonOK(id);
			}
			else {
				std::string msg = VizSnprintf("Unable to write file %s, err=%s",
					fpath.c_str(), err.c_str());
				return jsonError(-32000, msg.c_str(), id);
			}
		}
		if (api == "play_midifile") {
			std::string filename = ss->_getMidiFile();
			if (filename == "") {
				std::string err = VizSnprintf("play_midifile called with no midifile set, use set_midifile first");
				return jsonError(-32000, err, id);
			}
			std::string fpath = VizPath("midifiles\\" + filename);
			MidiPhrase* ph = newMidiPhraseFromFile(fpath);
			if (ph == NULL) {
				std::string err = VizSnprintf("Error reading phrase from file: %s", fpath.c_str());
				return jsonError(-32000, err, id);
			}
			ss->_scheduleMidiPhrase(ph, ss->SchedulerCurrentClick(), 0, false, NULL);
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
		if (api == "get_debugmidinotes" ) {
			return jsonIntResult(VizDebugMidiNotes,id);
		}
		if (api == "set_debugmidinotes" ) {
			VizDebugMidiNotes = jsonNeedBool(params, "onoff");
			return jsonOK(id);
		}
		if (api == "get_debugmidiall" ) {
			return jsonIntResult(VizDebugMidiAll,id);
		}
		if (api == "set_debugmidiall" ) {
			VizDebugMidiAll = jsonNeedBool(params, "onoff");
			return jsonOK(id);
		}
		if (api == "set_debughttp" ) {
			ss->m_daemon->DebugRequests(jsonNeedBool(params, "onoff"));
			return jsonOK(id);
		}
		if (api == "get_debughttp" ) {
			return jsonIntResult(ss->m_daemon->DebugRequests(),id);
		}

		if (api == "toggledebug" ) {
			ss->m_debugApi = !ss->m_debugApi;
			return jsonOK(id);
		}

		std::string err = VizSnprintf("VizServer - Unrecognized method '%s'", api.c_str());
		return jsonError(-32000, err, id);
	}

	VizServerApiCallback* cb = m_callbacks.findprefixcallback(prefix);
	if (cb == NULL) {
		std::string err = VizSnprintf("Unable to find Json API match for prefix=%s", prefix.c_str());
		return jsonError(-32000, err, id);
	}

	jsoncallback_t jsoncb = (jsoncallback_t)(cb->m_cb);

	// NOTE: The interface in client DLLs uses char* rather than std::string
	// AND: it's responsibility of the caller (here) to free the returned value
	const char *result = jsoncb(cb->m_data, api.c_str(), params, id);
	std::string s;
	if (result == NULL) {
		DEBUGPRINT(("HEY! NULL return from json callback!?"));
		s = jsonError(-32000, "NULL return from json callback", id);
		result = _strdup(s.c_str());
	}
	else if (result[0] != '{') {
		DEBUGPRINT(("HEY! result from json callback doesn't start with '{' !?  result=%s",result));
		// DO NOT FREE IT!  This is a sign of corruption somewhere...
		// free((void*)result);
		s = jsonError(-32000, "Result from json callback doesn't start with curly brace", id);
		result = _strdup(s.c_str());
	}
	else {
		// leave the value in result alone
	}
	return result;
}

//////////// VizServerOscProcessor

bool
checkAddrPattern(const char *addr, char *patt)
{
	return (strncmp(addr, patt, strlen(patt)) == 0);
}

class VizServerOscProcessor : public VizOscListener {

public:
	VizServerOscProcessor() {
		m_vizserver = NULL;
	}
	void setVizServer(VizServer* ss) {
		m_vizserver = ss;
	}
	void processOsc(const char* source, const osc::ReceivedMessage& m) {
		if ( m_vizserver == NULL ) {
			throw VizException("VizServerOscProcessor::processOsc called before VizServer is set!?");
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

		bool is25Dblb = (checkAddrPattern(addr, "/tuio/25Dblb"));
		bool is25Dcur = (checkAddrPattern(addr, "/tuio/25Dcur"));

		if (is25Dblb || is25Dcur) {

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
				double area;
				if (is25Dblb) {
					area = ArgAsFloat(m, 8);   // Area
				}
				else {
					area = z;
				}
				// y = 1.0f - y;
				m_vizserver->_setCursor(sidnum, source, VizPos(x, y, z), area, NULL, NULL);
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

				m_vizserver->_setCursor(sidnum, source, VizPos(x, y, z), tuio_f, NULL, NULL);
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
				VizErrorOutput("Unable to parse json in OSC /api: %s", jsonstr.c_str());
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

class VizServerMidiProcessor : public VizMidiListener {
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
			DEBUGPRINT1(("processCursor A"));
			std::map<void*, VizServerCursorCallback*>::iterator it;
			for (it = m_callbacks.begin(); it != m_callbacks.end(); it++) {
				VizServerCursorCallback* sscb = it->second;
				if (c->matches(sscb->m_cf)) {
					// XXX - need to check if plugin is enabled
					cursorcallback_t cb = (cursorcallback_t)(sscb->m_cb);
					DEBUGPRINT1(("processCursor B"));
					cb(sscb->m_data, c, downdragup);
					DEBUGPRINT1(("processCursor C"));
				}
			}
			DEBUGPRINT1(("processCursor D"));
		}
		catch (VizException& e) {
			DEBUGPRINT(("VizException in processCursor: %s", e.message()));
		}
		catch (...) {
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
		catch (VizException& e) {
			DEBUGPRINT(("VizException in processKeystroke: %s", e.message()));
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
		if (click > (lastclick + 10)) {
			DEBUGPRINT(("warning: advanced click by %d,  click is now %d", click - lastclick,  click));
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
		catch (VizException& e) {
			DEBUGPRINT(("VizException in processAdvanceClickTo: %s", e.message()));
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

	m_errorCallback = NULL;
	m_errorCallbackData = NULL;

	VizParams::Initialize();

	m_started = false;
	m_debugApi = false;
	m_mmtt_seqnum = 0;
	m_htmlpath = _strdup(VizPath("html").c_str());
	m_midifile = "jsbach.mid";
	m_midioutput = -1;
	m_next_cursorid = 1;

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
	m_scheduler = new VizScheduler();

	m_daemon = NULL;
	m_cursors = new std::list<VizCursor*>();
	VizLockInit(&_cursors_mutex, "cursors");
	VizLockInit(&_vizserver_mutex, "vizserver");

	// m_midi_input_list = "";
	// m_midi_output_list = "";
	// m_midi_merge = "";
	m_do_sharedmem = false;
	m_sharedmem_outlines = NULL;
	m_sharedmemname = "mmtt_outlines";
	m_do_ano = false;
	m_maxcallbacks = 0;

	m_osc_input_port = -1;
	m_osc_input_host = "";

	VizTimeInit();
}

VizServer::~VizServer() {
	DEBUGPRINT(("VizServer destructor called"));
	Stop();
}

void
VizServer::DoFFFF(const char* pipeset) {
	try {
		CATCH_NULL_POINTERS;

		FFFF* F = new FFFF();
		F->StartStuff();
		F->LoadPipeset(pipeset);
		F->RunStuff();
		F->StopStuff();

	} catch (VizException& e) {
		VizErrorOutput("VizException!! - %s",e.message());
		exit(EXIT_FAILURE);
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		VizErrorOutput("Some other kind of exception occured in main!?");
		exit(EXIT_FAILURE);
	}

}

void
VizServer::SetErrorCallback(ErrorCallbackFuncType cb, void* data) {
	m_errorCallback = cb;
	m_errorCallbackData = data;
}

bool
VizServer::SendOsc(const char* host, int port, const char *data, int leng) {
	return SendToUDPServer(host, port, data, leng);
}

VizCursor*
VizServer::_getCursor(int sidnum, std::string sidsource, bool lockit) {

	VizCursor* retc = NULL;

	if (lockit) {
		LockCursors();
	}
	for (std::list<VizCursor*>::iterator i = m_cursors->begin(); i != m_cursors->end(); i++) {
		VizCursor* c = *i;
		VizAssert(c);
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
		VizAssert(c);
		double now = SchedulerCurrentTimeInSeconds();
		double dt = SchedulerCurrentTimeInSeconds() - c->m_lasttouchedInSeconds;
		// This timeout used to be 4ms, but when camera input
		// is being used, the frame rate can be slow, so 20ms
		// is more appropriate.  With other stuff (e.g. HTTP processing, etc)
		// I've made it even longer (100 ms) to avoid spurious CURSOR_UPs.
		// XXX - Probably should make this a global parameter.
		if (dt > 0.100) {
			DEBUGPRINT1(("doing CURSOR_UP cursor c sid=%d  cursors size=%d", c->sid, m_cursors->size()));
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
VizServer::_setCursor(int sidnum, std::string sidsource, VizPos pos, double area, OutlineMem* om, MMTT_SharedMemHeader* hdr)
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
		int cursorid = m_next_cursorid++;
		c = new VizCursor(this, sidnum, sidsource, cursorid, pos, area, om, hdr);
		DEBUGPRINT1(("_setCursor NEW id=%d c=%ld pos= %.3f %.3f", cursorid, (long)c, pos.x, pos.y));
		m_cursors->push_back(c);
		m_cursorprocessor->processCursor(c, CURSOR_DOWN);
	}
	double seconds = SchedulerCurrentTimeInSeconds();
	c->advanceTo(seconds);
	c->touch(seconds);
	UnlockCursors();
}

void VizServer::_setCursorSid(int sidnum, const char* source, double x, double y, double z, double tuio_f, OutlineMem* om, MMTT_SharedMemHeader* hdr) {
	_setCursor(sidnum, source, VizPos(x, y, z), tuio_f, om, hdr);
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

double VizServer::SchedulerCurrentTimeInSeconds() {
	return VizTimeElapsedInSeconds();
}

click_t VizServer::SchedulerCurrentClick() {
	if (m_scheduler == NULL) {
		return 0;
	}
	else {
		return m_scheduler->CurrentClick();
	}
}

click_t VizServer::SchedulerClicksPerSecond() {
	return m_scheduler->ClicksPerSecond();
}

click_t VizServer::SchedulerClicksPerBeat() {
	return m_scheduler->ClicksPerBeat();
}

const char*
VizServer::ProcessJson(const char* fullmethod, cJSON* params, const char* id) {
	static std::string s;  // because we're returning its c_str()
	if (m_debugApi) {
		char *p = cJSON_PrintUnformatted(params);
		DEBUGPRINT(("VizServer API request: %s %s", fullmethod,p));
		cJSON_free(p);
	}
	s = m_jsonprocessor->processJson(fullmethod, params, id);
	if (m_debugApi) {
		DEBUGPRINT(("VizServer API response: %s", s.c_str()));
	}
	return s.c_str();
}

std::string
VizServer::_processJson(std::string fullmethod, cJSON* params, const char* id) {
	std::string s = m_jsonprocessor->processJson(fullmethod, params, id);
	return s;
}

cJSON*
VizServer::_readconfig(const char* fn) {

	std::string fname = std::string(fn);
	std::string err;

	cJSON* json = jsonReadFile(fname, err);
	if (!json) {
		VizErrorOutput("Unable to read/parse config file (name=%s, err=%s), disabling Freeframe plugin!\n", fname.c_str(), err.c_str());
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

#if 0
void
VizServer::SendControllerMsg(MidiMsg* m, const char* handle, bool smooth) {
	m_scheduler->SendControllerMsg(m, handle, smooth);
}

void
VizServer::SendPitchBendMsg(MidiMsg* m, const char* handle, bool smooth) {
	m_scheduler->SendPitchBendMsg(m, handle, smooth);
}

void
VizServer::IncomingNoteOff(click_t clk, int ch, int pitch, int vel, const char* handle) {
	m_scheduler->IncomingNoteOff(clk, ch, pitch, vel, handle);
}

void
VizServer::IncomingMidiMsg(MidiMsg* m, click_t clk, const char* handle) {
	m_scheduler->IncomingMidiMsg(m, clk, handle);
}
#endif

void
VizServer::_scheduleMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizAssert(m_scheduler);
	m_scheduler->ScheduleMidiPhrase(ph, clk, cursorid, looping, mp);
}

void
VizServer::_scheduleMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizAssert(m_scheduler);
	m_scheduler->ScheduleMidiMsg(m, clk, cursorid, looping, mp);
}

void
VizServer::_scheduleClearAll() {
	VizAssert(m_scheduler);
	m_scheduler->ScheduleClear(SCHEDID_ALL);
}

void
VizServer::QueueMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizAssert(m_scheduler);
	m_scheduler->QueueMidiPhrase(ph, clk, cursorid, looping, mp);
}

void
VizServer::QueueMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping, MidiVizParams* mp) {
	VizAssert(m_scheduler);
	m_scheduler->QueueMidiMsg(m, clk, cursorid, looping, mp);
	DEBUGPRINT1(("After VizServer::QueueMidiMsg clk=%d type=%s",clk,m->MidiTypeName(),m_scheduler->NumEventsOfSid(cursorid)));
}

void
VizServer::QueueRemoveBefore(int cursorsid, click_t clk){
	DEBUGPRINT1(("VizServer::QueueRemoveBefore sid=%d clk=%d", cursorsid, clk));
	m_scheduler->QueueRemoveBefore(cursorsid, clk);
}

int
VizServer::NumQueuedOfId(int id) {
	return m_scheduler->NumQueuedEventsOfSid(id);
}

int
VizServer::NumScheduledOfId(int id) {
	return m_scheduler->NumScheduledEventsOfSid(id);
}

void
VizServer::QueueClear() {
	VizAssert(m_scheduler);
	m_scheduler->QueueClear();
}

void
VizServer::ScheduleClear(int cursorid) {
	VizAssert(m_scheduler);
	m_scheduler->ScheduleClear(cursorid);
}

void
VizServer::SetClicksPerSecond(int clicks) {
	VizAssert(m_scheduler);
	m_scheduler->SetClicksPerSecond(clicks);
}

void
VizServer::SetTempoFactor(float f) {
	m_scheduler->SetTempoFactor(f);
}

void
VizServer::ANO(int channel) {
	VizAssert(m_scheduler);
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

	try {
		std::string configpath = VizConfigPath("vizserver.json");
		DEBUGPRINT1(("configpath = %s", configpath.c_str()));

		cJSON* config = _readconfig(configpath.c_str());
		if (config == NULL) {
			DEBUGPRINT(("Unable to load config?  path=%s", configpath.c_str()));
		}
		else {
			_processServerConfig(config);
			// NOTE: DO NOT FREE config - some of the values in it get saved/used later.
		}

		m_scheduler->setPeriodicANO(m_do_ano);
		m_scheduler->SetMidiInputListener(m_midiinputprocessor);
		m_scheduler->SetMidiOutputListener(m_midioutputprocessor);

		m_oscprocessor->setVizServer(this);

		m_daemon = new VizDaemon(m_osc_input_port, m_osc_input_host, m_oscprocessor,
			m_httpport, m_htmlpath, m_jsonprocessor);

		m_started = true;

		_openSharedMemOutlines();

		m_clickprocessor = new VizServerClickProcessor(this);
		m_scheduler->SetClickProcessor(m_clickprocessor);

		m_scheduler->StartMidi(config);
	}
	catch (VizException& e) {
		DEBUGPRINT(("VizException: %s", e.message()));
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
		VizAssert(c);
		c->advanceTo(SchedulerCurrentTimeInSeconds());
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
	VizAssert(mem);

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
		_scheduleClearAll();
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
		int warning_repeat_seconds = 300;  // warn every 5 minutes = 300 seconds
		if (last_warning == 0 || (now - last_warning) > (warning_repeat_seconds*1000)) {
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
	if ((j = jsonGetArray(json, "midiinputs")) != NULL) {
		m_midiinputs = j;
	}
	if ((j = jsonGetString(json, "midimerge")) != NULL) {
		m_midimerges = j;
	}
	if ((j = jsonGetArray(json, "midioutputs")) != NULL) {
		m_midioutputs = j;
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
		VizDebugToConsole = j->valueint ? TRUE : FALSE;
	}
	if ((j = jsonGetNumber(json, "debugmidinotes")) != NULL) {
		VizDebugMidiNotes = j->valueint ? TRUE : FALSE;
	}
	if ((j = jsonGetNumber(json, "debugmidiall")) != NULL) {
		VizDebugMidiAll = j->valueint ? TRUE : FALSE;
	}
	if ((j = jsonGetNumber(json, "debugtolog")) != NULL) {
		bool b = j->valueint ? TRUE : FALSE;
		// If we're turning debugtolog off, put a final
		// message out so we know that!
		if (VizDebugToLog && !b) {
			DEBUGPRINT(("ALERT: VizDebugToLog is being set to false!"));
		}
		VizDebugToLog = b;
	}
	if ((j = jsonGetNumber(json, "debugautoflush")) != NULL) {
		VizDebugAutoFlush = j->valueint ? TRUE : FALSE;
	}
}

VizServer*
VizServer::GetServer(const char* vizpath) {

	if (vizpath != NULL && vizpath[0] != '\0') {
		SetVizPath(vizpath);
	}
	if (OneServer == NULL) {
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
VizServer::ClearJsonCallbacks() {
	m_jsonprocessor->ClearCallbacks();
	_setMaxCallbacks();
}

void
VizServer::ClearJsonCallbacksWithPrefix(std::string prefix) {
	m_jsonprocessor->ClearCallbacksWithPrefix(prefix);
	_setMaxCallbacks();
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
		c->touch(SchedulerCurrentTimeInSeconds());
	}
}

VizCursor::VizCursor(VizServer* ss, int sid_, std::string source_, int cursorid_,
	VizPos pos_, double area_, OutlineMem* om_, MMTT_SharedMemHeader* hdr_) {

	sid = sid_;
	source = source_;
	cursorid = cursorid_;
	pos = pos_;
	target_pos = pos_;
	area = area_;
	outline = om_;
	hdr = hdr_;

	m_vizserver = ss;
	m_lasttouchedInSeconds = 0;
	curr_speed = 0;
	curr_degrees = 0;
	m_target_depth = 0;
	m_last_tm = 0;
	m_target_degrees = 0;
	m_g_firstdir = true;
	m_smooth_degrees_factor = 0.2;
	m_pending_noteoff = NULL;
	m_noteon_click = 0;
	m_noteon_loopclicks = 0;
	m_noteon_depth = 0;

	touch(ss->SchedulerCurrentTimeInSeconds());
}

VizCursor::~VizCursor() {
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

	VizPos dpos = target_pos.sub(pos);
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

	VizPos finaldpos = pos.sub(m_last_pos);
	double final_distance = finaldpos.mag();

	/////////////// smooth the depth
	double depthsmoothfactor = 0.3f;
	double smoothdepth = depth() + ((target_depth() - depth())*depthsmoothfactor);

	setdepth(smoothdepth);

	/////////////// smooth the degrees
	double tooshort = 0.01f; // 0.05f;
	if (raw_distance < tooshort) {
		// VizDebug("   raw_distance=%.3f too small %s\n",
		// 	raw_distance,DebugString().c_str());
	}
	else {
		VizPos dp = pos.sub(m_last_pos);
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
