#include "client_slave.h"
#include "client.h"

CClientSlave::CClientSlave()
{
}

CClientSlave::~CClientSlave()
{
}

void CClientSlave::getFlow(std::string ip,int port,int flowSize,int flowId)
{
    m_nFlowId = flowId;
    if (!this->Connect(ip.c_str(), port)) {
        printf("fail connect slave %s:%d\n",ip.c_str(),port);
        return;
    }
    CGetFlowMsg msg{flowSize};
    this->SendUvMessage(msg, msg.MSG_ID);
}

void CClientSlave::setAsyncHandle(uv_async_t * async)
{
    m_asyncHandle = async;
}

int CClientSlave::OnUvMessage(const CPushFlowMsg & msg, TcpClientCtx * pClient)
{
    if (msg.currentPacketNum == msg.totalPacketNum) {
        printf("flow finish,id %d\n");
        CClient *mainClient = reinterpret_cast<CClient*>(m_asyncHandle->data);
        mainClient->pushEndedFlowId(m_nFlowId,uv_now(this->GetLoop()));
    }
    return 0;
}
