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

#include <pthread.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <strstream>
#include <cstdlib> // for srand, rand
#include <ctime>   // for time
#include <sys/stat.h>

#include "UT_SharedMem.h"
#include "NosuchUtil.h"
#include "NosuchMidi.h"
#include "NosuchScheduler.h"
#include "Vizlet.h"
#include "Vizletutil.h"
#include "NosuchJSON.h"

#include "FFGLLib.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscReceivedElements.h"
#include "NosuchOsc.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

// These functions need to be defined in a vizlet's source file.
extern std::string vizlet_name();
extern bool vizlet_setdll(std::string(dllpath));
CFFGLPluginInfo& vizlet_plugininfo();

void vizlet_setid(CFFGLPluginInfo& plugininfo, std::string name)
{
	char id[5];
	// Compute a hash of the plugin name and use two 4-bit values
	// from it to produce the last 2 characters of the unique ID.
	// It's possible there will be a collision.
	int hash = 0;
	for ( const char* p = name.c_str(); *p!='\0'; p++ ) {
		hash += *p;
	}
	id[0] = 'V';
	id[1] = 'Z';
	id[2] = 'A' + (hash & 0xf);
	id[3] = 'A' + ((hash >> 4) & 0xf);
	id[4] = '\0';
	plugininfo.SetPluginIdAndName(id,name.c_str());
}

char dllpath[MAX_PATH];
std::string dllpathstr;

std::string dll_pathname() {
	return(dllpathstr);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	GetModuleFileNameA((HMODULE)hModule, dllpath, MAX_PATH);
	dllpathstr = std::string(dllpath);

	if (ul_reason_for_call == DLL_PROCESS_ATTACH ) {
		// Initialize once for each new process.
		// Return FALSE if we fail to load DLL.
		dllpathstr = NosuchToLower(dllpathstr);
		if ( ! vizlet_setdll(dllpathstr) ) {
			DEBUGPRINT(("vizlet_setdll failed"));
			return FALSE;
		}
		vizlet_setdll(dllpathstr);
		std::string s = vizlet_name();
		DEBUGPRINT1(("After vizlet_setdll, dllpathstr=%s vizlet_name=%s",dllpathstr.c_str(),s.c_str()));
		vizlet_setid(vizlet_plugininfo(),s);
		DEBUGPRINT1(("DLL_PROCESS_ATTACH dll=%s",dllpath));
	}
	if (ul_reason_for_call == DLL_PROCESS_DETACH ) {
		DEBUGPRINT1(("DLL_PROCESS_DETACH dll=%s",dllpath));
	}
	if (ul_reason_for_call == DLL_THREAD_ATTACH ) {
		DEBUGPRINT1(("DLL_THREAD_ATTACH dll=%s",dllpath));
	}
	if (ul_reason_for_call == DLL_THREAD_DETACH ) {
		DEBUGPRINT1(("DLL_THREAD_DETACH dll=%s",dllpath));
	}
    return TRUE;
}

#endif

Vizlet::Vizlet() {

	DEBUGPRINT1(("=== Vizlet constructor, dll_pathname=%s",dll_pathname().c_str()));

	VizParams::Initialize();

	m_defaultparams = new AllVizParams(true);
	m_useparamcache = false;

	// AllVizParams* spdefault = getAllVizParams(VizParamPath("default"));
	// if ( spdefault ) {
	// 	m_defaultparams->applyVizParamsFrom(spdefault);
	// }

	m_callbacksInitialized = false;
	m_passthru = true;
	m_call_RealProcessOpenGL = false;
	m_spritelist = new VizSpriteList();
	m_defaultmidiparams = defaultParams();
	m_framenum = 0;

	m_vizserver = VizServer::GetServer();

	m_viztag = vizlet_name();

	m_af = ApiFilter(m_viztag.c_str());
	m_mf = MidiFilter();  // ALL Midi
	m_cf = CursorFilter();  // ALL Cursors

	// These are default values, which can be overridden by the config file.

	// _do_errorpopup = false;

#ifdef PALETTE_PYTHON
	_recompileFunc = NULL;
	_python_enabled = FALSE;
	m_python_events_disabled = TRUE;
	NosuchLockInit(&python_mutex,"python");
#endif

	NosuchLockInit(&vizlet_mutex,"vizlet");

	NosuchLockInit(&json_mutex,"json");
	json_cond = PTHREAD_COND_INITIALIZER;
	json_pending = false;

	m_disabled = false;
	m_disable_on_exception = false;

	m_stopped = false;

	// The most common reason for being disabled at this point
	// is when the config JSON file can't be parsed.
	if ( m_disabled ) {
		DEBUGPRINT(("WARNING! Vizlet (viztag=%s) has been disabled!",VizTag().c_str()));
	}

	SetTimeSupported(true);

	SetMinInputs(1);
	SetMaxInputs(1);

#ifdef VIZTAG_PARAMETER
	SetParamInfo(0,"viztag", FF_TYPE_TEXT, VizTag().c_str());
#endif
}

