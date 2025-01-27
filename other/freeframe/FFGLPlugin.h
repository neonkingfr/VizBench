#ifndef FFGLPlugin_H
#define FFGLPlugin_H

#include "VizDebug.h"
#include <FFGL.h>
#include <string>

typedef struct FFGLParameterDef {
	std::string name;
	int num;
	int type;
	float default_float_val;
	std::string default_string_val;

} FFGLParameterDef;

typedef struct FFGLParameterInstance {
	FFGLParameterDef* _paramdef;
	float current_float;
	std::string current_string;
} FFGLParameterInstance;

class FFGLPluginDef {

public:
	//each platform implements this and returns
	//a class that derives from FFGLPluginDef
	//(that class implements the real Load and Unload methods which
	//by default return FF_FAIL below)
	static FFGLPluginDef *NewPluginDef();
  
	FFGLPluginDef();
	virtual ~FFGLPluginDef();
  
	//these methods are virtual because each platform implements
	//dynamic libraries differently
	virtual DWORD Load(const char *filename) { return FF_FAIL; }
	virtual DWORD Unload() { return FF_FAIL; }

	std::string	GetPluginName() const;
	const PluginInfoStruct *GetInfo() const;
	const PluginExtendedInfoStruct *GetExtendedInfo() const;

	std::string GetParameterName(int paramNum);
	FFGLParameterDef* findparamdef(std::string pnm);
	int getParamNum(std::string pnm);

	std::string name;
	std::string m_dll;

	int m_numparams;  
	FFGLParameterDef *m_paramdefs;
	FF_Main_FuncPtr m_mainfunc;
  
protected:
  
	//calls plugMain(FF_INITIALISE) and gets the parameter names
	DWORD InitPluginLibrary();
	DWORD DeinitPluginLibrary();
  
	FFGLParameterDef* SetParameterName(int paramNum, const char *srcString);
};

class FFGLPluginInstance {
public:
	FFGLPluginInstance(FFGLPluginDef* d, std::string viztag);
	virtual ~FFGLPluginInstance() {
		DEBUGPRINT1(("Destructor for %s in FFGLPluginInstance",m_viztag.c_str()));
		DeInstantiateGL();
	}

	bool ProcessConnect();
	bool ProcessDisconnect();
	bool isConnected();
	std::string GetParameterDisplay(int paramNum);
	float GetFloatParameter(int paramNum);
	bool GetBoolParameter(int paramNum);
	void SetFloatParameter(int paramNum, float value);
	void SetStringParameter(int paramNum, std::string value);
	void SetTime(double curTime);
	bool setparam(std::string pnm, float v);
	bool setparam(std::string pnm, std::string v);
	float getparam(std::string pnm);
	std::string getParamJsonResult(FFGLParameterDef* pd, FFGLPluginInstance* pi, const char* id);

	DWORD CallProcessOpenGL(ProcessOpenGLStructTag &t);

	//calls plugMain(FF_INSTANTIATEGL) and assigns
	//each parameter its default value
	DWORD InstantiateGL(const FFGLViewportStruct *vp);
	DWORD DeInstantiateGL();

	void setEnable(bool b){ m_enabled = b; }
	bool isEnabled(){ return m_enabled; }

	void setMoveable(bool b){ m_moveable = b; }
	bool isMoveable(){ return m_moveable; }

	std::string viztag() { return m_viztag; }
	FFGLPluginDef* plugindef() { return m_plugindef; }
	DWORD instanceid() { return m_instanceid; }

private:
	std::string m_viztag;
	bool m_enabled;
	bool m_moveable;
	FFGLPluginDef* m_plugindef;
	FFGLParameterInstance *m_params;
	FF_Main_FuncPtr m_main;

	//many plugins will return 0x00000000 as the first valid instance,
	//so we use 0xFFFFFFFF to represent an uninitialized/invalid instance
	enum { INVALIDINSTANCE=0xFFFFFFFF };
	DWORD m_instanceid;
};

#endif
