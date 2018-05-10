#include <iostream>
#include <string>
#include <algorithm>
#include "simple_uv/config.h"
#include "simple_uv/log4z.h"
#include "simple_uv/common.h"
#include "master.h"

int scheduleAlgorithm;
int coflowNum;
uint32_t slaveNum = 1;
int finishedCoflow = 0;
CMasterServer::CMasterServer() :
    m_strMasterIp("127.0.0.1"),
    m_nMasterPort(10000)
{
    static uv_timer_t timerSchedule,timerAutorun;
    timerSchedule.data = this;
    timerAutorun.data = this;
    uv_timer_init(this->GetLoop(), &timerSchedule);
    uv_timer_init(this->GetLoop(), &timerAutorun);
    uv_timer_start(&timerSchedule, scheduleTimer , 5, 5);
    uv_timer_start(&timerAutorun, [](uv_timer_t * timer) {
        CMasterServer *master = reinterpret_cast<CMasterServer*>(timer->data);
        static int stage = 0;
        switch (stage) {
        case 0: {//客户端连接上后，启动coflow SJF测试
            if (master->m_vectorLoginUser.size() == coflowNum && slaveNum == master->m_vectorLoginSlave.size()) {
                ++stage;
                master->startCoflowTest(ScheduleAlgorithm::SJF);
            }
            break;
        }
        case 1: {
            if (finishedCoflow == coflowNum) {
                finishedCoflow = 0;
                static int curScheduleAlgorithm = ScheduleAlgorithm::SJF;
                switch (curScheduleAlgorithm) {
                case ScheduleAlgorithm::SJF:
                case ScheduleAlgorithm::HRRN:
                case ScheduleAlgorithm::SCF:++curScheduleAlgorithm; break;
                case ScheduleAlgorithm::CHRRN: {
                    ++stage;
                    master->killSlave();
                    master->endCoflowTest();
                    break;
                    }
                }
                if(1==stage)
                    master->startCoflowTest(curScheduleAlgorithm);
            }
            break;
        }
        case 2: {
            if (finishedCoflow == coflowNum) {
                master->killClient();
                master->analyseCoflow();
                stage++;
            }
            break;
        }
        }
    }, 200, 200);
}


CMasterServer::~CMasterServer()
{
}

bool CMasterServer::Start()
{
    LOGFMTI("master server start at %s:%u", m_strMasterIp.c_str(), m_nMasterPort);
    return CTCPServer::Start(m_strMasterIp.c_str(), m_nMasterPort);
}

void CMasterServer::startCoflowTest(int _scheduleAlgorithm)
{
    scheduleAlgorithm = _scheduleAlgorithm;
    CStartCoflowTestMsg msg{ scheduleAlgorithm };
    for (auto element : m_vectorLoginUser) 
        SendUvMessage(msg, msg.MSG_ID, element);
}

void CMasterServer::killSlave()
{
    for (auto &slave : m_vectorLoginSlave)
        SendUvMessage(CKillSlaveMsg{}, CKillSlaveMsg::MSG_ID, slave.socket);
}

void CMasterServer::killClient()
{
    for (auto element : m_vectorLoginUser)
        SendUvMessage(CKillClientMsg{}, CKillClientMsg::MSG_ID, element);
}

void CMasterServer::endCoflowTest()
{
    for (auto element : m_vectorLoginUser)
        SendUvMessage(CEndCoflowTestMsg{}, CEndCoflowTestMsg::MSG_ID, element);
}