Vizlet::~Vizlet()
{
	DEBUGPRINT1(("=== Vizlet destructor is called viztag=%s", m_viztag.c_str()));
	_stopstuff();
}

DWORD Vizlet::SetTime(double tm) {
	m_time = tm;
	m_vizserver->SetTime(tm);
	return FF_SUCCESS;
}

DWORD Vizlet::SetParameter(const SetParameterStruct* pParam) {

	return FF_FAIL;
#ifdef VIZTAG_PARAMETER
	DWORD r = FF_FAIL;

	// Sometimes SetParameter is called before ProcessOpenGL,
	// so make sure the VizServer is started.
	StartVizServer();
	InitCallbacks();

	switch ( pParam->ParameterNumber ) {
	case 0:		// shape
		if ( VizTag() != std::string(pParam->u.NewTextValue) ) {
			SetVizTag(pParam->u.NewTextValue);
			m_af = ApiFilter(VizTag().c_str());
			ChangeVizTag(pParam->u.NewTextValue);
		}
		r = FF_SUCCESS;
	}
	return r;
#endif
}

DWORD Vizlet::GetParameter(DWORD n) {
#ifdef VIZTAG_PARAMETER
	switch ( n ) {
	case 0:		// shape
		return (DWORD)(VizTag().c_str());
	}
#endif
	return FF_FAIL;
}

char* Vizlet::GetParameterDisplay(DWORD n)
{
#ifdef VIZTAG_PARAMETER
	switch ( n ) {
	case 0:
	    strncpy_s(_disp,DISPLEN,VizTag().c_str(),VizTag().size());
	    _disp[DISPLEN-1] = 0;
		return _disp;
	}
#endif
	return "";
}

const char* vizlet_json(void* data,const char *method, cJSON* params, const char* id) {
	Vizlet* v = (Vizlet*)data;
	if ( v == NULL ) {
		static std::string err = error_json(-32000,"v is NULL is vizlet_json?",id);
		return err.c_str();
	}
	else {
		// A few methods are built-in.  If it isn't one of those,
		// it is given to the plugin to handle.
		if (std::string(method) == "description") {
			std::string desc = vizlet_plugininfo().GetPluginExtendedInfo()->Description;
			v->m_json_result = jsonStringResult(desc, id);
		} else if (std::string(method) == "about") {
			std::string desc = vizlet_plugininfo().GetPluginExtendedInfo()->About;
			v->m_json_result = jsonStringResult(desc, id);
		} else {
			v->m_json_result = v->processJsonAndCatchExceptions(std::string(method), params, id);
		}
		return v->m_json_result.c_str();
	}
}

void vizlet_osc(void* data,const char *source, const osc::ReceivedMessage& m) {
	Vizlet* v = (Vizlet*)data;
	NosuchAssert(v);
	v->processOsc(source,m);
}

void vizlet_midiinput(void* data,MidiMsg* m) {
	Vizlet* v = (Vizlet*)data;
	NosuchAssert(v);
	v->processMidiInput(m);
}

void vizlet_midioutput(void* data,MidiMsg* m) {
	Vizlet* v = (Vizlet*)data;
	NosuchAssert(v);
	v->processMidiOutput(m);
}

void vizlet_cursor(void* data,VizCursor* c, int downdragup) {
	Vizlet* v = (Vizlet*)data;
	NosuchAssert(v);
	v->processCursor(c,downdragup);
}

void vizlet_keystroke(void* data,int key, int downup) {
	Vizlet* v = (Vizlet*)data;
	NosuchAssert(v);
	v->processKeystroke(key,downup);
}

void Vizlet::advanceCursorTo(VizCursor* c, double tm) {
	m_vizserver->AdvanceCursorTo(c,tm);
}

