#include "NosuchUtil.h"
#include "NosuchOscInput.h"
#include "NosuchOscUdpInput.h"

int
OscSocketError(char *s)
{
    int e = WSAGetLastError();
    DEBUGPRINT(("NSosc socket error: %s e=%d",s,e));
    return e;
}

NosuchOscUdpInput::NosuchOscUdpInput(std::string host, int port, NosuchOscListener* processor) : NosuchOscInput(processor) {
	DEBUGPRINT2(("NosuchOscUdpInput constructor"));
	_s = INVALID_SOCKET;
	_myhost = host;
	_myport = port;
}

NosuchOscUdpInput::~NosuchOscUdpInput() {
	DEBUGPRINT2(("NosuchOscUdpInput destructor"));
	if ( _s != INVALID_SOCKET ) {
		DEBUGPRINT(("HEY!  _info._s is still set in NSosc destructor!?"));
	}
}

int
NosuchOscUdpInput::Listen() {

    struct sockaddr_in sin;
    struct sockaddr_in sin2;
    int sin2_len = sizeof(sin2);

    DWORD nbio = 1;
    PHOSTENT phe;

	SOCKET s = socket(PF_INET, SOCK_DGRAM, 0);
    if ( s < 0 ) {
        DEBUGPRINT(("_openListener error 1"));
        return OscSocketError("unable to create socket");
    }
    sin.sin_family = AF_INET;
    // sin.sin_addr.s_addr = INADDR_ANY;

	if ( _myhost != "*" && _myhost != "" ) {
	    phe = gethostbyname(_myhost.c_str());
	    if (phe == NULL) {
	        return OscSocketError("unable to get hostname");
	    }
	    memcpy((struct sockaddr FAR *) &(sin.sin_addr),
	           *(char **)phe->h_addr_list, phe->h_length);
	    sin.sin_port = htons(_myport);
	} else {
		// Listen on all ip addresses
	    sin.sin_port = htons(_myport);
		sin.sin_addr.S_un.S_addr = INADDR_ANY;
	}

    if (  ioctlsocket(s,FIONBIO,&nbio) < 0 ) {
        DEBUGPRINT(("_openListener error 2"));
        return OscSocketError("unable to set socket to non-blocking");
    }
    if (bind(s, (LPSOCKADDR)&sin, sizeof (sin)) < 0) {
        int e = WSAGetLastError();
        DEBUGPRINT(("NSosc socket bind error: host=%s port=%d e=%d",_myhost.c_str(),_myport,e));
        return e;
        // return OscSocketError("unable to bind socket");
    }
    if ( getsockname(s,(LPSOCKADDR)&sin2, &sin2_len) != 0 ) {
        return OscSocketError("unable to getsockname after bind");
    }
    // *myport = ntohs(sin2.sin_port);
    DEBUGPRINT(("LISTENING for OSC on UDP port %d@%s",_myport,_myhost.c_str()));
    _s = s;
    return 0;
}

void
NosuchOscUdpInput::Check()
{
	if ( _s == INVALID_SOCKET )
		return;

    struct sockaddr_in sin;
    int sin_len = sizeof(sin);
    char buf[8096];

	long tm0 = timeGetTime();
	int toomany = 50;
	unsigned long toolong = tm0 + 200;   // Stop processing if it takes longer than this
    for ( int cnt=0; cnt<toomany; cnt++ ) {
		unsigned long tm = timeGetTime();
		if ( tm >= toolong ) {
			// DEBUGPRINT(("OSC processing taking too long, Check returning early, cnt=%d  tm0=%ld now=%ld\n",cnt,tm0,tm));
			break;
		}
        int i = recvfrom(_s,buf,sizeof(buf),0,(LPSOCKADDR)&sin, &sin_len);
        if ( i <= 0 ) {
            int e = WSAGetLastError();
			switch (e) {
			case WSAENOTSOCK:
				DEBUGPRINT(("NosuchOscUdpInput::Check e==WSAENOTSOCK"));
				_s = INVALID_SOCKET;
				break;
			case WSAEWOULDBLOCK:
				break;
			default:
                DEBUGPRINT(("Hmmm, B e=%d isn't EWOULDBLOCK or WSAENOTSOCK!?",e));
				break;
            }
            return;
        }
        osc::ReceivedPacket p( buf, i );
		std::string source = NosuchSnprintf("%d@%s",sin.sin_port,inet_ntoa(sin.sin_addr));
		ProcessReceivedPacket(source.c_str(),p);
    }
}

void
NosuchOscUdpInput::UnListen()
{
    DEBUGPRINT(("_oscUnlisten( _myport=%d)", _myport));
    closesocket(_s);
    _s = INVALID_SOCKET;
}

