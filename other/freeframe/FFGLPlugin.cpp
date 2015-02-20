#include "NosuchUtil.h"
#include "NosuchException.h"

#include "FFGLPlugin.h"

#include <stdio.h>

FFGLPluginDef:: FFGLPluginDef()
    :m_mainfunc(NULL),
	 m_paramdefs(NULL),
	 name("NULL"),
     m_numparams(0)
{
}

FFGLPluginDef::~FFGLPluginDef()
{
    if (m_mainfunc!=NULL) {
        DEBUGPRINT(("FFGLPluginDef deleted with m_mainfunc != NULL?"));
    }
}


FFGLParameterDef*
FFGLPluginDef::SetParameterName(int num, const char *name)
{
    if (num<0 || num>=m_numparams)
        return NULL;

    FFGLParameterDef* p = &(m_paramdefs[num]);
    p->name = trim(std::string(name));
    return p;
}

std::string FFGLPluginDef::GetParameterName(int num)
{
    if ( num < 0 || num >= m_numparams ) {
		DEBUGPRINT(("Bad value given to GetParameterName!? num=%d",num));
        return "";
	}
    return m_paramdefs[num].name;
}

FFGLParameterDef*
FFGLPluginDef::findparamdef(std::string pnm)
{
    for ( int n=0; n<m_numparams; n++ ) {
		if ( m_paramdefs[n].name == pnm ) {
			return &(m_paramdefs[n]);
		}
	}
	return NULL;
}

int
FFGLPluginDef::getParamNum(std::string pnm) {
	FFGLParameterDef* p = findparamdef(pnm);
	if ( p ) {
		return p->num;
	} else {
		return -1;
	}
}

FFGLPluginInstance::FFGLPluginInstance(FFGLPluginDef* d, std::string nm) :
	m_plugindef(d), next(NULL), m_params(NULL), m_name(nm), m_enabled(false),
	m_instanceid(INVALIDINSTANCE) {

	NosuchAssert ( d->m_mainfunc );
	m_main = d->m_mainfunc;
}

bool FFGLPluginInstance::setparam(std::string pnm, float v)
{
	int pnum = m_plugindef->getParamNum(pnm);
	if ( pnum >= 0 ) {
		SetFloatParameter(pnum, v);
	    return true;
	} else {
	    DEBUGPRINT(("Didn't find FFGL parameter pnm=%s in plugin=%s\n",pnm.c_str(),m_plugindef->GetPluginName().c_str()));
	    return false;
	}
}

bool FFGLPluginInstance::setparam(std::string pnm, std::string v)
{
	int pnum = m_plugindef->getParamNum(pnm);
	if ( pnum >= 0 ) {
		SetStringParameter(pnum, v);
	    return true;
	} else {
	    DEBUGPRINT(("Didn't find FFGL parameter pnm=%s in plugin=%s\n",pnm.c_str(),m_plugindef->GetPluginName().c_str()));
	    return false;
	}
}

float FFGLPluginInstance::getparam(std::string pnm)
{
	DEBUGPRINT(("FGLPlugin::getparam pnm=%s",pnm.c_str()));
	int pnum = m_plugindef->getParamNum(pnm);
	if ( pnum >= 0 ) {
		return GetFloatParameter(pnum);
	}
    DEBUGPRINT(("Didn't find FFGL parameter pnm=%s\n",pnm.c_str()));
	return 0.0;
}

void FFGLPluginInstance::SetFloatParameter(int paramNum, float value)
{
    //make sure its a float parameter type
    DWORD ffParameterType = m_main(FF_GETPARAMETERTYPE,(DWORD)paramNum,0).ivalue;
    if (ffParameterType!=FF_TYPE_TEXT) {
        SetParameterStruct ArgStruct;
        ArgStruct.ParameterNumber = paramNum;

        //be careful with this cast.. ArgStruct.NewParameterValue is DWORD
        //for this to compile correctly, sizeof(DWORD) must == sizeof(float)

        //   *((float *)(unsigned)&ArgStruct.NewParameterValue) = value;
        ArgStruct.u.NewFloatValue = value;
		// ArgStruct.NewParameterValue = (DWORD)value;

        m_main(FF_SETPARAMETER,(DWORD)(&ArgStruct), m_instanceid);
    } else {
		DEBUGPRINT(("HEY! SetFloatParameter called on TEXT parameter (paramnum=%d)",paramNum));
	}
}

void FFGLPluginInstance::SetStringParameter(int paramNum, std::string value)
{
    //make sure its a text parameter type
    DWORD ffParameterType = m_main(FF_GETPARAMETERTYPE,(DWORD)paramNum,0).ivalue;
    if (ffParameterType==FF_TYPE_TEXT) {
        SetParameterStruct ArgStruct;
        ArgStruct.ParameterNumber = paramNum;
        ArgStruct.u.NewTextValue = value.c_str();
        m_main(FF_SETPARAMETER,(DWORD)(&ArgStruct), m_instanceid);
    } else {
		DEBUGPRINT(("HEY! SetStringParameter called on non-TEXT parameter (paramnum=%d)",paramNum));
	}
}

