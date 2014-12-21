#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffutil.h"

#include <iostream>
#include <fstream>
#include <strstream>
#include <cstdlib> // for srand, rand
#include <ctime>   // for time

#ifdef _DEBUG
// We do this dance because we don't want Python.h to pull in python*_d.lib,
// since python*_d.lib is not part of the standard Python binary distribution.
#   undef _DEBUG
#   include "Python.h"
#   define _DEBUG
#else
#   include "Python.h"
#endif

#include "Vizlet.h"
#include "Pyffle.h"
#include "NosuchOsc.h"

#include "VizSprite.h"
#include "VizServer.h"

static CFFGLPluginInfo PluginInfo ( 
	Pyffle::CreateInstance,	// Create method
	"V998",		// Plugin unique ID
	"Pyffle",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Pyffle: a sample visual synth",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string PyfflePluginPath;
std::string PyfflePluginName = "Pyffle";
std::string PyfflePublicDir = "dummydir";
bool PyffleDisable = false;

std::string vizlet_name() {
	return "Pyffle_"+PyfflePluginName;
}
CFFGLPluginInfo& vizlet_plugininfo() {
	return PluginInfo;
}
void vizlet_setdll(std::string dllpath) {
	PyfflePluginPath = dllpath;
	std::string dllname = dllpath;
	size_t pos = dllname.find("pyffle");
	if ( pos == dllname.npos ) {
		NosuchDebug("Improperly-named pyffle dll file (%s), needs to be pyffle_*.dll",dllpath.c_str());
		PyffleDisable = true;
		return;
	}
	// Eliminate everything up to the Pyffle
	dllname = dllname.erase(0,pos+6);
	// Eliminate the .dll at the end
	dllname.erase(dllname.size()-4);

	if ( dllname.size() == 0 || dllname.at(0) != '_' ) {
		NosuchDebug("Improperly-named Pyffle dll file (%s), needs to be Pyffle_*.dll",dllpath.c_str());
		PyffleDisable = true;
		return;
	}
	PyfflePluginName = dllname.substr(1);  // remove the underscore
	PyfflePluginName = NosuchToUpper(PyfflePluginName.substr(0,1)) + PyfflePluginName.substr(1);
	DEBUGPRINT1(("Pyffle plugin name=%s dll=%s",PyfflePluginName.c_str(),dllpath.c_str()));
}

Pyffle::Pyffle() : Vizlet() {

	// _mmtt = NULL;
	_recompileFunc = NULL;
	_dotest = false;
	_passthru = true;

	_shutting_down = false;
	_dopython = true;
	_python_initialized = false;
	_python_disabled = false;
	_python_disable_on_exception = false;

	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

#ifdef PYFFLE_LOCK
	PyffleLockInit(&python_mutex,"python");
#endif

	if ( PyffleDisable ) {
		NosuchDebug("Disabling Pyffle plugin in constructor, dll=%s",PyfflePluginPath.c_str());
		DisableVizlet();
		_dopython = false;
	}
}

Pyffle::~Pyffle() {
}

DWORD __stdcall Pyffle::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	*ppInstance = new Pyffle();
	return (*ppInstance != NULL)? FF_SUCCESS : FF_FAIL;
}

void Pyffle::processCursor(VizCursor* c, int downdragup) {
	// NO OpenGL calls here
}

std::string Pyffle::processJson(std::string meth, cJSON *json, const char *id) {
	// NO OpenGL calls here
	throw NosuchException("Pyffle - Unrecognized method '%s'",meth.c_str());
}

void Pyffle::processMidiInput(MidiMsg* m) {
	// NO OpenGL calls here
}

void Pyffle::processMidiOutput(MidiMsg* m) {
	// NO OpenGL calls here
}

bool Pyffle::processDraw() {
	// OpenGL calls here

	if ( _shutting_down ) {
		return true;
	}

	if ( ! _python_initialized ) {
		if ( ! python_initStuff() ) {
			NosuchDebug("initStuff failed, disabling plugin!");
			_python_disabled = true;
			return false;
		}
		_python_initialized = true;
	}
	python_draw();

	return true;
}

