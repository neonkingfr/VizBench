#ifndef _VIZLET_H
#define _VIZLET_H

#define VIZLET

#include "FFGL.h"
#include "FFGLPluginSDK.h"

#include "osc/OscOutboundPacketStream.h"
#include "NosuchOscInput.h"
#include "NosuchOscManager.h"
#include "NosuchOscInput.h"
#include "NosuchColor.h"
#include "NosuchJSON.h"
#include "NosuchHttpServer.h"

#include "VizParams.h"
#include "SpriteVizParams.h"
#include "MidiVizParams.h"

#include "VizSprite.h"
#include "VizServer.h"
#include <ctime>

class Vizlet;
class VizletHttpServer;
class UT_SharedMem;
class MMTT_SharedMemHeader;

struct PointMem;
struct OutlineMem;

#define MAGIC_VAL_FOR_OVERRIDE_PARAMS -2
#define MAGIC_VAL_FOR_PALETTE_PARAMS -1

class Vizlet : public NosuchOscListener,
					public NosuchJsonListener,
					public NosuchMidiInputListener,
					public NosuchMidiOutputListener,
					// public ClickListener,
					public CursorListener,
					public KeystrokeListener,
					public CFreeFrameGLPlugin
{
public:
	Vizlet();
	virtual ~Vizlet();

	char* VizTag() { return m_viztag; }
	void SetVizTag(const char* s) {
		m_viztag = _strdup(s);
		for (char* p = m_viztag; *p; p++) {
			if (*p >= 'A' && *p <= 'Z') {
				*p = _tolower(*p);
			}
		}
		DEBUGPRINT1(("Vizlet::SetVizTag m_viztag is now %s",m_viztag));
	}
	VizSprite* makeAndInitVizSprite(SpriteVizParams* sp, NosuchPos pos);
	VizSprite* makeAndAddVizSprite(SpriteVizParams* sp, NosuchPos pos);

	void SetPassthru(bool b) { m_passthru = b; }

	VizServer* vizserver() { return m_vizserver; }
	void StartVizServer();
	void InitCallbacks();
	void ChangeVizTag(const char* newtag);
	void advanceCursorTo(VizCursor* c, double tm);
	double SchedulerCurrentTimeInSeconds();
	click_t SchedulerCurrentClick();
	click_t SchedulerClicksPerSecond();
	click_t SchedulerClicksPerBeat();
	void LockVizlet();
	void UnlockVizlet();

	bool IsConnected() {
		return m_connected;
	};
	bool InputEnabled() {
		return m_enableinput;
	};

	void LoadPipeline(std::string pipeline);

	void QueueMidiMsg(MidiMsg* m, click_t clk, int cursorid, bool looping=false, MidiVizParams* mp = 0);
	void QueueMidiPhrase(MidiPhrase* ph, click_t clk, int cursorid, bool looping=false, MidiVizParams* mp = 0);
	void QueueClear();
	void ScheduleClear();

	bool SendOsc(std::string host, int port, const char* data, size_t size);

	std::string MidiInputName(size_t n) { return m_vizserver->MidiInputName(n);  }
	std::string MidiOutputName(size_t n) { return m_vizserver->MidiOutputName(n);  }

	int MidiInputNumberOf(std::string name) {
		for (size_t n = 0; ; n++) {
			const char* nm = m_vizserver->MidiInputName(n);
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
			const char* nm = m_vizserver->MidiOutputName(n);
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
	VizSpriteList* GetVizSpriteList() { return m_spritelist; }
	VizSprite* MakeVizSprite(SpriteVizParams* sp);
	std::string VizPath2ConfigName(std::string path);

	// If there are more types of *VizParams, should probably use templates...
	SpriteVizParams* readSpriteVizParams(std::string fname, std::string default_fname = "");
	SpriteVizParams* checkSpriteVizParamsAndLoadIfModifiedSince(std::string fname, std::time_t& lastcheck, std::time_t& lastupdate);

	MidiVizParams* checkMidiVizParamsAndLoadIfModifiedSince(std::string fname, std::time_t& lastcheck, std::time_t& lastupdate);
	MidiVizParams* readMidiVizParams(std::string fname);

	VizSprite* defaultMidiVizSprite(MidiMsg* m);

	SpriteVizParams* defaultParams() { return m_defaultparams; }

	void SetDefaults();

	std::string HtmlDir();
	void ExecuteDump(std::string dump);

	std::string processJsonLockAndCatchExceptions(std::string meth, cJSON *params, const char *id);
	static bool checkAddrPattern(const char *addr, char *patt);

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
	
	// virtual void processAdvanceClickTo(int click) { }
	// virtual void processAdvanceTimeTo(double tm) { }

	NosuchGraphics graphics;

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	void DrawNotesDown();
	int FrameNum() { return m_framenum; }

	// std::string m_json_result;

protected:	

	void* Handle() { return (void*)this; }

	bool m_call_RealProcessOpenGL;

	MidiFilter m_mf;
	CursorFilter m_cf;
	
	pthread_mutex_t json_mutex;
	pthread_cond_t json_cond;
	pthread_mutex_t vizlet_mutex;

#if 0
	bool json_pending;
#endif
	std::string json_method;
	cJSON* json_params;
	const char *json_id;

	double defaultMovedir();

	// double movedirDegrees(SpriteVizParams* p);
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
	virtual DWORD ProcessConnect();
	virtual DWORD ProcessDisconnect();

	/////////////////////////////////////////////////////

	void _stopstuff();
	void _startApiCallbacks(const char* apiprefix, void* data);
	void _stopApiCallbacks();

	void _startNonApiCallbacks();
	void _stopNonApiCallbacks();

	void _startMidiCallbacks(MidiFilter mf, void* data);
	void _stopMidiCallbacks();
	void _startCursorCallbacks(CursorFilter mf, void* data);
	void _stopCursorCallbacks();
	void _startKeystrokeCallbacks(void* data);
	void _stopKeystrokeCallbacks();
	// void _startClickCallbacks(void* data);
	// void _stopClickCallbacks();

	void _drawnotes(std::list<MidiMsg*>& notes);

	int m_framenum;
	bool m_passthru;
	bool m_stopped;
	bool m_disabled;
	bool m_disable_on_exception;
	bool m_connected;
	VizServer* m_vizserver;
	bool m_callbacksInitialized;
	char* m_viztag;
	bool m_enableinput;
	VizSpriteList* m_spritelist;
	SpriteVizParams* m_defaultparams;
	SpriteVizParams* m_defaultmidiparams;

#define DISPLEN 128
	char m_disp[DISPLEN];
};

std::string dll_pathname();

#endif
