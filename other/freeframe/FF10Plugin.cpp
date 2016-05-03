// Copyleft 2006 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda

		revision history:
		rev		date	comments
		00		24jul06	initial version
		01		06nov06	call initialise and deInitialise
		02		21jan07	GetPluginPath must call ReleaseBuffer
		03		23nov07	support Unicode

		wrapper for freeframe plugin DLL

*/

#include "windows.h"
#include <string>
#include "stdio.h"
#include "VizUtil.h"
#include "FF10Plugin.h"
#include "NosuchUtil.h"
#include "NosuchException.h"
#include "NosuchJson.h"

// #define VERBOSE 1

FF10PluginDef::FF10PluginDef()
	:m_mainfunc(NULL),
	m_paramdefs(NULL),
	name("NULL"),
	m_numparams(0),
    m_hInst(NULL)
{
}

FF10PluginDef::~FF10PluginDef()
{
    Free();
}

bool FF10PluginDef::Load(std::string Path)
{
    DWORD dwError;

    Free();

	std::wstring wPath = s2ws(Path);
    // m_hInst = LoadLibrary(wPath.c_str());
    m_hInst = LoadLibrary(wPath.c_str());
	DEBUGPRINT1(("FF10PluginDef Load path=%s",Path.c_str()));
    if ( m_hInst == NULL ) {
        dwError = GetLastError();
        DEBUGPRINT(("LoadLibrary of %s failed, dwError = %d\n",Path.c_str(),dwError));
        return FALSE;
    }
    FF_Main_FuncPtr main = (FF_Main_FuncPtr)GetProcAddress(m_hInst, "plugMain");
    if (main == NULL) {
        dwError = GetLastError();
        DEBUGPRINT(("Unable to get plugMain address!? path=%s dwError=%d\n",Path.c_str(),dwError));
        return(FALSE);
    }
	m_mainfunc = main;

    plugMainUnion u = m_mainfunc(FF_INITIALISE, 0, 0);
    if ( u.ivalue == FF_SUCCESS ) {
        if ( ! LoadParamDefs() ) {
            DEBUGPRINT(("Unable to load parameter definitions!?  path=%s\n",Path.c_str()));
            return FALSE;
        }
        DEBUGPRINT1(("Successfully loaded Path=%s\n",Path.c_str()));
        return TRUE;
    }
    DEBUGPRINT(("Unable to initialise!? path=%s\n",Path.c_str()));
    return FALSE;
}

FF10ParameterDef*
FF10PluginDef::findparamdef(std::string pnm)
{
    for ( int n=0; n<m_numparams; n++ ) {
		std::string nm = m_paramdefs[n].name;
		if ( nm == pnm ) {
			return &(m_paramdefs[n]);
		}
	}
	return NULL;
}

int
FF10PluginDef::getParamNum(std::string pnm) {
	FF10ParameterDef* p = findparamdef(pnm);
	if ( p ) {
		return p->num;
	} else {
		return -1;
	}
}

bool FF10PluginDef::LoadParamDefs()
{
    plugMainUnion u = m_mainfunc(FF_GETNUMPARAMETERS, 0, 0);
    int np = u.ivalue;
    int n;
    m_numparams = np;
    m_paramdefs = new FF10ParameterDef[np];
	// DEBUGPRINT(("----- MALLOC new FF10ParameterDef[np]"));
    for ( n=0; n<np; n++ ) {
        FF10ParameterDef* p = &(m_paramdefs[n]);
		p->num = n;
        u = m_mainfunc(FF_GETPARAMETERNAME, n, 0);
		char rawname[17];
        memcpy(rawname,u.svalue,16);
        rawname[16] = 0;
		if ( strncmp("Rotate",rawname,6) == 0 ) {
			DEBUGPRINT(("Rotate!"));
		}
        p->name = trim(std::string(rawname));  // trim gets rid of whitespace

        u = m_mainfunc(FF_GETPARAMETERTYPE, n, 0);
        p->type = u.ivalue;
	    u = m_mainfunc(FF_GETPARAMETERDEFAULT, n, 0);
        if ( p->type != FF_TYPE_TEXT ) {
            p->default_float_val = u.fvalue;
            DEBUGPRINT1(("Float Parameter n=%d s=%s type=%d default=%lf\n",
                     n,p->name.c_str(),p->type,p->default_float_val));
        } else {
            p->default_string_val = CopyFFString16(u.svalue);
            DEBUGPRINT1(("String Parameter n=%d s=%s default=%s\n",n,p->name,p->default_string_val.c_str()));
        }
    }

    return TRUE;
}

