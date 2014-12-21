#include "Socket.h"

TCP::cSocket* http_Socket = 0;
void NS_debug(char *fmt, ...);

void
http_init(int port, int timeout)
{
    http_Socket = new TCP::cSocket();
    http_Socket->Listen(0, port, 0, timeout);
}

void
http_send(SOCKET h_Socket, char *s)
{
    if ( http_Socket == 0 ) {
        NS_debug("Hey, http_Socket is 0? http_send of s=%s\n",s);
        return;
    }
    int e = http_Socket->SendTo(h_Socket, s, strlen(s));
    if ( e != 0 ) {
        NS_debug("sendto returned non-zero value: %d\n",e);
    }
}

void
http_check()
{
    TCP::cSocket::cMemory* pi_RecvMem;
    SOCKET  h_Socket;
    DWORD u32_Event, u32_IP, u32_Read, u32_Sent;

    DWORD u32_Err = http_Socket->ProcessEvents(&u32_Event, &u32_IP, &h_Socket, &pi_RecvMem, &u32_Read,  &u32_Sent);

    if (u32_Err == ERROR_TIMEOUT) // 50 ms interval has elapsed
        return;

    if (u32_Event) // ATTENTION: u32_Event may be == 0 -> do nothing.
    {
        NS_debug("u32_Event = %d\n",u32_Event);
        if (u32_Event & FD_CONNECT) {
            NS_debug("GOT CONNECT in http_check! socket=%ld\n",h_Socket);
        }
        if (u32_Event & FD_READ && pi_RecvMem) // pi_RecvMem may be NULL if an error occurred!!
        {
            NS_debug("SHOULD BE PROCESSING http data!  read=%d\n",u32_Read);
            char z[1] = "";
            pi_RecvMem->Append(z,1);
            char *p = pi_RecvMem->GetBuffer();
            NS_debug("data=%s\n",p);
            http_send(h_Socket,"HTTP/1.1 200 OK\n\
Content-Type: text/html; charset=UTF-8\n\
\n\
<html><body><h1>Hello Three</h1> <b>bold</b></body></html>\r\n");
            http_Socket->DisconnectClient(h_Socket);
        }
    }

    if (u32_Err)
    {
        NS_debug("u32_Err = %d  time=%ld\n",u32_Err,timeGetTime());

        // mi_Socket.Close() has been called -> don't print this error message
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

