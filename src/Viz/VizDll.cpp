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