DWORD FF10PluginInstance::Instantiate(VideoInfoStruct *vis)
{
	DEBUGPRINT1(("Pre Instantiate"));
	if (m_instanceid != INVALIDINSTANCE) {
		DEBUGPRINT(("HEY!  Instantiate called when already instantiated!?"));
        //already instantiated
        return FF_SUCCESS;
	}

    m_instanceid = m_mainfunc(FF_INSTANTIATE, (DWORD)vis, 0).ivalue;
    if ( m_instanceid == FF_FAIL ) {
        DEBUGPRINT(("Unable to Instantiate!? plugin=%s\n",m_viztag.c_str()));
        return FF_FAIL;
    }
    DEBUGPRINT1(("SUCCESSFUL Instantiate id=%d\n",m_instanceid));
	DEBUGPRINT(("HEY!!!! should I be setting default param assignments here, like in GL?"));
    return FF_SUCCESS;
}

DWORD FF10PluginInstance::DeInstantiate()
{
    if (m_instanceid==INVALIDINSTANCE) {
		DEBUGPRINT(("Hey!  DeInstantiate called when already deleted!?"));
        return FF_SUCCESS;
    }
    DEBUGPRINT1(("DeInstantiate id=%d\n",m_instanceid));
    plugMainUnion u = m_mainfunc(FF_DEINSTANTIATE, 0, (DWORD)m_instanceid);
    if ( u.ivalue == FF_FAIL ) {
        DEBUGPRINT(("Unable to Instantiate!?\n"));
        return false;
    }
	return true;
}


bool FF10PluginDef::Free()
{
    if (m_mainfunc != NULL) {
        plugMainUnion u = m_mainfunc(FF_DEINITIALISE, 0, 0);
        m_mainfunc = NULL;
    }
    if (m_hInst != NULL) {
        FreeLibrary(m_hInst);
        m_hInst = NULL;
        return(TRUE);
    }
    return(FALSE);
}

const PluginInfoStruct *FF10PluginDef::GetInfo() const
{
    plugMainUnion u = m_mainfunc(FF_GETINFO, 0, 0);
    return(u.PISvalue);
}

const PluginExtendedInfoStruct *FF10PluginDef::GetExtendedInfo() const
{
    plugMainUnion u = m_mainfunc(FF_GETEXTENDEDINFO, 0, 0);
    return(u.PXISvalue);
}

bool FF10PluginDef::Process(int instanceid, unsigned char *pixels)
{
    plugMainUnion u = m_mainfunc(FF_PROCESSFRAME, (DWORD)(pixels), instanceid);
    if ( u.ivalue == FF_FAIL )
        return FALSE;
    else
        return TRUE;
}


bool FF10PluginDef::GetInfo(PluginInfoStruct& PlugInfo) const
{
    const PluginInfoStruct *pis = GetInfo();
    if (pis == NULL)
        return(FALSE);
    PlugInfo = *GetInfo();
    return(TRUE);
}


std::string FF10PluginDef::GetPluginName() const
{
    const PluginInfoStruct *pis = GetInfo();
    if (pis == NULL)
        return("");
    std::string name = CopyFFString16((char*)(pis->PluginName));
    return(name);
}

FF10PluginInstance::FF10PluginInstance(FF10PluginDef* d, std::string viztag) :
	m_plugindef(d), m_params(NULL), m_viztag(viztag), m_enabled(false),
	m_moveable(true), m_instanceid(INVALIDINSTANCE) {

	NosuchAssert ( d->m_mainfunc );
	m_mainfunc = d->m_mainfunc;
}

bool FF10PluginInstance::setparam(std::string pnm, float v)
{
	int pnum = m_plugindef->getParamNum(pnm);
	if ( pnum >= 0 ) {
		SetFloatParameter(pnum, v);
	    return true;
	} else {
	    DEBUGPRINT(("FF10PluginInstance::setparam float didn't find FF10 parameter pnm=%s in plugin=%s\n",pnm.c_str(),m_plugindef->GetPluginName().c_str()));
	    return false;
	}
}

