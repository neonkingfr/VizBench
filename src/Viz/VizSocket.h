/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _NOSUCH_SOCKET_H
#define _NOSUCH_SOCKET_H

#pragma warning(disable: 4996) // _vsnwprintf is deprecated

// Enable Trace output in DebugView from www.sysinternals.com
#define TRACE_EVENTS  _DEBUG && FALSE  // Trace Socket Events and Request/Lock Timer
#define TRACE_LOCK    _DEBUG && FALSE  // Trace Lock Mutex and LoopEvent

// With this buffer VizSocket reads data from Winsock
// This buffer should have a similar size as the internal Winsock buffer (which has 64 kB)
// There is no reason why you should modify this value! (except for testing)
#define READ_BUFFER_SIZE   64*1024

// With this size VizSocketMemory is initialized.
// If the value is too big you waste memory. (If 62 sockets are open you occupy at least 62 * MEMORY_INITIAL_SIZE Byte)
// If the value is too small there are multiple re-allocations necessary which are slow.
// This value depends on the size of the datablocks that you want to receive (e.g. when using length-prefixed mode)
// This value should be READ_BUFFER_SIZE or more.
#define MEMORY_INITIAL_SIZE  64*1024


// for older Visual Studio versions. (DWORD_PTR is required to compile correctly as 64 Bit)
#ifndef DWORD_PTR
#define DWORD_PTR DWORD
#endif

// This flag is used to notify the caller that the socket has been closed due to a timeout (idle for too long)
// This flag is always signaled in combination with FD_CLOSE
#define FD_TIMEOUT  (1 << 20)

class VizSocketMemory
{
public:
	VizSocketMemory(DWORD u32_InitialSize);
	~VizSocketMemory();
	char* GetBuffer();
	DWORD GetLength();
	void  Append(char* s8_Data, DWORD u32_Count);
	void  DeleteLeft(DWORD u32_Count);

private:
	char*  ms8_Mem;
	DWORD mu32_Size;
	DWORD mu32_Len;
};

class VizSocket;

class VizSocketConnection {
public:
	VizSocketConnection::VizSocketConnection() {
		parent = NULL;
		s8_SendBuf = NULL;
		pi_RecvMem = NULL;
	}
	VizSocketConnection::~VizSocketConnection() {
		if ( pi_RecvMem ) {
			delete pi_RecvMem;
			pi_RecvMem = NULL;
		}
		if ( s8_SendBuf ) {
			delete s8_SendBuf;
			s8_SendBuf = NULL;
		}
	}

	VizSocket *parent;
	SOCKET      h_Socket;
	DWORD       u32_IP;
	USHORT		u16_Port_dest;
	USHORT		u16_Port_source;
	char*		s8_SendBuf;
	DWORD		u32_SendLen;
	DWORD		u32_SendPos;
	LONGLONG	s64_IdleSince;
	BOOL        b_Closed;
	BOOL        b_Timeout;
	BOOL        b_Shutdown;
	VizSocketMemory*   pi_RecvMem;

	// SOCKET _socket;
	std::string m_url;
	std::string m_data;
	std::string _source;
	std::string m_buff_sofar;
	std::string _attempt_to_find_memory_leak;
	// std::string _content_type;
	bool m_collecting_post_data;
	bool m_is_websocket;
	std::string m_websocket_key;
	unsigned int m_content_length;
	int m_request_type;
	void _grab_request(int req, std::string& line);
	bool CollectHttpRequest(const char* p);
	bool CollectHttpHeader(std::string line);
	bool CollectPostData(std::string data);
};

std::string ip_port_source(DWORD u32_IP, USHORT u16_Port_source);

class VizSocket
{
public:
	enum eState
	{
		E_Disconnected = 0,
		E_Connected    = 1,
		E_Server       = 2,
		E_Client       = 4,
	};

	// Template classes must always be declared in the header file.
	// Example: tValue = DWORD, SOCKET, ULONGLONG,...
	template <class tKey, class tValue>
	class cHash
	{
	public:
		cHash(DWORD u32_InitialCount=10) 
			: mi_Keys(u32_InitialCount * sizeof(tKey)),
			  mi_Vals(u32_InitialCount * sizeof(tValue))
		{
		}
		void Append(tKey t_Key, tValue t_Value)
		{ 
			mi_Keys.Append((char*)&t_Key,   sizeof(tKey));
			mi_Vals.Append((char*)&t_Value, sizeof(tValue));
		}
		DWORD GetCount()
		{ 
			return mi_Keys.GetLength() / sizeof(tKey); 
		}
		void Clear()
		{
			mi_Keys.DeleteLeft(0xFFFFFFFF);
			mi_Vals.DeleteLeft(0xFFFFFFFF);
		}
		tKey GetKeyByIndex(DWORD u32_Index)
		{
			if (u32_Index >= GetCount()) return NULL;
			tKey* p_Key = (tKey*)mi_Keys.GetBuffer();
			return p_Key[u32_Index]; 
		}
		tValue GetValueByIndex(DWORD u32_Index)
		{
			if (u32_Index >= GetCount()) return NULL;
			tValue* p_Val = (tValue*)mi_Vals.GetBuffer();
			return p_Val[u32_Index]; 
		}
		tValue GetValueByKey(tKey t_Key)
		{
			tKey*   p_Key = (tKey*)   mi_Keys.GetBuffer();
			tValue* p_Val = (tValue*) mi_Vals.GetBuffer();
			for (DWORD i=0; i<GetCount(); i++)
			{
				if (p_Key[i] == t_Key) return p_Val[i]; 
			}
			return NULL;
		}

