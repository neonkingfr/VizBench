#include "NosuchUtil.h"

#include "FFGLPluginInstance.h"

#include <stdio.h>

FFGLPluginInstance:: FFGLPluginInstance()
    :m_ffPluginMain(NULL),
     m_ffInstanceID(INVALIDINSTANCE),
     m_numParameters(0)
{
    int i;
    for (i=0; i<MAX_PARAMETERS; i++) {
        m_paramNames[i] = NULL;
    }
    name = "NULL";
}

FFGLPluginInstance::~FFGLPluginInstance()
{
    if (m_ffPluginMain!=NULL || m_ffInstanceID!=INVALIDINSTANCE)
    {
        NosuchDebug("FFGLPluginInstance deleted without calling Unload()");
    }
}


FFGLParameterStruct*
FFGLPluginInstance::SetParameterName(int paramNum, const char *name)
{
    if (paramNum<0 || paramNum>=MAX_PARAMETERS)
        return NULL;

    int i = paramNum;

    if (m_paramNames[i]!=NULL)
        delete m_paramNames[i];

    int sz = strlen(name)+1;
    m_paramNames[i] = new char[sz];
    // strcpy(m_paramNames[i], name);
    strcpy_s(m_paramNames[i],sz,name);

    FFGLParameterStruct* p = &(m_params[paramNum]);
    // strcpy_s(p->rawname,sz,name);
    p->name = std::string(name);
    return p;
}

const char *FFGLPluginInstance::GetParameterName(int paramNum)
{
    if (paramNum<0 || paramNum>=MAX_PARAMETERS)
        return "";

    if (m_paramNames[paramNum]!=NULL)
        return m_paramNames[paramNum];

    return "";
}

bool FFGLPluginInstance::setparam(std::string pnm, float v)
{
    int n;
    const char *paramname = pnm.c_str();

    for ( n=0; n<m_numParameters; n++ ) {
		char *s = m_paramNames[n];
		if ( strcmp(s,paramname) == 0 ) {
			SetFloatParameter(n, v);
			return true;
		}
	}
    NosuchDebug("Didn't find FFGL parameter pnm=%s\n",pnm.c_str());
    return false;
}

void FFGLPluginInstance::SetFloatParameter(int paramNum, float value)
{
    if (paramNum<0 || paramNum>=m_numParameters ||
            m_ffInstanceID==INVALIDINSTANCE || m_ffPluginMain==NULL)
    {
        //the parameter or the plugin doesn't exist
		NosuchDebug("Hey, paramNum=%d value=%f doesn't exist?\n",paramNum,value);
        return;
    }

    //make sure its a float parameter type
    DWORD ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,(DWORD)paramNum,0).ivalue;
    if (ffParameterType!=FF_TYPE_TEXT)
    {
        SetParameterStruct ArgStruct;
        ArgStruct.ParameterNumber = paramNum;

        //be careful with this cast.. ArgStruct.NewParameterValue is DWORD
        //for this to compile correctly, sizeof(DWORD) must == sizeof(float)

        //   *((float *)(unsigned)&ArgStruct.NewParameterValue) = value;
        // ArgStruct.u.NewFloatValue = value;
		ArgStruct.NewParameterValue = (DWORD)value;

        m_ffPluginMain(FF_SETPARAMETER,(DWORD)(&ArgStruct), m_ffInstanceID);
    }
}

void FFGLPluginInstance::SetTime(double curTime)
{
    if (m_ffInstanceID==INVALIDINSTANCE || m_ffPluginMain==NULL)
    {
        NosuchDebug("Invalid SetTime call");
        return;
    }

    m_ffPluginMain(FF_SETTIME, (DWORD)(&curTime), m_ffInstanceID);
}

float FFGLPluginInstance::GetFloatParameter(int paramNum)
{
    if (paramNum<0 || paramNum>=m_numParameters ||
            m_ffInstanceID==INVALIDINSTANCE || m_ffPluginMain==NULL)
    {
        NosuchDebug("Invalid GetFloatParameter call");
        return 0.f;
    }

    //make sure its a float parameter type
    DWORD ffParameterType = m_ffPluginMain(FF_GETPARAMETERTYPE,(DWORD)paramNum,0).ivalue;
    if (ffParameterType!=FF_TYPE_TEXT)
    {
        plugMainUnion result = m_ffPluginMain(FF_GETPARAMETER,(DWORD)paramNum, m_ffInstanceID);

        //make sure the call to get the parameter succeeded before
        //reading the float value
        if (result.ivalue!=FF_FAIL)
        {
            return result.fvalue;
        }
    }

    return 0.f;
}

DWORD FFGLPluginInstance::CallProcessOpenGL(ProcessOpenGLStructTag &t)
{
    //make sure we have code to call otherwise return the unprocessed input
    if (m_ffPluginMain==NULL || m_ffInstanceID==INVALIDINSTANCE)
    {
        NosuchDebug("Invalid CallProcessOpenGL call");
        return FF_FAIL;
    }

    DWORD retVal = FF_FAIL;

    try
    {
        retVal = m_ffPluginMain(FF_PROCESSOPENGL, (DWORD)&t, m_ffInstanceID).ivalue;
    }
    catch (...)
    {
        NosuchDebug("Error on call to FreeFrame::ProcessFrame");
        retVal = FF_FAIL;
    }

    return retVal;
}

