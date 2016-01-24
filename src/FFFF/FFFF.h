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

// bool do_ffgl_plugin(FFGLPluginInstance* plugin1,int which, GLuint* output_texturehandle = NULL);

class FFFF;
class FFFFHttp;
struct CvCapture;
class Timer;

class FFFF : public NosuchJsonListener, NosuchOscListener {

public:
	FFFF(cJSON* config);
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
	std::string FFGLPipelineList(bool only_enabled);
	std::string FFGLParamVals(FFGLPluginInstance* pi, std::string linebreak);
	std::string FFGLParamInfo(std::string plugin, std::string param, const char* id);

	FFGLPluginInstance* FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string viztag);
	FFGLPluginInstance* FFGLFindPluginInstance(std::string viztag);
	FFGLPluginInstance* FFGLNeedPluginInstance(std::string viztag);
	std::string			FFGLParamList(std::string nm, const char* id);
	FFGLPluginInstance* FFGLAddToPipeline(std::string nm, std::string viztag, bool autoenable, cJSON* params);
	void				FFGLDeleteFromPipeline(std::string viztag);
	void				FFGLMoveUpInPipeline(std::string viztag);
	void				FFGLMoveDownInPipeline(std::string viztag);

	///////////////////////// FF10 stuff

	std::string FF10List();
	std::string FF10PipelineList(bool only_enabled);
	std::string FF10ParamVals(FF10PluginInstance* pi, std::string linebreak);
	std::string FF10ParamInfo(std::string plugin, std::string param, const char* id);

	FF10PluginInstance* FF10NewPluginInstance(FF10PluginDef* plugin, std::string viztag);
	FF10PluginInstance* FF10FindPluginInstance(std::string viztag);
	FF10PluginInstance* FF10NeedPluginInstance(std::string viztag);
	std::string			FF10ParamList(std::string nm, const char* id);
	FF10PluginInstance* FF10AddToPipeline(std::string pluginName, std::string viztag, bool autoenable, cJSON* params);
	void				FF10DeleteFromPipeline(std::string viztag);

	void loadAllPluginDefs(std::string ffdir, std::string ffgldir, int w, int h);

	std::string savePipeline(std::string nm, const char* id);

	void loadPipeline(std::string configname, bool synthesize);
	void loadPipelineJson(cJSON* json);
	void clearPipeline();
	void shufflePipeline();
	void randomizePipeline();
	bool initCamera(int camindex);
	// void setWidthHeight(int w, int h) { _width = w; _height = h; }
	IplImage* getCameraFrame();
	bool doOneFrame(bool use_camera,int window_width, int window_height);
	void CheckFPS();

	bool do_ffgl_plugin(FFGLPluginInstance* plugin, int which); // which: 0 = first one, 1 = middle, 2 = last, 3 = one and only one, 4 none

	void *imagewriter_thread(void *arg);
	void imagewriter_addimage(IplImage* img);

	void InsertKeystroke(int key, int downup);

	GLFWwindow* window;
	bool hidden;

	std::string m_json_result;

	bool m_spout;
	SpoutSender* m_spoutsender;
	// The Spout documentation says the sender name buffer must be at least 256 characters long.
	char m_sendername[256];

private:
	std::string m_pipelinename;
	IplImage* m_img1;
	IplImage* m_img2;
	IplImage* m_img_into_pipeline;
	VizServer* m_vizserver;
	CvCapture* m_capture;
	pthread_t m_imagewriter_thread;
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

	// FFGLPluginInstance* m_ffglpipeline;
	// FF10PluginInstance* m_ff10pipeline;

	std::vector < FFGLPluginInstance* > m_ffglpipeline;
	std::vector < FF10PluginInstance* > m_ff10pipeline;

};

void loadff10path(std::string ffpath);
void loadffglpath(std::string ffglpath);
int FFGLinit2();

#endif
