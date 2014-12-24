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

typedef struct FFParameterDef {
	// char rawname[17];
	std::string name;
	int num;
	int type;
	float default_float_val;
	std::string default_string_val;
} FFParameterDef;

class FFPluginDef {
public:
// Construction
	FFPluginDef();
	~FFPluginDef();
	bool	Load(std::string Path);
	bool	Free();
	bool	LoadParamDefs();
	bool	Process(int instanceid, unsigned char *pixels);

// Attributes
	bool	IsLoaded() const;
	const	PluginInfoStruct *GetInfo() const;
	bool	GetInfo(PluginInfoStruct& PlugInfo) const;
	int		Instantiate(VideoInfoStruct *vis);
	std::string	GetPluginName() const;

	std::string name;
	std::string m_dll;
	int m_instanceid;
	bool setparam(std::string pnm, float v);
	FF_Main_FuncPtr	m_pff;	// pointer to the plugin's main function
	int m_numparams;
	FFParameterDef *m_paramdefs;

protected:
// Member data
	HINSTANCE	m_hInst;	// instance handle of the plugin's DLL

};

inline bool FFPluginDef::IsLoaded() const
{
	return(m_pff != NULL);
}

#endif
