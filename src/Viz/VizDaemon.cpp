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

#include "VizUtil.h"
#include "VizException.h"
#include "VizOscInput.h"
#include "VizOscManager.h"
#include "VizHttpServer.h"
#include "VizDaemon.h"
#include <pthread.h>

void *network_threadfunc(void *arg)
{
	VizDaemon* b = (VizDaemon*)arg;
	return b->network_input_threadfunc(arg);
}

VizDaemon::VizDaemon(
	int osc_port,
	std::string osc_host,
	VizOscListener* oscproc,
	int http_port,
	std::string html_dir,
	VizJsonListener* jsonproc)
{
	DEBUGPRINT2(("VizDaemon CONSTRUCTOR!"));

	daemon_shutting_down = false;
	m_httpserver = NULL;
	m_oscinput = NULL;
	m_listening = false;
	m_network_thread_created = false;
	m_debugrequests = false;

	if ( VizNetworkInit() ) {
		DEBUGPRINT(("Unable to initialize networking in VizDaemon constructor, there will be no listeners"));
		return;
	}

	m_listening = true;

	m_httpserver = new VizHttpServer(jsonproc, http_port, html_dir, 60, 60000);

	if ( osc_port < 0 || oscproc == NULL ) {
		DEBUGPRINT(("NOT listening for OSC because oscport<0 or missing"));
		m_oscinput = NULL;
	} else {
		m_oscinput = new VizOscManager(oscproc,osc_host,osc_port);
		m_oscinput->Listen();
	}

	DEBUGPRINT2(("About to use pthread_create in VizDaemon"));
	int err = pthread_create(&_network_thread, NULL, network_threadfunc, this);
	if (err) {
		VizErrorOutput("pthread_create failed!? err=%d",err);
	} else {
		m_network_thread_created = true;
	}
}

VizDaemon::~VizDaemon()
{
	if (m_httpserver) {
		DEBUGPRINT(("About to shutdown httpserver in VizDaemon"));
		// XXXX - there probably needs to be better locking here.
		m_httpserver->SetShouldBeShutdown(true);
		while (m_httpserver!=NULL) {
			Sleep(1);
		}
		DEBUGPRINT(("After shutting down httpserver in VizDaemon"));
	}

	if ( m_oscinput ) {
		DEBUGPRINT(("About to UnListen on m_oscinput in VizDaemon"));
		m_oscinput->UnListen();
		DEBUGPRINT(("After UnListen on m_oscinput in VizDaemon"));
		delete m_oscinput;
		m_oscinput = NULL;
		DEBUGPRINT(("After deleting m_oscinput in VizDaemon"));
	}

	DEBUGPRINT(("About to shutdown network thread in VizDaemon"));
	daemon_shutting_down = true;
	if ( m_network_thread_created ) {
		// pthread_detach(_network_thread);
		pthread_join(_network_thread,NULL);
	}

}

bool
VizDaemon::DebugRequests(bool onoff) {
	m_debugrequests = onoff;
	m_httpserver->DebugRequests(onoff);
	return m_debugrequests;
}

bool
VizDaemon::DebugRequests() {
	return m_debugrequests;
}

void
VizDaemon::SendAllWebSocketClients(std::string msg)
{
	if ( m_httpserver ) {
		m_httpserver->SendAllWebSocketClients(msg);
	}
}

void *VizDaemon::network_input_threadfunc(void *arg)
{
	int textcount = 0;

	int cnt=100;
	while (cnt-- > 0) {
		Sleep(1);
	}
	while (daemon_shutting_down == false ) {

		if ( ! m_listening ) {
			Sleep(100);
			continue;
		}
		if ( m_httpserver ) {
			if ( m_httpserver->ShouldBeShutdown() ) {
				// Crashes happen when m_httpserver is deleted,
				// need better shutdown logic
				DEBUGPRINT(("Avoiding delete of m_httpserver"));
				// delete m_httpserver;
				m_httpserver = NULL;
			} else {
				m_httpserver->Check();
			}
		}
		if ( m_oscinput ) {
			if ( m_oscinput->ShouldBeShutdown() ) {
				m_oscinput->Shutdown();
				delete m_oscinput;
				m_oscinput = NULL;
			} else {
				m_oscinput->Check();
			}
		}
		Sleep(1);
	}

	return NULL;
}