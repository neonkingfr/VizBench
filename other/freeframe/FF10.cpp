////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FF10.cpp
//
// FreeFrame is an open-source cross-platform real-time video effects plugin system.
// It provides a framework for developing video effects plugins and hosts on Windows, 
// Linux and Mac OSX. 
// 
// Copyright (c) 2002, 2003, 2004, 2006 www.freeframe.org
// All rights reserved. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Redistribution and use in source and binary forms, with or without modification, 
//	are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of FreeFrame nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
//	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
//	IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
//	INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
//	OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
//	OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
//	OF THE POSSIBILITY OF SUCH DAMAGE. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// First version, Marcus Clements (marcus@freeframe.org) 
// www.freeframe.org
//
// FreeFrame 1.0 upgrade by Russell Blakeborough
// email: boblists@brightonart.org
//
// FreeFrame 1.0 - 03 upgrade 
// and implementation of FreeFrame SDK methods by Gualtiero Volpe
// email: Gualtiero.Volpe@poste.it
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Includes 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "FF10PluginSDK.h"
#include <memory.h>
#include "NosuchDebug.h"
#include "VizServer.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static and extern variables used in the FreeFrame SDK 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern CFF10PluginInfo* g_CurrPluginInfo;

static CFreeFrame10Plugin* s_pPrototype = NULL;
static VizServer* s_vizserver = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FreeFrame SDK default implementation of the FreeFrame global functions. 
// Such function are called by the plugMain function, the only function a plugin exposes.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *FF10getInfo() 
{
	return (void *)(g_CurrPluginInfo->GetPluginInfo());
}

DWORD FF10initialise()
{
	if (g_CurrPluginInfo==NULL)
		return FF_FAIL;

	if (s_pPrototype==NULL) {
		//get the instantiate function pointer
		FPCREATEINSTANCE10 *pInstantiate = g_CurrPluginInfo->GetFactoryMethod();

		//call the instantiate function
		DWORD dwRet = pInstantiate(&s_pPrototype);

		//make sure the instantiate call worked
		if ((dwRet == FF_FAIL) || (s_pPrototype == NULL))
			return FF_FAIL;

		return FF_SUCCESS;
	}

	return FF_SUCCESS; 
}

DWORD FF10deInitialise()
{
	DEBUGPRINT1(("FF10deInitialise in FF10.cpp called"));
	if (s_pPrototype != NULL) {
		delete s_pPrototype;
		s_pPrototype = NULL;
	}
	return FF_SUCCESS;
}

DWORD getNumParameters() 
{
	if (s_pPrototype == NULL) {
		DWORD dwRet = FF10initialise();
		if (dwRet == FF_FAIL) return FF_FAIL;
	}

	return (DWORD) s_pPrototype->GetNumParams();
}
							
char* getParameterName(DWORD index)
{
	if (s_pPrototype == NULL) {
		DWORD dwRet = FF10initialise();
		if (dwRet == FF_FAIL) return NULL;
	}
	
	return s_pPrototype->GetParamName(index);
}

DWORD getParameterDefault(DWORD index)
{
	if (s_pPrototype == NULL) {
		DWORD dwRet = FF10initialise();
		if (dwRet == FF_FAIL) return FF_FAIL;
	}

	void* pValue = s_pPrototype->GetParamDefault(index);
	if (pValue == NULL) return FF_FAIL;
	else {
		DWORD dwRet;
		memcpy(&dwRet, pValue, 4);
		return dwRet;
	}
}

DWORD getPluginCaps(DWORD index)
{
	int MinInputs = -1;
	int MaxInputs = -1;

	if (s_pPrototype == NULL) {
		DWORD dwRet = FF10initialise();
		if (dwRet == FF_FAIL) return FF_FAIL;
	}

	switch (index) {

	case FF_CAP_16BITVIDEO:
		return FF_FALSE;

	case FF_CAP_24BITVIDEO:
		return FF_FALSE;

	case FF_CAP_32BITVIDEO:
		return FF_FALSE;

	case FF_CAP_PROCESSFRAMECOPY:
		return FF_FALSE;

	case FF_CAP_MINIMUMINPUTFRAMES:
		MinInputs = s_pPrototype->GetMinInputs();
		if (MinInputs < 0) return FF_FALSE;
		return DWORD(MinInputs);

	case FF_CAP_MAXIMUMINPUTFRAMES:
		MaxInputs = s_pPrototype->GetMaxInputs();
		if (MaxInputs < 0) return FF_FALSE;
		return DWORD(MaxInputs);

	case FF_CAP_COPYORINPLACE:
		return FF_FALSE;

	default:
		return FF_FALSE;
	}
	
	return FF_FAIL;
}

