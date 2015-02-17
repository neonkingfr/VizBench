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
FFPluginDef* findffplugin(std::string nm);
FFGLPluginDef * findffglplugin(std::string nm);
FFParameterDef* findffparam(FFPluginDef* ff, std::string nm);
FFGLParameterDef* findffglparam(FFGLPluginDef* ff, std::string nm);
void non_of_init(int x, int y, int w, int h);

void joyinit(int millipoll);
void joyrelease();
void joycheck();
void joyanalog(int jn, int axis, int v);
void joybutton(int jn, int jb, int v);

void http_init(int port, int timeout);
void http_check();
void http_send(char *s);
void http_sendto_slip(char *host, int port, const char *data, int size);

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
	std::string FFList();
	std::string FFGLList();
	std::string PipelineList(bool only_enabled);
	std::string FFParamList(std::string nm, const char* id);
	std::string FFGLParamList(std::string nm, const char* id);
	std::string FFGLParamVals(FFGLPluginInstance* pi, const char* id);
	std::string FFParamInfo(std::string plugin, std::string param, const char* id);
	std::string FFGLParamInfo(std::string plugin, std::string param, const char* id);
	std::string saveFfffPatch(std::string nm, const char* id);
	std::string loadFfffPatch(std::string nm, const char* id);

	void loadPluginDefs(std::string ffdir, std::string ffgldir, int w, int h);
	FFGLPluginInstance* addToPipeline(std::string pluginName, std::string inm, bool autoenable);
	void deleteFromPipeline(std::string inm);

	std::string newInstanceName();
	FFGLPluginInstance* findInstance(std::string inm);
	FFGLPluginInstance* needFFGLPluginInstance(std::string inm);
	void deletePluginInstance(FFGLPluginInstance* p);
	FFGLPluginInstance* newPluginInstance(FFGLPluginDef* plugin, std::string inm);

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

	FFGLPluginInstance* m_pipeline;

};

void loadffpath(std::string ffpath);
void loadffglpath(std::string ffglpath);
int FFGLinit2();

#endif
