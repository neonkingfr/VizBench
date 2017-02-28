#include "VizUtil.h"
#include "FFGLPlugin.h"

class FFGLWinPluginDef : public FFGLPluginDef
{
public:
    FFGLWinPluginDef();

    DWORD Load(const char *filename);
    DWORD Unload();

    virtual ~FFGLWinPluginDef();

protected:
    HMODULE m_ffModule;
};

FFGLPluginDef *FFGLPluginDef::NewPluginDef()
{
    return new FFGLWinPluginDef();
}

FFGLWinPluginDef::FFGLWinPluginDef()
    :m_ffModule(NULL)
{}

DWORD FFGLWinPluginDef::Load(const char *fname)
{
    //warning_printf("FreeFrame Plugin Load Failed: %s", fname);
    if (fname==NULL || fname[0]==0)
        return FF_FAIL;

    Unload();

	std::wstring wfname = s2ws(fname);
    // m_ffModule = LoadLibrary(wfname.c_str());
    m_ffModule = LoadLibraryA(fname);
	DEBUGPRINT1(("FFGLPluginDef Load fname=%s",fname));
    if (m_ffModule==NULL) {
		long err = GetLastError();
        DEBUGPRINT(("LoadLibrary of %s failed with err=%ld",wfname.c_str(),err));
        return FF_FAIL;
	}

    FF_Main_FuncPtr pFreeFrameMain = (FF_Main_FuncPtr)GetProcAddress(m_ffModule, "plugMain");

    if (pFreeFrameMain==NULL)
    {
        FreeLibrary(m_ffModule);
        m_ffModule=NULL;
        return FF_FAIL;
    }

    m_mainfunc = pFreeFrameMain;

    DWORD rval = InitPluginLibrary();
    if (rval!=FF_SUCCESS)
        return rval;

    return FF_SUCCESS;
}

DWORD FFGLWinPluginDef::Unload()
{
    DeinitPluginLibrary();

    if (m_ffModule!=NULL)
    {
        FreeLibrary(m_ffModule);
        m_ffModule=NULL;
    }

    return FF_SUCCESS;
}

FFGLWinPluginDef::~FFGLWinPluginDef()
{
    if (m_ffModule!=NULL)
    {
        DEBUGPRINT(("plugin deleted without calling Unload()"));
    }
}
