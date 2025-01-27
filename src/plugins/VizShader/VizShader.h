//
//		VizShader.h
//
//		------------------------------------------------------------
//		Copyright (C) 2015. Lynn Jarvis, Leading Edge. Pty. Ltd.
//
//		This program is free software: you can redistribute it and/or modify
//		it under the terms of the GNU Lesser General Public License as published by
//		the Free Software Foundation, either version 3 of the License, or
//		(at your option) any later version.
//
//		This program is distributed in the hope that it will be useful,
//		but WITHOUT ANY WARRANTY; without even the implied warranty of
//		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//		GNU Lesser General Public License for more details.
//
//		You will receive a copy of the GNU Lesser General Public License along 
//		with this program.  If not, see http://www.gnu.org/licenses/.
//		--------------------------------------------------------------
//
#pragma once
#ifndef VIZSHADERX_H
#define VIZSHADERX_H

#include "FFGL.h"
#include "FFGLLib.h"
#include "FFGLShader.h"
#include "FFGLPluginSDK.h"
#include "Vizlet.h"
#define NewParameterValue u.NewParameterValue

#define GL_SHADING_LANGUAGE_VERSION	0x8B8C
#define GL_READ_FRAMEBUFFER_EXT		0x8CA8
#define GL_TEXTURE_WRAP_R			0x8072

class VizShader : public Vizlet
{

public:

	VizShader();
	virtual ~VizShader();

	///////////////////////////////////////////////////
	// FreeFrameGL plugin methods
	///////////////////////////////////////////////////
	
	DWORD SetParameter(const SetParameterStruct* pParam);		
	DWORD GetParameter(DWORD dwIndex);					
	DWORD RealProcessOpenGL(ProcessOpenGLStruct* pGL);
	DWORD InitGL(const FFGLViewportStruct *vp);
	DWORD DeInitGL();

	DWORD GetInputStatus(DWORD dwIndex);
	char * GetParameterDisplay(DWORD dwIndex);

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////
	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)  {
  		*ppOutInstance = new VizShader();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

protected:	

	// FFGL user parameters
	char  m_DisplayValue[16];
	float m_UserSpeed;
	float m_UserMouseX;
	float m_UserMouseY;
	float m_UserMouseLeftX;
	float m_UserMouseLeftY;
	float m_UserRed;
	float m_UserGreen;
	float m_UserBlue;
	float m_UserAlpha;

	// Flags
	bool bInitialized;

	// Local fbo and texture
	GLuint m_glTexture0;
	GLuint m_glTexture1;
	GLuint m_glTexture2;
	GLuint m_glTexture3;
	GLuint m_fbo;

	// Viewport
	float m_vpWidth;
	float m_vpHeight;
	
	// Time
	double startTime, elapsedTime, lastTime, PCFreq;
	__int64 CounterStart;

	//
	// Shader uniforms
	//

	// Time
	float m_time;

	// Date (year, month, day, time in seconds)
	float m_dateYear;
	float m_dateMonth;
	float m_dateDay;
	float m_dateTime;

	// Channel playback time (in seconds)
	// iChannelTime components are always equal to iGlobalTime
	float m_channelTime[4];

	// Channel resolution in pixels - 4 channels with width, height, depth each
	float m_channelResolution[4][3];

	// Mouse
	float m_mouseX;
	float m_mouseY;

	// Mouse left and right
	float m_mouseLeftX;
	float m_mouseLeftY;
	float m_mouseRightX;
	float m_mouseRightY;

	int m_initResources;
	FFGLExtensions m_extensions;
    FFGLShader m_shader;

	GLint m_inputTextureLocation;
	GLint m_inputTextureLocation1;
	GLint m_inputTextureLocation2;
	GLint m_inputTextureLocation3;
	
	GLint m_timeLocation;
	GLint m_dateLocation;
	GLint m_channeltimeLocation;
	GLint m_channelresolutionLocation;
	GLint m_mouseLocation;
	GLint m_resolutionLocation;
	GLint m_mouseLocationVec4;
	GLint m_screenLocation;
	GLint m_surfaceSizeLocation;
	GLint m_surfacePositionLocation;
	GLint m_vertexPositionLocation;

	// Extras
	GLint m_inputColourLocation;

	void SetDefaults();
	void StartCounter();
	double GetCounter();
	bool LoadShader(std::string shaderString);
	void CreateRectangleTexture(FFGLTextureStruct Texture, FFGLTexCoords maxCoords, GLuint &glTexture, GLenum texunit, GLuint &fbo, GLuint hostFbo);

	void processCursor(VizCursor* c, int downdragup);
	std::string processJson(std::string meth, cJSON *json, const char *id);
	bool m_disconnect_mouseXY;
	bool ReadAndLoadShader(std::string filename);

	std::string m_shaderfile;
	std::string m_newshaderfile;
	const FFGLViewportStruct *m_savevp;
};

#endif