void FFGLPluginInstance::SetTime(double curTime) {
    m_main(FF_SETTIME, (DWORD)(&curTime), m_instanceid);
}

float FFGLPluginInstance::GetFloatParameter(int paramNum) {
    //make sure its a float parameter type
    DWORD ffParameterType = m_main(FF_GETPARAMETERTYPE,(DWORD)paramNum,0).ivalue;
    if (ffParameterType!=FF_TYPE_TEXT)
    {
        plugMainUnion result = m_main(FF_GETPARAMETER,(DWORD)paramNum, m_instanceid);

        //make sure the call to get the parameter succeeded before
        //reading the float value
        if (result.ivalue!=FF_FAIL)
        {
            return result.fvalue;
        }
    }
    return 0.f;
}

bool FFGLPluginInstance::GetBoolParameter(int paramNum) {
    //make sure its a float parameter type
    DWORD ffParameterType = m_main(FF_GETPARAMETERTYPE,(DWORD)paramNum,0).ivalue;
    if (ffParameterType==FF_TYPE_BOOLEAN)
    {
        plugMainUnion r = m_main(FF_GETPARAMETER,(DWORD)paramNum, m_instanceid);

        //make sure the call to get the parameter succeeded before
        //reading the float value
		DEBUGPRINT(("BOOL paramNum=%d ivalue=%d fvalue=%f",paramNum,r.ivalue,r.fvalue));
		if ( r.fvalue == 0.0 ) {
			return false;
		} else {
			return true;
		}
    }
	DEBUGPRINT(("GetBoolParameter called on non-BOOL parameter?"));
    return 0.0f;
}

std::string FFGLPluginInstance::GetParameterDisplay(int paramNum)
{
    plugMainUnion r = m_main(FF_GETPARAMETERDISPLAY,(DWORD)paramNum,m_instanceid);
    char nm[17];
    memcpy(nm,r.svalue,16);
    nm[16] = 0;
	std::string display = std::string(nm);
	DEBUGPRINT(("GetParameterDisplay, display=%s",display.c_str()));
    return display;
}

DWORD FFGLPluginInstance::CallProcessOpenGL(ProcessOpenGLStructTag &t)
{
    //make sure we have code to call otherwise return the unprocessed input

    DWORD retVal = FF_FAIL;

    try
    {
        retVal = m_main(FF_PROCESSOPENGL, (DWORD)&t, m_instanceid).ivalue;
    }
    catch (...)
    {
        DEBUGPRINT(("Error on call to FreeFrame::ProcessFrame"));
        retVal = FF_FAIL;
    }

    return retVal;
}

DWORD FFGLPluginDef::InitPluginLibrary()
{
    DWORD rval = FF_FAIL;

    if (m_mainfunc==NULL) {
		DEBUGPRINT(("HEY!  m_mainfunc is NULL in InitPluginLibrary!?"));
        return rval;
	}

    //initialize the plugin
    rval = m_mainfunc(FF_INITIALISE,0,0).ivalue;
    if (rval!=FF_SUCCESS)
        return rval;

    //get the parameter names
    m_numparams = (int)m_mainfunc(FF_GETNUMPARAMETERS, 0, 0).ivalue;

    m_paramdefs = new FFGLParameterDef[m_numparams];
    int n;
    for (n=0; n<m_numparams; n++) {

        plugMainUnion u = m_mainfunc(FF_GETPARAMETERNAME,(DWORD)n,0);

        if (u.ivalue!=FF_FAIL && u.svalue!=NULL) {
            //create a temporary copy as a cstring w/null termination
            char newParamName[32];

            const char *c = u.svalue;
            char *t = newParamName;

            //FreeFrame spec defines parameter names to be 16 characters long MAX
            int numChars = 0;
            while (*c && numChars<16) {
                *t = *c;
                t++;
                c++;
                numChars++;
            }

            //make sure there's a null at the end
            *t = 0;

            FFGLParameterDef* p;
            p = SetParameterName(n, newParamName);
            u = m_mainfunc(FF_GETPARAMETERTYPE,(DWORD)n,0);
            p->type = u.ivalue;
	        u = m_mainfunc(FF_GETPARAMETERDEFAULT,(DWORD)n,0);
            if ( p->type != FF_TYPE_TEXT ) {
                p->default_float_val = u.fvalue;
	            DEBUGPRINT1(("Float Parameter n=%d s=%s type=%d default=%lf\n",
	                     n,p->name.c_str(),p->type,p->default_float_val));
            } else {
                p->default_string_val = CopyFFString16(u.svalue);
	            DEBUGPRINT1(("String Parameter n=%d s=%s",n,p->name.c_str()));
            }
			p->num = n;
        }
        else
        {
            SetParameterName(n, "Untitled");
        }
    }

    return FF_SUCCESS;
}

