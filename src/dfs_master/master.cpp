#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include "simple_uv/config.h"
#include "simple_uv/log4z.h"
#include "simple_uv/common.h"
#include "master.h"

int scheduleAlgorithm;
int coflowNum;
uint32_t slaveNum = 1;
int finishedCoflow = 0;
int mode = 0;
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
        case 0: {//�ͻ��������Ϻ�����coflow SJF����
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
    if (mode == 0) {//clientͬһʱ�̷���coflow
        CStartCoflowTestMsg msg{ scheduleAlgorithm };
        for (auto &element : m_vectorLoginUser)
            SendUvMessage(msg, msg.MSG_ID, element);
    }
    else {//client���η���coflow
        static uv_timer_t timer;
        static bool init = false;
        if (init == false) {
            timer.data = this;
            uv_timer_init(this->GetLoop(), &timer);
        }
        uv_timer_start(&timer, [](uv_timer_t *timer) {
            CMasterServer *master = reinterpret_cast<CMasterServer*>(timer->data);
            static int id = 0;
            master->startCoflowTestById(id++);
            if (id == master->m_vectorLoginUser.size()) {
                uv_timer_stop(timer);
                id = 0;
            }
        }, 0, 1000);
    }
}

void CMasterServer::startCoflowTestById(int id)
{
    CStartCoflowTestMsg msg{ scheduleAlgorithm };
    this->SendUvMessage(msg, msg.MSG_ID, m_vectorLoginUser[id]);
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
    for (auto &element : m_mapCoflowStatistic) {//����coflow����ʱ��Ϳ�ʼʱ��
        for (int i = ScheduleAlgorithm::SJF; i <= ScheduleAlgorithm::CHRRN; ++i) {
            element.second.endTime[i] = max_element(element.second.flows.begin(), element.second.flows.end(), [i](const pair<const uint32_t, Flow> &a, const pair<const uint32_t, Flow> &b) {
                return a.second.endTime.at(i) < b.second.endTime.at(i);
            })->second.endTime[i];
            element.second.startTime[i] = min_element(element.second.flows.begin(), element.second.flows.end(), [i](const pair<const uint32_t, Flow> &a, const pair<const uint32_t, Flow> &b) {
                return a.second.startTime.at(i) < b.second.startTime.at(i);
            })->second.endTime[i];
        }
    }
#include <fstream>
    fstream out("result.txt",ios::app);
    out << "------------timestamp:" << uv_now(this->GetLoop()) << "------------" << endl;
    uint64_t cct[4] = {};
    uint64_t cdt[4] = {};
    uint64_t mdt[4] = {};
    string scf="scf=[", chrrn="chrrn=[";
    out << "flow config:" << endl;
    out << "coflowId flowId flowSize targetServer "<<endl;
    for (int i = 0; i < m_mapCoflowStatistic.size();++i) {
        auto element = m_mapCoflowStatistic.at(i);
        for (auto &flow : element.flows) {
            out << i << " & " << flow.first << " & " << flow.second.flowSize << " & " << flow.second.targetServer << endl;
        }
        for (int i = ScheduleAlgorithm::SJF; i <= ScheduleAlgorithm::CHRRN; ++i) {
            cct[i] += element.endTime[i] - element.createTime[i];
            cdt[i] += element.startTime[i] - element.createTime[i];
            mdt[i] = max(mdt[i], element.startTime[i] - element.createTime[i]);
        }
        scf+=to_string( element.endTime[SCF] - element.createTime[SCF])+ ",";
        chrrn += to_string(element.endTime[CHRRN] - element.createTime[CHRRN]) + ",";
    }
    scf.pop_back(),chrrn.pop_back();
    scf += "]", chrrn += "]";
    out << scf << endl << chrrn << endl;
    out << "method\tCCT\tMST\tCST\t" << endl;
    out << "SJF\t" << cct[SJF] << "\t" <<mdt[SJF]<<"\t"<< cdt[SJF]<< endl;
    out << "HRRN\t" << cct[HRRN] << "\t" << mdt[HRRN] << "\t" << cdt[HRRN] << endl;
    out << "SCF\t" << cct[SCF] << "\t" << mdt[SCF] << "\t" << cdt[SCF]<< endl;
    out << "CHRRN\t" << cct[CHRRN] << "\t" << mdt[CHRRN] << "\t" << cdt[CHRRN]<< endl;
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
        if (queueRequest.size()) {//�����з�������δ�������
            busyServer.push_back(serverId);//������������server��idle�����ƶ���busy����
            auto it = min_element(queueRequest.begin(), queueRequest.end(), [now](const Request&a,const Request &b) {
                //�ҵ����ȼ���ߵ�����
                switch (scheduleAlgorithm)
                {
                case ScheduleAlgorithm::HRRN:
                    //���������Ϊ1kB/ms�ķ����ٶȣ����flowSize=Ԥ�ƴ���ʱ�䣨��λ��ms��
                    return double(now - a.requestTime) / a.flowSize>double(now - b.requestTime) / b.flowSize;
                case ScheduleAlgorithm::SJF:
                    return a.flowSize < b.flowSize;
                case ScheduleAlgorithm::SCF:
                    return a.coflowSize < b.coflowSize;
                case ScheduleAlgorithm::CHRRN:
                    cout<< double(now - a.requestTime) / (2 * a.coflowSize / slaveNum)<<"&"<< double(now - b.requestTime) / (2 * b.coflowSize / slaveNum)<<endl;
                    return double(now - a.requestTime) / (2*a.coflowSize/slaveNum)>double(now - b.requestTime) / (2*b.coflowSize/slaveNum);
                }
            });
            //����client��slaveServer����
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
    if (argc < 3) {
        printf("usage:programName slaveNum coflowNum [mode(option)].if mode=0,coflow start together,else coflow start one by one\n.e.g.:master.exe 1 1\n");
        return -1;
    }
    slaveNum = stoi(argv[1]);
    coflowNum = stoi(argv[2]);
    if(argc==4)
        mode = stoi(argv[3]);
    //zsummer::log4z::ILog4zManager::getRef().start();
    CMasterServer server;
    if (!server.Start()) {
        fprintf(stdout, "Start Server error:%s\n", server.GetLastErrMsg());
    }
    while (1) {
        uv_thread_sleep(100000);
    }
    return 0;
}