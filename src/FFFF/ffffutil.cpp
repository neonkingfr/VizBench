/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <functional> 
#include <cctype>
#include <algorithm>

#include "NosuchDebug.h"
#include "NosuchUtil.h"
#include "ffffutil.h"

#include "FF10Plugin.h"

#include <sys/types.h>
#include <sys/stat.h>

double curFrameTime = 0.0;

int nff10plugindefs;
FF10PluginDef *ff10plugindefs[MAXPLUGINS];
int nffglplugindefs;
FFGLPluginDef *ffglplugindefs[MAXPLUGINS];

FF10PluginDef *preplugins[NPREPLUGINS];

// I don't think these initial values actually matter
int	ffWidth = 640;
int	ffHeight = 480;

FFGLViewportStruct fboViewport;
FFGLViewportStruct windowViewport;
FFGLTextureStruct mapTexture;
int fboWidth;
int fboHeight;
FFGLExtensions glExtensions;
FFGLFBO fbo1;
FFGLFBO fbo2;
FFGLFBO* fbo_input;
FFGLFBO* fbo_output;

std::string
CopyFFString16(const char *src)
{
    static char buff[17];   // static shouldn't be needed, hack to try to figure something out?

	int cnt = 0;
	for ( int n=0; n<16; n++ ) {
		buff[n] = src[n];
		if ( buff[n] == 0 ) {
			break;
		}
	}
    buff[16] = 0;
    return std::string(buff);
}

FF10PluginDef *
findff10plugin(std::string nm) {
    for ( int n=0; n<nff10plugindefs; n++ ) {
        if ( ff10plugindefs[n]->name == nm ) {
            return ff10plugindefs[n];
        }
    }
    return NULL;
}

FFGLPluginDef *
findffglplugin(std::string nm) {
	// NOTE: case-insensitive lookup of plugin name!
    for ( int n=0; n<nffglplugindefs; n++ ) {
        if ( _stricmp(ffglplugindefs[n]->name.c_str(),nm.c_str()) == 0 ) {
            return ffglplugindefs[n];
        }
    }
    return NULL;
}

FFGLParameterDef *
findffglparam(FFGLPluginDef* ffgl, std::string nm) {
    for ( int i=0; i<ffgl->m_numparams; i++ ) {
        FFGLParameterDef* p = &(ffgl->m_paramdefs[i]);
        if ( p->name == nm ) {
            return p;
        }
    }
    return NULL;
}


// trim from start
std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}


FFGLTextureStruct CreateOpenGLTexture(int textureWidth, int textureHeight)
{
    //note - there must be an active opengl context when this is called
    //ie, wglMakeCurrent(someHDC, someHGLRC)

    int glTextureWidth;
    int glTextureHeight;

// #define POT_TEXTURE_SIZE
#ifdef POT_TEXTURE_SIZE
    //find smallest power of two sized
    //texture that can contain the texture
    glTextureWidth = 1;
    while (glTextureWidth<textureWidth) glTextureWidth *= 2;

    glTextureHeight = 1;
    while (glTextureHeight<textureHeight) glTextureHeight *= 2;
#else
    glTextureWidth = textureWidth;
    glTextureHeight = textureHeight;
#endif

    //create and setup the gl texture
    GLuint glTextureHandle = 0;
    glGenTextures(1, &glTextureHandle);

    //bind this new texture so that glTex* calls apply to it
    glBindTexture(GL_TEXTURE_2D, glTextureHandle);

    //use bilinear interpolation when the texture is scaled larger
    //than its true size
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //no mipmapping (for when the texture is scaled smaller than its
    //true size)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //no wrapping (for when texture coordinates reference outside the
    //bounds of the texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    //this allocates room for the gl texture, but doesn't fill it with any pixels
    //(the NULL would otherwise contain a pointer to the texture data)
    glTexImage2D(GL_TEXTURE_2D,
                 0, 3, //we assume a 24bit image, which has 3 bytes per pixel
                 glTextureWidth,
                 glTextureHeight,
                 0, GL_BGR_EXT,
                 GL_UNSIGNED_BYTE,
                 NULL);

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //fill the OpenGLTextureStruct
    FFGLTextureStruct t;

    t.Handle = glTextureHandle;

    t.Width = textureWidth;
    t.Height = textureHeight;

#ifdef TRYTHIS
    t.Width = glTextureWidth;
    t.Height = glTextureHeight;
#endif

    t.HardwareWidth = glTextureWidth;
    t.HardwareHeight = glTextureHeight;

    return t;
}

int FFGLinit2(int ffgl_width, int ffgl_height)
{
    glExtensions.Initialize();
    if (glExtensions.EXT_framebuffer_object==0)
    {
        DEBUGPRINT(("FBO not detected, cannot continue"));
        return 0;
    }

    //set swap control so that the framerate is capped
    //at the monitor refresh rate
    if (glExtensions.WGL_EXT_swap_control)
        glExtensions.wglSwapIntervalEXT(0);

    fboWidth = ffgl_width;
    fboHeight = ffgl_height;

    if (!fbo1.Create(fboWidth, fboHeight, glExtensions))
    {
        DEBUGPRINT(("Framebuffer Object Init Failed"));
        return 0;
    }
    if (!fbo2.Create(fboWidth, fboHeight, glExtensions))
    {
        DEBUGPRINT(("Framebuffer Object Init Failed"));
        return 0;
    }
    //instantiate the first plugin with the FBO viewport
    fboViewport.x = 0;
    fboViewport.y = 0;
    fboViewport.width = fboWidth;
    fboViewport.height = fboHeight;

	fbo_input = NULL;
	fbo_output = NULL;

    //instantiate the second plugin with the output window
    //viewport
    // FFGLViewportStruct windowViewport;
    windowViewport.x = 0;
    windowViewport.y = 0;
    windowViewport.width = ffgl_width;
    windowViewport.height = ffgl_height;

    //allocate a texture for the map
    mapTexture = CreateOpenGLTexture(fboWidth,fboHeight);
    if (mapTexture.Handle==0)
    {
        DEBUGPRINT(("Texture allocation failed"));
        return 0;
    }

    return 1;
}


