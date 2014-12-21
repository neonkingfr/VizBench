#include <FFGL.h>
#include <FFGLLib.h>
#include "FFGLPluginSDK.h"
#include "FFGLPluginInfo.h"

#define FFGL_ALREADY_DEFINED
#include "PaletteFFHost.h"

#include <pthread.h>
#include <iostream>
#include <fstream>
#include <strstream>
#include <cstdlib> // for srand, rand
#include <ctime>   // for time
#include <sys/stat.h>

#include "NosuchUtil.h"

#define FFPARAM_HTTPPORT (0)
#define FFPARAM_OSCPORT (1)

static CFFGLPluginInfo PluginInfo ( 
	PaletteFFHost::CreateInstance,	// Create method
	"NSPL",		// Plugin unique ID
	"Palette",	// Plugin name	
	1,			// API major version number
	000,		// API minor version number	
	1,			// Plugin major version number
	000,		// Plugin minor version number
	FF_EFFECT,	// Plugin type
	"Space Palette: TUIO-controlled graphics and music",	// description
	"by Tim Thompson - me@timthompson.com" 			// About
);

std::string PalettePluginName = "";

DWORD __stdcall PaletteFFHost::CreateInstance(CFreeFrameGLPlugin **ppInstance) {
	extern std::string PalettePluginName;
	NosuchDebug("PaletteFFHost CreatInstance is creating!  PalettePluginName = %s\n",PalettePluginName.c_str());

	StaticInitialization();
	std::string configfile = NosuchSnprintf("../config/palette_%s.json",PalettePluginName.c_str());
	int palettenum = atoi(PalettePluginName.c_str());
	*ppInstance = new PaletteFFHost(NosuchFullPath(configfile),palettenum);
	if (*ppInstance != NULL)
		return FF_SUCCESS;
	return FF_FAIL;
}

PaletteFFHost::PaletteFFHost(std::string defaultsfile, int palettenum) : CFreeFrameGLPlugin(), PaletteHost(defaultsfile,palettenum)
{
	NosuchDebug(1,"PaletteFFHost is being constructed.");
	SetMinInputs(1);
	SetMaxInputs(1);

#ifdef PORT_PARAMS
	SetParamInfo(FFPARAM_HTTPPORT, "HTTP Port", FF_TYPE_STANDARD, 0.0f);
	SetParamInfo(FFPARAM_OSCPORT, "OSC Port", FF_TYPE_STANDARD, 0.0f);
	m_httpport = 0.0f;
	m_oscport = 0.0f;
#endif
}

PaletteFFHost::~PaletteFFHost() {
	NosuchDebug(1,"PaletteFFHost is being destroyed!");
}

DWORD PaletteFFHost::GetParameter(DWORD dwIndex)
{
    //sizeof(DWORD) must == sizeof(float)
#ifdef PORT_PARAMS
	DWORD dwRet;

	switch (dwIndex) {

	case FFPARAM_HTTPPORT:
	    *((float *)(unsigned)(&dwRet)) = m_httpport;
		return dwRet;

	case FFPARAM_OSCPORT:
	    *((float *)(unsigned)(&dwRet)) = m_oscport;
		return dwRet;

	default:
		return FF_FAIL;
	}
#else
	return FF_FAIL;
#endif
}

#ifdef PORT_PARAMS
int float2httpport(float f) {
	if ( f < 0.333 )
		return 4445;
	if ( f < 0.666 )
		return 4446;
	return 4447;
}
int float2oscport(float f) {
	if ( f < 0.333 )
		return 3333;
	if ( f < 0.666 )
		return 3334;
	return 3335;
}
#endif

DWORD PaletteFFHost::SetParameter(const SetParameterStruct* pParam)
{
#ifdef PORT_PARAMS
	if (pParam == NULL) {
		return FF_FAIL;
	}
		
	float newval;
	int newport;

	switch (pParam->ParameterNumber) {

	case FFPARAM_HTTPPORT:
		newval = *((float *)(unsigned)&(pParam->NewParameterValue));
		// if the actual mapped port value has changed, stop/start the listener.
		newport = float2httpport(newval);
		if ( float2httpport(m_httpport) != newport ) {
			NosuchDebug("http port changed, SHOULD BE STOP/STARTING HTTP!");
			ChangeHttpPort(newport);
		}
		m_httpport = newval;
		break;

	case FFPARAM_OSCPORT:
		newval = *((float *)(unsigned)&(pParam->NewParameterValue));
		// if the actual mapped port value has changed, stop/start the listener.
		newport = float2oscport(newval);
		if ( float2oscport(m_oscport) != newport ) {
			NosuchDebug("osc port changed, SHOULD BE STOP/STARTING OSC!");
			ChangeOscPort(newport);
		}
		m_oscport = newval;
		break;

	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
#else
	return FF_FAIL;
#endif
}

DWORD PaletteFFHost::ProcessOpenGL(ProcessOpenGLStruct *pGL) {
	return PaletteHostProcessOpenGL(pGL);
}

extern "C"
{

bool
ffgl_setdll(std::string dllpath)
{
	dllpath = NosuchToLower(dllpath);

	size_t lastslash = dllpath.find_last_of("/\\");
	size_t lastunder = dllpath.find_last_of("_");
	size_t lastdot = dllpath.find_last_of(".");
	std::string suffix = (lastdot==dllpath.npos?"":dllpath.substr(lastdot));

	if ( suffix != ".dll" ) {
		NosuchDebug("Hey! dll name (%s) isn't of the form *.dll!?",dllpath.c_str());
		return FALSE;
	}

	std::string look_for_prefix = "palette_";
	int look_for_len = look_for_prefix.size();

	std::string dir = dllpath.substr(0,lastslash);
	std::string prefix = dllpath.substr(lastslash+1,lastdot-lastslash-1);
	if ( NosuchToLower(prefix.substr(0,look_for_len)) != look_for_prefix ) {
		NosuchDebug("Hey! plugin name name (%s) isn't of the form */Palette_Name.dll, PLUGIN IS NOW DISABLED!",dllpath.c_str());
		return FALSE;
	}

	PalettePluginName = prefix.substr(look_for_len);  // i.e. remove the pyffle_
	size_t i = PalettePluginName.find("_debug");
	if ( i > 0 ) {
		PalettePluginName = PalettePluginName.substr(0,i);
	}

	NosuchCurrentDir = dir;

	NosuchDebugSetLogDirFile(dir,"debug.txt");

	struct _stat statbuff;
	int e = _stat(NosuchCurrentDir.c_str(),&statbuff);
	if ( ! (e == 0 && (statbuff.st_mode | _S_IFDIR) != 0) ) {
		NosuchDebug("Hey! No directory %s!?",NosuchCurrentDir.c_str());
		return FALSE;
	}

	NosuchDebug("Setting NosuchCurrentDir = %s",NosuchCurrentDir.c_str());

	char id[5];
	// Compute a hash of the plugin name and use two 4-bit values
	// from it to produce the last 2 characters of the unique ID.
	// It's possible there will be a collision.
	int hash = 0;
	for ( const char* p = PalettePluginName.c_str(); *p!='\0'; p++ ) {
		hash += *p;
	}
	id[0] = 'P';
	id[1] = 'A';
	id[2] = 'A' + (hash & 0xf);
	id[3] = 'A' + ((hash >> 4) & 0xf);
	id[4] = '\0';
	PluginInfo.SetPluginIdAndName(id,("Palette_"+PalettePluginName).c_str());

	return TRUE;
}
}

