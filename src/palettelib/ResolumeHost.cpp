#include "NosuchUtil.h"
#include "FreeFrameHost.h"
#include "ResolumeHost.h"

#if 0
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <strstream>
#include <cstdlib> // for srand, rand
#include <ctime>   // for time
#endif

ResolumeHost::ResolumeHost()
{
	NosuchDebug(1, "=== ResolumeHost is being constructed.");

	_resolume_output_host = "127.0.0.1";  // This should always be the case
	_resolume_output_port = DEFAULT_RESOLUME_PORT;

	_pyffle_output_host = "127.0.0.1";  // This should always be the case
	_pyffle_output_port = DEFAULT_PYFFLE_PORT;
}

ResolumeHost::~ResolumeHost()
{
	NosuchDebug(1, "ResolumeHost destructor called");
}

int ResolumeHost::NumEffectSet() {
	return 12;
}

int
ResolumeHost::EnableEffect(int effectnum, bool enabled)
{
	char buffer[1024];
	osc::OutboundPacketStream p(buffer, sizeof(buffer));
	// The effectnum internally is 0-12 (or whatever the number of effects is)
	// but Resolume knows them as effect 2-14 (effect 1 is the VizPalette plugin)
	effectnum += 2;
	std::string addr = NosuchSnprintf("/activeclip/video/effect%d/bypassed", effectnum);
	int bypassed = enabled ? 0 : 1;
	p << osc::BeginMessage(addr.c_str()) << bypassed << osc::EndMessage;
	return SendToResolume(p);
}

int ResolumeHost::SendToResolume(osc::OutboundPacketStream& p) {
	NosuchDebug(1, "SendToResolume host=%s port=%d", _resolume_output_host, _resolume_output_port);
	return SendToUDPServer(_resolume_output_host, _resolume_output_port, p.Data(), (int)p.Size());
}

void
ResolumeHost::ShowChoice(std::string bn, std::string text, int x, int y, int timeout) {
	char buffer[1024];
	osc::OutboundPacketStream p(buffer, sizeof(buffer));
	// p << osc::BeginMessage( "/set_text" ) << text.c_str() << osc::EndMessage;
	p << osc::BeginMessage("/set_choice") << text.c_str() << bn.c_str() << osc::EndMessage;
	SendToUDPServer(_pyffle_output_host, _pyffle_output_port, p.Data(), (int)p.Size());
	p.Clear();
	p << osc::BeginMessage("/set_pos") << x << y << osc::EndMessage;
	SendToUDPServer(_pyffle_output_host, _pyffle_output_port, p.Data(), (int)p.Size());
}

void
ResolumeHost::ShowAttract(int onoff) {
	char buffer[1024];
	osc::OutboundPacketStream p(buffer, sizeof(buffer));
	p << osc::BeginMessage("/attract") << (onoff ? 1 : 0) << osc::EndMessage;
	SendToUDPServer(_pyffle_output_host, _pyffle_output_port, p.Data(), (int)p.Size());
	p.Clear();
}