void *getExtendedInfo()
{
	return (void *)(g_CurrPluginInfo->GetPluginExtendedInfo());
}

DWORD getParameterType(DWORD index)
{
	if (s_pPrototype == NULL) {
		DWORD dwRet = FF10initialise();
		if (dwRet == FF_FAIL) return FF_FAIL;
	}
	
	return s_pPrototype->GetParamType(index);
}

DWORD instantiate(const VideoInfoStruct *pVideoInfo)
{
	if (g_CurrPluginInfo != NULL) {
		if (s_pPrototype == NULL) {
			DWORD dwRet = FF10initialise();
			if ((dwRet == FF_FAIL) || s_pPrototype == NULL) {
				return FF_FAIL;
			}
		}
		// Creating plugin instance
		CFreeFrame10Plugin* pInstance = NULL;
		DWORD dwRet = (*(g_CurrPluginInfo->GetFactoryMethod()))(&pInstance);
		if ((dwRet == FF_FAIL) || (pInstance == NULL)) return FF_FAIL;
		pInstance->m_pPlugin = pInstance;

		// Initializing instance with default values
		for (int i = 0; i < s_pPrototype->GetNumParams(); ++i) {
			DWORD dwType = s_pPrototype->GetParamType(DWORD(i));
			void* pValue = s_pPrototype->GetParamDefault(DWORD(i));
			SetParameterStruct ParamStruct;
			ParamStruct.ParameterNumber = DWORD(i);
			memcpy(&ParamStruct.u.NewParameterValue, pValue, 4);
			dwRet = pInstance->SetParameter(&ParamStruct);
			if (dwRet == FF_FAIL) return FF_FAIL;
		}

		// Saving data in the VideoInfoStruct in an internal data structure
		pInstance->SetVideoInfo(pVideoInfo);

		return DWORD(pInstance);
	}
	return FF_FAIL;
#if 0

	// If the plugin is not initialized, initialize it
	if (s_pPrototype == NULL) {
		DWORD dwRet = FF10initialise();
		if ((dwRet == FF_FAIL) || (s_pPrototype == NULL))
			return FF_FAIL;
	}
		
	if ( s_vizserver == NULL ) {
		s_vizserver = VizServer::GetServer();
		s_vizserver->Start();
	}

	//get the instantiate function pointer
	FPCREATEINSTANCEGL *pInstantiate = g_CurrPluginInfo->GetFactoryMethod();

	CFreeFrameGLPlugin *pInstance = NULL;

	//call the instantiate function
	DWORD dwRet = pInstantiate(&pInstance);

	//make sure the instantiate call worked
	if ((dwRet == FF_FAIL) || (pInstance == NULL))
		return FF_FAIL;

	pInstance->m_pPlugin = pInstance;

	// Initializing instance with default values
	int nparams = s_pPrototype->GetNumParams();
	for (int i = 0; i < nparams; ++i) {
		//DWORD dwType = s_pPrototype->GetParamType(DWORD(i));
		void* pValue = s_pPrototype->GetParamDefault(DWORD(i));
		SetParameterStruct ParamStruct;
		ParamStruct.ParameterNumber = DWORD(i);
		memcpy(&ParamStruct.u.NewParameterValue, pValue, 4);
		dwRet = pInstance->SetParameter(&ParamStruct);
		if (dwRet == FF_FAIL) {
			//SetParameter failed, delete the instance
			delete pInstance;
			return FF_FAIL;
		}
	}

	//call the InitGL method
	if (pInstance->InitGL(pGLViewport)==FF_SUCCESS) {
		//succes? we're done.
		return (DWORD)pInstance;
	}

	//InitGL failed, delete the instance
	pInstance->DeInitGL();
	delete pInstance;

	return FF_FAIL;

#endif
}

