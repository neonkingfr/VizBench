#ifndef _FFFF_H
#define _FFFF_H

#include "NosuchDebug.h"
#include "NosuchException.h"
#include <pthread.h>
#include <opencv/cv.h>
#include <GLFW/glfw3.h>

#include "VizServer.h"
#include "AudioHost.h"

#include "ffffutil.h"

#include "spout.h"

int FFGLinit();
int FFGLinit2(int width, int height);
void socket_check();
void bonjour_check();

extern int 		camWidth;
extern int 		camHeight;
extern int		ffWidth;
extern int		ffHeight;

extern std::string FfffOutputPrefix;
extern int FfffOutputFPS;
extern FILE* FfffOutputFile;

void loadffplugindef(std::string fn);

FF10PluginDef* findff10plugindef(std::string nm);
FF10ParameterDef* findff10param(FF10PluginDef* ff, std::string nm);

FFGLPluginDef * findffglplugindef(std::string nm);
FFGLParameterDef* findffglparam(FFGLPluginDef* ff, std::string nm);

void non_of_init(int x, int y, int w, int h);

class FFFF;
class FFFFHttp;
struct CvCapture;
class Timer;

typedef std::vector < FFGLPluginInstance* > FFGLPluginList;

class FFGLPipeline {
public:
	FFGLPipeline() {
		m_enabled = false;
		m_sidmin = 0;
		m_sidmax = 99999;
	};

	bool do_ffgl_plugin(FFGLPluginInstance* plugin, int which); // which: 0 = first one, 1 = middle, 2 = last, 3 = one and only one, 4 none
	bool doPipeline(int window_width, int window_height);
	FFGLPluginInstance* FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string viztag);

	void clear() {
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			DEBUGPRINT1(("--- deleteing FFGLPluginInstance *it=%ld", (long)(*it)));
			delete *it;
		}
		m_pluginlist.clear();
	}
	FFGLPluginInstance* find_plugin(std::string viztag) {
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			if (viztag == (*it)->viztag()) {
				return *it;
			}
		}
		return NULL;
	}
	void delete_plugin(std::string viztag) {
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end();) {
			if (viztag == (*it)->viztag()) {
				// DEBUGPRINT1(("--- deleteing BB FFGLPluginInstance *it=%ld", (long)(*it)));
				delete *it;
				it = m_pluginlist.erase(it);
			}
			else {
				it++;
			}
		}
	}
	void append_plugin(FFGLPluginInstance* p) {
		m_pluginlist.insert(m_pluginlist.end(), p);
	}
	void shuffle() {
		size_t sz = m_pluginlist.size();
		// Just swap things randomly
		for (size_t n = 1; n < sz; n++) {
			if ((rand() % 2) == 0) {
				if (m_pluginlist[n - 1]->isMoveable() && m_pluginlist[n]->isMoveable()) {
					FFGLPluginInstance* t = m_pluginlist[n - 1];
					m_pluginlist[n - 1] = m_pluginlist[n];
					m_pluginlist[n] = t;
				}
			}
		}
	}
	void randomize() {
		size_t sz = m_pluginlist.size();
		// Just swap things randomly
		for (size_t n = 1; n<sz; n++ ) {
			int n1 = rand() % sz;
			int n2 = rand() % sz;
			if ( n1 != n2 ) {
				if (m_pluginlist[n1]->isMoveable() && m_pluginlist[n2]->isMoveable()) {
					FFGLPluginInstance* t = m_pluginlist[n1];
					m_pluginlist[n1] = m_pluginlist[n2];
					m_pluginlist[n2] = t;
				}
			}
		}
	}
	void swap(int n1, int n2) {
		FFGLPluginInstance* t = m_pluginlist[n1];
		m_pluginlist[n1] = m_pluginlist[n2];
		m_pluginlist[n2] = t;
	}
	void moveup(std::string viztag) {
		size_t sz = m_pluginlist.size();
		for (size_t n = 0; n < sz; n++) {
			if (viztag == m_pluginlist[n]->viztag()) {
				if (n > 0) {
					swap(n - 1, n);
				}
				return;
			}
		}
	}
	void movedown(std::string viztag) {
		size_t sz = m_pluginlist.size();
		for (size_t n = 0; n<sz; n++ ) {
			if (viztag == m_pluginlist[n]->viztag()) {
				if (n < (sz - 1)) {
					swap(n + 1, n);
				}
				return;
			}
		}
	}

	FFGLPluginList m_pluginlist;
	bool m_enabled;
	std::string m_name;
	int m_sidmin;
	int m_sidmax;

	FFGLViewportStruct fboViewport;
	FFGLTextureStruct m_texture;
	FFGLFBO fbo1;
	FFGLFBO fbo2;
	FFGLFBO* fbo_input;
	FFGLFBO* fbo_output;
};

// typedef std::vector < FFGLPluginInstance* > FFGLPipeline;
typedef std::vector < FF10PluginInstance* > FF10Pipeline;

class FFFF : public NosuchJsonListener, NosuchOscListener {

public:
	FFFF(cJSON* config);