bool
ff_passthru(ProcessOpenGLStruct *pGL)
{
	if (pGL->numInputTextures<1)
		return false;

	if (pGL->inputTextures[0]==NULL)
		return false;
  
	FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

	//bind the texture handle
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);

	glEnable(GL_TEXTURE_2D); 

	// glColor4f(0.0,0.0,1.0,0.5);
	glColor4f(1.0,1.0,1.0,1.0);
	glLineWidth((GLfloat)10.0f);

	glBegin(GL_QUADS);
	glTexCoord2d(0.0,maxCoords.t);
	glVertex3f(-1.0f, 1.0f, 0.0f);	// Top Left

	glTexCoord2d(maxCoords.s,maxCoords.t);
	glVertex3f( 1.0f, 1.0f, 0.0f);	// Top Right

	glTexCoord2d(maxCoords.s,0.0);
	glVertex3f( 1.0f,-1.0f, 0.0f);	// Bottom Right

	glTexCoord2d(0.0,0.0);
	glVertex3f(-1.0f,-1.0f, 0.0f);	// Bottom Left

	glEnd();

	glDisable(GL_TEXTURE_2D);
	return true;
}

void ResetIdentity()
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

bool
do_ffgl_plugin(FFGLPluginInstance* plugin, int which) // which: 0 = first one, 1 = middle, 2 = last, 3 = one and only one, 4 none
{
	DEBUGPRINT1(("do_ffgl_plugin which=%d", which));

	//prepare the structure used to call
	//the plugin's ProcessOpenGL method
	ProcessOpenGLStructTag processStruct;

	FFGLTextureStruct *inputTextures[1];

	if (which == 0) {
		fbo_output = &fbo1;
		fbo_input = NULL;
		//activate the fbo as our render target
		if (!fbo_output->BindAsRenderTarget(glExtensions))
		{
			DEBUGPRINT(("FBO Bind As Render Target Failed!\n"));
			return false;
		}
		//create the array of OpenGLTextureStruct * to be passed to the plugin
		inputTextures[0] = &mapTexture;
		processStruct.numInputTextures = 1;
		processStruct.inputTextures = inputTextures;

		//we must let the plugin know that it is rendering into a FBO
		//by sharing with it the handle to the currently bound FBO
		processStruct.HostFBO = fbo_output->GetFBOHandle();
	}
	else if (which == 1) {

		fbo_output->UnbindAsRenderTarget(glExtensions);

		FFGLFBO* fbo_tmp = fbo_input;
		fbo_input = fbo_output;
		fbo_output = fbo_tmp;

		if (fbo_output == NULL) {
			fbo_output = &fbo2;
		}

		if (!fbo_output->BindAsRenderTarget(glExtensions))
		{
			DEBUGPRINT(("FBO Bind As Render Target Failed!\n"));
			return false;
		}

		//create the array of OpenGLTextureStruct * to be passed
		//to the plugin
		FFGLTextureStruct fboTexture = fbo_input->GetTextureInfo();
		inputTextures[0] = &fboTexture;
		processStruct.numInputTextures = 1;
		processStruct.inputTextures = inputTextures;

		//we must let the plugin know that it is rendering into a FBO
		//by sharing with it the handle to the currently bound FBO
		processStruct.HostFBO = fbo_output->GetFBOHandle();
	}
	else if (which == 2) {
		FFGLFBO* fbo_tmp = fbo_input;
		fbo_input = fbo_output;
		fbo_output = fbo_tmp;

		//now pass the contents of the FBO as a texture to the plugin
		FFGLTextureStruct fboTexture = fbo_input->GetTextureInfo();
		inputTextures[0] = &fboTexture;
		processStruct.numInputTextures = 1;
		processStruct.inputTextures = inputTextures;
		processStruct.HostFBO = 0;
	}
	else if (which == 3) {
		fbo_input = NULL;
		fbo_output = NULL;
		inputTextures[0] = &mapTexture;
		processStruct.numInputTextures = 1;
		processStruct.inputTextures = inputTextures;
		processStruct.HostFBO = 0;
	}
	else if (which == 4) {
		fbo_input = NULL;
		fbo_output = NULL;
		inputTextures[0] = &mapTexture;
		processStruct.numInputTextures = 1;
		processStruct.inputTextures = inputTextures;
		processStruct.HostFBO = 0;
	}
	else {
		DEBUGPRINT(("Unexpected value of which = %d !?", which));
		return false;
	}

	//set the gl viewport to equal the size of the FBO
	glViewport(
		fboViewport.x,
		fboViewport.y,
		fboViewport.width,
		fboViewport.height);

	//make sure all the matrices are reset
	ResetIdentity();

	//clear the depth and color buffers
	glClearColor(0, 0, 0, 0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// glScaled(2.0,2.0,1.0);
	// glTranslated(-0.5,-0.5,0.0);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	if (plugin) {
		//tell plugin 1 what time it is
		plugin->SetTime(curFrameTime);

		//call the plugin's ProcessOpenGL
		if (plugin->CallProcessOpenGL(processStruct) == FF_SUCCESS)
		{
			//if the plugin call succeeds, the drawning is complete
		}
		else {
			DEBUGPRINT(("Plugin 1's ProcessOpenGL failed"));
			return false;
		}
	}
	else {
		ff_passthru(&processStruct);
	}
	return true;
}