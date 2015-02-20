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
// Last modified: Oct 25 2006 by Trey Harrison
// email:trey@harrisondigitalmedia.com

#include "FF10PluginManager.h"
#include "FF10PluginSDK.h"

#include <stdlib.h> 
#include <memory.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFF10PluginManager constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFF10PluginManager::CFF10PluginManager()
{
	m_iMinInputs = 0;
	m_iMaxInputs = 0;
	m_timeSupported = 0;

	m_NParams = 0;
	m_pFirst = NULL;
	m_pLast = NULL;

	m_VideoInfo.BitDepth = FF_DEPTH_24;
	m_VideoInfo.FrameHeight = 0;
	m_VideoInfo.FrameWidth = 0;
	m_VideoInfo.Orientation = FF_ORIENTATION_TL;

}

CFF10PluginManager::~CFF10PluginManager()
{
	if (m_pFirst != NULL)
  {
		ParamInfo* pCurr = m_pFirst;
		ParamInfo* pNext = m_pFirst;

		while (pCurr != NULL)
    {
			pNext = pCurr->pNext;

			if ( (pCurr->dwType == FF_TYPE_TEXT) &&
				 (pCurr->StrDefaultValue != NULL) )
			{
				free(pCurr->StrDefaultValue);
			}

			delete pCurr;
			pCurr = pNext;
		}
	}

	m_pFirst = NULL;
	m_pLast = NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFF10PluginManager methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFF10PluginManager::SetMinInputs(int iMinInputs)
{
	m_iMinInputs = iMinInputs;
}

void CFF10PluginManager::SetMaxInputs(int iMaxInputs)
{
	m_iMaxInputs = iMaxInputs;
}

void CFF10PluginManager::SetParamInfo(DWORD dwIndex, const char* pchName, DWORD dwType, float fDefaultValue)
{
	ParamInfo* pInfo = new ParamInfo;
	pInfo->ID = dwIndex;
	
	bool bEndFound = false;
	for (int i = 0; i < 16; ++i) {
		if (pchName[i] == 0) bEndFound = true;
		pInfo->Name[i] = (bEndFound) ?  0 : pchName[i];
	}
	
	pInfo->dwType = dwType;
	if (fDefaultValue > 1.0) fDefaultValue = 1.0;
	if (fDefaultValue < 0.0) fDefaultValue = 0.0;
	pInfo->DefaultValue = fDefaultValue;
	pInfo->StrDefaultValue = NULL;
	pInfo->pNext = NULL;
	if (m_pFirst == NULL) m_pFirst = pInfo; 
	if (m_pLast != NULL) m_pLast->pNext = pInfo;
	m_pLast = pInfo;
	m_NParams++;
}

void CFF10PluginManager::SetParamInfo(DWORD dwIndex, const char* pchName, DWORD dwType, bool bDefaultValue)
{
	ParamInfo* pInfo = new ParamInfo;
	pInfo->ID = dwIndex;
	
	bool bEndFound = false;
	for (int i = 0; i < 16; ++i) {
		if (pchName[i] == 0) bEndFound = true;
		pInfo->Name[i] = (bEndFound) ?  0 : pchName[i];
	}
	
	pInfo->dwType = dwType;
	pInfo->DefaultValue = bDefaultValue ? 1.0f : 0.0f;
	pInfo->StrDefaultValue = NULL;
	pInfo->pNext = NULL;
	if (m_pFirst == NULL) m_pFirst = pInfo; 
	if (m_pLast != NULL) m_pLast->pNext = pInfo;
	m_pLast = pInfo;
	m_NParams++;
}

void CFF10PluginManager::SetParamInfo(DWORD dwIndex, const char* pchName, DWORD dwType, const char* pchDefaultValue)
{
	ParamInfo* pInfo = new ParamInfo;
	pInfo->ID = dwIndex;
	
	bool bEndFound = false;
	for (int i = 0; i < 16; ++i) {
		if (pchName[i] == 0) bEndFound = true;
		pInfo->Name[i] = (bEndFound) ?  0 : pchName[i];
	}

	pInfo->dwType = dwType;
	pInfo->DefaultValue = 0;
	pInfo->StrDefaultValue = _strdup(pchDefaultValue);
	pInfo->pNext = NULL;
	if (m_pFirst == NULL) m_pFirst = pInfo; 
	if (m_pLast != NULL) m_pLast->pNext = pInfo;
	m_pLast = pInfo;
	m_NParams++;
}

void CFF10PluginManager::SetTimeSupported(bool supported)
{
  m_timeSupported = supported;
}

char* CFF10PluginManager::GetParamName(DWORD dwIndex) const
{
	ParamInfo* pCurr = m_pFirst;
	bool bFound = false;
	while (pCurr != NULL) {
		if (pCurr->ID == dwIndex) {
			bFound = true;
			break;
		}
		pCurr = pCurr->pNext;
	}
	if (bFound) return pCurr->Name;
	return NULL;
}
	
DWORD CFF10PluginManager::GetParamType(DWORD dwIndex) const
{
	ParamInfo* pCurr = m_pFirst;
	bool bFound = false;
	while (pCurr != NULL) {
		if (pCurr->ID == dwIndex) {
			bFound = true;
			break;
		}
		pCurr = pCurr->pNext;
	}
	if (bFound) return pCurr->dwType;
	return FF_FAIL;
}

void* CFF10PluginManager::GetParamDefault(DWORD dwIndex) const
{
	ParamInfo* pCurr = m_pFirst;
	bool bFound = false;
	while (pCurr != NULL) {
		if (pCurr->ID == dwIndex) {
			bFound = true;
			break;
		}
		pCurr = pCurr->pNext;
	}
	if (bFound) {
		if (GetParamType(dwIndex) == FF_TYPE_TEXT) {
			// XXX - TJT change here.
			return (void*)&(pCurr->StrDefaultValue);
		} else {
			return (void*) &pCurr->DefaultValue;
		}
	}
	return NULL;
}

bool CFF10PluginManager::GetTimeSupported() const
{
  return m_timeSupported;
}

void CFF10PluginManager::SetVideoInfo(const VideoInfoStruct* pVideoInfo)
{
	if (pVideoInfo != NULL) {
		memcpy(&m_VideoInfo, pVideoInfo, sizeof(VideoInfoStruct));
	}
}