DWORD deInstantiate(void *instanceID)
{
	DEBUGPRINT(("deInstantiateGL in FF10.cpp called"));
	return FF_FAIL;
#if 0
	CFreeFrame10Plugin *p = (CFreeFrame10Plugin *)instanceID;

	if (p != NULL) {
		p->DeInitGL();
		delete p;

		return FF_SUCCESS;
	}

	return FF_FAIL;
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of plugMain, the one and only exposed function
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32

plugMainUnion __stdcall plugMain(DWORD functionCode, DWORD inputValue, DWORD instanceID) 

#elif TARGET_OS_MAC

plugMainUnion plugMain(DWORD functionCode, DWORD inputValue, DWORD instanceID) 

#elif __linux__

plugMainUnion plugMain(DWORD functionCode, DWORD inputValue, DWORD instanceID)

#endif	

{
	plugMainUnion retval;

	// declare pPlugObj - pointer to this instance
	CFreeFrame10Plugin* pPlugObj;

	// typecast DWORD into pointer to a CFreeFrame10Plugin
	pPlugObj = (CFreeFrame10Plugin*) instanceID;

 switch (functionCode) {

	case FF_GETINFO:
		retval.PISvalue = (PluginInfoStruct*)FF10getInfo();
		break;

	case FF_INITIALISE:
		retval.ivalue = FF10initialise();
		break;

	case FF_DEINITIALISE:
		retval.ivalue = FF10deInitialise();	
		break;

	case FF_GETNUMPARAMETERS:
		retval.ivalue = getNumParameters();
		break;

	case FF_GETPARAMETERNAME:
		retval.svalue = getParameterName(inputValue);
		break;
	
	case FF_GETPARAMETERDEFAULT:
		retval.ivalue = getParameterDefault(inputValue);
		break;

	case FF_GETPLUGINCAPS:
		retval.ivalue = getPluginCaps(inputValue);
		break;

	case FF_GETEXTENDEDINFO: 
		retval.ivalue = (DWORD) getExtendedInfo();
		break;

	case FF_GETPARAMETERTYPE:		
		retval.ivalue = getParameterType(inputValue);
		break;

	case FF_GETPARAMETERDISPLAY:
		if (pPlugObj != NULL) 
			retval.svalue = pPlugObj->GetParameterDisplay(inputValue);
		else
			retval.svalue = (char*)FF_FAIL;
		break;
		
	case FF_SETPARAMETER:
		if (pPlugObj != NULL)
			retval.ivalue = pPlugObj->SetParameter((const SetParameterStruct*) inputValue);
		else
			retval.ivalue = FF_FAIL;
		break;
	
	case FF_GETPARAMETER:
		if (pPlugObj != NULL) 
			retval.ivalue = pPlugObj->GetParameter(inputValue);
		else 
			retval.ivalue = FF_FAIL;
		break;
		
	case FF_INSTANTIATE:
		retval.ivalue = (DWORD)instantiate((const VideoInfoStruct *)inputValue);
		break;

	case FF_DEINSTANTIATE:
		if (pPlugObj != NULL)
			retval.ivalue = deInstantiate(pPlugObj);
		else
			retval.ivalue = FF_FAIL;
		break;
	
	case FF_GETIPUTSTATUS:
		if (pPlugObj != NULL)
			retval.ivalue = pPlugObj->GetInputStatus(inputValue);
		else
			retval.ivalue = FF_FAIL;
		break;

	case FF_PROCESSFRAME:
		if (pPlugObj != NULL)
			retval.ivalue = pPlugObj->ProcessFrame((void*)inputValue);
		else
			retval.ivalue = FF_FAIL;
		break;

	case FF_PROCESSFRAMECOPY:
		retval.ivalue = FF_FAIL;
		break;

	// Resolume sends this code when a clip goes unselected (not active)
	// I use it to disable the reception/handling of OSC.
	case 22:
		DEBUGPRINT1(("functionCode 22 (from Resolume?)"));
		if (pPlugObj != NULL) {
			retval.ivalue = pPlugObj->ResolumeDeactivate();
		} else {
			retval.ivalue = FF_FAIL;
		}
		break;

	// Resolume sends this code when a clip is selected (I think)
	case 33:
		DEBUGPRINT1(("functionCode 33 (from Resolume?)"));
		retval.ivalue = FF_SUCCESS;
		break;

	default:
		DEBUGPRINT(("Unrecognized functionCode in plugMain: %d",functionCode));
		retval.ivalue = FF_FAIL;
		break;
	}
	
	return retval;
}
