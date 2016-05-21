#ifndef _FFFF_H
#define _FFFF_H

#include <opencv/cv.h>
#include "VizServer.h"
#include "AudioHost.h"
#include "VizUtil.h"
#include "FFGLPipeline.h"
#include <GLFW/glfw3.h>

class FFFF;
class Timer;
class SpoutSender;
struct CvCapture;

class FFFFState {
public:
	FFFFState() {
		statepath = VizConfigPath("state.json");
		state = NULL;
		if (NosuchFileExists(statepath)) {
			std::string err;
			state = jsonReadFile(statepath, err);
			if (!state) {
				NosuchErrorOutput("Unable to parse state file!? path=%s", statepath.c_str());
			}
		}
		if (!state) {
			state = cJSON_Parse("{ }");
			NosuchAssert(state);
		}
	};
	void save() {
		std::string err;
		if (!jsonWriteFile(statepath, state, err)) {
			std::string err = NosuchSnprintf("Unable to write state file!? path=%s", statepath.c_str());
			throw NosuchException(err.c_str());
		}
	}
	void set_pipeset(std::string pipeset) {
		cJSON_ReplaceItemInObject(state, "pipeset",
			cJSON_CreateString(pipeset.c_str()));
	}
	std::string pipeset() {
		return jsonNeedString(state,"pipeset","");
	}
private:
	cJSON* state;
	std::string statepath;
};

class FFFF : public NosuchJsonListener, NosuchOscListener {

public:
	FFFF();

	void InitGlExtensions();
	void FFGLinit2();
	void spoutInitTexture();
	void spoutInit();
	void InitCamera();

	void StartStuff();
	void RunStuff();
	void StopStuff();
	void ErrorPopup(const char* msg);

	// submitJson gets called from a different thread than the thread that calls
	// draw().  submitJson should save JSON things and then wait for draw() to
	// be called and actually execute them, before returning.
	virtual std::string submitJson(std::string method, cJSON *params, const char* id);

	void checkAndExecuteJSON();
	std::string executeJsonAndCatchExceptions(const std::string meth, cJSON *params, const char* id);
	std::string executeJson(const std::string meth, cJSON *params, const char* id);

	void parseVizTag(std::string viztag, int& pipenum, std::string& vtag);

	///////////////////////// FFGL stuff

	std::string FFGLList();
	std::string FFGLParamVals(FFGLPluginInstance* pi, std::string linebreak);
	std::string FFGLParamInfo(std::string plugin, std::string param, const char* id);

	// XXX - all the methods here should eventually go into FFGLPipeline
	std::string			FFGLParamList(std::string nm, const char* id);

	std::string savePipeline(int pipenum, std::string nm, const char* id);
	void clearPipeline(int pipenum);

	void doPipeline(int pipenum, int width, int height);

	///////////////////////// FF10 stuff

	std::string FF10List();
	std::string FF10PipelineList(int pipenum, bool only_enabled);
	std::string FF10ParamVals(FF10PluginInstance* pi, std::string linebreak);
	std::string FF10ParamInfo(std::string plugin, std::string param, const char* id);

	bool doCameraAndFF10Pipeline(int pipenum, GLuint texturehandle);

	FF10PluginInstance* FF10NewPluginInstance(FF10PluginDef* plugin, std::string viztag);
	FF10PluginInstance* FF10FindPluginInstance(int pipenum, std::string viztag);
	FF10PluginInstance* FF10NeedPluginInstance(int pipenum, std::string viztag);
	std::string			FF10ParamList(std::string nm, const char* id);
	FF10PluginInstance* FF10AddToPipeline(int pipenum, std::string pluginName, std::string viztag, bool autoenable, cJSON* params);
	void				FF10DeleteFromPipeline(int pipenum, std::string viztag);

	void loadAllPluginDefs();

	void savePipeset(std::string nm);
	void LoadPipeset(std::string nm);
	void loadPipesetJson(cJSON* json);

	std::string copyFile(cJSON *params, PathGenerator pathgen, const char* id);

	void CreateWindows();
	void drawWindowFinish();
	void drawPrefixFinish();
	void drawWindowPipelines();
	void drawPrefixPipelines();
	// void setWidthHeight(int w, int h) { _width = w; _height = h; }
	IplImage* getCameraFrame();
	void sendSpout();
	void CheckFPS();
	void CheckAutoload();

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

	GLFWwindow* window;
	GLFWwindow* preview;
	bool hidden;

	std::string m_json_result;

	std::string m_fontname;
	int m_fontsize;
	struct dtx_font *m_font;

	bool m_spout;
	SpoutSender* m_spoutsender;
	// The Spout documentation says the sender name buffer must be at least 256 characters long.
	char m_sendername[256];

	FFFFState* m_state;

private:
	static VizServer* m_vizserver;

	bool m_use_camera;
	int m_camera_index;
	IplImage* m_img1;
	IplImage* m_img2;
	IplImage* m_img_into_pipeline;
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
	int m_window_x;
	int m_window_y;

	int m_preview_width;
	int m_preview_height;
	int m_preview_x;
	int m_preview_y;

	int m_camWidth;
	int m_camHeight;

    std::string m_ffglpath;
    std::string m_ff10path;

	std::string m_pipesetname;
	std::string m_pipesetpath;

#define NPIPELINES 4

	FFGLPipeline m_ffglpipeline[NPIPELINES];
	FF10Pipeline m_ff10pipeline[NPIPELINES];
	bool m_autoload;
	bool m_autosave;

#ifdef DUMPOBJECTS
	_CrtMemState m_s0;
#endif
};

void loadff10path(std::string ffpath);
void loadffglpath(std::string ffglpath);

#endif
