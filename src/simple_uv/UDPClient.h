#pragma once
#include "UDPHandle.h"

class SUV_EXPORT CUDPClient : public CUDPHandle
{
public:
	CUDPClient(uv_loop_t* pLoop);
	virtual ~CUDPClient();
	bool  Connect(const char *ip, int port);//Start the client, ipv4

protected:
	sockaddr_in m_serverAddr;
	bool  send(const char*msg, const unsigned int msg_len);
};
