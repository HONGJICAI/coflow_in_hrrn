#include "slave_client.h"

CSlaveClient::CSlaveClient()
{
}

CSlaveClient::~CSlaveClient()
{
}

int CSlaveClient::OnUvMessage(const CLoginRspMsg & msg, TcpClientCtx * pClient)
{
    id = msg.id;
    return 0;
}
