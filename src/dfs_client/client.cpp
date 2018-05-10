#include <random>
#include <unordered_map>
#include "client_slave.h"
#include "client.h"
#include "simple_uv/common.h"

int scheduleAlgorithm;
CClient::CClient()
{
    uv_async_init(this->GetLoop(), &m_asyncHandle, AfterFlowEnd);
    m_asyncHandle.data = this;
}

CClient::~CClient()
{
}

void CClient::startCoflowTest()
{
    string str;
    switch (scheduleAlgorithm) {
    case ScheduleAlgorithm::HRRN:str = "HRRN"; break;
    case ScheduleAlgorithm::SJF:str = "SJF"; break;
    case ScheduleAlgorithm::SCF:str = "SCF"; break;
    case ScheduleAlgorithm::CHRRN:str = "CHRRN"; break;
    }
    printf("start %s coflow test\n",str.c_str());
    m_coflow.createTime[scheduleAlgorithm] = uv_now(this->GetLoop());
    for (auto &flow : m_coflow.flows) {
        CCreateFlowJobMsg msg;
        msg.flowSize = flow.second.flowSize;
        msg.targetServerId = flow.second.targetServer;
        msg.flowId = flow.first;
        msg.coflowSize = m_coflow.coflowSize;
        this->SendUvMessage(msg, msg.MSG_ID);
    }
}
void CClient::AfterFlowEnd(uv_async_t * async)
{
    CClient* client = reinterpret_cast<CClient*>(async->data);
    CUVAutoLock lock(&client->m_mutex);
    for (auto &element : client->m_vectorEndedFlowId) {
        client->m_coflow.flows[element.first].endTime[scheduleAlgorithm] = element.second;
        client->hashmap.erase(element.first);
    }
    client->finifshedFlowNum += client->m_vectorEndedFlowId.size();
    if (client->finifshedFlowNum == client->m_coflow.flows.size()) {
        printf("coflow finished\n");
        client->SendUvMessage(CEndCoflowTestMsg{}, CEndCoflowTestMsg::MSG_ID);
        client->finifshedFlowNum = 0;
    }
    client->m_vectorEndedFlowId.clear();
}
Coflow CClient::generateCoflow()//生成coflow
{
    while (this->serverNumber == -1)//获取到后结束循环
        uv_thread_sleep(1);
    const static uint32_t flowSize[] = { 100,1000,5000,10000 };
    std::random_device rd;
    std::mt19937 gen(rd());
    uniform_int_distribution<uint32_t> u1(0, this->serverNumber - 1);//随机的目标服务器
    uniform_int_distribution<uint32_t> u2(0, sizeof(flowSize) / sizeof(int) - 1); //随机的flowSize
    uniform_int_distribution<uint32_t> u3(0, 0xffffffff); //随机的flowId
    static std::unordered_set<uint32_t>usedFlowId;
    uint32_t job = 5 * u1(gen) + 10;//随机个数的flow
    uint32_t id;
    Coflow res;
    for (int i = 0; i < job; ++i) {//随机分配在服务器中
        do {
            id = u3(gen);
        } while (usedFlowId.find(id) != usedFlowId.end());
        usedFlowId.insert(id);
        auto retPair = res.flows.insert(make_pair(id, Flow{ u1(gen),flowSize[u2(gen)] }));
        res.coflowSize += retPair.first->second.flowSize;
    }
    finifshedFlowNum = 0;
    return res;
}

void CClient::pushEndedFlowId(int flowId,uint64_t endTime)
{
    CUVAutoLock lock(&m_mutex);
    m_vectorEndedFlowId.push_back(make_pair(flowId,endTime));
    uv_async_send(&m_asyncHandle);
}

int CClient::OnUvMessage(const CStartFlowRequestMsg & msg, TcpClientCtx * pClient)
{
    hashmap[msg.flowId].setAsyncHandle(&m_asyncHandle);
    hashmap[msg.flowId].getFlow(ip_int2string(msg.ip),htons(msg.port),msg.flowSize,msg.flowId);
    m_coflow.flows[msg.flowId].startTime[scheduleAlgorithm] = uv_now(GetLoop());
    return 0;
}

int CClient::OnUvMessage(const CStartCoflowTestMsg & msg, TcpClientCtx * pClient)
{
    scheduleAlgorithm = msg.scheduleAlgorithm;
    static bool once = true;
    if (once) {
        m_coflow = generateCoflow();
        once = false;
    }
    startCoflowTest();
    return 0;
}

int CClient::OnUvMessage(const CClientLoginMsg & msg, TcpClientCtx * pClient)
{
    clientId = msg.userId;
    return 0;
}
#include <stdlib.h>
int CClient::OnUvMessage(const CKillClientMsg & msg, TcpClientCtx * pClient)
{
    ::exit(0);
    return 0;
}

int CClient::OnUvMessage(const CEndCoflowTestMsg & msg, TcpClientCtx * pClient)
{
    CReportFlowStatistic statistic;
    statistic.userId = this->clientId;
    statistic.HRRNCreateTime = m_coflow.createTime[ScheduleAlgorithm::HRRN];
    statistic.SJFCreateTime = m_coflow.createTime[ScheduleAlgorithm::SJF];
    statistic.CHRRNCreateTime = m_coflow.createTime[ScheduleAlgorithm::CHRRN];
    statistic.SCFCreateTime = m_coflow.createTime[ScheduleAlgorithm::SCF];
    for (auto &element : m_coflow.flows) {
        statistic.flowSize = element.second.flowSize;
        statistic.flowId = element.first;
        statistic.HRRNStartTime = element.second.startTime[ScheduleAlgorithm::HRRN];
        statistic.SJFStartTime = element.second.startTime[ScheduleAlgorithm::SJF];
        statistic.CHRRNStartTime = element.second.startTime[ScheduleAlgorithm::CHRRN];
        statistic.SCFStartTime = element.second.startTime[ScheduleAlgorithm::SCF];
        statistic.HRRNEndTime = element.second.endTime[ScheduleAlgorithm::HRRN];
        statistic.SJFEndTime = element.second.endTime[ScheduleAlgorithm::SJF];
        statistic.CHRRNEndTime = element.second.endTime[ScheduleAlgorithm::CHRRN];
        statistic.SCFEndTime = element.second.endTime[ScheduleAlgorithm::SCF];
        SendUvMessage(statistic, statistic.MSG_ID);
    }
    SendUvMessage(CEndCoflowTestMsg{}, CEndCoflowTestMsg::MSG_ID);
    return 0;
}

int CClient::OnUvMessage(const CGetServerNumberMsg & msg, TcpClientCtx * pClient)
{
    serverNumber = msg.number;
    return 0;
}
#include <iostream>
int main() {
    CClient client;
    if (client.Connect("127.0.0.1", 10000) == false) {
        printf("error on connect");
        return -1;
    }
    client.SendUvMessage(CClientLoginMsg{}, CClientLoginMsg::MSG_ID);
    client.SendUvMessage(CGetServerNumberMsg{}, CGetServerNumberMsg::MSG_ID);//获取服务器总数

    while (1)
        uv_thread_sleep(10);
}