void CMasterServer::analyseCoflow()
{
    for (auto &element : m_mapCoflowStatistic) {
        for(int i=ScheduleAlgorithm::SJF;i<=ScheduleAlgorithm::CHRRN;++i)
            element.second.endTime[i]= max_element(element.second.flows.begin(), element.second.flows.end(), [i](const pair<const uint32_t, Flow> &a, const pair<const uint32_t, Flow> &b) {
                return a.second.endTime.at(i) < b.second.endTime.at(i);
            })->second.endTime[i];
    }
#include <fstream>
    fstream out("result.txt",ios::app);
    //out << "flowId\tflowSize\tHRRN\tSJF\tCHRRN\tSCF" << endl;
    //uint64_t HRRNCCT = 0, SJFCCT = 0,SCFCCT=0,CHRRNCCT=0;
    //for (auto &element : m_mapCoflowStatistic) {
    //    for (auto flow : element.second.flows) {
    //        out << flow.first << "\t" << flow.second.flowSize<<"\t"
    //            <<flow.second.startTime[ScheduleAlgorithm::HRRN] - element.second.createTime[ScheduleAlgorithm::HRRN] << "\t" 
    //            << flow.second.startTime[SJF]-element.second.createTime[SJF] << "\t"
    //            <<flow.second.startTime[ScheduleAlgorithm::CHRRN] - element.second.createTime[ScheduleAlgorithm::CHRRN] << "\t" 
    //            << flow.second.startTime[SCF]-element.second.createTime[SCF] << "\t"
    //            << endl;
    //    }
    //    out << "HRRN:" << element.second.endTime[HRRN] - element.second.createTime[HRRN] << endl;
    //    out << "SJF:" << element.second.endTime[SJF] - element.second.createTime[SJF] << endl;
    //    out << "CHRRN:" << element.second.endTime[CHRRN] - element.second.createTime[CHRRN] << endl;
    //    out << "SCF:" << element.second.endTime[SCF] - element.second.createTime[SCF] << endl;
    //    HRRNCCT += (element.second.endTime[HRRN] - element.second.createTime[HRRN]) / double(coflowNum);
    //    SJFCCT += (element.second.endTime[SJF] - element.second.createTime[SJF]) / double(coflowNum);
    //    CHRRNCCT += (element.second.endTime[CHRRN] - element.second.createTime[CHRRN]) / double(coflowNum);
    //    SCFCCT += (element.second.endTime[SCF] - element.second.createTime[SCF]) / double(coflowNum);
    //}
    //out << "HRRN CCT:" << HRRNCCT << ",SJF CCT:" << SJFCCT;
    //out << ",CHRRN CCT:" << CHRRNCCT << ",SCF CCT:" << SCFCCT << endl;
    uint64_t fct[4] = {};
    uint64_t fdt[4] = {};
    uint64_t cct[4] = {};
    for (auto &element : m_mapCoflowStatistic) {
        for (auto &flow : element.second.flows) {
            for (int i = ScheduleAlgorithm::SJF; i <= ScheduleAlgorithm::CHRRN; ++i) {
                fct[i] += flow.second.endTime[i] - flow.second.startTime[i];
                fdt[i] += flow.second.startTime[i] - element.second.createTime[i];
            }
        }
        for (int i = ScheduleAlgorithm::SJF; i <= ScheduleAlgorithm::CHRRN; ++i) {
            cct[i] += element.second.endTime[i] - element.second.createTime[i];
        }
    }
    out << "HRRN CCT:" << cct[HRRN] << ",SJF CCT:" << cct[SJF] << ",CHRRN CCT:" << cct[CHRRN] << ",SCF CCT:" << cct[SCF] << endl;
    out << "HRRN FDT:" << fdt[HRRN] << ",SJF FDT:" << fdt[SJF] << ",CHRRN FDT:" << fdt[CHRRN] << ",SCF FDT:" << fdt[SCF] << endl;
    out << "HRRN FCT:" << fct[HRRN] << ",SJF FCT:" << fct[SJF] << ",CHRRN FCT:" << fct[CHRRN] << ",SCF FCT:" << fct[SCF] << endl;
    out.close();
    ::exit(0);
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
                switch (scheduleAlgorithm)
                {
                case ScheduleAlgorithm::HRRN:
                    //服务器设计为1kB/ms的发包速度，因此flowSize=预计处理时间（单位：ms）
                    return double(now - a.requestTime) / a.flowSize>double(now - b.requestTime) / b.flowSize;
                case ScheduleAlgorithm::SJF:
                    return a.flowSize < b.flowSize;
                case ScheduleAlgorithm::SCF:
                    return a.coflowSize < b.coflowSize;
                case ScheduleAlgorithm::CHRRN:
                    return double(now - a.requestTime) / (2*a.coflowSize/slaveNum)>double(now - b.requestTime) / (2*b.coflowSize/slaveNum);
                }
            });
            //允许client向slaveServer请求
            CStartFlowRequestMsg msg = { serverId,it->flowSize,it->flowId,master->m_vectorLoginSlave[serverId].ip,master->m_vectorLoginSlave[serverId].port };
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
    m_queueRequest[msg.targetServerId].push_back(Request{pClient,uv_now(this->GetLoop()),msg.flowSize,msg.flowId,msg.coflowSize});
    return 0;
}