	int FFGLinit2(int width, int height);

	void ErrorPopup(const char* msg);
	bool StartStuff();
	void StopStuff();

	// submitJson gets called from a different thread than the thread that calls
	// draw().  submitJson should save JSON things and then wait for draw() to
	// be called and actually execute them, before returning.
	virtual std::string submitJson(std::string method, cJSON *params, const char* id);

	void checkAndExecuteJSON();
	std::string executeJsonAndCatchExceptions(const std::string meth, cJSON *params, const char* id);
	std::string executeJson(const std::string meth, cJSON *params, const char* id);

	///////////////////////// FFGL stuff

	std::string FFGLList();
	std::string FFGLPipelineList(int pipenum, bool only_enabled);
	std::string FFGLParamVals(FFGLPluginInstance* pi, std::string linebreak);
	std::string FFGLParamInfo(std::string plugin, std::string param, const char* id);

	// FFGLPluginInstance* FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string viztag);

	// XXX - all the methods here should eventually go into FFGLPipeline
	FFGLPluginInstance* FFGLFindPluginInstance(int pipenum, std::string viztag);
	FFGLPluginInstance* FFGLNeedPluginInstance(int pipenum, std::string viztag);
	std::string			FFGLParamList(std::string nm, const char* id);
	FFGLPluginInstance* FFGLAddToPipeline(int pipenum, std::string nm, std::string viztag, bool autoenable, cJSON* params);
	void				FFGLDeleteFromPipeline(int pipenum, std::string viztag);
	void				FFGLMoveUpInPipeline(int pipenum, std::string viztag);
	void				FFGLMoveDownInPipeline(int pipenum, std::string viztag);

	///////////////////////// FF10 stuff

	std::string FF10List();
	std::string FF10PipelineList(int pipenum, bool only_enabled);
	std::string FF10ParamVals(FF10PluginInstance* pi, std::string linebreak);
	std::string FF10ParamInfo(std::string plugin, std::string param, const char* id);

	FF10PluginInstance* FF10NewPluginInstance(FF10PluginDef* plugin, std::string viztag);
	FF10PluginInstance* FF10FindPluginInstance(int pipenum, std::string viztag);
	FF10PluginInstance* FF10NeedPluginInstance(int pipenum, std::string viztag);
	std::string			FF10ParamList(std::string nm, const char* id);
	FF10PluginInstance* FF10AddToPipeline(int pipenum, std::string pluginName, std::string viztag, bool autoenable, cJSON* params);
	void				FF10DeleteFromPipeline(int pipenum, std::string viztag);

	void loadAllPluginDefs(std::string ffdir, std::string ffgldir, int w, int h);

	std::string savePipeline(int pipenum, std::string nm, const char* id);
	void loadPipeline(int pipenum, std::string configname, bool synthesize);
	void loadPipelineJson(int pipenum, std::string configname, cJSON* json);
	void clearPipeline(int pipenum);
	void shufflePipeline(int pipenum);

	void savePipeset(std::string nm);
	void loadPipeset(std::string nm);
	void loadPipesetJson(cJSON* json);

	void randomizePipeline(int pipenum);
	bool initCamera(int camindex);
	// void setWidthHeight(int w, int h) { _width = w; _height = h; }
	IplImage* getCameraFrame();
	bool doCameraAndFF10Pipeline(int pipenum, bool use_camera, GLuint texturehandle);
	bool doPipeline(int pipenum, int width, int height);
	void sendSpout(int width, int height);
	void CheckFPS();

	void InsertKeystroke(int key, int downup);

	GLFWwindow* window;
	bool hidden;

	std::string m_json_result;

	bool m_spout;
	SpoutSender* m_spoutsender;
	// The Spout documentation says the sender name buffer must be at least 256 characters long.
	char m_sendername[256];

private:
	IplImage* m_img1;
	IplImage* m_img2;
	IplImage* m_img_into_pipeline;
	VizServer* m_vizserver;
	CvCapture* m_capture;
	bool m_showfps;
	bool m_record;
	AudioHost* m_audiohost;
	std::string m_audiohost_type;
	pthread_mutex_t _json_mutex;
	pthread_cond_t m_json_cond;
	bool m_json_pending;
	std::string m_json_method;
	cJSON* m_json_params;
	const char* m_json_id;

	GLubyte *m_output_framedata;
	double m_output_lastwrite;  // in seconds
	int m_output_framenum;

	// FPS stuff
	Timer* m_timer;
	double m_desired_FPS;
	double m_throttle_timePerFrame;
	double m_throttle_lasttime;
	int m_fps_accumulator;
	double m_fps_lasttime;

	int m_window_width;
	int m_window_height;

	std::string m_pipeset_filename;

#define NPIPELINES 4

	FFGLPipeline m_ffglpipeline[NPIPELINES];
	FF10Pipeline m_ff10pipeline[NPIPELINES];
};

void loadff10path(std::string ffpath);
void loadffglpath(std::string ffglpath);
int FFGLinit2();

#endif
