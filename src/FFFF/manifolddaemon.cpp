/*
	Manifold

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

#if 0

#include "FFFF.h"

void *manifolddaemon_threadfunc(void *arg)
{
	ManifoldDaemon* b = (ManifoldDaemon*)arg;
	return b->network_input_threadfunc(arg);
}

ManifoldDaemon::ManifoldDaemon(Manifold* m, int httpport, std::string htmldir) {

	// constants
	_msec_timeout = 60;		// Event listening timeout. Even if you make it 1,
							// the CPU load should be minimal.

	_msec_idletime = 60000;  // connection idle timeout

	// initialize
	_network_thread_created = false;
	_daemon_shutting_down = false;
	_manifoldx = m;

	_httpserver = new ManifoldHttp(this,httpport,htmldir,_msec_timeout,_msec_idletime);

	int err = pthread_create(&_network_thread, NULL, manifolddaemon_threadfunc, this);
	if (err) {
		NosuchDebug("pthread_create failed!? err=%d\n",err);
		NosuchErrorOutput("pthread_create failed!?");
	} else {
		_network_thread_created = true;
		NosuchDebug("NosuchDaemon is running");
	}
}

std::string
ManifoldDaemon::RespondToJson(const std::string method, cJSON *params, const char* id) {
	return _manifoldx->RespondToJson(method, params, id);
}



void *
ManifoldDaemon::network_input_threadfunc(void *arg)
{
	int textcount = 0;
	while (_daemon_shutting_down == false ) {

		if ( ! _manifoldx->isListening() ) {
			Sleep(100);
			continue;
		}
		if ( _httpserver ) {
			if ( _httpserver->ShouldBeShutdown() ) {
				_httpserver->Shutdown();
				delete _httpserver;
				// As soon as _httpserver is set to NULL, the waiting thread
				// that is in ChangeHttpPort will open a new one and set it.
				_httpserver = NULL;
			} else {
				_httpserver->Check();
			}
		}
#if 0
		if ( _oscinput ) {
			if ( _oscinput->ShouldBeShutdown() ) {
				_oscinput->Shutdown();
				delete _oscinput;
				// As soon as _oscinput is set to NULL, the waiting thread
				// that is in ChangeOscPort will open a new one and set it.
				_oscinput = NULL;
			} else {
				_oscinput->Check();
			}
		}
#endif
		Sleep(1);
	}
	return NULL;
}

#endif
