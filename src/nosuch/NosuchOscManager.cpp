#include <math.h>
#include <string>
#include <sstream>
#include <intrin.h>
#include <float.h>

#include "NosuchUtil.h"
#include "NosuchOscInput.h"
#include "NosuchOscManager.h"
#include "NosuchOscTcpInput.h"
#include "NosuchOscUdpInput.h"

NosuchOscManager::NosuchOscManager(NosuchOscListener* server, std::string host, int port) {

	DEBUGPRINT2(("NosuchOscManager constructor port=%d",port));
	// _seq = -1;
	// _tcp = new NosuchOscTcpInput(host,port);
	m_tcp = NULL;
	m_udp = new NosuchOscUdpInput(host,port,server);
	m_shouldbeshutdown = false;
}

NosuchOscManager::~NosuchOscManager() {
	if ( m_tcp )
		delete m_tcp;
	if ( m_udp )
		delete m_udp;
}

void
NosuchOscManager::Shutdown() {
	DEBUGPRINT(("NosuchOscManager::Shutdown called, closing listening_socket"));
	UnListen();
}

bool NosuchOscManager::ShouldBeShutdown() {
	return m_shouldbeshutdown;
}

void
NosuchOscManager::SetShouldBeShutdown(bool b) {
	m_shouldbeshutdown = b;
}

void
NosuchOscManager::Check() {
	if ( m_tcp )
		m_tcp->Check();
	if ( m_udp )
		m_udp->Check();
}

void
NosuchOscManager::UnListen() {
	if ( m_tcp )
		m_tcp->UnListen();
	if ( m_udp )
		m_udp->UnListen();
}

int
NosuchOscManager::Listen() {
	int e;
	if ( m_tcp ) {
		if ( (e=m_tcp->Listen()) != 0 ) {
			if ( e == WSAEADDRINUSE ) {
				NosuchErrorOutput("TCP port/address (%d/%s) is already in use?",m_tcp->Port(),m_tcp->Host().c_str());
			} else {
				NosuchErrorOutput("Error in _tcp->Listen = %d\n",e);
			}
			return e;
		}
	}
	if ( m_udp ) {
		if ( (e=m_udp->Listen()) != 0 ) {
			if ( e == WSAEADDRINUSE ) {
				NosuchErrorOutput("UDP port/address (%d/%s) is already in use?",m_udp->Port(),m_udp->Host().c_str());
			} else {
				NosuchErrorOutput("Error in _udp->Listen = %d\n",e);
			}
			return e;
		}
	}
	return 0;
}
