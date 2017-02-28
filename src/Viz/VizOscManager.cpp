#include <math.h>
#include <string>
#include <sstream>
#include <intrin.h>
#include <float.h>

#include "VizUtil.h"
#include "VizOscInput.h"
#include "VizOscManager.h"
#include "VizOscTcpInput.h"
#include "VizOscUdpInput.h"

VizOscManager::VizOscManager(VizOscListener* server, std::string host, int port) {

	DEBUGPRINT2(("VizOscManager constructor port=%d",port));
	// _seq = -1;
	// _tcp = new VizOscTcpInput(host,port);
	m_tcp = NULL;
	m_udp = new VizOscUdpInput(host,port,server);
	m_shouldbeshutdown = false;
}

VizOscManager::~VizOscManager() {
	if ( m_tcp )
		delete m_tcp;
	if ( m_udp )
		delete m_udp;
}

void
VizOscManager::Shutdown() {
	DEBUGPRINT(("VizOscManager::Shutdown called, closing listening_socket"));
	UnListen();
}

bool VizOscManager::ShouldBeShutdown() {
	return m_shouldbeshutdown;
}

void
VizOscManager::SetShouldBeShutdown(bool b) {
	m_shouldbeshutdown = b;
}

void
VizOscManager::Check() {
	if ( m_tcp )
		m_tcp->Check();
	if ( m_udp )
		m_udp->Check();
}

void
VizOscManager::UnListen() {
	if ( m_tcp )
		m_tcp->UnListen();
	if ( m_udp )
		m_udp->UnListen();
}

int
VizOscManager::Listen() {
	int e;
	if ( m_tcp ) {
		if ( (e=m_tcp->Listen()) != 0 ) {
			if ( e == WSAEADDRINUSE ) {
				VizErrorOutput("TCP port/address (%d/%s) is already in use?",m_tcp->Port(),m_tcp->Host().c_str());
			} else {
				VizErrorOutput("Error in _tcp->Listen = %d\n",e);
			}
			return e;
		}
	}
	if ( m_udp ) {
		if ( (e=m_udp->Listen()) != 0 ) {
			if ( e == WSAEADDRINUSE ) {
				VizErrorOutput("UDP port/address (%d/%s) is already in use?",m_udp->Port(),m_udp->Host().c_str());
			} else {
				VizErrorOutput("Error in _udp->Listen = %d\n",e);
			}
			return e;
		}
	}
	return 0;
}
