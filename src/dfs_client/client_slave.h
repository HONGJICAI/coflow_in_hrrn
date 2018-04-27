#pragma once
#include <unordered_set>
#include <string>
#include <simple_uv/tcpclient.h>
#include "../message.h"


class CClientSlave :public CTCPClient {
public:
    CClientSlave();
    virtual ~CClientSlave();
    void getFlow(std::string , int port, int flowSize, int flowId);
    void setAsyncHandle(uv_async_t *async);
protected:
    uv_async_t *m_asyncHandle;
    int m_nFlowId;
    BEGIN_UV_BIND
        UV_BIND(CPushFlowMsg::MSG_ID, CPushFlowMsg)
    END_UV_BIND(CTCPClient)

    int OnUvMessage(const CPushFlowMsg &msg, TcpClientCtx *pClient);
};