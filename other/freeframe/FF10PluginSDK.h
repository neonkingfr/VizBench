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
// Developer: Trey Harrison
// www.harrisondigitalmedia.com
//
// Last modified: October 26 2006
//

#ifndef FF10PLUGINSDK_H
#define FF10PLUGINSDK_H

#include "FF10PluginManager.h"
#include "FF10PluginInfo.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class		CFreeFrame10Plugin
///	\brief		CFreeFrame10Plugin is the base class for all FreeFrame10 plugins developed with the FreeFrame10 SDK. 
/// \author		Gualtiero Volpe
/// \version	1.0.0.2
///
/// The CFreeFrame10Plugin class is the base class for every FreeFrame10 plugins developed with the FreeFrame10 SDK. 
/// It is derived from CFF10PluginManager, so that most of the plugin management and communication with the host 
/// can be transparently handled through the default implementations of the methods of CFF10PluginManager. 
/// While CFF10PluginManager is used by the global FreeFrame methods, CFreeFrame10Plugin provides a default implementation 
/// of the instance specific FreeFrame functions. Note that CFreeFrame10Plugin methods are virtual methods: any given 
/// FreeFrame10 plugin developed with the FreeFrame10 SDK will be a derived class of CFreeFrame10Plugin and will have to 
/// provide a custom implementation of most of such methods. Except for CFreeFrame10Plugin::GetParameterDisplay and 
/// CFreeFrame10Plugin::GetInputStatus, all the default methods of CFreeFrame10Plugin just return FF_FAIL: every derived 
/// plugin is responsible of providing its specific implementation of such default methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CFreeFrame10Plugin : public CFF10PluginManager
{
public:

	/// The standard destructor of CFreeFrame10Plugin.
	virtual ~CFreeFrame10Plugin();

	/// Default implementation of the FreeFrame getParameterDisplay instance specific function. It provides a string 
	/// to display as the value of the plugin parameter whose index is passed as parameter to the method. This default 
	/// implementation just returns the string representation of the float value of the plugin parameter. A custom 
	/// implementation may be provided by every specific plugin.
	///
	/// \param		dwIndex		The index of the parameter whose display value is queried. 
	///							It should be in the range [0, Number of plugin parameters).
	/// \return					The display value of the plugin parameter or NULL in case of error
	virtual char* GetParameterDisplay(DWORD dwIndex);

	/// Default implementation of the FreeFrame setParameter instance specific function. It allows setting the current 
	/// value of the plugin parameter whose index is passed as parameter to the method. This default implementation 
	/// always returns FF_FAIL. A custom implementation must be provided by every specific plugin.
	///
	/// \param		pParam		A pointer to a SetParameterStruct (see FreeFrame.h and FreeFrame specification for 
	///							further information) containing the index and the new value of the plugin parameter 
	///							whose value is going to be set. The parameter index should be in the range 
	///							[0, Number of plugin parameters). 
	/// \return					The default implementation always returns FF_FAIL. 
	///							A custom implementation must be provided.
	virtual DWORD SetParameter(const SetParameterStruct* pParam);
	
	/// Default implementation of the FreeFrame getParameter instance specific function. It allows getting the current 
	/// value of the plugin parameter whose index is passed as parameter to the method. This default implementation 
	/// always returns FF_FAIL. A custom implementation must be provided by every specific plugin.
	///
	/// \param		dwIndex		The index of the parameter whose current value is queried.
	///							It should be in the range [0, Number of plugin parameters).
	/// \return					The default implementation always returns FF_FAIL. 
	///							A custom implementation must be provided by every specific plugin
	virtual DWORD GetParameter(DWORD dwIndex);
	
	/// Default implementation of the FreeFrame getInputStatus instance specific function. This function is called 
	/// to know whether a given input is currently in use. For the default implementation every input is always in use. 
	/// A custom implementation may be provided by every specific plugin.
	///
	/// \param		dwIndex		The index of the input whose status is queried.
	///							It should be in the range [Minimum number of inputs, Maximum number of inputs).
	/// \return					The default implementation always returns FF_FF_INPUT_INUSE or FF_FAIL if the index 
	///							is out of range. A custom implementation may be provided by every specific plugin.
	virtual DWORD GetInputStatus(DWORD dwIndex);

	virtual DWORD ResolumeDeactivate() { return FF_FAIL; }

	/// The only public data field CFreeFrame10Plugin contains is m_pPlugin, a pointer to the plugin instance. 
	/// Subclasses may use this pointer for self-referencing (e.g., a plugin may pass this pointer to external modules, 
	/// so that they can use it for calling the plugin methods).
	CFreeFrame10Plugin *m_pPlugin;

protected:

	/// The only protected function of CFreeFrame10Plugin is its constructor. In fact, nor CFF10PluginManager objects nor 
	/// CFreeFrame10Plugin objects should be created directly, but only objects of the subclasses implementing specific 
	/// plugins should be instantiated. Moreover, subclasses should define and provide a factory method to be used by 
	/// the FreeFrame SDK for instantiating plugin objects.
	CFreeFrame10Plugin();
};


#endif
