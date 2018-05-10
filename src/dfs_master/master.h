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
    uint32_t flowSize;
    uint32_t flowId;
    uint32_t coflowSize;
};
struct Slave {
    uint32_t ip;
    uint16_t port;
    TcpClientCtx *socket;
};
class CMasterServer :
    public CTCPServer
{
public:
    CMasterServer();
    virtual ~CMasterServer();
    bool Start();
    void startCoflowTest(int _scheduleAlgorithm);
    void killSlave();
    void killClient();
    void endCoflowTest();
    void analyseCoflow();

    vector<list<Request>>m_queueRequest;
    vector<Slave>m_vectorLoginSlave;
    vector<TcpClientCtx*>m_vectorLoginUser;
    unordered_set<int>m_setIdleSlaveServer;
    static void scheduleTimer(uv_timer_t *timer);
    std::string m_strMasterIp;
    uint16_t m_nMasterPort;
    uint32_t allocateId = 0;
    uint32_t allocateUserId = 0;
    unordered_map<uint32_t, Coflow>m_mapCoflowStatistic;
protected:
    BEGIN_UV_BIND
        UV_BIND(CLoginMsg::MSG_ID, CLoginMsg)
        UV_BIND(CIdleMsg::MSG_ID, CIdleMsg)
        UV_BIND(CCreateFlowJobMsg::MSG_ID, CCreateFlowJobMsg)
        UV_BIND(CGetServerNumberMsg::MSG_ID,CGetServerNumberMsg)
        //UV_BIND(CEditSchedulerMsg::MSG_ID, CEditSchedulerMsg)
        //UV_BIND(CCheckMasterIdleMsg::MSG_ID, CCheckMasterIdleMsg)
        UV_BIND(CClientLoginMsg::MSG_ID, CClientLoginMsg)
        UV_BIND(CReportFlowStatistic::MSG_ID, CReportFlowStatistic)
        UV_BIND(CEndCoflowTestMsg::MSG_ID, CEndCoflowTestMsg)
    END_UV_BIND(CTCPServer)

    int OnUvMessage(const CLoginMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CIdleMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CCreateFlowJobMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CGetServerNumberMsg &msg, TcpClientCtx *pClient);
    //int OnUvMessage(const CEditSchedulerMsg &msg, TcpClientCtx *pClient);
    //int OnUvMessage(const CCheckMasterIdleMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CClientLoginMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CReportFlowStatistic &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CEndCoflowTestMsg &msg, TcpClientCtx *pClient);
private:
};