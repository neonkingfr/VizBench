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

#include <windows.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include "tstring.h"

#include "porttime.h"
#include "pthread.h"
#include "VizJSON.h"
#include "VizServer.h"

#include <list>

#include "VizUtil.h"

using namespace std;

int VizDebugLevel = 0;
bool VizDebugToConsole = true;
bool VizDebugTimeTag = true;
bool VizDebugThread = true;
bool VizDebugToLog = true;
bool VizDebugToLogWarned = false;
bool VizDebugAutoFlush = true;

typedef void (*ErrorPopupFuncType)(const char* msg); 

std::string VizDebugPrefix = "";
std::string VizDebugLogFile = "nosuch.debug";
std::string VizDebugLogDir = ".";
std::string VizDebugLogPath;

std::string m_VizPath = "";

#ifdef DEBUG_TO_BUFFER
bool VizDebugToBuffer = true;
size_t VizDebugBufferSize = 8;
static std::list<std::string> DebugBuffer;
#endif

std::list<std::string> DebugLogBuffer;
bool DebugInitialized = FALSE;
long VizTime0;

HANDLE dMutex;

void
VizDebugSetLogDirFile(std::string logdir, std::string logfile)
{
	// If it's a full path starting with a drive letter or /)
	if ( logfile.find(":")!=logfile.npos 
		|| logfile.find("/")!=logfile.npos
		|| logfile.find("\\")!=logfile.npos ) {
		VizDebugLogFile = logfile;
		VizDebugLogDir = logdir;
		VizDebugLogPath = logfile;
	} else {
		VizDebugLogFile = logfile;
		VizDebugLogDir = logdir;
		VizDebugLogPath = logdir + "\\" + logfile;
	}
}

std::string mmmm;

void
RealDebugDumpLog() {
	// XXX - this code needs to handle situations when the file
	// doesn't already exist.
	mmmm = m_VizPath;
	std::ofstream f(VizDebugLogPath.c_str(),ios::app);
	if ( ! f.is_open() ) {
		VizDebugLogPath = "c:/tmp/viz.debug";
		f.open(VizDebugLogPath.c_str(),ios::app);
		if ( ! f.is_open() ) {
			return;
		}
	}

	while (!DebugLogBuffer.empty()) {
		std::string s = DebugLogBuffer.front();
	    f << s;
		DebugLogBuffer.pop_front();
	}
	f.close();
}

void
VizDebugDumpLog()
{
	DWORD wait = WaitForSingleObject( dMutex, INFINITE);
	if ( wait == WAIT_ABANDONED )
		return;

	RealDebugDumpLog();

	ReleaseMutex(dMutex);
}

void
VizDebugInit() {
	if ( ! DebugInitialized ) {
		dMutex = CreateMutex(NULL, FALSE, NULL);
		VizTime0 = timeGetTime();
		DebugInitialized = TRUE;
	}
}

void
VizDebugCleanup() {
	if ( DebugInitialized ) {
		ReleaseMutex(dMutex);
	}
}

bool VizDebugHack = false;

void
RealVizDebug(int level, char const *fmt, va_list args)
{
	VizDebugInit();
	if ( level > VizDebugLevel )
		return;

	DWORD wait = WaitForSingleObject( dMutex, INFINITE);
	if ( wait == WAIT_ABANDONED )
		return;

    // va_list args;
    char msg[10000];
	char* pmsg = msg;
	int msgsize = sizeof(msg)-2;

	if ( VizDebugPrefix != "" ) {
		int nchars = _snprintf_s(pmsg,msgsize,_TRUNCATE,"%s",VizDebugPrefix.c_str());
		pmsg += nchars;
		msgsize -= nchars;
	}
	if ( VizDebugTimeTag ) {
		int nchars;
		VizServer* vserver = VizServer::GetServer();
		double secs = vserver ? vserver->SchedulerCurrentTimeInSeconds() : 0.0;
		if ( VizDebugThread ) {
			nchars = _snprintf_s(pmsg,msgsize,_TRUNCATE,"[%.3f,T%ld,C%d] ",secs,(int)pthread_self().p,vserver->SchedulerCurrentClick());
		} else {
			nchars = _snprintf_s(msg,msgsize,_TRUNCATE,"[%.3f] ",secs);
		}
		pmsg += nchars;
		msgsize -= nchars;

#if 0
		{
			static double lastsecs = 0.0;
			char ttt[1024];
			nchars = _snprintf_s(ttt, 1024, _TRUNCATE, "secs=%.3f  lastsecs=%.3f  time0=%ld\n", secs, lastsecs, VizTime0);
			OutputDebugStringA(ttt);
			if (secs < lastsecs) {
				OutputDebugStringA("TIME HAS GONE BACKWARDS!");
			}
			lastsecs = secs;
		}
#endif

	}

    // va_start(args, fmt);
    vsprintf_s(pmsg,msgsize,fmt,args);

	char *p = strchr(msg,'\0');
	if ( p != NULL && p != msg && *(p-1) != '\n' ) {
		strcat_s(msg,msgsize,"\n");
	}

	if ( VizDebugToConsole ) {
		OutputDebugStringA(msg);
	}
	if ( VizDebugToLog ) {
		DebugLogBuffer.push_back(msg);
		if ( VizDebugAutoFlush )
			RealDebugDumpLog();
	}

#ifdef DEBUG_TO_BUFFER
	if ( VizDebugToBuffer ) {
		// We want the entries in the DebugBuffer to be single lines,
		// so that someone can request a specific number of lines.
		std::istringstream iss(msg);
		std::string line;
		while (std::getline(iss, line)) {
			DebugBuffer.push_back(line+"\n");
		}
		while ( DebugBuffer.size() >= VizDebugBufferSize ) {
			DebugBuffer.pop_front();
		}
	}
#endif

    // va_end(args);

	ReleaseMutex(dMutex);
}

