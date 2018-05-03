#pragma once
#include <simple_uv/tcpclient.h>
#include "../message.h"

class CSlaveClient :public CTCPClient{
public:
    CSlaveClient();
    virtual ~CSlaveClient();
    uint32_t id;
protected:
    BEGIN_UV_BIND
        UV_BIND(CLoginRspMsg::MSG_ID, CLoginRspMsg)
        UV_BIND(CKillSlaveMsg::MSG_ID, CKillSlaveMsg)
    END_UV_BIND(CTCPClient)

    int OnUvMessage(const CLoginRspMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CKillSlaveMsg &msg, TcpClientCtx *pClient);
};