#pragma once
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <simple_uv/tcpserver.h>
#include "../message.h"

class CSlaveClient;
class CSlaveServer :
    public CTCPServer
{
public:
    CSlaveServer();
    virtual ~CSlaveServer();
    void LoadConfig();
    bool Start();

    void LoginOnMaster();
protected:
    BEGIN_UV_BIND
        UV_BIND(CGetFlowMsg::MSG_ID, CGetFlowMsg)
        END_UV_BIND(CTCPServer)

    int OnUvMessage(const CGetFlowMsg &msg, TcpClientCtx *pClient);
private:
    std::string m_strLocalIp;
    int m_nLocalPort;
    shared_ptr<CSlaveClient> m_pTcpClient;
};