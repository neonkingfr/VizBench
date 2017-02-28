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
#include "ffutil.h"
#include "FFPlugin.h"
#include "VizUtil.h"

// #define VERBOSE 1

FFPluginDef::FFPluginDef()
{
    m_hInst = NULL;
    m_pff = NULL;
	m_instanceid = 0;
	m_numparams = 0;
	m_paramdefs = NULL;
}

FFPluginDef::~FFPluginDef()
{
    Free();
}

bool FFPluginDef::Load(std::string Path)
{
    DWORD dwError;

    Free();

	// std::wstring wPath = s2ws(Path);
    // m_hInst = LoadLibrary(wPath.c_str());
    m_hInst = LoadLibrary(Path.c_str());
    if ( m_hInst == NULL ) {
        dwError = GetLastError();
        DEBUGPRINT(("LoadLibrary of %s failed, dwError = %d\n",Path.c_str(),dwError));
        return FALSE;
    }
    m_pff = (FF_Main_FuncPtr)GetProcAddress(m_hInst, "plugMain");
    if (m_pff == NULL) {
        dwError = GetLastError();
        DEBUGPRINT(("Unable to get plugMain address!? path=%s dwError=%d\n",Path.c_str(),dwError));
        return(FALSE);
    }
    plugMainUnion u = m_pff(FF_INITIALISE, 0, 0);
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

bool FFPluginDef::setparam(std::string pnm, float v)
{
    int n;
    // const char *paramname = pnm.c_str();

    for ( n=0; n<m_numparams; n++ ) {
        FFParameterDef* p = &m_paramdefs[n];
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
            u = m_pff(FF_SETPARAMETER, (DWORD)(&ps), (DWORD)m_instanceid);
            // printf("SETPARAMETER results = %d\n",u.ivalue);
            if ( u.ivalue == FF_FAIL ) {
                DEBUGPRINT(("ERROR!!! SETPARAMETER of %s failed!?\n",pnm.c_str()));
                return false;
            }

            // u = m_pff(FF_GETPARAMETER, (DWORD)n, (DWORD)instanceid);
            // printf("GETPARAMETER n=%d v=%f\n",n,u.fvalue);
            return true;
        }
    }
    DEBUGPRINT(("Didn't find parameter pnm=%s\n",pnm.c_str()));
    return false;
}

bool FFPluginDef::LoadParamDefs()
{
    plugMainUnion u = m_pff(FF_GETNUMPARAMETERS, 0, 0);
    int np = u.ivalue;
    int n;
    m_numparams = np;
    m_paramdefs = new FFParameterDef[np];
    for ( n=0; n<np; n++ ) {
        FFParameterDef* p = &(m_paramdefs[n]);
        u = m_pff(FF_GETPARAMETERNAME, n, 0);
		char rawname[17];
        memcpy(rawname,u.svalue,16);
        rawname[16] = 0;
		if ( strncmp("Rotate",rawname,6) == 0 ) {
			DEBUGPRINT(("Rotate!"));
		}
        p->name = trim(std::string(rawname));  // trim gets rid of whitespace

        u = m_pff(FF_GETPARAMETERTYPE, n, 0);
        p->type = u.ivalue;
	    u = m_pff(FF_GETPARAMETERDEFAULT, n, 0);
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

int FFPluginDef::Instantiate(VideoInfoStruct *vis)
{
    plugMainUnion u = m_pff(FF_INSTANTIATE, (DWORD)vis, 0);
    if ( u.ivalue == FF_FAIL ) {
        printf("Unable to Instantiate!?\n");
        return FALSE;
    }
    // printf("SUCCESSFUL Instantiate id=%d\n",u.ivalue);
    return u.ivalue;
}


bool FFPluginDef::Free()
{
    if (m_pff != NULL) {
        m_pff(FF_DEINITIALISE, 0, 0);
        m_pff = NULL;
    }
    if (m_hInst != NULL) {
        FreeLibrary(m_hInst);
        m_hInst = NULL;
        return(TRUE);
    }
    return(FALSE);
}

const PluginInfoStruct *FFPluginDef::GetInfo() const
{
    plugMainUnion u = m_pff(FF_GETINFO, 0, 0);
    return(u.PISvalue);
}

bool FFPluginDef::Process(int instanceid, unsigned char *pixels)
{
    plugMainUnion u = m_pff(FF_PROCESSFRAME, (DWORD)(pixels), instanceid);
    if ( u.ivalue == FF_FAIL )
        return FALSE;
    else
        return TRUE;
}


bool FFPluginDef::GetInfo(PluginInfoStruct& PlugInfo) const
{
    const PluginInfoStruct *pis = GetInfo();
    if (pis == NULL)
        return(FALSE);
    PlugInfo = *GetInfo();
    return(TRUE);
}


std::string FFPluginDef::GetPluginName() const
{
    const PluginInfoStruct *pis = GetInfo();
    if (pis == NULL)
        return("");
    std::string name = CopyFFString16((char*)(pis->PluginName));
    return(name);
}
