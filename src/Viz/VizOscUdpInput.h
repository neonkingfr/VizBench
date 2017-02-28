#ifndef _NSOSCUDP
#define _NSOSCUDP

#include "VizOscInput.h"
#include "winsock.h"

class VizOscUdpInput : public VizOscInput {

public:
	VizOscUdpInput(std::string host, int port, VizOscListener* processor);
	virtual ~VizOscUdpInput();
	int Listen();
	void Check();
	void UnListen();
	std::string Host() { return m_myhost; }
	int Port() { return m_myport; }

private:
	SOCKET m_sock;
	int m_myport;
	std::string m_myhost;
	// VizOscListener* m_processor;
};

#endif