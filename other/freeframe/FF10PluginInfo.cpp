//
// Copyright (c) 2004 - InfoMus Lab - DIST - University of Genova
//
// InfoMus Lab (Laboratorio di Informatica Musicale)
// DIST - University of Genova 
//
// http://www.infomus.dist.unige.it
// news://infomus.dist.unige.it
// mailto:staff@infomus.dist.unige.it
//
// Developer: Gualtiero Volpe
// mailto:volpe@infomus.dist.unige.it
//
// Last modified: 2004-11-10
//

#include "FF10PluginInfo.h"

#include <stdlib.h> 
#include <memory.h>

CFF10PluginInfo* g_CurrPluginInfo;

	
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFF10PluginInfo constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFF10PluginInfo::CFF10PluginInfo(
		FPCREATEINSTANCE10* pCreateInstance,
		const char* pchUniqueID,
		const char* pchPluginName,
		DWORD dwAPIMajorVersion,
		DWORD dwAPIMinorVersion,
		DWORD dwPluginMajorVersion,
		DWORD dwPluginMinorVersion,
		DWORD dwPluginType,
		const char* pchDescription,
		const char* pchAbout,
		DWORD dwFreeFrameExtendedDataSize,
		const void* pFreeFrameExtendedDataBlock
	)
{
	m_pCreateInstance = pCreateInstance;

	// Filling PluginInfoStruct
	m_PluginInfo.APIMajorVersion = dwAPIMajorVersion;
	m_PluginInfo.APIMinorVersion = dwAPIMinorVersion;
	
	bool bEndFound = false;
	for (int i = 0; (i < 16) && (!bEndFound); ++i) {
		if (pchPluginName[i] == 0) bEndFound = true;
		(m_PluginInfo.PluginName)[i] = (bEndFound) ?  0 : pchPluginName[i];
	}

	bEndFound = false;
	for (int j = 0; (j < 4) && (!bEndFound); ++j) {
		if (pchUniqueID[j] == 0) bEndFound = true;
		(m_PluginInfo.PluginUniqueID)[j] = (bEndFound) ?  0 : pchUniqueID[j];
	}

	m_PluginInfo.PluginType = dwPluginType;

	// Filling PluginExtendedInfoStruct
	m_PluginExtendedInfo.About = _strdup(pchAbout);
	m_PluginExtendedInfo.Description = _strdup(pchDescription);
	m_PluginExtendedInfo.PluginMajorVersion = dwPluginMajorVersion;
	m_PluginExtendedInfo.PluginMinorVersion = dwPluginMinorVersion;
	if ((dwFreeFrameExtendedDataSize > 0) && (pFreeFrameExtendedDataBlock != NULL)) {
		memcpy(m_PluginExtendedInfo.FreeFrameExtendedDataBlock, pFreeFrameExtendedDataBlock, dwFreeFrameExtendedDataSize);
		m_PluginExtendedInfo.FreeFrameExtendedDataSize = dwFreeFrameExtendedDataSize;
	}
	else {
		m_PluginExtendedInfo.FreeFrameExtendedDataBlock = NULL;
		m_PluginExtendedInfo.FreeFrameExtendedDataSize = 0;
	}

	g_CurrPluginInfo = this;
}

CFF10PluginInfo::~CFF10PluginInfo()
{
	free(m_PluginExtendedInfo.About);
	free(m_PluginExtendedInfo.Description);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFF10PluginInfo methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const PluginInfoStruct* CFF10PluginInfo::GetPluginInfo() const
{
	return &m_PluginInfo;
}

const PluginExtendedInfoStruct* CFF10PluginInfo::GetPluginExtendedInfo() const
{
	return &m_PluginExtendedInfo;
}

FPCREATEINSTANCE10* CFF10PluginInfo::GetFactoryMethod() const
{
	return m_pCreateInstance;
}

void
CFF10PluginInfo::SetPluginIdAndName(const char* pchUniqueID, const char* pchPluginName) {
	bool bEndFound = false;
	for (int i = 0; (i < 16) && (!bEndFound); ++i) {
		if (pchPluginName[i] == 0) bEndFound = true;
		(m_PluginInfo.PluginName)[i] = (bEndFound) ?  0 : pchPluginName[i];
	}

	bEndFound = false;
	for (int j = 0; (j < 4) && (!bEndFound); ++j) {
		if (pchUniqueID[j] == 0) bEndFound = true;
		(m_PluginInfo.PluginUniqueID)[j] = (bEndFound) ?  0 : pchUniqueID[j];
	}
}