void Vizlet::ChangeVizTag(const char* p) {
	m_vizserver->ChangeVizTag(Handle(),p);
}

void Vizlet::_startApiCallbacks(ApiFilter af, void* data) {
	NosuchAssert(m_vizserver);
	m_vizserver->AddJsonCallback(Handle(),af,vizlet_json,data);
}

void Vizlet::_startMidiCallbacks(MidiFilter mf, void* data) {
	NosuchAssert(m_vizserver);
	m_vizserver->AddMidiInputCallback(Handle(),mf,vizlet_midiinput,data);
	m_vizserver->AddMidiOutputCallback(Handle(),mf,vizlet_midioutput,data);
}

void Vizlet::_startCursorCallbacks(CursorFilter cf, void* data) {
	NosuchAssert(m_vizserver);
	m_vizserver->AddCursorCallback(Handle(),cf,vizlet_cursor,data);
}

void Vizlet::_startKeystrokeCallbacks(void* data) {
	NosuchAssert(m_vizserver);
	m_vizserver->AddKeystrokeCallback(Handle(),vizlet_keystroke,data);
}

void Vizlet::_stopCallbacks() {
	_stopApiCallbacks();
	_stopMidiCallbacks();
	_stopCursorCallbacks();
	_stopKeystrokeCallbacks();
}

void Vizlet::_stopApiCallbacks() {
	NosuchAssert(m_vizserver);
	m_vizserver->RemoveJsonCallback(Handle());
}

void Vizlet::_stopMidiCallbacks() {
	NosuchAssert(m_vizserver);
	m_vizserver->RemoveMidiInputCallback(Handle());
	m_vizserver->RemoveMidiOutputCallback(Handle());
}

void Vizlet::_stopCursorCallbacks() {
	NosuchAssert(m_vizserver);
	m_vizserver->RemoveCursorCallback(Handle());
}

void Vizlet::_stopKeystrokeCallbacks() {
	NosuchAssert(m_vizserver);
	m_vizserver->RemoveKeystrokeCallback(Handle());
}

double Vizlet::GetTime() {
	return m_time;
}

int Vizlet::SchedulerCurrentClick() {
	if ( m_vizserver == NULL ) {
		DEBUGPRINT(("Vizlet::CurrentClick() - _vizserver is NULL!"));
		return 0;
	} else {
		return m_vizserver->SchedulerCurrentClick();
	}
}

void Vizlet::SendMidiMsg() {
	DEBUGPRINT(("Vizlet::SendMidiMsg called - this should go away eventually"));
	MidiMsg* msg = MidiNoteOn::make(1,80,100);
	// _vizserver->MakeNoteOn(1,80,100);
	m_vizserver->SendMidiMsg(msg);
}


void
Vizlet::_drawnotes(std::list<MidiMsg*>& notes) {

	for ( std::list<MidiMsg*>::const_iterator ci = notes.begin(); ci != notes.end(); ) {
		MidiMsg* m = *ci++;
		if (m->MidiType() == MIDI_NOTE_ON) {
			processDrawNote(m);
		}
	}
}

#if 0
		if (m->MidiType() != MIDI_NOTE_ON ) {
			continue;
		}
		int pitch = m->Pitch();
		float y = pitch  / 128.0f;
		y = y * 2.0f - 1.0f;
		float x = _x;
		_x += 0.01f;
		if ( _x >= 1.0 ) {
			_x = -0.9f;
		}
		float sz = 0.15f;

		glBegin(GL_LINE_LOOP);
		glVertex3f(x-sz, y-sz, 0.0f);	// Top Left
		glVertex3f(x-sz, y+sz, 0.0f);	// Top Right
		glVertex3f(x+sz, y+sz, 0.0f);	// Bottom Right
		glVertex3f(x+sz, y-sz, 0.0f);	// Bottom Left
		glEnd();
	}
}
#endif

DWORD Vizlet::InitGL(const FFGLViewportStruct *vp) {
	return FF_SUCCESS;
}

void Vizlet::_stopstuff() {
	if ( m_stopped )
		return;
	m_stopped = true;
	_stopCallbacks();
	if ( m_vizserver ) {
		int ncb = m_vizserver->NumCallbacks();
		int mcb = m_vizserver->MaxCallbacks();
		DEBUGPRINT1(("Vizlet::_stopstuff - VizServer has %d callbacks, max=%d!",ncb,mcb));
		if ( ncb == 0 && mcb > 0 ) {
			m_vizserver->Stop();
		}
	}
}