DWORD FFGLPluginInstance::InstantiateGL(const FFGLViewportStruct *viewport)
{
    if (m_instanceid!=INVALIDINSTANCE) {
		DEBUGPRINT(("HEY!  InstantiateGL called when already instantiated!?"));
        //already instantiated
        return FF_SUCCESS;
    }

    //instantiate 1 of the plugins
    m_instanceid = m_main(FF_INSTANTIATEGL, (DWORD)viewport, 0).ivalue;

    //if it instantiated ok, return success
    if (m_instanceid==INVALIDINSTANCE)
        return FF_FAIL;

    //make default param assignments
    int i;
	int numparams = m_plugindef->m_numparams;
    for (i=0; i<numparams; i++) {
        plugMainUnion result = m_main(FF_GETPARAMETERDEFAULT,(DWORD)i,0);
        if (result.ivalue!=FF_FAIL) {
		    DWORD ffParameterType = m_main(FF_GETPARAMETERTYPE,(DWORD)i,0).ivalue;
		    if (ffParameterType!=FF_TYPE_TEXT) {
	            SetFloatParameter(i,result.fvalue);
			} else {
	            SetStringParameter(i,result.svalue);
			}
        }
    }

    return FF_SUCCESS;
}

DWORD FFGLPluginInstance::DeInstantiateGL()
{
    if (m_instanceid==INVALIDINSTANCE) {
        //already deleted
		DEBUGPRINT(("Hey!  DeInstantiateGL called when already deleted!?"));
        return FF_SUCCESS;
    }

    DWORD rval = FF_FAIL;

    try {
        rval = m_main(FF_DEINSTANTIATEGL, 0, (DWORD)m_instanceid).ivalue;
    }
    catch (...) {
        DEBUGPRINT(("FreeFrame Exception on DEINSTANTIATE"));
    }

    m_instanceid = INVALIDINSTANCE;
    return rval;
}

const PluginInfoStruct *FFGLPluginDef::GetInfo() const
{
    plugMainUnion u = m_mainfunc(FF_GETINFO, 0, 0);
    return(u.PISvalue);
}


std::string FFGLPluginDef::GetPluginName() const
{
    const PluginInfoStruct *pis = GetInfo();
    if (pis == NULL) {
        return("");
	}
    std::string name = CopyFFString16((char*)(pis->PluginName));
    return(name);
}


DWORD FFGLPluginDef::DeinitPluginLibrary()
{
    DWORD rval = FF_FAIL;

    if (m_mainfunc!=NULL) {
        rval = m_mainfunc(FF_DEINITIALISE,0,0).ivalue;
        if (rval != FF_SUCCESS) {
            DEBUGPRINT(("FreeFrame DeInit failed"));
        }
        m_mainfunc=NULL;
    }

    return rval;
}

const char *ffglplugin_fname(const char *fn)
{
	static char fname[256];
	sprintf_s(fname, sizeof(fname), "ffglplugins/%s", fn);
	return fname;
}

void loadffglplugindef(std::string ffgldir, std::string dllnm)
{
	FFGLPluginDef *plugin = FFGLPluginDef::NewPluginDef();
	std::string dll_fname = ffgldir + "/" + dllnm;

	const char *dll = dll_fname.c_str();
	if (plugin->Load(dll) == FF_FAIL) {
		DEBUGPRINT(("Unable to load %s\n", dll));
	}
	else {
		plugin->m_dll = dllnm;
		plugin->name = plugin->GetPluginName();
		DEBUGPRINT(("Loaded FFGL plugin file=%s name=%s", dll, plugin->name.c_str()));
		ffglplugindefs[nffglplugindefs] = plugin;
		nffglplugindefs++;
	}
	return;
}

void loadffgldir(std::string ffgldir)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;

	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	int nfound = 0;

	std::string pathexpr = ffgldir + "\\*";
	std::wstring wpath = s2ws(pathexpr);

	hFind = FindFirstFile(pathexpr.c_str(), &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		return;
	}
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;

			// std::wstring wcfname = ffd.cFileName;
			// std::string cfname = NosuchToLower(ws2s(wcfname));
			std::string cfname = NosuchToLower(ffd.cFileName);

			if (NosuchEndsWith(cfname, ".dll")) {
				loadffglplugindef(ffgldir, cfname.c_str());
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		DEBUGPRINT(("loadffgldir, dwError=%ld", dwError));
	}

	FindClose(hFind);
}

void loadffglpath(std::string path) {
	std::vector<std::string> dirs = NosuchSplitOnString(path, ";");
	for (size_t i = 0; i<dirs.size(); i++) {
		loadffgldir(dirs[i]);
	}
}
