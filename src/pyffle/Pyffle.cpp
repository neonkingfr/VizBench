/*
	Space Manifold - a variety of tools for Kinect and FreeFrame

	Copyright (c) 2011-2012 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <FFGL.h>
#include <FFGLLib.h>
#include <stdint.h>

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

#include "NosuchException.h"
#include "NosuchUtil.h"
#include "Pyffle.h"
#include "NosuchDebug.h"

#include <pthread.h>

// #define FFPARAM_BRIGHTNESS (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
	Pyffle::CreateInstance,		// Create method
	"PY00",						// Plugin unique ID
	"Pyffle",					// Plugin name											
	1,							// API major version number 													
	000,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"Pyffle: Python FreeFrame Language Extension",	// Plugin description
	"by Tim Thompson - me@timthompson.com" // About
);

bool Pyffle::StaticInitialized = false;

Pyffle* ThisPyffle = NULL;

void Pyffle::StaticInitialization()
{
	// Default debugging stuff
	// This currently duplicates the values in the definitions, but
	// eventually these values should come from an init file

	NosuchDebug(1,"Pyffle StaticInitialization!");
}

Pyffle::Pyffle() : CFreeFrameGLPlugin()
{
	ThisPyffle = this;
	NosuchDebug(1,"Pyffle constructor this=%ld",(long)this);

	// _mmtt = NULL;
	_recompileFunc = NULL;
	_dopython = true;
	_python_disabled = false;
	_dotest = false;
	_passthru = true;

	initialized = false;
	gl_shutting_down = false;

	width = 1.0f;
	height = 1.0f;

	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

#ifdef PYFFLE_LOCK
	PyffleLockInit(&python_mutex,"python");
#endif

	disabled = false;
	disable_on_exception = false;
}

Pyffle::~Pyffle()
{
	NosuchDebug(1,"Pyffle destructor called");
	// app_destroy();
	gl_shutting_down = true;
}

static PyObject*
getpythonfunc(PyObject *module, char *name)
{
	PyObject *f = PyObject_GetAttrString(module, name);
	if (!(f && PyCallable_Check(f))) {
		NosuchErrorOutput("Unable to find python function: %s",name);
		return NULL;
	}
	return f;
}

PyObject*
Pyffle::python_lock_and_call(PyObject* func, PyObject *pArgs)
{
	lock_python();
	PyObject *msgValue = PyObject_CallObject(func, pArgs);
	unlock_python();
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
	NosuchDebug("Running script=%s",script.c_str());
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
	NosuchErrorOutput("python is being disabled!  msg=%s",msg.c_str());
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

void Pyffle::_initialize() {
	NosuchDebug("Pyffle::_initialize start");
	if ( _dopython ) {
		if ( ! python_init() ) {
			NosuchDebug("python_init failed!");
		} else {
			NosuchDebug("python_init succeeded!");
			NosuchDebug("NosuchDebug python_init succeeded!");
		}
	}
}

#if 0
bool Pyffle::enable_mmtt()
{
	if ( _mmtt ) {
		NosuchDebug("Hey, Pyffle::enableMmtt called when it's already enabled!?");
		return true;
	}
	NosuchDebug("About to call app_create for this=%ld",(long)this);
	if ( ThisApp ) {
		NosuchDebug("NOT calling app_create because ThisApp is non-NULL");
	} else {
		app_create(0,NULL);
		_mmtt = ThisApp;
		app_execute("set_drawmode","{ \"mode\": \"python\" }");
	}
	return true;
}
#endif

bool Pyffle::initStuff() {

	NosuchDebug(2,"initStuff starts");

	bool r = true;
	try {
		// static initializations
		Pyffle::_initialize();
	} catch (NosuchException& e) {
		NosuchDebug("NosuchException: %s",e.message());
		r = false;
	} catch (...) {
		// Does this really work?  Not sure
		NosuchDebug("Some other kind of exception occured!?");
		r = false;
	}
	NosuchDebug(2,"initStuff returns %s\n",r?"true":"false");
	return r;
}

DWORD Pyffle::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	if ( gl_shutting_down ) {
		return FF_SUCCESS;
	}
	if ( disabled ) {
		return FF_SUCCESS;
	}

	if ( ! initialized ) {
		if ( ! initStuff() ) {
			NosuchDebug("initStuff failed, disabling plugin!");
			disabled = true;
			return FF_FAIL;
		}
		initialized = true;
	}

#if 0
	if ( _mmtt ) {
		app_update();
	}
#endif

#ifdef FRAMELOOPINGTEST
	static int framenum = 0;
	static bool framelooping = FALSE;
#endif

	if ( _passthru ) {
		if (pGL->numInputTextures<1)
			return FF_FAIL;

		if (pGL->inputTextures[0]==NULL)
			return FF_FAIL;
  
		FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

		//bind the texture handle
		glBindTexture(GL_TEXTURE_2D, Texture.Handle);
	}

#ifdef DOITINPYTHON
	glDisable(GL_TEXTURE_2D); 

	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glLineWidth((GLfloat)3.0f);
#endif

	bool gotexception = false;
	try {
		CATCH_NULL_POINTERS;

		NosuchDebug(2,"ProcessOpenGL");
		if ( _dopython && ! _python_disabled ) {
			python_draw();
		}
	} catch (NosuchException& e ) {
		NosuchDebug("NosuchException in python_draw : %s",e.message());
		gotexception = true;
	} catch (...) {
		NosuchDebug("UNKNOWN Exception in python_draw!");
		gotexception = true;
	}

	if ( gotexception && disable_on_exception ) {
		NosuchDebug("DISABLING Pyffle due to exception!!!!!");
		disabled = true;
	}

#if 0
	if ( _mmtt ) {
		app_draw();
	}
#endif

	glDisable(GL_BLEND); 
	glEnable(GL_TEXTURE_2D); 

#ifdef FRAMELOOPINGTEST
	int w = Texture.Width;
	int h = Texture.Height;
#define NFRAMES 300
	static GLubyte* pixelArray[NFRAMES];
	if ( framelooping ) {
		glRasterPos2i(-1,-1);
		glDrawPixels(w,h,GL_RGB,GL_UNSIGNED_BYTE,pixelArray[framenum]);
		framenum = (framenum+1)%NFRAMES;
	} else {
		if ( framenum < NFRAMES ) {
			pixelArray[framenum] = new GLubyte[w*h*3];
			glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixelArray[framenum]);
			framenum++;
		} else {
			framelooping = TRUE;
			framenum = 0;
		}
	}
#endif

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);
	
	//restore default color
	glColor4f(1.f,1.f,1.f,1.f);
	
	return FF_SUCCESS;
}

void Pyffle::lock_python() {
	// We don't actually need this, right now, since FreeFrame plugins should never
	// be running simultaneously.
#ifdef PYFFLE_LOCK
	PyffleLock(&python_mutex,"python");
#endif
}

void Pyffle::unlock_python() {
#ifdef PYFFLE_LOCK
	PyffleUnlock(&python_mutex,"python");
#endif
}

DWORD Pyffle::GetParameter(DWORD dwIndex)
{
	return FF_FAIL;  // no parameters
}

DWORD Pyffle::SetParameter(const SetParameterStruct* pParam)
{
	return FF_FAIL;  // no parameters
}
std::string
PyffleForwardSlash(std::string filepath) {
	size_t i;
	while ( (i=filepath.find("\\")) != filepath.npos ) {
		filepath.replace(i,1,"/");
	}
	return filepath;
}

std::string PyfflePublicDir = "";
std::string PyfflePluginName = "";
std::string PyfflePythonDir = "";

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

extern "C" {

bool
ffgl_setdll(std::string dllpath)
{
	// No longer convert entire string to lowercase, because the plugin names are case-sensitive
	// dllpath = NosuchToLower(dllpath);

	size_t lastslash = dllpath.find_last_of("/\\");
	size_t lastunder = dllpath.find_last_of("_");
	size_t lastdot = dllpath.find_last_of(".");
	std::string suffix = (lastdot==dllpath.npos?"":dllpath.substr(lastdot));

	if ( NosuchToLower(suffix) != ".dll"
		|| lastslash == dllpath.npos
		|| lastunder == dllpath.npos
		|| lastdot == dllpath.npos ) {

		NosuchDebug("Hey! dll name (%s) isn't of the form */Pyffle_Name.dll, PLUGIN IS NOW DISABLED!",dllpath.c_str());
		return FALSE;
	}

	std::string look_for_prefix = "pyffle_";
	int look_for_len = look_for_prefix.size();

	std::string dir = dllpath.substr(0,lastslash);
	std::string prefix = dllpath.substr(lastslash+1,lastdot-lastslash-1);
	if ( NosuchToLower(prefix.substr(0,look_for_len)) != look_for_prefix ) {
		NosuchDebug("Hey! plugin name name (%s) isn't of the form */Pyffle_Name.dll, PLUGIN IS NOW DISABLED!",dllpath.c_str());
		return FALSE;
	}

	PyfflePluginName = prefix.substr(look_for_len);  // i.e. remove the pyffle_
	size_t i = PyfflePluginName.find("_debug");
	if ( i > 0 ) {
		PyfflePluginName = PyfflePluginName.substr(0,i);
	}
	// PyffleDebugPrefix = "Pyffle_"+PyfflePluginName+": ";
	PyfflePublicDir = dir;
	PyfflePythonDir = dir + "\\python";

	NosuchCurrentDir = dir;

	NosuchDebugSetLogDirFile(dir,"debug.txt");

	NosuchDebug(1,"Setting PyfflePublicDir = %s",PyfflePublicDir.c_str());

	struct _stat statbuff;
	int e = _stat(PyfflePublicDir.c_str(),&statbuff);
	if ( ! (e == 0 && (statbuff.st_mode | _S_IFDIR) != 0) ) {
		NosuchDebug("Hey! No directory %s!?",PyfflePublicDir.c_str());
		return FALSE;
	}

	char id[5];
	// Compute a hash of the plugin name and use two 4-bit values
	// from it to produce the last 2 characters of the unique ID.
	// It's possible there will be a collision.
	int hash = 0;
	for ( const char* p = PyfflePluginName.c_str(); *p!='\0'; p++ ) {
		hash += *p;
	}
	id[0] = 'P';
	id[1] = 'Y';
	id[2] = 'A' + (hash & 0xf);
	id[3] = 'A' + ((hash >> 4) & 0xf);
	id[4] = '\0';
	PluginInfo.SetPluginIdAndName(id,("Pyffle_"+PyfflePluginName).c_str());

	return TRUE;
}

}
