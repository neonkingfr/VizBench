#ifndef _PALETTEFFHOST_H
#define _PALETTEFFHOST_H

// Initially, in order to handle multiple PaletteFF plugins, I created
// Freeframe-accessible parameters for the HTTP and OSC ports.
// This turned out to be the wrong approach, though I did succeed in
// making it possible to change the HTTP and OSC ports on the fly, which
// may come in handy someday.
// #define PORT_PARAMS

#include "FFGL.h"
#include "FFGLPluginInfo.h"
#include "PaletteHost.h"

class PaletteFFHost : public PaletteHost, public CFreeFrameGLPlugin
{

public:

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance);

	PaletteFFHost(std::string defaultsfile,int palettenum);
	~PaletteFFHost();
	DWORD GetParameter(DWORD dwIndex);
	DWORD SetParameter(const SetParameterStruct* pParam);
	DWORD ProcessOpenGL(ProcessOpenGLStruct *pGL);
	DWORD ResolumeDeactivate() {
		NosuchDebug("HI FROM RESOLUMEDEACTIVATE in PaletteFFHost");
		_listening = false;
		return FF_FAIL;
	}

#ifdef PORT_PARAMS
protected:
	// Parameters
	float m_httpport;
	float m_oscport;
#endif

};

#endif
