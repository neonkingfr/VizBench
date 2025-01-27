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

#ifndef NSHTTP_H
#define NSHTTP_H

#include "VizJSON.h"
#include "VizDaemon.h"
#include <list>

#define REQUEST_GET 1
#define REQUEST_POST 2

class VizSocketConnection;
class VizSocket;

class VizHttpServer {
public:
	VizHttpServer(VizJsonListener* jproc, int port, std::string htmldir, int timeout, int idletime);
	VizHttpServer::~VizHttpServer();
	void Check();
	void SetHtmlDir(std::string d) { m_htmldir = d; }
	void RespondToGetOrPost(VizSocketConnection*);
	void InitializeWebSocket(VizSocketConnection *kd);
	void CloseWebSocket(VizSocketConnection *kd);
	void WebSocketMessage(VizSocketConnection *kdata, std::string msg);
	void SendAllWebSocketClients(std::string msg);
	void SetShouldBeShutdown(bool);
	bool ShouldBeShutdown();

	bool DebugRequests(bool onoff) { m_debugrequests = onoff; return m_debugrequests; }
	bool DebugRequests() { return m_debugrequests; }

private:

	void _init(std::string host, int port, int timeout);
	std::list<VizSocketConnection *> _WebSocket_Clients;
	// void AddWebSocketClient(VizSocketConnection* conn);

	VizJsonListener* m_json_processor;
	VizSocket* m_listening_socket;
	std::string m_htmldir;
	bool m_shouldbeshutdown;
	int m_port;
	bool m_debugrequests;
};

#endif
