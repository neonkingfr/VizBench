#ifndef FFGLPluginInstance_H
#define FFGLPluginInstance_H

#include "ffutil.h"
#include <FFGL.h>
#include <string>

typedef struct FFGLParameterStructTag {
	// char rawname[17];
	std::string name;
	int num;
	int type;
	float default_float_val;
	std::string default_string_val;

} FFGLParameterStruct;

class FFGLPlugin
{
public:
  //each platform implements this and returns
  //a class that derives from FFGLPlugin
  //(that class implements the real Load and Unload methods which
  //by default return FF_FAIL below)
  static FFGLPlugin *New();
  
  FFGLPlugin();
  
  //these methods are virtual because each platform implements
  //dynamic libraries differently
  virtual DWORD Load(const char *filename)
  {
    return FF_FAIL;
  }
  
  //calls plugMain(FF_INSTANTIATEGL) and assigns
  //each parameter its default value
  DWORD InstantiateGL(const FFGLViewportStruct *vp);
  
  //calls plugMain(FF_DEINSTANTIATEGL)
  DWORD DeInstantiateGL();

  virtual DWORD Unload()
  {
    return FF_FAIL;
  }

  std::string	GetPluginName() const;

  //these methods are shared by the
  //platform-specific implementations
  const char *GetParameterName(int paramNum);
  float GetFloatParameter(int paramNum);
  void SetFloatParameter(int paramNum, float value);
  void SetTime(double curTime);
  bool setparam(std::string pnm, float v);
  DWORD CallProcessOpenGL(ProcessOpenGLStructTag &t);
  
  virtual ~FFGLPlugin();

  const	PluginInfoStruct *GetInfo() const;

  std::string name;
  std::string m_dll;

  enum { MAX_PARAMETERS = 64 };
  int m_numParameters;  
  char *m_paramNames[MAX_PARAMETERS];
  FFGLParameterStruct *m_params;
  
protected:
  FF_Main_FuncPtr m_ffPluginMain;
  
  //many plugins will return 0x00000000 as the first valid instance,
  //so we use 0xFFFFFFFF to represent an uninitialized/invalid instance
  enum { INVALIDINSTANCE=0xFFFFFFFF };
  DWORD m_ffInstanceID;
  

  //helper methods  
  //calls plugMain(FF_INITIALISE) and gets the
  //parameter names
  DWORD InitPluginLibrary();
  
  //calls DeletePluginInstance if needed, calls
  //ReleaseParamNames, then calls plugMain(FF_DEINITIALISE)
  DWORD DeinitPluginLibrary();
  
  FFGLParameterStruct* SetParameterName(int paramNum, const char *srcString);
  void ReleaseParamNames();


};

#endif
