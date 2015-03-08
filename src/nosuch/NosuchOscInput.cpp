#include "NosuchUtil.h"
#include "NosuchOscInput.h"

#include "winerror.h"
#include "winsock.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscReceivedElements.h"

#include <iostream>
#include <fstream>
using namespace std;

NosuchOscInput::NosuchOscInput(NosuchOscListener* p) {

    // _enabled = 0;
	DEBUGPRINT2(("NosuchOscInput constructor"));
	m_processor = p;
}

NosuchOscInput::~NosuchOscInput() {
    DEBUGPRINT2(("NosuchOscInput destructor"));
}

void
NosuchOscInput::processOscBundle( const char *source, const osc::ReceivedBundle& b )
{
    // ignore bundle time tag for now

    for( osc::ReceivedBundle::const_iterator i = b.ElementsBegin();
		i != b.ElementsEnd();
		++i ) {

		if( i->IsBundle() ) {
            processOscBundle( source, osc::ReceivedBundle(*i) );
		} else {
	        m_processor->processOsc( source, osc::ReceivedMessage(*i) );
		}
    }
}

int
SendToUDPServer(char *serverhost, int serverport, const char *data, int leng)
{
    SOCKET s;
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);
    int i;
    DWORD nbio = 1;
    PHOSTENT phe;

    phe = gethostbyname(serverhost);
    if (phe == NULL) {
        DEBUGPRINT(("SendToUDPServer: gethostbyname(localhost) fails?"));
        return 1;
    }
    s = socket(PF_INET, SOCK_DGRAM, 0);
    if ( s < 0 ) {
        DEBUGPRINT(("SendToUDPServer: unable to create socket!?"));
        return 1;
    }
    sin.sin_family = AF_INET;
    memcpy((struct sockaddr FAR *) &(sin.sin_addr),
           *(char **)phe->h_addr_list, phe->h_length);
    sin.sin_port = htons(serverport);

    i = sendto(s,data,leng,0,(LPSOCKADDR)&sin,sin_len);

    closesocket(s);
    return 0;
}

int
RegisterWithAServer(char *serverhost, int serverport, char *myhost, int myport)
{
    char buffer[1024];
    osc::OutboundPacketStream p( buffer, sizeof(buffer) );
    p << osc::BeginMessage( "/registerclient" )
      << "localhost" << myport << osc::EndMessage;
    return SendToUDPServer(serverhost,serverport,p.Data(),(int)p.Size());
}

int
UnRegisterWithAServer(char *serverhost, int serverport, char *myhost, int myport)
{
    char buffer[1024];
    osc::OutboundPacketStream p( buffer, sizeof(buffer) );
    p << osc::BeginMessage( "/unregisterclient" )
      << "localhost" << myport << osc::EndMessage;
    return SendToUDPServer(serverhost,serverport,p.Data(),(int)p.Size());
}


