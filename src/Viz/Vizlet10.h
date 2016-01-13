#ifndef _VIZLET10_H
#define _VIZLET10_H

#define VIZLET10

// #include "ffutil.h"
#include "FF10.h"
#include "FF10PluginSDK.h"

#include "osc/OscOutboundPacketStream.h"
#include "NosuchOscInput.h"
#include "NosuchOscManager.h"
#include "NosuchOscInput.h"
#include "NosuchColor.h"
#include "NosuchJSON.h"
#include "NosuchHttpServer.h"
// #include "VizSprite.h"
#include "VizParams.h"
#include "SpriteVizParams.h"
#include "VizServer.h"
#include <ctime>

class Vizlet10;
class VizletHttpServer;
class UT_SharedMem;
class MMTT_SharedMemHeader;

struct PointMem;
struct OutlineMem;

#define MAGIC_VAL_FOR_OVERRIDE_PARAMS -2
#define MAGIC_VAL_FOR_PALETTE_PARAMS -1

class Vizlet10 : public NosuchOscListener,
					public NosuchJsonListener,
					public NosuchMidiInputListener,
					public NosuchMidiOutputListener,
					public CursorListener,
					public KeystrokeListener,
					public CFreeFrame10Plugin
{
public:
	Vizlet10();

	virtual ~Vizlet10();

	std::string VizTag() { return m_viztag; }
	void SetVizTag(std::string s) { m_viztag = s; }

	void SetPassthru(bool b) { m_passthru = b; }

	VizServer* vizserver() { return m_vizserver; }
	void StartVizServer();
	void InitCallbacks();
	void RemoveCallbacks();
	void ChangeVizTag(const char* newtag);
	void advanceCursorTo(VizCursor* c, double tm);
	double GetTimeInSeconds();
	click_t SchedulerCurrentClick();
	void LockVizlet();
	void UnlockVizlet();

	void QueueMidiMsg(MidiMsg* m, click_t clk);
	void QueueMidiPhrase(MidiPhrase* ph, click_t clk);
	void QueueClear();

	DWORD SetVideoInfo(const VideoInfoStruct* pvi);
	IplImage* FrameImage();
	DWORD ProcessFrame(void* pFrame);
	DWORD ProcessFrameCopy(ProcessFrameCopyStruct* pFrameData);
	VideoInfoStruct* GetVideoInfo() {
		return &m_VideoInfo;
	}
	int FrameWidth() { return m_VideoInfo.FrameWidth; }
	int FrameHeight() { return m_VideoInfo.FrameHeight; }

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

	// void AddVizSprite(VizSprite* s);
	// void DrawVizSprites();
	// VizSpriteList* GetVizSpriteList() { return m_spritelist; }
	// VizSprite* MakeVizSprite(SpriteVizParams* sp);

	std::string VizPath2ConfigName(std::string path);
	SpriteVizParams* getSpriteVizParams(std::string path);
	SpriteVizParams* findSpriteVizParams(std::string cachename);
	SpriteVizParams* checkSpriteVizParamsAndLoadIfModifiedSince(std::string fname, std::time_t& lastcheck, std::time_t& lastupdate);

	// VizSprite* defaultMidiVizSprite(MidiMsg* m);

	SpriteVizParams* defaultParams() { return m_defaultparams; }

	void SetDefaults();

	std::string HtmlDir();
	std::string ParamConfigDir();

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

	virtual void processAdvanceClickTo(int click) { }
	virtual void processAdvanceTimeTo(double tm) { }

	virtual bool processFrame24Bit() {
		return false;
	}
	virtual bool processFrame32Bit() {
		return false;
	}

	NosuchGraphics graphics;

	// void SendMidiMsg();
	// void DrawNotesDown();
	int FrameNum() { return m_framenum; }

	std::string m_json_result;

	CvSize m_imagesize;
	IplImage* m_image;

protected:	

	void* Handle() { return (void*)this; }

	MidiFilter m_mf;
	CursorFilter m_cf;
	
	pthread_mutex_t json_mutex;
	pthread_cond_t json_cond;
	pthread_mutex_t vizlet10_mutex;

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
	// methods for CFreeFrame10Plugin
	virtual DWORD SetParameter(const SetParameterStruct* pParam);
	virtual DWORD GetParameter(DWORD dwIndex);
	virtual char* GetParameterDisplay(DWORD dwIndex);

	/////////////////////////////////////////////////////

	void _stopstuff();
	void _stopCallbacks();
	void _startApiCallbacks(const char* apiprefix, void* data);
	void _stopApiCallbacks();
	void _startMidiCallbacks(MidiFilter mf, void* data);
	void _stopMidiCallbacks();
	void _startCursorCallbacks(CursorFilter mf, void* data);
	void _stopCursorCallbacks();
	void _startKeystrokeCallbacks(void* data);
	void _stopKeystrokeCallbacks();

	// void _drawnotes(std::list<MidiMsg*>& notes);

	VideoInfoStruct m_VideoInfo;
	int m_framenum;
	double m_time;
	bool m_passthru;
	bool m_stopped;
	bool m_disabled;
	bool m_disable_on_exception;
	VizServer* m_vizserver;
	bool m_callbacksInitialized;
	std::string m_viztag;

	// VizSpriteList* m_spritelist;

	SpriteVizParams* m_defaultparams;
	SpriteVizParams* m_defaultmidiparams;
	bool m_useparamcache;
	std::map<std::string, SpriteVizParams*> m_paramcache;

#define DISPLEN 128
	char m_disp[DISPLEN];
};

std::string dll_pathname();

#endif
