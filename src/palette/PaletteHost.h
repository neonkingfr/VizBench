#ifndef _PALETTEHOST_H
#define _PALETTEHOST_H

#ifdef _WIN32
#include <windows.h>
#include <gl/gl.h>
#endif

class Palette;
class Vizlet;
class VizServer;
class NosuchScheduler;
struct cJSON;

#include "NosuchOsc.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscReceivedElements.h"

#include "NosuchColor.h"

class PaletteHost;
class VizCursor;
class UT_SharedMem;
class MMTT_SharedMemHeader;
class Region;
class GraphicBehaviour;
class MusicBehaviour;

struct PointMem;
struct OutlineMem;

#define DEFAULT_VIZSERVER_OSC_PORT 3333
#define DEFAULT_VIZSERVER_HTTP_PORT 4448

#define MAGIC_VAL_FOR_PALETTE_PARAMS -1

extern PaletteHost* RealPaletteHost;

class PaletteHost {

public:
	PaletteHost(Vizlet* v);
	virtual ~PaletteHost();

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////
	
	bool hostProcessDraw();

	void test_stuff();
	bool initStuff1();
	bool initStuff2();

	std::string _configpath;
	bool disable_on_exception;
	bool disabled;
	int frame;

	void LoadGlobalDefaults();
	bool LoadPaletteConfig(std::string jstr);

	std::string ParamConfigDir();
	// std::string PatchFilename(std::string name);
	std::string ParamsFileName(std::string name);

	std::string ExecuteJson(std::string meth, cJSON *params, const char *id);

	void processOsc(const char *source, const osc::ReceivedMessage& m);

	int SendToVizServer(osc::OutboundPacketStream& p);
	int EnableEffect(std::string effect, bool enabled);

	int _textEraseTime;
	void LoadEffectSet(std::string effects);

	void ShowText(std::string text, int x, int y, int timeout);

	static bool checkAddrPattern(const char *addr, char *patt);

	std::string jsonMethError(std::string e, const char* id);
	std::string jsonError(int code, std::string e, const char* id);
	std::string jsonConfigResult(std::string name, const char *id);

	// GRAPHICS ROUTINES
	double width;
	double height;

	bool m_filled;
	NosuchColor m_fill_color;
	double m_fill_alpha;
	bool m_stroked;
	NosuchColor m_stroke_color;
	double m_stroke_alpha;

	void fill(NosuchColor c, double alpha);
	void noFill();
	void stroke(NosuchColor c, double alpha);
	void noStroke();
	void strokeWeight(double w);
	void background(int);
	void rect(double x, double y, double width, double height);
	void pushMatrix();
	void popMatrix();
	void translate(double x, double y);
	void scale(double x, double y);
	void rotate(double degrees);
	void line(double x0, double y0, double x1, double y1);
	void triangle(double x0, double y0, double x1, double y1, double x2, double y2);
	void quad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3);
	void ellipse(double x0, double y0, double w, double h);
	void polygon(PointMem* p, int npoints);

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	VizServer* vizserver() { return _vizserver; }
	AllVizParams* defaultParams();
	std::string defaultPatch() { return _defaultPatch; }
	AllVizParams* regionParamsOf(std::string name);
	int CurrentClick();

	NosuchScheduler* scheduler() {
		NosuchAssert(_scheduler);
		return _scheduler;
	}

	Region* RegionOfRegionName(std::string s);
	Region* RegionOfButtonName(std::string s);
	Palette* palette() { return _palette; }
	Vizlet* vizlet();

	static void ErrorPopup(const char* msg);
	void CheckTimeouts(int millinow);
	void CheckVizCursorUp(int millinow);

	bool _do_tonicchange;
	std::string _tonicchangeclients;
	std::vector<std::string> _tonicchangeclienthost;
	std::vector<int> _tonicchangeclientport;


protected:	

	// Parameters
	// double m_brightness;

	Palette* _palette;
	NosuchScheduler* _scheduler;
	
	pthread_mutex_t palette_mutex;

	bool initialized;

	static bool StaticInitialized;
	static void StaticInitialization();

	void test_draw();

private:

	VizServer* _vizserver;
	Vizlet *_vizlet;
	bool _dotest;
	void readMidiConfig(std::ifstream& f);
	void initMidiConfig();
	std::string _midi_input_name;
	std::string _midi_output_name;

	// int _osc_input_port;
	// std::string _osc_input_host;
	// int _http_input_port;

	bool _do_tuio;
	bool _do_sharedmem;
	std::string _sharedmemname;
	bool _do_errorpopup;
	std::string _midiconfigFile;
	std::string _defaultPatch;
	std::vector<std::string> _alleffects;
	// std::string _patchFile;

	int _vizserver_osc_port;			// This is the port we're sending output TO
	std::string _vizserver_osc_host;
	int _pyffle_osc_port;		// This is the port we're sending output TO
	std::string _pyffle_osc_host;


};

#endif
