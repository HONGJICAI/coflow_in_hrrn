#pragma once
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <list>
#include <simple_uv/tcpserver.h>
#include "../message.h"

struct Request {
    TcpClientCtx *client;
    uint64_t requestTime;
    int flowSize;
    int flowId;
};
class CMasterServer :
    public CTCPServer
{
public:
    CMasterServer();
    virtual ~CMasterServer();
    void LoadConfig();
    bool Start();

    vector<list<Request>>m_queueRequest;
    vector<pair<uint32_t, uint16_t>>m_vectorLoginSlave;
    unordered_set<int>m_setIdleSlaveServer;
    static void scheduleTimer(uv_timer_t *timer);
    std::string m_strMasterIp;
    int m_nMasterPort;
    int allocateId = 0;
protected:
    BEGIN_UV_BIND
        UV_BIND(CLoginMsg::MSG_ID, CLoginMsg)
        UV_BIND(CIdleMsg::MSG_ID, CIdleMsg)
        UV_BIND(CCreateFlowJobMsg::MSG_ID, CCreateFlowJobMsg)
        UV_BIND(CGetServerNumberMsg::MSG_ID,CGetServerNumberMsg)
        UV_BIND(CEditSchedulerMsg::MSG_ID, CEditSchedulerMsg)
        UV_BIND(CCheckMasterIdleMsg::MSG_ID, CCheckMasterIdleMsg)
    END_UV_BIND(CTCPServer)

    int OnUvMessage(const CLoginMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CIdleMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CCreateFlowJobMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CGetServerNumberMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CEditSchedulerMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CCheckMasterIdleMsg &msg, TcpClientCtx *pClient);
private:
};