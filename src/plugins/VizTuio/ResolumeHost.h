#ifndef _RESOLUMEHOST_H
#define _RESOLUMEHOST_H

#include <string>
#include "osc/OscOutboundPacketStream.h"

#define DEFAULT_RESOLUME_PORT 7000
#define DEFAULT_PYFFLE_PORT 9876

class ResolumeHost : public FreeFrameHost {

public:
	ResolumeHost();
	~ResolumeHost();

	int NumEffectSet();
	int EnableEffect(int effectnum, bool enabled);
	void ShowChoice(std::string bn, std::string text, int x, int y, int timeout);
	void ShowAttract(int onoff);

private:
	int _resolume_output_port;  // This is the port we're sending output TO
	std::string _resolume_output_host;
	int _pyffle_output_port;  // This is the port we're sending output TO
	std::string _pyffle_output_host;

	int SendToResolume(osc::OutboundPacketStream& p);
};

#endif