bool FF10PluginInstance::setparam(std::string pnm, std::string v)
{
	int pnum = m_plugindef->getParamNum(pnm);
	if ( pnum >= 0 ) {
		SetStringParameter(pnum, v);
	    return true;
	} else {
	    DEBUGPRINT(("FF10PluginInstance::setparam string didn't find FF10 parameter pnm=%s in plugin=%s\n",pnm.c_str(),m_plugindef->GetPluginName().c_str()));
	    return false;
	}
}

#if 0
    for ( n=0; n<m_numparams; n++ ) {
        FF10ParameterDef* p = &m_paramdefs[n];
        // printf("Comparing paramname=%s to p->name=%s\n",paramname,p->name);
        if ( pnm == p->name ) {
            // printf("Found parameter = %s\n",paramname);
            // if ( p->type != FF_TYPE_STANDARD ) {
            // 	printf("Unable to set non-FF_TYPE_STANDARD parameter = %s  type=%d\n",paramname,p->type);
            // 	return false;
            // }
            SetParameterStruct ps;
            ps.ParameterNumber = n;
            // ps.NewParameterValue = (DWORD) v;
            ps.u.NewFloatValue = v;
            // printf("paramstruct ps=%d fvalue=%f v=%f\n",
            //	ps.ParameterNumber,ps.NewParameterValue.fvalue,v);
            plugMainUnion u;
            u = m_mainfunc(FF_SETPARAMETER, (DWORD)(&ps), (DWORD)m_instanceid);
            // printf("SETPARAMETER results = %d\n",u.ivalue);
            if ( u.ivalue == FF_FAIL ) {
                DEBUGPRINT(("ERROR!!! SETPARAMETER of %s failed!?\n",pnm.c_str()));
                return false;
            }

            // u = m_mainfunc(FF_GETPARAMETER, (DWORD)n, (DWORD)instanceid);
            // printf("GETPARAMETER n=%d v=%f\n",n,u.fvalue);
            return true;
        }
    }
    DEBUGPRINT(("Didn't find parameter pnm=%s\n",pnm.c_str()));
#endif

float FF10PluginInstance::getparam(std::string pnm)
{
	DEBUGPRINT(("F10Plugin::getparam pnm=%s", pnm.c_str()));
	int pnum = m_plugindef->getParamNum(pnm);
	if (pnum >= 0) {
		return GetFloatParameter(pnum);
	}
	DEBUGPRINT(("FF10PluginInstance::getparam didn't find FF10 parameter pnm=%s\n", pnm.c_str()));
	return 0.0;
}

std::string FF10PluginInstance::getParamJsonResult(FF10ParameterDef* pd, FF10PluginInstance* pi, const char* id)
{
	std::string s = pi->GetParameterDisplay(pd->num);
	float v;
	switch (pd->type){
	case FF_TYPE_TEXT:
		return jsonStringResult(s, id);
		break;
	case FF_TYPE_BOOLEAN:
		v = pi->GetBoolParameter(pd->num);
		return jsonDoubleResult(v, id);
	case FF_TYPE_STANDARD:
	default:
		v = pi->GetFloatParameter(pd->num);
		return jsonDoubleResult(v, id);
	}
	throw NosuchException("UNIMPLEMENTED parameter type (%d) in get API!", pd->type);
}

void FF10PluginInstance::SetFloatParameter(int paramNum, float value)
{
	//make sure its a float parameter type
	DWORD ffParameterType = m_mainfunc(FF_GETPARAMETERTYPE, (DWORD)paramNum, 0).ivalue;
	if (ffParameterType != FF_TYPE_TEXT) {
		SetParameterStruct ArgStruct;
		ArgStruct.ParameterNumber = paramNum;

		//be careful with this cast.. ArgStruct.NewParameterValue is DWORD
		//for this to compile correctly, sizeof(DWORD) must == sizeof(float)

		//   *((float *)(unsigned)&ArgStruct.NewParameterValue) = value;
		ArgStruct.u.NewFloatValue = value;
		// ArgStruct.NewParameterValue = (DWORD)value;

		m_mainfunc(FF_SETPARAMETER, (DWORD)(&ArgStruct), m_instanceid);
	}
	else {
		DEBUGPRINT(("HEY! SetFloatParameter called on TEXT parameter (paramnum=%d)", paramNum));
	}
}