DWORD Vizlet::DeInitGL() {
	_stopstuff();
	return FF_SUCCESS;
}

void Vizlet::AddVizSprite(VizSprite* s) {
	m_spritelist->add(s,defaultParams()->nsprites);
}

void Vizlet::DrawVizSprites() {
	m_spritelist->draw(&graphics);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

void
Vizlet::QueueMidiPhrase(MidiPhrase* ph, click_t clk) {
	m_vizserver->QueueMidiPhrase(ph,clk);
}

void
Vizlet::QueueMidiMsg(MidiMsg* m, click_t clk) {
	m_vizserver->QueueMidiMsg(m,clk);
}

void
Vizlet::QueueClear() {
	m_vizserver->QueueClear();
}

void
Vizlet::StartVizServer() {
	if ( ! m_vizserver->Started() ) {
		m_vizserver->Start();
		srand( (unsigned int)(m_vizserver->GetTime()) );
	}
}

void
Vizlet::InitCallbacks() {
	if ( ! m_callbacksInitialized ) {

		_startApiCallbacks(m_af,(void*)this);
		_startMidiCallbacks(m_mf,(void*)this);
		_startCursorCallbacks(m_cf,(void*)this);
		_startKeystrokeCallbacks((void*)this);

		m_callbacksInitialized = true;
	}
}

DWORD Vizlet::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	if ( m_stopped ) {
		return FF_SUCCESS;
	}
	if ( m_disabled ) {
		return FF_SUCCESS;
	}

	StartVizServer();
	InitCallbacks();

	m_framenum++;

#ifdef FRAMELOOPINGTEST
	static int framenum = 0;
	static bool framelooping = FALSE;
#endif

	NosuchLock(&json_mutex,"json");
	if (json_pending) {
		// Execute json stuff and generate response
	
		m_json_result = processJsonAndCatchExceptions(json_method, json_params, json_id);
		json_pending = false;
		int e = pthread_cond_signal(&json_cond);
		if ( e ) {
			DEBUGPRINT(("ERROR from pthread_cond_signal e=%d\n",e));
		}
	}
	NosuchUnlock(&json_mutex,"json");

	if ( m_passthru && pGL != NULL ) {
		if ( ! ff_passthru(pGL) ) {
			return FF_FAIL;
		}
	}

	LockVizlet();

	// _frame++;

	bool gotexception = false;
	try {
		CATCH_NULL_POINTERS;

		glDisable(GL_TEXTURE_2D); 
		glEnable(GL_BLEND); 
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
		glLineWidth((GLfloat)1.0f);

		glScaled(2.0,2.0,1.0);
		glTranslated(-0.5,-0.5,0.0);

		double tm = GetTime();
		bool r;
		if (m_call_RealProcessOpenGL) {
			// This is used when adapting to existing FFGL plugin code
			r = (RealProcessOpenGL(pGL)==FF_SUCCESS);
		}
		else {
			r = processDraw();			// Call the vizlet's processDraw()
		}

		glDisable(GL_TEXTURE_2D);
		glColor4f(1.f,1.f,1.f,1.f); //restore default color

		m_spritelist->advanceTo(tm);
		processAdvanceTimeTo(tm);

		if ( ! r ) {
			DEBUGPRINT(("Palette::draw returned failure? (r=%d)\n",r));
			gotexception = true;
		}
	} catch (NosuchException& e ) {
		DEBUGPRINT(("NosuchException in Palette::draw : %s",e.message()));
		gotexception = true;
	} catch (...) {
		DEBUGPRINT(("UNKNOWN Exception in Palette::draw!"));
		gotexception = true;
	}

	if ( gotexception && m_disable_on_exception ) {
		DEBUGPRINT(("DISABLING Vizlet due to exception!!!!!"));
		m_disabled = true;
	}

	UnlockVizlet();

	glDisable(GL_BLEND); 
	glEnable(GL_TEXTURE_2D); 
	// END NEW CODE

#ifdef FRAMELOOPINGTEST
	int w = Texture.Width;
	int h = Texture.Height;
#define NFRAMES 300
	static GLubyte* pixelArray[NFRAMES];
	if ( framelooping ) {
		glRasterPos2i(-1,-1);
		glDrawPixels(w,h,GL_RGB,GL_UNSIGNED_BYTE,pixelArray[framenum]);
		framenum = (framenum+1)%NFRAMES;
	} else {
		if ( framenum < NFRAMES ) {
			pixelArray[framenum] = new GLubyte[w*h*3];
			glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixelArray[framenum]);
			framenum++;
		} else {
			framelooping = TRUE;
			framenum = 0;
		}
	}
