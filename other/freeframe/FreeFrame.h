////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FreeFrame.h
//
// FreeFrame is an open-source cross-platform real-time video effects plugin system.
// It provides a framework for developing video effects plugins and hosts on Windows, 
// Linux and Mac OSX. 
// 
// Copyright (c) 2002, 2003, 2004, 2005, 2006 www.freeframe.org
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
// FreeFrame 1.0 upgrade by Pete Warden
// www.petewarden.com
//
// FreeFrame 1.0 - 03 upgrade by Gualtiero Volpe
// Gualtiero.Volpe@poste.it
//
// #ifdef tweaks for FreeFrameGL upgrade by Trey Harrison
// www.harrisondigitalmedia.com
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __FREEFRAME_H__
#define __FREEFRAME_H__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

extern "C" {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FreeFrame defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Function codes
#define FF_GETINFO					0
#define FF_INITIALISE				1
#define FF_DEINITIALISE				2
#define FF_PROCESSFRAME				3
#define FF_GETNUMPARAMETERS			4
#define FF_GETPARAMETERNAME			5
#define FF_GETPARAMETERDEFAULT		6
#define FF_GETPARAMETERDISPLAY		7
#define FF_SETPARAMETER				8
#define FF_GETPARAMETER				9
#define FF_GETPLUGINCAPS			10
#define FF_INSTANTIATE				11
#define FF_DEINSTANTIATE			12
#define FF_GETEXTENDEDINFO			13
#define FF_PROCESSFRAMECOPY			14
#define FF_GETPARAMETERTYPE			15
#define FF_GETIPUTSTATUS			16

// These are non-standard extensions, used by Resolume (and now FFFF/Vizlets)
#define FF_CONNECT					21
#define FF_DISCONNECT				22

// Return values
#define FF_SUCCESS					0
#define FF_FAIL						0xFFFFFFFF
#define FF_TRUE						1
#define FF_FALSE					0
#define	FF_SUPPORTED				1 
#define FF_UNSUPPORTED				0 

// Plugin types
#define FF_EFFECT					0
#define FF_SOURCE					1

// Plugin capabilities
#define FF_CAP_16BITVIDEO			0
#define FF_CAP_24BITVIDEO			1
#define FF_CAP_32BITVIDEO			2
#define FF_CAP_PROCESSFRAMECOPY		3
#define FF_CAP_MINIMUMINPUTFRAMES	10
#define FF_CAP_MAXIMUMINPUTFRAMES	11
#define FF_CAP_COPYORINPLACE		15

// Plugin optimization
#define FF_CAP_PREFER_NONE			0
#define FF_CAP_PREFER_INPLACE		1
#define FF_CAP_PREFER_COPY			2
#define	FF_CAP_PREFER_BOTH			3

// Parameter types
#define FF_TYPE_BOOLEAN				0    
#define FF_TYPE_EVENT				1
#define FF_TYPE_RED					2 
#define FF_TYPE_GREEN				3
#define FF_TYPE_BLUE				4
#define FF_TYPE_XPOS				5
#define FF_TYPE_YPOS				6
#define FF_TYPE_STANDARD			10
#define FF_TYPE_TEXT				100

// Input status
#define FF_INPUT_NOTINUSE			0
#define	FF_INPUT_INUSE				1

// Image depth
#define FF_DEPTH_16					0
#define FF_DEPTH_24					1
#define FF_DEPTH_32					2	

// Image orientation
#define FF_ORIENTATION_TL			1
#define FF_ORIENTATION_BL			2


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FreeFrame Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Typedefs for Linux and MacOS - in Windows these are defined in files included by windows.h
#ifndef _WIN32
typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef void *LPVOID;
#endif

// PluginInfoStruct
typedef struct PluginInfoStruct {
	DWORD	APIMajorVersion;
	DWORD	APIMinorVersion;
	BYTE	PluginUniqueID[4];		// 4 chars uniqueID - not null terminated
	BYTE	PluginName[16];			// 16 chars plugin friendly name - not null terminated
	DWORD	PluginType;				// Effect or source
} PluginInfoStruct;

// PluginExtendedInfoStruct   
typedef struct PluginExtendedInfoStructTag {
	DWORD PluginMajorVersion;
	DWORD PluginMinorVersion;
	char* Description;
	char* About;
	DWORD FreeFrameExtendedDataSize;
	void* FreeFrameExtendedDataBlock;
} PluginExtendedInfoStruct;

// VideoInfoStruct
typedef struct VideoInfoStructTag {
	DWORD FrameWidth;				// width of frame in pixels
	DWORD FrameHeight;				// height of frame in pixels
	DWORD BitDepth;					// enumerated indicator of bit depth of video: 0 = 16 bit 5-6-5   1 = 24bit packed   2 = 32bit
	DWORD Orientation;			
} VideoInfoStruct;

// ProcessFrameCopyStruct
typedef struct ProcessFrameCopyStructTag {
	DWORD numInputFrames;
	void** ppInputFrames;
	void* pOutputFrame;
} ProcessFrameCopyStruct;

// SetParameterStruct
typedef struct SetParameterStruct {
	DWORD ParameterNumber;
	union {
		float NewFloatValue;
		DWORD NewParameterValue;
		const char* NewTextValue;
	} u;
	// DWORD NewParameterValue;
} SetParameterStruct;

// plugMain function return values
typedef union plugMainUnionTag {
	DWORD ivalue;
	float fvalue;
	VideoInfoStruct* VISvalue;
	PluginInfoStruct* PISvalue;
	PluginExtendedInfoStruct* PXISvalue;
	char* svalue;
} plugMainUnion;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// plugMain - The one and only exposed function
// parameters: 
//	functionCode - tells the plugin which function is being called
//  pParam - 32-bit parameter or 32-bit pointer to parameter structure
//
// PLUGIN DEVELOPERS:  you shouldn't need to change this function
//
// All parameters are cast as 32-bit untyped pointers and cast to appropriate
// types here
// 
// All return values are cast to 32-bit untyped pointers here before return to 
// the host
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

__declspec(dllexport) plugMainUnion __stdcall plugMain(DWORD functionCode, DWORD inputValue, DWORD instanceID);
typedef __declspec(dllimport) plugMainUnion (__stdcall *FF_Main_FuncPtr)(DWORD, DWORD, DWORD);

}

#endif