	private:
		VizSocketMemory mi_Keys;
		VizSocketMemory mi_Vals;
	};

protected:
	class cList
	{
	public:
		 cList();
		~cList();
		void     RemoveAll();
		void     RemoveClosed();
		VizSocketConnection*   Add(SOCKET h_Sock, HANDLE h_Event);
		BOOL     Remove(DWORD u32_Index);
		int      FindSocket(SOCKET h_Socket);

		DWORD    mu32_Count;
		eState   me_State;

		// The first event is used for the lock. It is not associated with a connection.
		HANDLE  mh_Events[WSA_MAXIMUM_WAIT_EVENTS];
		VizSocketConnection   mk_Data  [WSA_MAXIMUM_WAIT_EVENTS-1];
	};

	struct kLock
	{
	public:
		 kLock();
		~kLock();
		DWORD  Init();

		HANDLE h_Mutex;
		HANDLE h_ExitTimer;  // set to escape from WSAWaitForMultipleEvents
		HANDLE h_LoopEvent;  // blocks the endless loop ProcessEvents()
	};

	class cLock
	{
	public:
		 cLock();
		~cLock();
		DWORD Request(kLock* pk_Lock);
		DWORD Loop   (kLock* pk_Lock);

	private:
		HANDLE mh_Mutex;
	};

public:
	 VizSocket();
	~VizSocket();

	DWORD  Close();
	DWORD  GetSocketCount();
	DWORD  GetAllConnectedSockets(cHash<SOCKET,DWORD>* pi_SockList);
	DWORD  Listen   (DWORD u32_BindIP, USHORT u16_Port, DWORD u32_EventTimeout, DWORD u32_MaxIdleTime=0);
	DWORD  ConnectTo(DWORD u32_ServIP, USHORT u16_Port, DWORD u32_EventTimeout, DWORD u32_MaxIdleTime=0);
	DWORD  DisconnectClient(SOCKET h_Socket);
	DWORD  ProcessEvents(DWORD* pu32_Events, DWORD* pu32_IP,
		USHORT* pu16_Port_source, SOCKET* ph_Socket, VizSocketConnection** ph_connection, VizSocketMemory** ppi_RecvMem, DWORD* pu32_Read, DWORD* pu32_Sent);
	DWORD  SendTo(SOCKET h_Socket, char* s8_SendBuf, DWORD u32_Len);
	DWORD  GetLocalIPs(cHash<DWORD,DWORD>* pi_IpList);
	eState GetState();
	void   FormatEvents(DWORD u32_Events, char* s8_Buf);

	static void TraceA(const char* s8_Format, ...);
	
protected:
	DWORD    CreateSocket();
	DWORD    Initialize();
	DWORD    SendDataBlock(SOCKET h_Socket, char* s8_Buf, DWORD* pu32_Pos, DWORD u32_Len);
	DWORD    WSAWaitForMultipleEventsEx(DWORD u32_Count, DWORD* pu32_Index, WSAEVENT* ph_Events, DWORD u32_Timeout);
#if 0
	DWORD    ProcessIdleSockets(char* s8_Caller);
#endif
	LONGLONG GetTickCount64();
	
	static int CALLBACK AcceptCondition(LPWSABUF pk_CallerId, LPWSABUF pk_CallerData, LPQOS pk_SQOS, LPQOS pk_GQOS, 
										 LPWSABUF pk_CalleeId, LPWSABUF pk_CalleeData, GROUP FAR* pu32_Group, DWORD_PTR p_Param);

	BOOL       mb_Initialized;
	DWORD    mu32_WaitIndex;
public:
	cList      mi_List;
protected:
	kLock      mk_Lock;
	char*     ms8_ReadBuffer;
	LONGLONG ms64_MaxIdleTime;
	DWORD    mu32_EventTimeout; // UNSIGNED!!
	DWORD    mu32_Tick64Lo;     // UNSIGNED!!
	DWORD    mu32_Tick64Hi;
};

int  CALLBACK AcceptCondition(LPWSABUF pk_CallerId, LPWSABUF pk_CallerData, LPQOS pk_SQOS, LPQOS pk_GQOS, LPWSABUF pk_CalleeId, LPWSABUF pk_CalleeData, GROUP FAR * pu32_Group, DWORD_PTR p_Param);

#endif
