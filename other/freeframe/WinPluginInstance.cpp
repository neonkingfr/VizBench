#include "NosuchUtil.h"
#include "FFGLPlugin.h"

class WinPluginDef :
    public FFGLPluginDef
{
public:
    WinPluginDef();

    DWORD Load(const char *filename);
    DWORD Unload();

    virtual ~WinPluginDef();

protected:
    HMODULE m_ffModule;
};

FFGLPluginDef *FFGLPluginDef::NewPluginDef()
{
    return new WinPluginDef();
}

WinPluginDef::WinPluginDef()
    :m_ffModule(NULL)
{}

DWORD WinPluginDef::Load(const char *fname)
{
    //warning_printf("FreeFrame Plugin Load Failed: %s", fname);
    if (fname==NULL || fname[0]==0)
        return FF_FAIL;

    Unload();

	std::wstring wfname = s2ws(fname);
    // m_ffModule = LoadLibrary(wfname.c_str());
    m_ffModule = LoadLibrary(fname);
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

    m_ffPluginMain = pFreeFrameMain;

    DWORD rval = InitPluginLibrary();
    if (rval!=FF_SUCCESS)
        return rval;

    return FF_SUCCESS;
}

DWORD WinPluginDef::Unload()
{
    DeinitPluginLibrary();

    if (m_ffModule!=NULL)
    {
        FreeLibrary(m_ffModule);
        m_ffModule=NULL;
    }

    return FF_SUCCESS;
}

WinPluginDef::~WinPluginDef()
{
    if (m_ffModule!=NULL)
    {
        DEBUGPRINT(("plugin deleted without calling Unload()"));
    }
}
