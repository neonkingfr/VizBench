#ifndef _NOSUCH_DAEMON_H
#define _NOSUCH_DAEMON_H

#include <pthread.h>
#include <string>

class NosuchJsonListener;
class NosuchHttpServer;
class NosuchOscManager;
class NosuchOscListener;

class NosuchDaemon {
public:
	NosuchDaemon(
		int osc_port,
		std::string osc_host,
		NosuchOscListener* oscproc,
		int http_port,
		std::string html_dir,
		NosuchJsonListener* jsonproc);
	virtual ~NosuchDaemon();
	void *network_input_threadfunc(void *arg);

	void SendAllWebSocketClients(std::string msg);

	bool DebugRequests(bool onoff);
	bool DebugRequests();

private:
	bool m_network_thread_created;
	bool daemon_shutting_down;
	pthread_t _network_thread;
	NosuchOscManager* m_oscinput;
	NosuchHttpServer* m_httpserver;
	// std::list<VizletHttpClient*> _httpclients;

	bool m_listening;
	bool m_debugrequests;
};

#endif