void
VizDebug(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	RealVizDebug(0,fmt,args);
    va_end(args);
}

void
VizDebug(int level, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	RealVizDebug(level,fmt,args);
    va_end(args);
}

void
VizErrorOutput(const char *fmt, ...)
{
	VizDebugInit();

	if ( fmt == NULL ) {
		// Yes, this is recursive, but we're passing in a non-NULL fmt...
		VizErrorOutput("fmt==NULL in VizErrorOutput!?\n");
		return;
	}

    va_list args;
    va_start(args, fmt);

    char msg[10000];
    vsprintf_s(msg,sizeof(msg)-2,fmt,args);
    va_end(args);

	char *p = strchr(msg,'\0');
	if ( p != NULL && p != msg && *(p-1) != '\n' ) {
		strcat_s(msg,sizeof(msg),"\n");
	}

	OutputDebugStringA(msg);

	VizDebug("VizErrorOutput: %s",msg);
}

std::string
VizSnprintf(const char *fmt, ...)
{
	static char *buff = NULL;
	static int bufflen = 1024;
	va_list args;

	if ( buff == NULL ) {
		buff = (char*)malloc(bufflen);
		if (buff == NULL) {
			throw VizException("Out of memory in VizSnprintf!?");
		}
	}

	while (1) {
		va_start(args, fmt);
		int written = vsnprintf_s(buff,bufflen,_TRUNCATE,fmt,args);
		va_end(args);
		// The written value does NOT include the terminating NULL.
		if ( written >= 0 && written < (bufflen-1) ) {
			return std::string(buff);
		}
		free(buff);
		bufflen *= 2;
		buff = (char*)malloc(bufflen);
	}
}

// Replace ALL backslashes with forward slashes
std::string
VizForwardSlash(std::string filepath) {
	size_t i;
	while ( (i=filepath.find("\\")) != filepath.npos ) {
		filepath.replace(i,1,"/");
	}
	return filepath;
}

std::string
VizPath(std::string fn)
{
	if ( m_VizPath == "" ) {
		std::string msg = VizSnprintf("Error: VIZPATH is not set!\n");
		MessageBoxA(NULL,msg.c_str(),"Viz",MB_OK);
		m_VizPath = ".";
	}
	if (fn == "") {
		return m_VizPath;
	}
	return VizSnprintf("%s\\%s",m_VizPath.c_str(),fn.c_str());
}

void
SetVizPath(std::string vb) {
	m_VizPath = vb;
	VizDebugSetLogDirFile(VizPath("log"),"viz.debug");
}

std::string
VizConfigPath(std::string f1, std::string f2, std::string f3)
{
	static char* v = NULL;
	if (v == NULL) {
		v = getenv("VIZCONFIG");
		if (v == NULL) {
			v = "config";
		}
	}

	std::string path = VizPath(v);
	// XXX - Should use varargs
	if (f1 != "") {
		path += "\\" + f1;
	}
	if (f2 != "") {
		path += "\\" + f2;
	}
	if (f3 != "") {
		path += "\\" + f3;
	}
	return path;
}


