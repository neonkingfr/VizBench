#ifndef _Pyffle_H
#define _Pyffle_H

class Pyffle : public Vizlet
{
public:
	Pyffle();
	~Pyffle();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	std::string processJson(std::string meth, cJSON *jsonparams, const char *id);
	void processMidiInput(MidiMsg* m);
	void processMidiOutput(MidiMsg* m);
	void processCursor(VizCursor* c, int downdragup);
	bool processDraw();
	void processDrawNote(MidiMsg* m);

private:
	// Put private things here.

	bool		python_recompileModule(const char *modulename);
	bool		python_initStuff();
	void		python_lock();
	void		python_unlock();
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
	PyObject *_processorObj;
	PyObject *_processorDrawFunc;
	PyObject *_getProcessorFunc;
	PyObject *_callBoundFunc;
    PyObject *_PyffleUtilModule;

	bool _python_initialized;
	bool _shutting_down;
	bool _dopython;
	bool _passthru;
	bool _python_disabled;
	bool _python_disable_on_exception;
	bool _dotest;
	void _initialize();

};

std::string PyfflePath(std::string file);
std::string PyfflePublicPath(std::string file);
std::string PyffleForwardSlash(std::string s);
extern std::string PyfflePluginName;

#endif
