#ifndef _VizOscTcpInput
#define _VizOscTcpInput

#include "VizOscInput.h"

class VizSocket;
class VizSocketMemory;

#define SLIP_END 192
#define SLIP_ESC 219
#define SLIP_ESC2 221

#define IS_SLIP_END(c) (((c)&0xff)==SLIP_END)
#define IS_SLIP_ESC(c) (((c)&0xff)==SLIP_ESC)
#define IS_SLIP_ESC2(c) (((c)&0xff)==SLIP_ESC2)

class VizOscTcpInput : public VizOscInput {

public:
	VizOscTcpInput(std::string host, int port, VizOscListener* processor);
	virtual ~VizOscTcpInput();
	int Listen();
	void Check();
	void UnListen();
	std::string Host() { return m_myhost; }
	int Port() { return m_myport; }

private:
	void ProcessBytes(const char *source, VizSocketMemory* buff);
	int ProcessOneOscMessage( const char *source, VizSocketMemory* buff);
	int SlipBoundaries(char *p, int leng, char** pbegin, char** pend);
	VizSocketMemory* m_oscmsg;
	VizSocket* mi_Socket;
	int m_myport;
	std::string m_myhost;
};

#endif