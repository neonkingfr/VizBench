//
// Copyright (c) 2004 - InfoMus Lab - DIST - University of Genova
//
// InfoMus Lab (Laboratorio di Informatica Musicale)
// DIST - University of Genova 
//
// http://www.infomus.dist.unige.it
// news://infomus.dist.unige.it
// mailto:staff@infomus.dist.unige.it
//
// Developer: Gualtiero Volpe
// mailto:volpe@infomus.dist.unige.it
//
// Developer: Trey Harrison
// mailto:trey@treyharrison.com
//
// Last modified: Oct. 26 2006
//

#include "FFGLPluginSDK.h"
#include <stdio.h>
#include <memory.h>

// Buffer used by the default implementation of getParameterDisplay
#define DISPLAY_VALUE_LENGTH 10
static char s_DisplayValue[DISPLAY_VALUE_LENGTH];


////////////////////////////////////////////////////////
// CFreeFrameGLPlugin constructor and destructor
////////////////////////////////////////////////////////

CFreeFrameGLPlugin::CFreeFrameGLPlugin() : CFFGLPluginManager()
{
	m_pPlugin = NULL;
}

CFreeFrameGLPlugin::~CFreeFrameGLPlugin() 
{
}


////////////////////////////////////////////////////////
// Default implementation of CFreeFrameGLPlugin methods
////////////////////////////////////////////////////////

char* CFreeFrameGLPlugin::GetParameterDisplay(DWORD dwIndex) 
{	
	DWORD dwType = m_pPlugin->GetParamType(dwIndex);
	DWORD dwValue = m_pPlugin->GetParameter(dwIndex);

	if ((dwValue != FF_FAIL) && (dwType != FF_FAIL))
  {
		if (dwType == FF_TYPE_TEXT)
    {
			return (char *)dwValue;
    }
		else
    {
			float fValue;
			memcpy(&fValue, &dwValue, 4);
			memset(s_DisplayValue, 0, DISPLAY_VALUE_LENGTH);
			sprintf_s(s_DisplayValue, DISPLAY_VALUE_LENGTH, "%f", fValue);
			return s_DisplayValue;
		}
	}
	return NULL;
}			

DWORD CFreeFrameGLPlugin::SetParameter(const SetParameterStruct* pParam) 
{
	return FF_FAIL;
}		

DWORD CFreeFrameGLPlugin::GetParameter(DWORD dwIndex) 
{ 
	return FF_FAIL;
}					

DWORD CFreeFrameGLPlugin::GetInputStatus(DWORD dwIndex)
{
	if (dwIndex >= (DWORD)GetMaxInputs()) return FF_FAIL;
	return FF_INPUT_INUSE;
}