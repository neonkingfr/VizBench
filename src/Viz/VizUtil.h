#ifndef FFFFUTIL_H
#define FFFFUTIL_H

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

#include "FFGL.h"
#include "FFGLPlugin.h"
#include "FFGLLib.h"
#include "FF10Plugin.h"
#include "FreeFrame.h"

#include <FFGLFBO.h>

#define MAXPLUGINS 512

class FF10PluginDef;
class FFGLPluginDef;

typedef std::vector < FFGLPluginInstance* > FFGLPluginList;
typedef std::string(*PathGenerator)(std::string f);

// XXX - someday, either the FF10 stuff will be deleted, or
// an FF10Pipeline class to match FFGLPipeline should be created.
typedef std::vector < FF10PluginInstance* > FF10Pipeline;

extern int nff10plugindefs;
extern FF10PluginDef *ff10plugindefs[MAXPLUGINS];

extern int nffglplugindefs;
extern FFGLPluginDef *ffglplugindefs[MAXPLUGINS];

extern double curFrameTime;

extern FFGLTextureStruct mapTexture;
extern FFGLTextureStruct spoutTexture;
extern FFGLExtensions glExtensions;
extern int	ffWidth;
extern int	ffHeight;

struct CvCapture;
class FFGLPluginInstance;

std::string CopyFFString16(const char *src);
#define FFString CopyFFString16
bool ff_passthru(ProcessOpenGLStruct *pGL);

std::string &trim(std::string &s);

FF10PluginDef* findff10plugindef(std::string nm);
FF10ParameterDef* findff10param(FF10PluginDef* ff, std::string nm);

FFGLPluginDef * findffglplugindef(std::string nm);
FFGLParameterDef* findffglparam(FFGLPluginDef* ff, std::string nm);

#endif
