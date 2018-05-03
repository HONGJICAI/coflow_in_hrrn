#include <random>
#include <unordered_map>
#include "client_slave.h"
#include "client.h"
#include "simple_uv/common.h"

bool HRRN;
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
    printf("start %s coflow test\n",HRRN?"HRRN":"SJF");
    if (HRRN)
        m_coflow.HRRNCreateTime = uv_now(this->GetLoop());
    else
        m_coflow.SJFCreateTime = uv_now(this->GetLoop());
    for (auto &flow : m_coflow.flows) {
        CCreateFlowJobMsg msg;
        msg.flowSize = flow.second.flowSize;
        msg.targetServerId = flow.second.targetServer;
        msg.flowId = flow.first;
        this->SendUvMessage(msg, msg.MSG_ID);
    }
}
void CClient::AfterFlowEnd(uv_async_t * async)
{
    CClient* client = reinterpret_cast<CClient*>(async->data);
    CUVAutoLock lock(&client->m_mutex);
    for (auto &element : client->m_vectorEndedFlowId) {
        if (HRRN)
            client->m_coflow.flows[element.first].HRRNEndTime = element.second;
        else
            client->m_coflow.flows[element.first].SJFEndTime = element.second;
        client->hashmap.erase(element.first);
    }
    client->m_coflow.finifshedFlowNum += client->m_vectorEndedFlowId.size();
    if (client->m_coflow.finifshedFlowNum == client->m_coflow.flows.size()) {
        printf("coflow finished\n");
        client->SendUvMessage(CEndCoflowTestMsg{}, CEndCoflowTestMsg::MSG_ID);
        client->m_coflow.finifshedFlowNum = 0;
        if (HRRN)
            client->HRRNOver = true;
        else
            client->SJFOver = true;
    }
    client->m_vectorEndedFlowId.clear();
}
Coflow CClient::generateCoflow()//生成coflow
{
    while (this->serverNumber == -1)//获取到后结束循环
        uv_thread_sleep(1);
    const static uint32_t flowSize[] = { 100,1000,10000 };
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
        res.flows.insert(make_pair(id, Flow{ u1(gen),flowSize[u2(gen)] }));
    }
    res.finifshedFlowNum = 0;
    return res;
}

void CClient::analyseCoflow(Coflow & coflow)
{
    auto it = max_element(coflow.flows.begin(), coflow.flows.end(), [](const pair<const int,Flow> &a, const pair<const int, Flow> &b){
        if (HRRN)
            return a.second.HRRNEndTime < b.second.HRRNEndTime;
        return a.second.SJFEndTime < b.second.SJFEndTime;
    });
    uint64_t time;
    if (HRRN)
        time = it->second.HRRNEndTime - coflow.HRRNCreateTime;
    else
        time = it->second.SJFEndTime - coflow.SJFCreateTime;
    printf("%s takes %llums\n", HRRN ? "HRRN" : "SJF", time);
}

void CClient::pushEndedFlowId(int flowId,uint64_t endTime)
{
    CUVAutoLock lock(&m_mutex);
    m_vectorEndedFlowId.push_back(make_pair(flowId,endTime));
    uv_async_send(&m_asyncHandle);
}

void CClient::checkMasterIdle()
{
    masterIdle = false;
    CCheckMasterIdleMsg msg{};
    SendUvMessage(msg, msg.MSG_ID);
}

void CClient::editMasterScheduler(bool hrrn)
{
    CEditSchedulerMsg msg{ hrrn };
    SendUvMessage(msg, msg.MSG_ID);
}

int CClient::OnUvMessage(const CStartFlowRequestMsg & msg, TcpClientCtx * pClient)
{
    hashmap[msg.flowId].setAsyncHandle(&m_asyncHandle);
    hashmap[msg.flowId].getFlow(ip_int2string(msg.ip),htons(msg.port),msg.flowSize,msg.flowId);
    if (HRRN)
        m_coflow.flows[msg.flowId].HRRNStartTime = uv_now(GetLoop());
    else
        m_coflow.flows[msg.flowId].SJFStartTime = uv_now(GetLoop());
    return 0;
}

int CClient::OnUvMessage(const CCheckMasterIdleMsg & msg, TcpClientCtx * pClient)
{
    masterIdle = msg.idle;
    return 0;
}

int CClient::OnUvMessage(const CEditSchedulerMsg & msg, TcpClientCtx * pClient)
{
    HRRN = msg.hrrn;
    return 0;
}

int CClient::OnUvMessage(const CStartCoflowTestMsg & msg, TcpClientCtx * pClient)
{
    HRRN = msg.hrrn;
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
    statistic.HRRNCreateTime = m_coflow.HRRNCreateTime;
    statistic.SJFCreateTime = m_coflow.SJFCreateTime;
    for (auto &element : m_coflow.flows) {
        statistic.flowSize = element.second.flowSize;
        statistic.flowId = element.first;
        statistic.HRRNStartTime = element.second.HRRNStartTime;
        statistic.SJFStartTime = element.second.SJFStartTime;
        statistic.HRRNEndTime = element.second.HRRNEndTime;
        statistic.SJFEndTime = element.second.SJFEndTime;
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
    if (0) {
        client.m_coflow = client.generateCoflow();
        client.startCoflowTest();//开始SJF coflow测试 
        while (!client.SJFOver)
            uv_thread_sleep(10);
        CClient::analyseCoflow(client.m_coflow);

        uv_thread_sleep(1000);
        while (!client.masterIdle) {
            client.checkMasterIdle();
            uv_thread_sleep(10);
        }
        client.editMasterScheduler(1);
        while (!HRRN)
            uv_thread_sleep(10);
        client.startCoflowTest();//开始HRRN coflow测试
        while (!client.HRRNOver)
            uv_thread_sleep(10);
        CClient::analyseCoflow(client.m_coflow);
    }
    while (!client.exit)
        uv_thread_sleep(10);
}