void Pyffle::processDrawNote(MidiMsg* m) {
	// OpenGL calls here
}

////////////////////////////////////////////////////////////
//////////////////// PYTHON STUFF //////////////////////////
////////////////////////////////////////////////////////////

static PyObject*
getpythonfunc(PyObject *module, char *name)
{
	PyObject *f = PyObject_GetAttrString(module, name);
	if (!(f && PyCallable_Check(f))) {
		NosuchDebug("Unable to find python function: %s",name);
		return NULL;
	}
	return f;
}

PyObject*
Pyffle::python_lock_and_call(PyObject* func, PyObject *pArgs)
{
	python_lock();
	PyObject *msgValue = PyObject_CallObject(func, pArgs);
	python_unlock();
	return msgValue;
}

bool
Pyffle::python_recompileModule(const char *modulename)
{
	PyObject *pArgs;
	bool r = FALSE;

	if ( _recompileFunc == NULL ) {
		NosuchDebug("Hey, _recompileFunc is NULL!?");
		return FALSE;
	}

	pArgs = Py_BuildValue("(s)", modulename);
	if ( pArgs == NULL ) {
		NosuchDebug("Cannot create python arguments to recompile");
		goto getout;
	}            
	PyObject *msgValue = python_lock_and_call(_recompileFunc, pArgs);
	Py_DECREF(pArgs);
	if (msgValue == NULL) {
		NosuchDebug("Call to recompile of %s failed\n",modulename);
	} else if (msgValue == Py_None) {
		NosuchDebug("Call to recompile of %s returned None?\n",modulename);
	} else {
		char *msg = PyString_AsString(msgValue);
		r = (*msg != '\0') ? FALSE : TRUE;
		if ( r == FALSE ) {
			NosuchDebug("Call to recompile of %s failed, msg=%s\n",modulename,msg);
		}
		Py_DECREF(msgValue);
	}
getout:
	return r;
}

std::string
Pyffle::python_draw()
{
	if ( _processorDrawFunc == NULL ) {
		// This is expected when there's been
		// a syntax or execution error in the python code
		NosuchDebug(1,"_processorDrawFunc==NULL?");
		return "";
	}
	if ( _callBoundFunc == NULL ) {
		NosuchDebug("_callBoundFunc==NULL?");
		return "";
	}

	PyObject *pArgs = Py_BuildValue("(O)",_processorDrawFunc);
	if ( pArgs == NULL ) {
		return "Cannot create python arguments to _callBoundFunc";
	}            
	PyObject *msgobj = python_lock_and_call(_callBoundFunc, pArgs);
	Py_DECREF(pArgs);
	
	if (msgobj == NULL) {
		return "Call to _callBoundFunc failed";
	}
	if (msgobj == Py_None) {
		return "Call to _callBoundFunc returned None?";
	}
	std::string msg = std::string(PyString_AsString(msgobj));
	Py_DECREF(msgobj);
	if ( msg != "" ) {
		NosuchDebug("python_callprocessor returned msg = %s",msg.c_str());
		NosuchDebug("Disabling drawing function for plugin=%s",PyfflePluginName.c_str());
		_processorDrawFunc = NULL;
	}
	return msg;
}

PyObject*
Pyffle::python_getProcessorObject(std::string btype)
{
	NosuchDebug(1,"python_getProcessorObject A");
	const char* b = btype.c_str();
	PyObject *pArgs = Py_BuildValue("(s)", b);
	if ( pArgs == NULL ) {
		NosuchDebug("Cannot create python arguments to _getProcessorFunc");
		return NULL;
	}            
	NosuchDebug(1,"python_getProcessorObject B btype=%s getProcessorFunc=%ld",b,(long)_getProcessorFunc);
	PyObject *obj = python_lock_and_call(_getProcessorFunc, pArgs);
	Py_DECREF(pArgs);
	if (obj == NULL) {
		NosuchDebug("Call to _getProcessorFunc failed");
		return NULL;
	}
	NosuchDebug(1,"python_getProcessorObject C");
	if (obj == Py_None) {
		NosuchDebug("Call to _getProcessorFunc returned None?");
		return NULL;
	}
	NosuchDebug(1,"python_getProcessorObject D");
	return obj;
}

