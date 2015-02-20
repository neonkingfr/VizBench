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
#include "ffutil.h"

// #include <opencv/cv.h>
// #include <opencv/highgui.h>
#include <sys/types.h>
#include <sys/stat.h>

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

extern "C" {
bool
vizlet_setdll(std::string dllpath)
{
	dllpath = NosuchToLower(dllpath);

	size_t pos = dllpath.find_last_of("/\\");
	if ( pos != dllpath.npos && pos > 0 ) {
		std::string parent = dllpath.substr(0,pos);
		pos = dllpath.substr(0,pos-1).find_last_of("/\\");
		if ( pos != parent.npos && pos > 0) {
			SetVizPath(parent.substr(0,pos));
		}
	}

	return TRUE;
}
}
