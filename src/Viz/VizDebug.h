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

#ifndef VizDebug_H
#define VizDebug_H

#include <string>

#include "VizException.h"

extern int VizDebugLevel;
extern bool VizDebugToConsole;
extern bool VizDebugToLog;
extern bool VizDebugTimeTag;
extern bool VizDebugThread;
extern bool VizDebugAutoFlush;
extern std::string VizDebugLogPath;
extern std::string VizDebugLogFile;
extern std::string VizDebugLogDir;
extern std::string VizDebugPrefix;

std::string VizSnprintf(const char *fmt, ...);

void VizDebugInit();
void VizDebugCleanup();

void VizDebugSetLogDirFile(std::string logdir, std::string logfile);
void VizDebugDumpLog();
void VizDebug(char const *fmt, ... );
void VizDebug(int level, char const *fmt, ... );
void VizErrorOutput(const char *fmt, ...);
std::string VizForwardSlash(std::string filepath);

void SetVizPath(std::string path);
std::string VizConfigPath(std::string f1 = "", std::string f2 = "", std::string f3 = "");
std::string VizPath(std::string fname);

#define DBGLEVEL 0

#if DBGLEVEL >= 0
#define DEBUGPRINT(x) VizDebug x
#else
#define DEBUGPRINT(x)
#endif

#if DBGLEVEL >= 1
#define DEBUGPRINT1(x) VizDebug x
#else
#define DEBUGPRINT1(x)
#endif

#if DBGLEVEL >= 2
#define DEBUGPRINT2(x) VizDebug x
#else
#define DEBUGPRINT2(x)
#endif

#define VizAssert(expr) if(!(expr)){ throw VizException("VizAssert (%s) failed at %s:%d",#expr,__FILE__,__LINE__);}

typedef void (*ErrorCallbackFuncType)(void* data, const char* msg); 

#endif