static PyObject* nosuchmedia_publicpath(PyObject* self, PyObject* args)
{
    const char* filename;
 
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
 
	std::string path = PyfflePath(std::string(filename));
	NosuchDebug("(python) path= %s",path.c_str());
	return PyString_FromString(path.c_str());
}
 
static PyObject* nosuchmedia_debug(PyObject* self, PyObject* args)
{
    const char* name;
 
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
 
	NosuchDebug("(python) %s",name);
 
    Py_RETURN_NONE;
}

static PyObject* nosuchmedia_glVertex2f(PyObject* self, PyObject* args)
{
	float v1, v2;

    if (!PyArg_ParseTuple(args, "ff", &v1, &v2))
        return NULL;
 
	// NosuchDebug("(python) glVertex2f %f %f",v1,v2);
	glVertex2f( v1,v2);
 
    Py_RETURN_NONE;
}
  
static PyMethodDef PyffleMethods[] =
{
     {"publicpath", nosuchmedia_publicpath, METH_VARARGS, "Return path to a public file."},
     {"debug", nosuchmedia_debug, METH_VARARGS, "Log debug output."},
     {"glVertex2f", nosuchmedia_glVertex2f, METH_VARARGS, "Invoke glVertext2f."},
     {NULL, NULL, 0, NULL}
};

bool
Pyffle::python_getUtilValues()
{
	if (!(_recompileFunc = getpythonfunc(_PyffleUtilModule,"recompile"))) {
		python_disable("Can't get recompile function from pyffle module?!");
		return FALSE;
	}

	if (!(_callBoundFunc=getpythonfunc(_PyffleUtilModule,"callboundfunc"))) {
		python_disable("Unable to find callboundfunc func");
		return FALSE;
	}

	if (!(_getProcessorFunc=getpythonfunc(_PyffleUtilModule,"getprocessor"))) {
		python_disable("Unable to find getprocessor func");
		return FALSE;
	}
	return TRUE;
}
  
bool Pyffle::python_init() {
	if ( ! Py_IsInitialized() ) {
		NosuchDebug("Pyffle(%s): initializing python",PyfflePluginName.c_str());
		Py_Initialize();
		if ( ! Py_IsInitialized() ) {
			python_disable("Unable to initialize python?");
			return FALSE;
		}
	} else {
		NosuchDebug("NOT initializing python, already running!");
	}

     (void) Py_InitModule("pyffle.builtin", PyffleMethods);

	// We want to add our directory to the sys.path
	std::string script = NosuchSnprintf(
		"import sys\n"
		"sys.path.insert(0,'%s')\n",
			PyffleForwardSlash(NosuchFullPath("../python")).c_str()
		);
	DEBUGPRINT1(("Running script=%s",script.c_str()));
	PyRun_SimpleString(script.c_str());

	const char* pyffleutil = "pyffle.util";

	PyObject *pName = PyString_FromString(pyffleutil);
    _PyffleUtilModule = PyImport_Import(pName);
    Py_DECREF(pName);

	if ( _PyffleUtilModule == NULL) {
		python_disable("Unable to import "+PyfflePluginName+" module");
		return FALSE;
	}

	if ( !python_getUtilValues() ) {
		python_disable("Failed in python_getUtilValues");
		return FALSE;
	}

	// Note: it's always pyffle, no matter what the Plugin name is, see comment above
	if ( python_recompileModule(pyffleutil) == FALSE ) {
		python_disable("Unable to recompile pyffle module");
		return FALSE;
	}

	// Not really sure this re-getting of the UtilValues is needed, or
	// makes a difference.  There's some kind of bug that happens occasionally
	// when there's an error (syntax or execution) in recompiling the module,
	// and this was an attempt to figure it out.
	if ( !python_getUtilValues() ) {
		python_disable("Failed in python_getUtilValues (second phase)");
		return FALSE;
	}
	
	if ( !python_change_processor(PyfflePluginName) ) {
		NosuchDebug("Unable to change processor to %s",PyfflePluginName.c_str());
		// No longer disable python when there's a problem in changing the processor.
		// python_disable(NosuchSnprintf("Unable to change processor to %s",PyfflePluginName.c_str()));
		return FALSE;
	}

	return TRUE;
}