#endif

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);
	
	//restore default color
	glColor4d(1.f,1.f,1.f,1.f);
	
	return FF_SUCCESS;
}

void Vizlet::DrawNotesDown() {
	m_vizserver->LockNotesDown();
	try {
		CATCH_NULL_POINTERS;
		_drawnotes(m_vizserver->NotesDown());
	} catch (NosuchException& e) {
		DEBUGPRINT(("NosuchException while drawing notes: %s",e.message()));
	} catch (...) {
		DEBUGPRINT(("Some other kind of exception occured while drawing notes!?"));
	}
	m_vizserver->UnlockNotesDown();
}

void Vizlet::LockVizlet() {
	NosuchLock(&vizlet_mutex,"Vizlet");
}

void Vizlet::UnlockVizlet() {
	NosuchUnlock(&vizlet_mutex,"Vizlet");
}

std::string Vizlet::processJsonAndCatchExceptions(std::string meth, cJSON *params, const char *id) {
	std::string r;
	try {
		CATCH_NULL_POINTERS;

		// Here, we should intercept and interpret some APIs that are common to all vizlets
		if ( meth == "echo"  ) {
			std::string val = jsonNeedString(params,"value");
			r = jsonStringResult(val,id);
		} else if ( meth == "time"  ) {
			r = jsonDoubleResult(GetTime(),id);
		} else if ( meth == "name"  ) {
		    std::string nm = CopyFFString16((const char *)(vizlet_plugininfo().GetPluginInfo()->PluginName));
			r = jsonStringResult(nm,id);
		} else if ( meth == "dllpathname"  ) {
			r = jsonStringResult(dll_pathname(),id);
		} else {
			// If not one of the standard vizlet APIs, call the vizlet-plugin-specific API processor
			r = processJson(meth,params,id);
		}
	} catch (NosuchException& e) {
		std::string s = NosuchSnprintf("NosuchException in executeJson!! - %s",e.message());
		r = error_json(-32000,s.c_str(),id);
	} catch (...) {
		// This doesn't seem to work - it doesn't seem to catch other exceptions...
		std::string s = NosuchSnprintf("Some other kind of exception occured in executeJson!?");
		r = error_json(-32000,s.c_str(),id);
	}
	return r;
}

AllVizParams*
Vizlet::findAllVizParams(std::string cachename) {
	std::map<std::string,AllVizParams*>::iterator it = m_paramcache.find(cachename);
	if ( it == m_paramcache.end() ) {
		return NULL;
	}
	return it->second;
}

AllVizParams*
readVizParams(std::string path) {
	std::string err;
	cJSON* json = jsonReadFile(path,err);
	if ( !json ) {
		DEBUGPRINT(("Unable to load vizlet params: path=%s, err=%s",
			path.c_str(),err.c_str()));
		return NULL;
	}
	AllVizParams* s = new AllVizParams(false);
	s->loadJson(json);
	// XXX - should json be freed, here?
	return s;
}

std::string
Vizlet::VizPath2ConfigName(std::string path) {
	size_t pos = path.find_last_of("/\\");
	if (pos != path.npos) {
		path = path.substr(pos+1);
	}
	pos = path.find_last_of(".");
	if (pos != path.npos) {
		path = path.substr(0,pos);
	}
	return(path);
}

std::string
Vizlet::VizParamPath(std::string configname) {
	if (!NosuchEndsWith(configname, ".json")) {
		configname += ".json";
	}
	return VizConfigPath("params\\"+configname);
}

AllVizParams*
Vizlet::getAllVizParams(std::string path) {
	if (m_useparamcache) {
		std::map<std::string, AllVizParams*>::iterator it = m_paramcache.find(path);
		if (it == m_paramcache.end()) {
			m_paramcache[path] = readVizParams(path);
			return m_paramcache[path];
		}
		else {
			return it->second;
		}
	}
	else {
		return readVizParams(path);
	}
}

