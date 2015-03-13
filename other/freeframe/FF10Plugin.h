// Copyleft 2006 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*	
		chris korda

		revision history:
		rev		date	comments
		00		24jul06	initial version
		01		23nov07	support Unicode

		wrapper for freeframe plugin DLL

*/

#ifndef FFPLUGIN_H
#define FFPLUGIN_H

#include "FreeFrame.h"

std::string CopyFFString(const char* src);

class FF10ParameterDef {
public:
	FF10ParameterDef() {
		name = "";
		num = -1;
		type = FF_TYPE_STANDARD;
		default_float_val = 0.0;
		default_string_val = "";
	}
	std::string name;
	int num;
	int type;
	float default_float_val;
	std::string default_string_val;
};

typedef struct FF10ParameterInstance {
	FF10ParameterDef* _paramdef;
	float current_float;
	std::string current_string;
} FF10ParameterInstance;

class FF10PluginDef {
public:
// Construction
	FF10PluginDef();
	virtual ~FF10PluginDef();
	bool	Load(std::string Path);
	bool	Free();
	bool	LoadParamDefs();
	bool	Process(int instanceid, unsigned char *pixels);

// Attributes
	bool	IsLoaded() const;
	const	PluginInfoStruct *GetInfo() const;
	bool	GetInfo(PluginInfoStruct& PlugInfo) const;
	std::string	GetPluginName() const;
	std::string GetParameterName(int paramNum);
	FF10ParameterDef* findparamdef(std::string pnm);
	int getParamNum(std::string pnm);

	std::string name;
	std::string m_dll;

	int m_numparams;
	FF10ParameterDef *m_paramdefs;
	FF_Main_FuncPtr	m_mainfunc;

protected:
// Member data
	HINSTANCE	m_hInst;	// instance handle of the plugin's DLL

};

class FF10PluginInstance {
public:
	FF10PluginInstance(FF10PluginDef* d, std::string nm);
	virtual ~FF10PluginInstance() {
		DEBUGPRINT1(("Destructor for %s in FF10PluginInstance",m_name.c_str()));
		DeInstantiate();
	}

	std::string GetParameterDisplay(int paramNum);
	float GetFloatParameter(int paramNum);
	bool GetBoolParameter(int paramNum);
	void SetFloatParameter(int paramNum, float value);
	void SetStringParameter(int paramNum, std::string value);
	bool setparam(std::string pnm, float v);
	bool setparam(std::string pnm, std::string v);
	float getparam(std::string pnm);
	std::string getParamJsonResult(FF10ParameterDef* pd, FF10PluginInstance* pi, const char* id);

	DWORD		Instantiate(VideoInfoStruct *vis);
	DWORD	DeInstantiate();

	void enable(){ m_enabled = true; }
	void disable() { m_enabled = false; }
	bool isEnabled(){ return m_enabled; }
	std::string name() { return m_name; }
	FF10PluginDef* plugindef() { return m_plugindef; }
	DWORD instanceid() { return m_instanceid; }

	// void setInstanceID(DWORD id) { m_instanceid = id; }

private:
	std::string m_name;
	bool m_enabled;
	FF10PluginDef* m_plugindef;
	FF10ParameterInstance *m_params;
	FF_Main_FuncPtr m_mainfunc;

	//many plugins will return 0x00000000 as the first valid instance,
	//so we use 0xFFFFFFFF to represent an uninitialized/invalid instance
	enum { INVALIDINSTANCE = 0xFFFFFFFF };
	DWORD m_instanceid;
};

inline bool FF10PluginDef::IsLoaded() const
{
	return(m_mainfunc != NULL);
}

#endif
