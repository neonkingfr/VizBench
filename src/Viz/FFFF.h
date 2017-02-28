#ifndef _FFFF_H
#define _FFFF_H

#include <opencv/cv.h>
#include "VizServer.h"
#include "AudioHost.h"
#include "VizFFUtil.h"
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
		if (VizFileExists(statepath)) {
			std::string err;
			state = jsonReadFile(statepath, err);
			if (!state) {
				VizErrorOutput("Unable to parse state file!? path=%s", statepath.c_str());
			}
		}
		if (!state) {
			state = cJSON_Parse("{ }");
			VizAssert(state);
		}
	};
	void save() {
		std::string err;
		if (!jsonWriteFile(statepath, state, err)) {
			std::string err = VizSnprintf("Unable to write state file!? path=%s", statepath.c_str());
			throw VizException(err.c_str());
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

class FFFF : public VizJsonListener, VizOscListener {

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

	bool paintInitialTexture(IplImage* camframe, GLuint texturehandle, bool flipx, bool flipy);

	void loadAllPluginDefs();

	void savePipeset(std::string nm);
	void LoadPipeset(std::string nm);
	void loadPipesetJson(cJSON* json);

	std::string copyFile(cJSON *params, PathGenerator pathgen, const char* id);

	void CreateWindows();
	void computePipelineTextures();
	void drawWindowFinish();
	void drawPreviewFinish();
	void drawWindowPipelines();
	void drawPreviewPipelines();
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

	int m_camera_index;
	bool m_camera_flipx;
	bool m_camera_flipy;
	IplImage* m_camera_image_raw;
	IplImage* m_camera_image_flipped;
	IplImage* m_img_into_pipeline;
	CvCapture* m_camera_capture;
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

	std::string m_pipesetname;
	std::string m_pipesetpath;

	// At some point, a Pipeset class would be nice
#define NPIPELINES 4

	FFGLPipeline m_ffglpipeline[NPIPELINES];
	bool m_pipeline_enabled[NPIPELINES];
	bool m_pipeline_camera_enabled[NPIPELINES];
	std::string m_pipeline_spriteparams[NPIPELINES];
	std::string m_pipeline_midiparams[NPIPELINES];

	bool m_autoload;
	bool m_autosave;

#ifdef DUMPOBJECTS
	_CrtMemState m_s0;
#endif
};

void loadffglpath(std::string ffglpath);

#endif
