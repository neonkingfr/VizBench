/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

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

#include "FFGLPluginSDK.h"
#include "ffutil.h"
#include "NosuchDebug.h"

#define FF_PARAM_ONETEXT (0)

class EmptyB : public CFreeFrameGLPlugin
{
public:
	EmptyB();
	~EmptyB();

	DWORD InitGL(const FFGLViewportStruct *vp) {
		return FF_SUCCESS; // Create OpenGL resources here
	}
	DWORD DeInitGL() {
		return FF_SUCCESS;
	}
	virtual char* GetParameterDisplay(DWORD dwIndex) {
		if ( _bad ) {
			return "Bad!";
		} else {
			return "Good!";
		}
	}
	DWORD	SetParameter(const SetParameterStruct* pParam) {
		if ( pParam == NULL ) {
			return FF_FAIL;
		}
		DWORD dwIndex = pParam->ParameterNumber;
		switch (dwIndex) {
		case FF_PARAM_ONETEXT:
			NosuchDebug("SetParameter in EmptyB!  val=%s",pParam->u.NewTextValue);
			if ( strcmp(pParam->u.NewTextValue,"bad") == 0 ) {
				_bad = true;
				return FF_FAIL;
			}
			break;
		default:
			return FF_FAIL;
		}
		return FF_SUCCESS;
	}
	DWORD	GetParameter(DWORD dwIndex) {
		DWORD dwRet;
		switch (dwIndex) {
		case FF_PARAM_ONETEXT:
			if ( _bad ) {
				*((char **)(unsigned)(&dwRet)) = "Bad2!";
			} else {
				*((char **)(unsigned)(&dwRet)) = "newText";
			}
			return dwRet;
		}
		return FF_FAIL;
	}

	DWORD	ProcessOpenGL(ProcessOpenGLStruct* pGL);

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance) {
		*ppInstance = new EmptyB();
		if (*ppInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

private:
	bool _passthru;
	bool _bad;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
	EmptyB::CreateInstance,		// Create method
	"NSEB",						// Plugin unique ID
	"EmptyB",					// Plugin name											
	1,							// API major version number 													
	000,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"EmptyB: Example FreeFrame Plugin This is a really long sentence to see what happens.",	// Plugin description
	"by Tim Thompson - me@timthompson.com" // About
);

EmptyB::EmptyB() : CFreeFrameGLPlugin()
{
	_passthru = true;  // if true, we need to have input textures
	_bad = false;

	// Input properties
	SetMinInputs(0);
	SetMaxInputs(1);

	SetParamInfo(FF_PARAM_ONETEXT,"OneText", FF_TYPE_TEXT, "defText");
}

EmptyB::~EmptyB()
{
	NosuchDebug(1,"EmptyB destructor called");
}

DWORD EmptyB::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	glDisable(GL_BLEND);
	// glEnable(GL_BLEND); // If this is enabled, alpha matters

	if ( _passthru ) {
		if ( ! ff_passthru(pGL) ) {
			return FF_FAIL;
		}
	}

	glColor4f(0.0,1.0,1.0,0.5);
	glLineWidth((GLfloat)10.0f);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-0.85f, 0.85f, 0.0f);	// Top Left
	glVertex3f( 0.85f, 0.85f, 0.0f);	// Top Right
	glVertex3f( 0.85f,-0.85f, 0.0f);	// Bottom Right
	glVertex3f(-0.85f,-0.85f, 0.0f);	// Bottom Left
	glEnd();

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);

	//restore default color
	glColor4f(1.f,1.f,1.f,1.f);
	
	return FF_SUCCESS;
}

#ifdef _WIN32
WINDOWS_DLLMAIN_FUNCTION(default_setdll)
#endif