AllVizParams*
Vizlet::checkAndLoadIfModifiedSince(std::string path, std::time_t& lastcheck, std::time_t& lastupdate) {
	std::time_t throttle = 1;  // don't check more often than this number of seconds
	std::time_t tm = time(0);
	if ((tm - lastcheck) < throttle) {
		return NULL;
	}
	lastcheck = tm;
	struct _stat statbuff;
	int e = _stat(path.c_str(), &statbuff);
	if (e != 0) {
		throw NosuchException("Error in checkAndLoad - e=%d", e);
	}
	if (lastupdate == statbuff.st_mtime) {
		return NULL;
	}
	AllVizParams* p = getAllVizParams(path);
	if (!p) {
		throw NosuchException("Bad params file? path=%s", path.c_str());
	}
	lastupdate = statbuff.st_mtime;
	return p;
}


VizSprite*
Vizlet::makeAndAddVizSprite(AllVizParams* p, NosuchPos pos) {
		VizSprite* s = makeAndInitVizSprite(p,pos);
		AddVizSprite(s);
		return s;
}

VizSprite*
Vizlet::makeAndInitVizSprite(AllVizParams* p, NosuchPos pos) {

	VizSprite* s = VizSprite::makeVizSprite(p);
	s->m_framenum = FrameNum();

	double movedir;
	if ( p->movedirrandom.get() ) {
		movedir = 360.0f * ((double)(rand()))/ RAND_MAX;
	} else if ( p->movefollowcursor.get() ) {
		// eventually, keep track of cursor movement direction
		// for the moment it's random
		movedir = 360.0f * ((double)(rand()))/ RAND_MAX;
	} else {
		movedir = p->movedir.get();
	}

	s->initVizSpriteState(GetTime(),Handle(),pos,movedir);
	DEBUGPRINT1(("Vizlet::makeAndInitVizSprite size=%f",s->m_state.size));
	return s;
}

NosuchColor
Vizlet::channelColor(int ch) {
	double hue = (ch * 360.0f) / 16.0f;
	return NosuchColor(hue,0.5,1.0);
}

double Vizlet::movedirDegrees(AllVizParams* p) {

	if ( p->movedirrandom.get() ) {
		double f = ((double)(rand()))/ RAND_MAX;
		return f * 360.0f;
	}
	if ( p->movefollowcursor.get() ) {
		// eventually, keep track of cursor movement direction
		// for the moment it's random
		double f = ((double)(rand()))/ RAND_MAX;
		return f * 360.0f;
	}
	return p->movedir.get();
}

VizSprite* Vizlet::defaultMidiVizSprite(MidiMsg* m) {

	int minpitch = 0;
	int maxpitch = 127;
	VizSprite* s = NULL;

	if ( m->MidiType() == MIDI_NOTE_ON ) {
		if ( m->Velocity() == 0 ) {
			DEBUGPRINT(("Vizlet1 sees noteon with 0 velocity, ignoring"));
			return s;
		}
		NosuchPos pos;
		pos.x = 0.9f;
		pos.y = (m->Pitch()-minpitch) / float(maxpitch-minpitch);
		pos.z = (m->Velocity()*m->Velocity()) / (128.0*128.0);

		m_defaultmidiparams->shape.set("circle");

		double fadetime = 5.0;
		m_defaultmidiparams->movedir.set(180.0);
		m_defaultmidiparams->speedinitial.set(0.2);
		m_defaultmidiparams->sizeinitial.set(0.1);
		m_defaultmidiparams->sizefinal.set(0.0);
		m_defaultmidiparams->sizetime.set(fadetime);

		m_defaultmidiparams->lifetime.set(fadetime);

		// control color with channel
		NosuchColor clr = channelColor(m->Channel());
		double hue = clr.hue();

		m_defaultmidiparams->alphainitial.set(1.0);
		m_defaultmidiparams->alphafinal.set(0.0);
		m_defaultmidiparams->alphatime.set(fadetime);

		m_defaultmidiparams->hueinitial.set(hue);
		m_defaultmidiparams->huefinal.set(hue);
		m_defaultmidiparams->huefillinitial.set(hue);
		m_defaultmidiparams->huefillfinal.set(hue);

		s = makeAndAddVizSprite(m_defaultmidiparams, pos);
	}
	return s;
}