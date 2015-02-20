#ifndef _FFFF_H
#define _FFFF_H

#include "pthread.h"
#include "NosuchDebug.h"
#include "NosuchException.h"
#include "opencv/cv.h"
#include <GLFW/glfw3.h>

#include "VizServer.h"

#include "ffutil.h"

int FFGLinit();
int FFGLinit2(int width, int height);
void socket_check();
void bonjour_check();

extern int 		camWidth;
extern int 		camHeight;
extern int		ffWidth;
extern int		ffHeight;
extern int		CV_interp;

void loadffplugindef(std::string fn);

FF10PluginDef* findff10plugin(std::string nm);
FF10ParameterDef* findff10param(FF10PluginDef* ff, std::string nm);

FFGLPluginDef * findffglplugin(std::string nm);
FFGLParameterDef* findffglparam(FFGLPluginDef* ff, std::string nm);

void non_of_init(int x, int y, int w, int h);

#if 0
void joyinit(int millipoll);
void joyrelease();
void joycheck();
void joyanalog(int jn, int axis, int v);
void joybutton(int jn, int jb, int v);

void http_init(int port, int timeout);
void http_check();
void http_send(char *s);
void http_sendto_slip(char *host, int port, const char *data, int size);
#endif

class FFFF;
class FFFFHttp;
struct CvCapture;
class Timer;

class FFFF : public NosuchJsonListener, NosuchOscListener {

public:
	FFFF( );
	static void ErrorPopup(const char* msg);
	void StartStuff();
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
	std::string FFGLParamVals(FFGLPluginInstance* pi, const char* id);
	std::string FFGLParamInfo(std::string plugin, std::string param, const char* id);

	FFGLPluginInstance* FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string inm);
	FFGLPluginInstance* FFGLFindPluginInstance(std::string inm);
	FFGLPluginInstance* FFGLNeedPluginInstance(std::string inm);
	void				FFGLDeletePluginInstance(FFGLPluginInstance* p);
	std::string			FFGLParamList(std::string nm, const char* id);
	FFGLPluginInstance* FFGLAddToPipeline(std::string nm, std::string inm, bool autoenable, cJSON* params);
	void				FFGLDeleteFromPipeline(std::string inm);

	///////////////////////// FF10 stuff

	std::string FF10List();
	std::string FF10PipelineList(bool only_enabled);
	std::string FF10ParamVals(FF10PluginInstance* pi, const char* id);
	std::string FF10ParamInfo(std::string plugin, std::string param, const char* id);

	FF10PluginInstance* FF10NewPluginInstance(FF10PluginDef* plugin, std::string inm);
	FF10PluginInstance* FF10FindPluginInstance(std::string inm);
	FF10PluginInstance* FF10NeedPluginInstance(std::string inm);
	void				FF10DeletePluginInstance(FF10PluginInstance* p);
	std::string			FF10ParamList(std::string nm, const char* id);
	FF10PluginInstance* FF10AddToPipeline(std::string pluginName, std::string inm, bool autoenable, cJSON* params);
	void				FF10DeleteFromPipeline(std::string inm);

	void loadFFPlugins(std::string ffdir, std::string ffgldir, int w, int h);

	std::string saveFfffPatch(std::string nm, const char* id);
	std::string loadFfffPatch(std::string nm, const char* id);

	void loadPipeline(std::string configname);
	void loadPipelineJson(cJSON* json);
	void clearPipeline();
	bool initCamera(int camindex);
	// void setWidthHeight(int w, int h) { _width = w; _height = h; }
	IplImage* getCameraFrame();
	bool doOneFrame(bool use_camera,int window_width, int window_height);
	void CheckFPS();

	void InsertKeystroke(int key, int downup);

	void SetShowFPS(bool b) { m_showfps = b; }
	GLFWwindow* window;
	bool hidden;

	std::string m_json_result;

private:
	VizServer* m_vizserver;
	CvCapture* m_capture;
	bool m_showfps;
	pthread_mutex_t _json_mutex;
	pthread_cond_t m_json_cond;
	bool m_json_pending;
	std::string m_json_method;
	cJSON* m_json_params;
	const char* m_json_id;

	// FPS stuff
	Timer* m_timer;
	double m_desired_FPS;
	double m_throttle_timePerFrame;
	double m_throttle_lasttime;
	int m_fps_accumulator;
	double m_fps_lasttime;

	FFGLPluginInstance* m_ffglpipeline;
	FF10PluginInstance* m_ff10pipeline;

};

void loadffpath(std::string ffpath);
void loadffglpath(std::string ffglpath);
int FFGLinit2();

#endif