void FFGLPluginInstance::ReleaseParamNames()
{
    int i;
    for (i=0; i<MAX_PARAMETERS; i++)
    {
        if (m_paramNames[i]!=NULL)
        {
            delete [] m_paramNames[i];
            m_paramNames[i] = NULL;
        }
    }

    m_numParameters = 0;
}

DWORD FFGLPluginInstance::InitPluginLibrary()
{
    DWORD rval = FF_FAIL;

    if (m_ffPluginMain==NULL)
        return rval;

    //initialize the plugin
    rval = m_ffPluginMain(FF_INITIALISE,0,0).ivalue;
    if (rval!=FF_SUCCESS)
        return rval;

    //get the parameter names
    m_numParameters = (int)m_ffPluginMain(FF_GETNUMPARAMETERS, 0, 0).ivalue;

    m_params = new FFGLParameterStruct[m_numParameters];
    int i;
    for (i=0; i<m_numParameters; i++)
    {

        plugMainUnion u = m_ffPluginMain(FF_GETPARAMETERNAME,(DWORD)i,0);

        if (u.ivalue!=FF_FAIL && u.svalue!=NULL)
        {
            //create a temporary copy as a cstring w/null termination
            char newParamName[32];

            const char *c = u.svalue;
            char *t = newParamName;

            //FreeFrame spec defines parameter names to be 16 characters long MAX
            int numChars = 0;
            while (*c && numChars<16)
            {
                *t = *c;

                t++;
                c++;

                numChars++;
            }

            //make sure there's a null at the end
            *t = 0;

            FFGLParameterStruct* p;
            p = SetParameterName(i, newParamName);
            u = m_ffPluginMain(FF_GETPARAMETERTYPE,(DWORD)i,0);
            p->type = u.ivalue;
            u = m_ffPluginMain(FF_GETPARAMETERDEFAULT,(DWORD)i,0);
            if ( p->type != FF_TYPE_TEXT ) {
                p->default_float_val = u.fvalue;
            } else {
                p->default_string_val = CopyFFString16(u.svalue);
            }
        }
        else
        {
            SetParameterName(i, "Untitled");
        }
    }

    return FF_SUCCESS;
}

DWORD FFGLPluginInstance::InstantiateGL(const FFGLViewportStruct *viewport)
{
    if (m_ffInstanceID!=INVALIDINSTANCE)
    {
        //already instantiated
        return FF_SUCCESS;
    }

    //instantiate 1 of the plugins
    m_ffInstanceID = m_ffPluginMain(FF_INSTANTIATEGL, (DWORD)viewport, 0).ivalue;

    //if it instantiated ok, return success
    if (m_ffInstanceID==INVALIDINSTANCE)
        return FF_FAIL;

    //make default param assignments
    int i;
    for (i=0; i<MAX_PARAMETERS && i<m_numParameters; i++)
    {
        plugMainUnion result = m_ffPluginMain(FF_GETPARAMETERDEFAULT,(DWORD)i,0);
        if (result.ivalue!=FF_FAIL)
        {
            SetFloatParameter(i,result.fvalue);
        }
    }

    return FF_SUCCESS;
}

DWORD FFGLPluginInstance::DeInstantiateGL()
{
    if (m_ffInstanceID==INVALIDINSTANCE)
    {
        //already deleted
        return FF_SUCCESS;
    }

    if (m_ffPluginMain==NULL)
    {
        //no main function available to call deinstantiate.. failure
        m_ffInstanceID=INVALIDINSTANCE;
        return FF_FAIL;
    }

    DWORD rval = FF_FAIL;

    try
    {
        rval = m_ffPluginMain(FF_DEINSTANTIATEGL, 0, (DWORD)m_ffInstanceID).ivalue;
    }
    catch (...)
    {
        NosuchDebug("FreeFrame Exception on DEINSTANTIATE");
    }

    m_ffInstanceID = INVALIDINSTANCE;
    return rval;
}

const PluginInfoStruct *FFGLPluginInstance::GetInfo() const
{
    plugMainUnion u = m_ffPluginMain(FF_GETINFO, 0, 0);
    return(u.PISvalue);
}


std::string FFGLPluginInstance::GetPluginName() const
{
    const PluginInfoStruct *pis = GetInfo();
    if (pis == NULL)
        return("");
    std::string name = CopyFFString16((char*)(pis->PluginName));
    return(name);
}


DWORD FFGLPluginInstance::DeinitPluginLibrary()
{
    if (m_ffInstanceID!=INVALIDINSTANCE)
    {
        //we can't call DeInstantiate because we must
        //guarantee an active OpenGL context
        NosuchDebug("Failed to call DeInstantiateGL() before calling DeinitPluginLibrary()");
        return FF_FAIL;
    }

    ReleaseParamNames();

    DWORD rval = FF_FAIL;

    if (m_ffPluginMain!=NULL)
    {
        rval = m_ffPluginMain(FF_DEINITIALISE,0,0).ivalue;
        if (rval != FF_SUCCESS)
        {
            NosuchDebug("FreeFrame DeInit failed");
        }
        m_ffPluginMain=NULL;
    }

    return rval;
}
