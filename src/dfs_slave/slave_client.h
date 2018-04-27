#pragma once
#include <simple_uv/tcpclient.h>
#include "../message.h"

class CSlaveClient :public CTCPClient{
public:
    CSlaveClient();
    virtual ~CSlaveClient();
    int id;
protected:
    BEGIN_UV_BIND
        UV_BIND(CLoginRspMsg::MSG_ID, CLoginRspMsg)
    END_UV_BIND(CTCPClient)

    int OnUvMessage(const CLoginRspMsg &msg, TcpClientCtx *pClient);
};