bool
Pyffle::python_change_processor(std::string behavename) {

	PyObject *new_processorObj;
	PyObject *new_processorDrawFunc;

	NosuchDebug("python_change_processor behavename=%s",behavename.c_str());
	if ( !(new_processorObj = python_getProcessorObject(behavename))) {
		NosuchDebug("python_getProcessorObject returned NULL!");
		// _processorObj = NULL;
		_processorDrawFunc = NULL;
		return FALSE;
	}
	if ( !(new_processorDrawFunc = getpythonfunc(new_processorObj, "processOpenGL")) ) {
		// _processorObj = NULL;
		_processorDrawFunc = NULL;
		return FALSE;
	}

	// _processorObj = new_processorObj;
	_processorDrawFunc = new_processorDrawFunc;
	return TRUE;
}

void Pyffle::python_disable(std::string msg) {
	DEBUGPRINT(("python is being disabled!  msg=%s",msg.c_str()));
	_python_disabled = TRUE;
}

bool Pyffle::python_reloadPyffleUtilModule() {

	PyObject* newmod = PyImport_ReloadModule(_PyffleUtilModule);
	if ( newmod == NULL) {
		python_disable("Unable to reload pyffle module");
		return FALSE;
	}
	_PyffleUtilModule = newmod;

	return TRUE;
}

int Pyffle::python_runfile(std::string filename) {
	std::string fullpath = PyfflePath("python") + "\\" + filename;
	std::ifstream f(fullpath.c_str(), std::ifstream::in);
	if ( ! f.is_open() ) {
		NosuchErrorOutput("Unable to open python file: %s",fullpath.c_str());
		return 1;
	}
	std::string contents;
	std::string line;
	while (!std::getline(f,line,'\n').eof()) {
		contents += (line+"\n");
	}
	f.close();
	int r = PyRun_SimpleString(contents.c_str());
	if ( r != 0 ) {
		NosuchErrorOutput("Error executing contents of: %s",fullpath.c_str());
	}
	return r;
}

static bool
istrue(std::string s)
{
	return(s == "true" || s == "True" || s == "1");
}

bool Pyffle::python_initStuff() {

	NosuchDebug(2,"_python_initStuff starts");

	// test_stuff();

	bool r = false;
	try {
		// static initializations
		if ( _dopython ) {
			if ( ! python_init() ) {
				NosuchDebug("python_init failed!");
			} else {
				NosuchDebug("python_init succeeded!");
				r = true;
			}
		}
	} catch (NosuchException& e) {
		NosuchDebug("NosuchException: %s",e.message());
	} catch (...) {
		// Does this really work?  Not sure
		NosuchDebug("Some other kind of exception occured!?");
	}
	NosuchDebug(2,"_python_initStuff returns %s\n",r?"true":"false");
	return r;
}

std::string
PyfflePath(std::string filepath)
{
	return PyfflePublicDir + "\\Pyffle\\" + filepath;
}

std::string
PyfflePublicPath(std::string filepath)
{
	return PyfflePublicDir + "\\" + filepath;
}

void Pyffle::python_lock() {
	// We don't actually need this, right now, since FreeFrame plugins should never
	// be running simultaneously.
#ifdef NOSUCH_LOCK
	NosuchLock(&python_mutex,"python");
#endif
}

void Pyffle::python_unlock() {
#ifdef NOSUCH_LOCK
	NosuchUnlock(&python_mutex,"python");
#endif
}

std::string
PyffleForwardSlash(std::string filepath) {
	size_t i;
	while ( (i=filepath.find("\\")) != filepath.npos ) {
		filepath.replace(i,1,"/");
	}
	return filepath;
}
