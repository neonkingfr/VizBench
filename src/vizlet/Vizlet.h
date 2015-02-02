#ifndef _VIZLET_H
#define _VIZLET_H

#define VIZLET

#include "ffutil.h"
#include "FFGL.h"
#include "FFGLPluginSDK.h"

#include "osc/OscOutboundPacketStream.h"
#include "NosuchOscInput.h"
#include "NosuchOscManager.h"
#include "NosuchOscInput.h"
#include "NosuchColor.h"
#include "NosuchJSON.h"
#include "NosuchHttpServer.h"
#include "VizSprite.h"
#include "VizParams.h"
#include "VizServer.h"
#include <ctime>

class Vizlet;
class VizletHttpServer;
class UT_SharedMem;
class MMTT_SharedMemHeader;

struct PointMem;
struct OutlineMem;

#define DEFAULT_RESOLUME_PORT 7000
#define DEFAULT_PYFFLE_PORT 9876

#define MAGIC_VAL_FOR_OVERRIDE_PARAMS -2
#define MAGIC_VAL_FOR_PALETTE_PARAMS -1

class Vizlet : public NosuchOscListener,
					public NosuchJsonListener,
					public NosuchMidiInputListener,
					public NosuchMidiOutputListener,
					public CursorListener,
					public KeystrokeListener,
					public CFreeFrameGLPlugin
{
public:
	Vizlet();
	virtual ~Vizlet();

	std::string VizTag() { return _viztag; }
	void SetVizTag(std::string s) { _viztag = s; }
	VizSprite* makeAndInitVizSprite(AllVizParams* sp, NosuchPos pos);
	VizSprite* makeAndAddVizSprite(AllVizParams* sp, NosuchPos pos);

	void SetPassthru(bool b) { _passthru = b; }

	VizServer* vizserver() { return _vizserver; }
	void StartVizServer();
	void InitCallbacks();
	void ChangeVizTag(const char* newtag);
	void advanceCursorTo(VizCursor* c, int tm);
	int MilliNow();
	click_t CurrentClick();
	void LockVizlet();
	void UnlockVizlet();
	void DisableVizlet() { _disabled = true; }

	void QueueMidiMsg(MidiMsg* m, click_t clk);
	void QueueMidiPhrase(MidiPhrase* ph, click_t clk);
	void QueueClear();

	std::string MidiInputName(size_t n) { return _vizserver->MidiInputName(n);  }
	std::string MidiOutputName(size_t n) { return _vizserver->MidiOutputName(n);  }

	int MidiInputNumberOf(std::string name) {
		for (size_t n = 0; ; n++) {
			const char* nm = _vizserver->MidiInputName(n);
			if (nm == NULL) {
				break;
			}
			if ( std::string(nm) == name) {
				return (int)n;
			}
		}
		return -1;
	}

	int MidiOutputNumberOf(std::string name) {
		for (size_t n = 0; ; n++) {
			const char* nm = _vizserver->MidiOutputName(n);
			if (nm == NULL) {
				break;
			}
			if ( std::string(nm) == name) {
				return (int)n;
			}
		}
		return -1;
	}

	void AddVizSprite(VizSprite* s);
	void DrawVizSprites();
	VizSpriteList* GetVizSpriteList() { return _spritelist; }
	VizSprite* MakeVizSprite(AllVizParams* sp);
	std::string VizParamPath(std::string configname);
	std::string VizPath2ConfigName(std::string path);
	AllVizParams* getAllVizParams(std::string path);
	AllVizParams* findAllVizParams(std::string cachename);
	AllVizParams* checkAndLoadIfModifiedSince(std::string path, std::time_t& lastcheck, std::time_t& lastupdate);
	VizSprite* defaultMidiVizSprite(MidiMsg* m);

	AllVizParams* defaultParams() { return _defaultparams; }

	void SetDefaults();

	std::string HtmlDir();
	std::string ParamConfigDir();
	std::string ConfigFileName(std::string name);

	std::string processJsonAndCatchExceptions(std::string meth, cJSON *params, const char *id);
	static bool checkAddrPattern(const char *addr, char *patt);

	std::string submitJsonForProcessing(std::string method, cJSON *params, const char *id);

	/////////////////////////////////////////////////////
	// These are the things that a Vizlet should override/define.
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	// methods for NosuchJsonListener
	virtual std::string processJson(std::string meth, cJSON *params, const char *id) {
		std::string err = NosuchSnprintf("Vizlet - Unrecognized method '%s'",meth.c_str());
		return jsonError(-32000, err.c_str(), id);
	}
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	// methods for NosuchOscListener
	virtual void processOsc(const char *source, const osc::ReceivedMessage& m) { }
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	// methods for NosuchMidiInputListener
	// called from low-level things, NO OpenGL or time-intensive calls should be made
	virtual void processMidiInput(MidiMsg* mm) { }
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	// methods for NosuchMidiInputListener
	// called from low-level things, NO OpenGL or time-intensive calls should be made
	virtual void processMidiOutput(MidiMsg* mm) { }
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	// methods for CursorListener
	virtual void processCursor(VizCursor* c, int downdragup) { }
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	// methods for KeystrokeListener
	virtual void processKeystroke(int key, int downup) { }
	/////////////////////////////////////////////////////

	virtual DWORD RealProcessOpenGL(ProcessOpenGLStruct* pGL) { return FF_FAIL; }
	virtual bool processDraw() { return false;  }
	virtual void processDrawNote(MidiMsg* m) { }
	virtual void processAdvanceClickTo(int click) { }
	virtual void processAdvanceTimeTo(int milli) { }

	NosuchGraphics graphics;

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	void SendMidiMsg();
	void DrawNotesDown();
	int FrameSeq() { return _vizserver->FrameSeq(); }

	std::string json_result;

protected:	

	void* Handle() { return (void*)this; }

	bool _call_RealProcessOpenGL;

	ApiFilter _af;
	MidiFilter _mf;
	CursorFilter _cf;
	
	pthread_mutex_t json_mutex;
	pthread_cond_t json_cond;
	pthread_mutex_t vizlet_mutex;

	bool json_pending;
	std::string json_method;
	cJSON* json_params;
	const char *json_id;

	double defaultMovedir();

	double movedirDegrees(AllVizParams* p);
	NosuchColor channelColor(int ch);

private:

	/////////////////////////////////////////////////////
	// methods for CFreeFrameGLPlugin
	virtual DWORD ProcessOpenGL(ProcessOpenGLStruct *pGL);
	virtual DWORD InitGL(const FFGLViewportStruct *vp);
	virtual DWORD DeInitGL();
	virtual DWORD SetParameter(const SetParameterStruct* pParam);
	virtual DWORD GetParameter(DWORD dwIndex);
	virtual char* GetParameterDisplay(DWORD dwIndex);
	/////////////////////////////////////////////////////

	void _stopstuff();
	void _stopCallbacks();
	void _startApiCallbacks(ApiFilter af, void* data);
	void _stopApiCallbacks();
	void _startMidiCallbacks(MidiFilter mf, void* data);
	void _stopMidiCallbacks();
	void _startCursorCallbacks(CursorFilter mf, void* data);
	void _stopCursorCallbacks();
	void _startKeystrokeCallbacks(void* data);
	void _stopKeystrokeCallbacks();

	// int _frame;
	bool _passthru;
	bool _stopped;
	bool _disabled;
	bool _disable_on_exception;
	VizServer* _vizserver;
	bool _callbacksInitialized;
	std::string _viztag;
	VizSpriteList* _spritelist;
	AllVizParams* _defaultparams;
	AllVizParams* _defaultmidiparams;
	bool _useparamcache;
	std::map<std::string, AllVizParams*> _paramcache;

	void _drawnotes(std::list<MidiMsg*>& notes);

#define DISPLEN 128
	char _disp[DISPLEN];
};

std::string dll_pathname();

#endif
