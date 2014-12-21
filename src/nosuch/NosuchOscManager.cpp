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
	_tcp = NULL;
	_udp = new NosuchOscUdpInput(host,port,server);
	_shouldbeshutdown = false;
	_shutdowncomplete = false;
}

NosuchOscManager::~NosuchOscManager() {
	if ( _tcp )
		delete _tcp;
	if ( _udp )
		delete _udp;
}

void
NosuchOscManager::Shutdown() {
	DEBUGPRINT(("NosuchOscManager::Shutdown called, closing listening_socket"));
	UnListen();
	_shutdowncomplete = true;
}

bool NosuchOscManager::IsShutdownComplete() {
	return _shutdowncomplete;
}

bool NosuchOscManager::ShouldBeShutdown() {
	return _shouldbeshutdown;
}

void
NosuchOscManager::SetShouldBeShutdown(bool b) {
	_shouldbeshutdown = b;
	_shutdowncomplete = false;
}

void
NosuchOscManager::Check() {
	if ( _tcp )
		_tcp->Check();
	if ( _udp )
		_udp->Check();
}

void
NosuchOscManager::UnListen() {
	if ( _tcp )
		_tcp->UnListen();
	if ( _udp )
		_udp->UnListen();
}

int
NosuchOscManager::Listen() {
	int e;
	if ( _tcp ) {
		if ( (e=_tcp->Listen()) != 0 ) {
			if ( e == WSAEADDRINUSE ) {
				NosuchErrorOutput("TCP port/address (%d/%s) is already in use?",_tcp->Port(),_tcp->Host().c_str());
			} else {
				NosuchErrorOutput("Error in _tcp->Listen = %d\n",e);
			}
			return e;
		}
	}
	if ( _udp ) {
		if ( (e=_udp->Listen()) != 0 ) {
			if ( e == WSAEADDRINUSE ) {
				NosuchErrorOutput("UDP port/address (%d/%s) is already in use?",_udp->Port(),_udp->Host().c_str());
			} else {
				NosuchErrorOutput("Error in _udp->Listen = %d\n",e);
			}
			return e;
		}
	}
	return 0;
}
