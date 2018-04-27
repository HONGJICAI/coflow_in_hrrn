#pragma once
#include <unordered_set>
#include <unordered_map>
#include <simple_uv/tcpclient.h>
#include "../message.h"

struct Flow {
    int targetServer;
    int flowSize;
    uint64_t SJFEndTime;
    uint64_t HRRNEndTime;
    int flowId;
};
struct Coflow {
    unordered_map<int,Flow>flows;
    uint64_t SJFCreateTime;
    uint64_t SJFEndTime;
    uint64_t HRRNCreateTime;
    uint64_t HRRNEndTime;
    int finifshedFlowNum;
};

class CClient :public CTCPClient {
public:
    CClient();
    virtual ~CClient();
    int serverNumber = -1;
    void startCoflowTest();
    static void AfterFlowEnd(uv_async_t *async);
    Coflow generateCoflow();
    static void analyseCoflow(Coflow &coflow);
    uv_async_t m_asyncHandle;
    CUVMutex m_mutex;
    void pushEndedFlowId(int flowId,uint64_t endTime);
    vector<pair<int,uint64_t>>m_vectorEndedFlowId;
    Coflow m_coflow;
    bool SJFOver = false;
    bool HRRNOver = false;
    bool masterIdle = false;
    void checkMasterIdle();
    void editMasterScheduler(bool hrrn);
protected:
    BEGIN_UV_BIND
        UV_BIND(CGetServerNumberMsg::MSG_ID, CGetServerNumberMsg)
        UV_BIND(CStartFlowRequestMsg::MSG_ID, CStartFlowRequestMsg)
        UV_BIND(CCheckMasterIdleMsg::MSG_ID, CCheckMasterIdleMsg)
        UV_BIND(CEditSchedulerMsg::MSG_ID, CEditSchedulerMsg)
    END_UV_BIND(CTCPClient)

    int OnUvMessage(const CGetServerNumberMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CStartFlowRequestMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CCheckMasterIdleMsg &msg, TcpClientCtx *pClient);
    int OnUvMessage(const CEditSchedulerMsg &msg, TcpClientCtx *pClient);
};