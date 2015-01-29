// VizDll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
// #include <string>
#include "NosuchUtil.h"
#include "mmtt_sharedmem.h"
#include "VizServer.h"
#include "ffutil.h"

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	char path[MAX_PATH];
	GetModuleFileNameA((HMODULE)hModule, path, MAX_PATH);
	std::string dllpath = std::string(path);
	// We want to take off the final filename AND the directory.
	// This assumes that the DLL is in either a bin or ffglplugins
	// subdirectory of the main Vizpath
	size_t pos = dllpath.find_last_of("/\\");
	if ( pos != dllpath.npos && pos > 0 ) {
		std::string parent = dllpath.substr(0,pos);
		pos = dllpath.substr(0,pos-1).find_last_of("/\\");
		if ( pos != parent.npos && pos > 0) {
			SetVizPath(parent.substr(0,pos));
		}
	}

	switch (ul_reason_for_call) {

	case DLL_PROCESS_ATTACH:
		{
			if ( ! default_setdll(std::string(dllpath)) ) {
				DEBUGPRINT(("default_setdll failed"));
				return FALSE;
			}
			dllpath = NosuchToLower(dllpath);
			size_t lastslash = dllpath.find_last_of("/\\");
			size_t lastdot = dllpath.find_last_of(".");
			std::string dir = dllpath.substr(0,lastslash);
			NosuchCurrentDir = dir;
		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
