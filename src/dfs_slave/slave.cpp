#include <iostream>
#include <string>
#include <algorithm>
#include "simple_uv/config.h"
#include "simple_uv/log4z.h"
#include "slave.h"
#include "slave_client.h"

#ifdef WIN32
#include <io.h>
//������ǰĿ¼�µ��ļ��к��ļ�,Ĭ���ǰ���ĸ˳�����  
bool TraverseFiles(string path){
    _finddata_t file_info;
    string current_path = path + "/*.*"; //���Զ������ĺ�׺Ϊ*.exe��*.txt���������ض���׺���ļ���*.*��ͨ�����ƥ����������,·�����ӷ��������б��/���ɿ�ƽ̨  
                                         //���ļ����Ҿ��  
    int handle = _findfirst(current_path.c_str(), &file_info);
    //����ֵΪ-1�����ʧ��  
    if (-1 == handle)
        return false;
    do{
        if (file_info.attrib == _A_SUBDIR) //��Ŀ¼  
            continue;
        //����ļ���Ϣ������,�ļ���(����׺)���ļ�����޸�ʱ�䡢�ļ��ֽ���(�ļ�����ʾ0)���ļ��Ƿ�Ŀ¼  
        cout << file_info.name << ' ' << file_info.time_write << ' ' << file_info.size << endl; //��õ�����޸�ʱ����time_t��ʽ�ĳ����ͣ���Ҫ����������ת������ʱ����ʾ  
    } while (!_findnext(handle, &file_info));
    _findclose(handle);
    return true;
}
#else
bool TraverseFiles(string path){
    struct dirent *direntp;
    DIR *dirp = opendir(path.c_str());
    if (dirp == NULL)
        return false;
    while ((direntp = readdir(dirp)) != NULL)
        printf("%s\n", direntp->d_name);
    closedir(dirp);
    return true;
}
#endif

int port=12345;
CSlaveServer::CSlaveServer() :
    m_strLocalIp("127.0.0.1"),
    m_nLocalPort(port)
{
    m_pTcpClient = make_shared<CSlaveClient>();
}


CSlaveServer::~CSlaveServer()
{
}

bool CSlaveServer::Start()
{
    LOGFMTI("slave server start at %s:%u", m_strLocalIp.c_str(), m_nLocalPort);
    return CTCPServer::Start(m_strLocalIp.c_str(), m_nLocalPort);
}

int totalSize;
TcpClientCtx * g_pClient;
int CSlaveServer::OnUvMessage(const CGetFlowMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("start execute job");
    static uv_timer_t timer;
    static bool init = false;
    if (!init) {
        uv_timer_init(this->GetLoop(), &timer);
        timer.data = this;
        init = true;
    }
    totalSize = msg.flowSize;
    g_pClient = pClient;
    uv_timer_start(&timer, [](uv_timer_t *timer) {
        CSlaveServer *slave = reinterpret_cast<CSlaveServer*>(timer->data);
        static int currentPacketNum = 1;
        CPushFlowMsg data;
        data.totalPacketNum = totalSize;
        data.currentPacketNum = currentPacketNum;
        slave->SendUvMessage(data, data.MSG_ID, g_pClient);
        ++currentPacketNum;
        if (currentPacketNum > totalSize) {
            currentPacketNum = 1;
            uv_timer_stop(timer);
            LOGFMTI("job completed");
            slave->m_pTcpClient->SendUvMessage(CIdleMsg{slave->m_pTcpClient->id}, CIdleMsg::MSG_ID);
        }
    },1,1);

    return 0;
}
#include "../simple_uv/common.h"
void CSlaveServer::LoginOnMaster()
{
    if (!m_pTcpClient->Connect("127.0.0.1", 10000)) {
        printf("connect on master fail\n");
        return;
    }
    sockaddr_storage addr;
    int l = sizeof(addr);
    uv_tcp_getsockname(&this->m_tcpHandle, reinterpret_cast<sockaddr*>(&addr), &l);
    sockaddr_in *pAddr = reinterpret_cast<sockaddr_in*>(&addr);
    CLoginMsg msg;
    msg.ip = *reinterpret_cast<uint32_t*>(&pAddr->sin_addr);
    msg.port = *reinterpret_cast<uint16_t*>(&pAddr->sin_port);
    m_pTcpClient->SendUvMessage(msg, msg.MSG_ID);
}

int main(int argc, char** argv) {
    if (argc == 2)
        port = stoi(argv[1]);
    zsummer::log4z::ILog4zManager::getRef().start();
    CSlaveServer server;
    if (!server.Start()) {
        fprintf(stdout, "Start Server error:%s\n", server.GetLastErrMsg());
        return -1;
    }
    server.LoginOnMaster();
    while (1) {
        uv_thread_sleep(100000);
    }
    return 0;
}