int CMasterServer::OnUvMessage(const CGetServerNumberMsg & msg, TcpClientCtx * pClient)
{
    this->SendUvMessage(CGetServerNumberMsg{ slaveNum }, CGetServerNumberMsg::MSG_ID, pClient);
    return 0;
}

//int CMasterServer::OnUvMessage(const CEditSchedulerMsg & msg, TcpClientCtx * pClient)
//{
//    HRRN = msg.hrrn;
//    this->SendUvMessage(msg, msg.MSG_ID, pClient);
//    return 0;
//}

//int CMasterServer::OnUvMessage(const CCheckMasterIdleMsg & msg, TcpClientCtx * pClient)
//{
//    CCheckMasterIdleMsg res{
//        this->m_setIdleSlaveServer.size() == this->m_vectorLoginSlave.size() };
//    this->SendUvMessage(res, res.MSG_ID, pClient);
//    return 0;
//}

int CMasterServer::OnUvMessage(const CClientLoginMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("client login,id %u", allocateUserId);
    m_vectorLoginUser.push_back(pClient);
    CClientLoginMsg loginMsg{ allocateUserId++ };
    SendUvMessage(loginMsg, loginMsg.MSG_ID, pClient);
    return 0;
}

int CMasterServer::OnUvMessage(const CReportFlowStatistic & msg, TcpClientCtx * pClient)
{
    m_mapCoflowStatistic[msg.userId].flows[msg.flowId] = Flow{msg.targetServer,msg.flowSize,
    { { ScheduleAlgorithm::SJF, msg.SJFStartTime },{ ScheduleAlgorithm::HRRN, msg.HRRNStartTime },{ ScheduleAlgorithm::SCF, msg.SCFStartTime },{ ScheduleAlgorithm::CHRRN, msg.CHRRNStartTime } },
    { { ScheduleAlgorithm::SJF, msg.SJFEndTime },{ ScheduleAlgorithm::HRRN, msg.HRRNEndTime }, { ScheduleAlgorithm::SCF, msg.SCFEndTime },{ ScheduleAlgorithm::CHRRN, msg.CHRRNEndTime }}
    };
    m_mapCoflowStatistic[msg.userId].createTime[ScheduleAlgorithm::HRRN] = msg.HRRNCreateTime;
    m_mapCoflowStatistic[msg.userId].createTime[ScheduleAlgorithm::SJF] = msg.SJFCreateTime;
    m_mapCoflowStatistic[msg.userId].createTime[ScheduleAlgorithm::CHRRN] = msg.CHRRNCreateTime;
    m_mapCoflowStatistic[msg.userId].createTime[ScheduleAlgorithm::SCF] = msg.SCFCreateTime;
    return 0;
}

int CMasterServer::OnUvMessage(const CEndCoflowTestMsg & msg, TcpClientCtx * pClient)
{
    ++finishedCoflow;
    return 0;
}

int CMasterServer::OnUvMessage(const CLoginMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("slave login,id = %u",allocateId);
    m_vectorLoginSlave.push_back(Slave{ msg.ip,msg.port,pClient });
    m_setIdleSlaveServer.insert(allocateId);
    m_queueRequest.push_back(list<Request>());
    this->SendUvMessage(CLoginRspMsg{ allocateId++ }, CLoginRspMsg::MSG_ID, pClient);
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("usage:programName slaveNum coflowNum.e.g.:master.exe 1 1\n");
        return -1;
    }
    slaveNum = stoi(argv[1]);
    coflowNum = stoi(argv[2]);
    zsummer::log4z::ILog4zManager::getRef().start();
    CMasterServer server;
    if (!server.Start()) {
        fprintf(stdout, "Start Server error:%s\n", server.GetLastErrMsg());
    }
    while (1) {
        uv_thread_sleep(100000);
    }
    return 0;
}