void FF10PluginInstance::SetStringParameter(int paramNum, std::string value)
{
	//make sure its a text parameter type
	DWORD ffParameterType = m_mainfunc(FF_GETPARAMETERTYPE, (DWORD)paramNum, 0).ivalue;
	if (ffParameterType == FF_TYPE_TEXT) {
		SetParameterStruct ArgStruct;
		ArgStruct.ParameterNumber = paramNum;
		ArgStruct.u.NewTextValue = value.c_str();
		m_mainfunc(FF_SETPARAMETER, (DWORD)(&ArgStruct), m_instanceid);
	}
	else {
		throw NosuchException("HEY! SetStringParameter called on non-TEXT parameter (paramnum=%d)", paramNum);
	}
}

float FF10PluginInstance::GetFloatParameter(int paramNum) {
	//make sure its a float parameter type
	DWORD ffParameterType = m_mainfunc(FF_GETPARAMETERTYPE, (DWORD)paramNum, 0).ivalue;
	if (ffParameterType != FF_TYPE_TEXT)
	{
		plugMainUnion result = m_mainfunc(FF_GETPARAMETER, (DWORD)paramNum, m_instanceid);

		//make sure the call to get the parameter succeeded before
		//reading the float value
		if (result.ivalue != FF_FAIL)
		{
			return result.fvalue;
		}
	}
	return 0.f;
}

bool FF10PluginInstance::GetBoolParameter(int paramNum) {
	//make sure its a float parameter type
	DWORD ffParameterType = m_mainfunc(FF_GETPARAMETERTYPE, (DWORD)paramNum, 0).ivalue;
	if (ffParameterType == FF_TYPE_BOOLEAN)
	{
		plugMainUnion r = m_mainfunc(FF_GETPARAMETER, (DWORD)paramNum, m_instanceid);

		//make sure the call to get the parameter succeeded before
		//reading the float value
		DEBUGPRINT(("BOOL paramNum=%d ivalue=%d fvalue=%f", paramNum, r.ivalue, r.fvalue));
		if (r.fvalue == 0.0) {
			return false;
		}
		else {
			return true;
		}
	}
	DEBUGPRINT(("GetBoolParameter called on non-BOOL parameter?"));
	return 0.0f;
}

std::string FF10PluginInstance::GetParameterDisplay(int paramNum)
{
	plugMainUnion r = m_mainfunc(FF_GETPARAMETERDISPLAY, (DWORD)paramNum, m_instanceid);
	char nm[17];
	memcpy(nm, r.svalue, 16);
	nm[16] = 0;
	std::string display = std::string(nm);
	return display;
}

void loadff10plugindef(std::string ffdir, std::string dllnm)
{
	FF10PluginDef *plugin = new FF10PluginDef();
	// DEBUGPRINT(("----- MALLOC new FF10PluginDef"));
	std::string dll_fname = ffdir + "/" + dllnm;

	if (!plugin->Load(dll_fname)) {
		DEBUGPRINT(("Unable to load %s\n", dll_fname.c_str()));
	}
	else {
		plugin->m_dll = dllnm;
		plugin->name = plugin->GetPluginName();
		DEBUGPRINT1(("Loaded FF10 plugin file=%s name=%s", dll_fname.c_str(), plugin->name.c_str()));
		ff10plugindefs[nff10plugindefs] = plugin;
		nff10plugindefs++;
	}
}

void loadffdir(std::string ffdir)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;

	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	int nfound = 0;

	std::string pathexpr = ffdir + "\\*";
	std::wstring wpath = s2ws(pathexpr);
	hFind = FindFirstFile(wpath.c_str(), &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		return;
	}
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;

			std::wstring wcfname = ffd.cFileName;
			std::string cfname = NosuchToLower(ws2s(wcfname));
			// std::string cfname = NosuchToLower(ffd.cFileName);

			if (NosuchEndsWith(cfname, ".dll")) {
				loadff10plugindef(ffdir, cfname.c_str());
			}
			else {
				DEBUGPRINT1(("Ignoring %s, not .dll", cfname.c_str()));
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		DEBUGPRINT(("loadffdir, dwError=%ld", dwError));
	}

	FindClose(hFind);
}

void loadff10path(std::string path) {
	std::vector<std::string> dirs = NosuchSplitOnString(path, ";");
	for (size_t i = 0; i<dirs.size(); i++) {
		loadffdir(VizPath(dirs[i]));
	}
}

FF10ParameterDef *
findff10param(FF10PluginDef* ff, std::string nm) {
    for ( int i=0; i<ff->m_numparams; i++ ) {
        FF10ParameterDef* pp = &(ff->m_paramdefs[i]);
        if ( pp->name == nm ) {
            return pp;
        }
    }
    return NULL;
}
