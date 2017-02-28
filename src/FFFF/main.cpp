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

#include "VizDebug.h"
#include <windows.h>
#include <stdio.h>
#include "VizServer.h"
#include "XGetopt.h"

std::vector<std::string> VizSplitOnAnyChar(std::string s, std::string sepchars)
{
	std::vector<std::string> result;
	const char *seps = sepchars.c_str();
	const char *str = s.c_str();

	while (1) {
		const char *begin = str;
		while (strchr(seps, *str) == NULL && *str) {
			str++;
		}
		result.push_back(std::string(begin, str));
		if (0 == *str) {
			break;
		}
		// skip one or more of the sepchars
		while (strchr(seps, *str) != NULL && *str) {
			str++;
		}
	}
	return result;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow )
{
	int r = -1;
	std::string vizpath = "";

	char pathbuff[MAX_PATH];
	HMODULE module = GetModuleHandleA(NULL);
	GetModuleFileNameA(module, pathbuff, MAX_PATH);
	std::string path = std::string(pathbuff);
	// We want to take off the final filename AND the directory.
	// This assumes that the DLL is in either a bin or ffglplugins
	// subdirectory of the main Vizpath
	size_t pos = path.find_last_of("/\\");
	if ( pos != path.npos && pos > 0 ) {
		std::string parent = path.substr(0,pos);
		pos = path.substr(0,pos-1).find_last_of("/\\");
		if ( pos != parent.npos && pos > 0) {
			vizpath = parent.substr(0,pos);
			// SetVizPath(parent.substr(0,pos));
		}
	}

	std::string cmdline = std::string(szCmdLine);
	std::vector<std::string> args = VizSplitOnAnyChar(cmdline, " \t\n");

	std::string pipeset = "";

	for (unsigned int n = 0; n < args.size(); n++) {
		std::string arg = args[n];
		if (arg.size() > 1 && arg[0] == '-') {
			switch (arg[1]) {
			case 'p':
				if ((n + 1) < args.size()) {
					pipeset = args[n + 1];
					n++;
				}
				break;
			}
		}
	}

	VizServer* server = VizServer::GetServer(vizpath.c_str());
	server->DoFFFF(pipeset.c_str());

    exit(EXIT_SUCCESS);
}
