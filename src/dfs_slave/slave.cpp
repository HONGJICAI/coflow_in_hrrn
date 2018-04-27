#include <iostream>
#include <string>
#include <algorithm>
#include "simple_uv/config.h"
#include "simple_uv/log4z.h"
#include "slave.h"
#include "slave_client.h"

#ifdef WIN32
#include <io.h>
//遍历当前目录下的文件夹和文件,默认是按字母顺序遍历  
bool TraverseFiles(string path){
    _finddata_t file_info;
    string current_path = path + "/*.*"; //可以定义后面的后缀为*.exe，*.txt等来查找特定后缀的文件，*.*是通配符，匹配所有类型,路径连接符最好是左斜杠/，可跨平台  
                                         //打开文件查找句柄  
    int handle = _findfirst(current_path.c_str(), &file_info);
    //返回值为-1则查找失败  
    if (-1 == handle)
        return false;
    do{
        if (file_info.attrib == _A_SUBDIR) //是目录  
            continue;
        //输出文件信息并计数,文件名(带后缀)、文件最后修改时间、文件字节数(文件夹显示0)、文件是否目录  
        cout << file_info.name << ' ' << file_info.time_write << ' ' << file_info.size << endl; //获得的最后修改时间是time_t格式的长整型，需要用其他方法转成正常时间显示  
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

int CSlaveServer::OnUvMessage(const CGetFlowMsg & msg, TcpClientCtx * pClient)
{
    LOGFMTI("start execute job");
    int totalSize = msg.flowSize;
    CPushFlowMsg data;
    data.totalPacketNum = msg.flowSize;
    data.currentPacketNum = 1;
    while (data.currentPacketNum <= data.totalPacketNum) {
        this->SendUvMessage(data, data.MSG_ID, pClient);
        ++data.currentPacketNum;
        uv_thread_sleep(1);
    }
    LOGFMTI("job completed");
    m_pTcpClient->SendUvMessage(CIdleMsg{m_pTcpClient->id}, CIdleMsg::MSG_ID);
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

void CSlaveServer::LoadConfig() {
    const string file = "slave.config";
    Config configure;
    if (!configure.FileExist(file)) {
        LOGFMTE("cannot find configure file");
        return;
    }
    configure.ReadFile(file);
    {
        //string list;
        //list = configure.Read("files", list);
        //stringstream ss(list);
        //string tuple;
        //while (!ss.eof()) {
        //    getline(ss, tuple, ';');
        //    int offset = tuple.find(':');
        //    if (offset == string::npos) {
        //        LOGE("error on file configure");
        //        continue;
        //    }
        //    m_mapLocalFile[tuple.substr(0, offset)] = (uint64_t)stoll(tuple.substr(offset + 1));
        //}
    }
    {
        //m_strMasterIp = "0.0.0.0";
        //m_strMasterIp = configure.Read("master_ip", m_strMasterIp);
        //m_nMasterPort = configure.Read("master_port", 10000);
    }
}

int main(int argc, char** argv) {
    if (argc == 2)
        port = stoi(argv[1]);
    zsummer::log4z::ILog4zManager::getRef().start();
    CSlaveServer server;
    //server.LoadConfig();
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