#include <random>
#include <unordered_map>
#include "client_slave.h"
#include "client.h"
#include "simple_uv/common.h"

bool HRRN;
static unordered_map<int, CClientSlave>hashmap;
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
        hashmap.erase(element.first);
    }
    client->m_coflow.finifshedFlowNum += client->m_vectorEndedFlowId.size();
    if (client->m_coflow.finifshedFlowNum == client->m_coflow.flows.size()) {
        printf("coflow finished\n");
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
    const static int flowSize[] = { 10,100,1000 };
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static uniform_int_distribution<int> u1(0, this->serverNumber - 1);
    static uniform_int_distribution<int> u2(0, sizeof(flowSize) / sizeof(int) - 1); //随机的flowSize
    static uniform_int_distribution<int> u3(0x80000000, 0x7fffffff); //随机的flowId
    static std::unordered_set<int>usedFlowId;
    int job = 5 * u1(gen) + 10;//随机个数的flow
    int id;
    Coflow res;
    for (int i = 0; i < job; ++i) {//随机分配在服务器中
        do {
            id = u3(gen);
        } while (usedFlowId.find(id) != usedFlowId.end());
        usedFlowId.insert(id);
        res.flows.insert(make_pair(id, Flow{ u1(gen),flowSize[u2(gen)],0,0,id }));
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
    return 0;
}

int CClient::OnUvMessage(const CCheckMasterIdleMsg & msg, TcpClientCtx * pClient)
{
    masterIdle = true;
    return 0;
}

int CClient::OnUvMessage(const CEditSchedulerMsg & msg, TcpClientCtx * pClient)
{
    HRRN = msg.hrrn;
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
    client.SendUvMessage(CGetServerNumberMsg{}, CGetServerNumberMsg::MSG_ID);//获取服务器总数
    while (client.serverNumber == -1)//获取到后结束循环
        uv_thread_sleep(1);

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
    std::cin >> HRRN;
}