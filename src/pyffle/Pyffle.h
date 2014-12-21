#ifndef PYFFLE_H
#define PYFFLE_H

#include "FFGLPluginSDK.h"
#include "NosuchDebug.h"

class Pyffle;
// class MmttApp;

#define DEFAULT_RESOLUME_PORT 7000

class Pyffle : public CFreeFrameGLPlugin
{
public:
	Pyffle();
	~Pyffle();

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////
	
	DWORD	SetParameter(const SetParameterStruct* pParam);		
	DWORD	GetParameter(DWORD dwIndex);					
	DWORD	ProcessOpenGL(ProcessOpenGLStruct* pGL);

	virtual DWORD InitGL(const FFGLViewportStruct *vp) {
		NosuchDebug(2,"Hi from Pyffle::InitGL!");
		return FF_SUCCESS;
	}
	virtual DWORD DeInitGL() {
		NosuchDebug(2,"Hi from Pyffle::DeInitGL!");
		return FF_SUCCESS;
	}

	void test_stuff();
	bool initStuff();
	void lock_python();
	void unlock_python();
	bool enable_mmtt();

	bool disable_on_exception;
	bool disabled;

	// GRAPHICS ROUTINES
	float width;
	float height;

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance) {
		NosuchDebug(2,"Pyffle.h CreatInstance is creating!\n");

		if ( ! StaticInitialized ) {
			StaticInitialization();
			StaticInitialized = true;
		}

		*ppInstance = new Pyffle();
		if (*ppInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

protected:	

#ifdef PYFFLE_LOCK
	pthread_mutex_t python_mutex;
#endif

	bool gl_shutting_down;
	bool initialized;

	static bool StaticInitialized;
	static void StaticInitialization();
	static bool read_config(std::string filename);

	bool		python_recompileModule(const char *modulename);
	bool		python_init();
	bool		python_getUtilValues();
	int			python_runfile(std::string filename);
	bool		python_reloadPyffleUtilModule();
	void		python_disable(std::string msg);
	std::string python_draw();
	bool		python_change_processor(std::string behavename);
	PyObject*	python_getProcessorObject(std::string btype);
	PyObject*	python_lock_and_call(PyObject* func, PyObject *pArgs);

	PyObject *_recompileFunc;
	// PyObject *_processorObj;
	PyObject *_processorDrawFunc;
	PyObject *_getProcessorFunc;
	PyObject *_callBoundFunc;
    PyObject *_PyffleUtilModule;

private:
	// MmttApp* _mmtt;
	bool _dopython;
	bool _passthru;
	bool _python_disabled;
	bool _dotest;
	void _initialize();
};

std::string PyfflePath(std::string file);
std::string PyfflePublicPath(std::string file);
std::string PyffleForwardSlash(std::string s);
extern std::string PyfflePluginName;

#endif
