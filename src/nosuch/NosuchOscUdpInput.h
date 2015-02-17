#ifndef _NSOSCUDP
#define _NSOSCUDP

#include "NosuchOscInput.h"
#include "winsock.h"

class NosuchOscUdpInput : public NosuchOscInput {

public:
	NosuchOscUdpInput(std::string host, int port, NosuchOscListener* processor);
	virtual ~NosuchOscUdpInput();
	int Listen();
	void Check();
	void UnListen();
	std::string Host() { return m_myhost; }
	int Port() { return m_myport; }

private:
	SOCKET m_sock;
	int m_myport;
	std::string m_myhost;
	// NosuchOscListener* m_processor;
};

#endif