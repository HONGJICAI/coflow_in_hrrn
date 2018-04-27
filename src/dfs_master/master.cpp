#include <iostream>
#include <string>
#include <algorithm>
#include "simple_uv/config.h"
#include "simple_uv/log4z.h"
#include "simple_uv/common.h"
#include "master.h"

static bool HRRN;
CMasterServer::CMasterServer() :
    m_strMasterIp("127.0.0.1"),
    m_nMasterPort(10000)
{
    static uv_timer_t timer;
    timer.data = this;
    uv_timer_init(this->GetLoop(), &timer);
    uv_timer_start(&timer, scheduleTimer , 1, 1);
}


CMasterServer::~CMasterServer()
{
}

bool CMasterServer::Start()
{
    LOGFMTI("master server start at %s:%u", m_strMasterIp.c_str(), m_nMasterPort);
    return CTCPServer::Start(m_strMasterIp.c_str(), m_nMasterPort);
}

void CMasterServer::scheduleTimer(uv_timer_t * timer)
{
    static vector<int>busyServer;
    CMasterServer *master = reinterpret_cast<CMasterServer*>(timer->data);
    uint64_t now = uv_now(master->GetLoop());
    for (auto serverId : master->m_setIdleSlaveServer) {
        auto &queueRequest = master->m_queueRequest[serverId];
        if (queueRequest.size()) {//若空闲服务器有未完成任务
            busyServer.push_back(serverId);//将有任务做的server从idle队列移动到busy队列
            auto it = min_element(queueRequest.begin(), queueRequest.end(), [master,now](const Request&a,const Request &b) {
                //找到优先级最高的任务
                if (HRRN) {
                    //服务器设计为1kB/ms的发包速度，因此flowSize=预计处理时间（单位：ms）
                    return double(now - a.requestTime + a.flowSize) / a.flowSize<double(now - b.requestTime + b.flowSize) / b.flowSize;
                }
                return a.flowSize < b.flowSize;
            });
            //允许client向slaveServer请求
            CStartFlowRequestMsg msg = { serverId,it->flowSize,it->flowId,master->m_vectorLoginSlave[serverId].first,master->m_vectorLoginSlave[serverId].second };
            master->SendUvMessage(msg, msg.MSG_ID, it->client);
            LOGFMTI("allow client send request to slave server,flowId %u", msg.flowId);
            queueRequest.erase(it);
        }
    }
    for (auto serverId : busyServer)
        master->m_setIdleSlaveServer.erase(serverId);
    busyServer.clear();
}

int CMasterServer::OnUvMessage(const CIdleMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("receive idle msg from server %u", msg.id);
    m_setIdleSlaveServer.insert(msg.id);
    return 0;
}

int CMasterServer::OnUvMessage(const CCreateFlowJobMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("receive create job msg,flow id %u,target server %u",msg.flowId,msg.targetServerId);
    m_queueRequest[msg.targetServerId].push_back(Request{pClient,uv_now(this->GetLoop()),msg.flowSize,msg.flowId});
    return 0;
}

int CMasterServer::OnUvMessage(const CGetServerNumberMsg & msg, TcpClientCtx * pClient)
{
    this->SendUvMessage(CGetServerNumberMsg{ (int)m_vectorLoginSlave.size() }, CGetServerNumberMsg::MSG_ID, pClient);
    return 0;
}

int CMasterServer::OnUvMessage(const CEditSchedulerMsg & msg, TcpClientCtx * pClient)
{
    HRRN = msg.hrrn;
    this->SendUvMessage(msg, msg.MSG_ID, pClient);
    return 0;
}

int CMasterServer::OnUvMessage(const CCheckMasterIdleMsg & msg, TcpClientCtx * pClient)
{
    CCheckMasterIdleMsg res{
        this->m_setIdleSlaveServer.size() == this->m_vectorLoginSlave.size() };
    this->SendUvMessage(res, res.MSG_ID, pClient);
    return 0;
}

int CMasterServer::OnUvMessage(const CLoginMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("slave login,id = %u",allocateId);
    m_vectorLoginSlave.push_back(make_pair(msg.ip, msg.port));
    m_setIdleSlaveServer.insert(allocateId);
    m_queueRequest.push_back(list<Request>());
    this->SendUvMessage(CLoginRspMsg{ allocateId++ }, CLoginRspMsg::MSG_ID, pClient);
    return 0;
}

void CMasterServer::LoadConfig() {
    const string file = "master.config";
    Config configure;
    if (!configure.FileExist(file)) {
        return;
    }
    configure.ReadFile(file);
    /*{
        string list;
        list = configure.Read("slave_node", list);
        stringstream ss(list);
        string ip;
        while (!ss.eof()) {
            getline(ss, ip, ';');
        }
    }*/
    {
        //m_strMasterIp = "0.0.0.0";
        //m_strMasterIp = configure.Read("master_ip", m_strMasterIp);
        m_nMasterPort = configure.Read("master_port", 10000);
    }
}

int main(int argc, char** argv) {
    zsummer::log4z::ILog4zManager::getRef().start();
    CMasterServer server;
    //server.LoadConfig();
    if (!server.Start()) {
        fprintf(stdout, "Start Server error:%s\n", server.GetLastErrMsg());
    }
    while (1) {
        uv_thread_sleep(100000);
    }
    return 0;
}