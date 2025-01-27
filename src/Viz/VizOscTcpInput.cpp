#include "winsock2.h"
#include "osc/OscReceivedElements.h"
#include "VizUtil.h"
#include "VizSocket.h"
#include "VizOscInput.h"
#include "VizOscTCPInput.h"

VizOscTcpInput::VizOscTcpInput(std::string host, int port, VizOscListener* processor) : VizOscInput(processor) {
	DEBUGPRINT2(("VizOscTcpInput constructor"));
	m_oscmsg = new VizSocketMemory(128);
	DEBUGPRINT(("VizOscTcpInput _oscmsg=%lx",(long)m_oscmsg));
	mi_Socket = new VizSocket();
	m_myhost = host;
	m_myport = port;
}

VizOscTcpInput::~VizOscTcpInput() {
	delete m_oscmsg;
	m_oscmsg = NULL;
	delete mi_Socket;
	mi_Socket = NULL;
}

int
VizOscTcpInput::Listen() {
	VizAssert(mi_Socket);
    mi_Socket->Listen(0, m_myport, 0, 0);
    return 0;
}

void
VizOscTcpInput::Check()
{
    VizSocketMemory* recvMem;
    SOCKET  h_Socket;
	VizSocketConnection *h_connection;
    DWORD u32_Event, u32_IP, u32_Read, u32_Sent;
	USHORT u16_Port_source;

	VizAssert(mi_Socket);
    DWORD u32_Err = mi_Socket->ProcessEvents(&u32_Event, &u32_IP,
		&u16_Port_source, &h_Socket, &h_connection,
		&recvMem, &u32_Read,  &u32_Sent);

    if (u32_Err == ERROR_TIMEOUT) // 50 ms interval has elapsed
        return;

    if (u32_Event) // ATTENTION: u32_Event may be == 0 -> do nothing.
    {
        if (u32_Event & FD_READ && recvMem) // recvMem may be NULL if an error occurred!!
        {
			struct in_addr a;
			a.S_un.S_addr = u32_IP;
			std::string source = VizSnprintf("%d@%s",u16_Port_source,inet_ntoa(a));
            ProcessBytes(source.c_str(),recvMem);
        }
    }

    if (u32_Err)
    {
        DEBUGPRINT(("u32_Err = %d\n",u32_Err));
        // mi_Socket->Close() has been called -> don't print this error message
        if (u32_Err == WSAENOTCONN)
            return;

        // An error normally means that the socket has a problem -> abort the loop.
        // A few errors should not abort the processing:
        if (u32_Err != WSAECONNABORTED && // e.g. after the other side was killed in TaskManager
                u32_Err != WSAECONNRESET   && // Connection reset by peer.
                u32_Err != WSAECONNREFUSED && // FD_ACCEPT with already 62 clients connected
                u32_Err != WSAESHUTDOWN)      // Sending data to a socket just in the short timespan
            return;                        //   between shutdown() and closesocket()
    }
}

int
VizOscTcpInput::SlipBoundaries(char *p, int leng, char** pbegin, char** pend)
{
    int bytesleft = leng;
    int found = 0;

    *pbegin = 0;
    *pend = 0;
    // int pn = (*p & 0xff);
    if ( IS_SLIP_END(*p) ) {
        *pbegin = p++;
        bytesleft--;
        found = 1;
    } else {
        // Scan for next unescaped SLIP_END
        p++;
        bytesleft--;
        while ( !found && bytesleft > 0 ) {
            if ( IS_SLIP_END(*p) && ! IS_SLIP_ESC(*(p-1)) ) {
                *pbegin = p;
                found = 1;
            }
            p++;
            bytesleft--;
        }
    }
    if ( ! found ) {
        return 0;
    }
    // We've got the beginning of a message, now look for
    // the end.
    found = 0;
    while ( !found && bytesleft > 0 ) {
        if ( IS_SLIP_END(*p) && ! IS_SLIP_ESC(*(p-1)) ) {
            *pend = p;
            found = 1;
        }
        p++;
        bytesleft--;
    }
    return found;
}

void
VizOscTcpInput::ProcessBytes(const char *source,	VizSocketMemory* buff)
{
    while ( ProcessOneOscMessage(source,buff) ) {
    }
}

int
VizOscTcpInput::ProcessOneOscMessage( const char *source, VizSocketMemory* buff)
{
    char *p = buff->GetBuffer();
    int nbytes = buff->GetLength();
    char* pbegin;
    char* pend;
    if ( SlipBoundaries(p,nbytes,&pbegin,&pend) == 0 ) {
        return 0;
    }
    int oscsize = (int)(pend - pbegin - 1);
    char *oscp = m_oscmsg->GetBuffer();
    if ( m_oscmsg->GetLength() != 0 ) {
        DEBUGPRINT(("HEY, _oscmsg isn't empty!?"));
        m_oscmsg->DeleteLeft(m_oscmsg->GetLength());
    }
    if ( ! IS_SLIP_END(*pbegin) || ! IS_SLIP_END(*pend) ) {
        // This indicates SlipBoundaries isn't doing its job.
        DEBUGPRINT(("HEY! pbegin/pend don't have SLIP_END !??"));
        return 0;
    }
    p = pbegin+1;
    int bytesleft = oscsize;
    while ( bytesleft > 0 ) {
        if ( IS_SLIP_ESC(*p) && bytesleft>1 && IS_SLIP_ESC2(*(p+1)) ) {
            m_oscmsg->Append(p,1);
            p += 2;
            bytesleft -= 2;
        } else if ( IS_SLIP_ESC(*p) && bytesleft>1 && IS_SLIP_END(*(p+1)) ) {
            m_oscmsg->Append(p+1,1);
            p += 2;
            bytesleft -= 2;
        } else {
            m_oscmsg->Append(p,1);
            p += 1;
            bytesleft -= 1;
        }
    }
    buff->DeleteLeft(oscsize+2);

    osc::ReceivedPacket rp( m_oscmsg->GetBuffer(), m_oscmsg->GetLength() );
	ProcessReceivedPacket(source,rp);

    m_oscmsg->DeleteLeft(m_oscmsg->GetLength());
    return 1;
}


void
VizOscTcpInput::UnListen()
{
	VizAssert(mi_Socket);
    mi_Socket->Close();
	mi_Socket = NULL;
}

