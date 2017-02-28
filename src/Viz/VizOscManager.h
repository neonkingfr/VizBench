/*
 *  Created by Tim Thompson on 1/31/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef NOSUCH_OSC_MANAGER_H
#define NOSUCH_OSC_MANAGER_H

#include "VizOsc.h"

class VizOscTcpInput;
class VizOscUdpInput;
class Vizlet;

class VizOscManager {

public:
	VizOscManager(VizOscListener* server, std::string host, int port);
	virtual ~VizOscManager();
	void Check();
	int Listen();
	void UnListen();
	void processOsc(const char *source, const osc::ReceivedMessage& m);
	void Shutdown();
	void SetShouldBeShutdown(bool);
	bool ShouldBeShutdown();

private:
	VizOscTcpInput* m_tcp;
	VizOscUdpInput* m_udp;
	bool m_shouldbeshutdown;
	bool m_shutdowncomplete;
};

#endif
