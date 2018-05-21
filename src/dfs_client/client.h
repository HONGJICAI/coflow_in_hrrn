#pragma once
#include <unordered_set>
#include <unordered_map>
#include <simple_uv/tcpclient.h>
#include "../message.h"


class CClient :public CTCPClient {
public:
    CClient();
    virtual ~CClient();
    int serverNumber = -1;
    uint32_t clientId;
    void startCoflowTest();
    static void AfterFlowEnd(uv_async_t *async);
    Coflow generateCoflow();
    uv_async_t m_asyncHandle;
    CUVMutex m_mutex;
    void pushEndedFlowId(uint32_t flowId,uint64_t endTime);
    vector<pair<uint32_t,uint64_t>>m_vectorEndedFlowId;
    Coflow m_coflow;
    unordered_map<uint32_t, CClientSlave>hashmap;
    uint32_t finifshedFlowNum=0;
protected:
    BEGIN_UV_BIND
        UV_BIND(CGetServerNumberMsg::MSG_ID, CGetServerNumberMsg)
        UV_BIND(CStartFlowRequestMsg::MSG_ID, CStartFlowRequestMsg)
        UV_BIND(CStartCoflowTestMsg::MSG_ID, CStartCoflowTestMsg)
        UV_BIND(CClientLoginMsg::MSG_ID, CClientLoginMsg)
        UV_BIND(CKillClientMsg::MSG_ID, CKillClientMsg)
        UV_BIND(CEndCoflowTestMsg::MSG_ID, CEndCoflowTestMsg)
    END_UV_BIND(CTCPClient)

    int OnUvMessage(const CGetServerNumberMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CStartFlowRequestMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CStartCoflowTestMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CClientLoginMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CKillClientMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CEndCoflowTestMsg &msg, TcpClientCtx *pClient);
};