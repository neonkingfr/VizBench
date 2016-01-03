/*
 *  Created by Tim Thompson on 1/31/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef NOSUCH_OSC_MANAGER_H
#define NOSUCH_OSC_MANAGER_H

#include "NosuchOsc.h"

class NosuchOscTcpInput;
class NosuchOscUdpInput;
class Vizlet;

class NosuchOscManager {

public:
	NosuchOscManager(NosuchOscListener* server, std::string host, int port);
	virtual ~NosuchOscManager();
	void Check();
	int Listen();
	void UnListen();
	void processOsc(const char *source, const osc::ReceivedMessage& m);
	void Shutdown();
	void SetShouldBeShutdown(bool);
	bool ShouldBeShutdown();

private:
	NosuchOscTcpInput* m_tcp;
	NosuchOscUdpInput* m_udp;
	bool m_shouldbeshutdown;
	bool m_shutdowncomplete;
};

#endif
