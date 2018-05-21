#pragma once
#include <stdint.h>
#include <unordered_map>
#pragma pack(1)
enum ScheduleAlgorithm {
    SJF,
    HRRN,
    SCF,
    CHRRN
};

struct Flow {
    uint32_t targetServer;
    uint32_t flowSize;
    unordered_map<int, uint64_t>startTime;
    unordered_map<int, uint64_t>endTime;
};
struct Coflow {
    unordered_map<uint32_t, Flow>flows;
    unordered_map<int, uint64_t>createTime;
    unordered_map<int, uint64_t>startTime;
    unordered_map<int, uint64_t>endTime;
    uint32_t coflowSize;
    uint32_t coflowLength;
    uint32_t coflowWidth;
};

enum SlaveAndMaster{
    LOGIN_MSG=1000,
    LOGIN_RSP_MSG,
    IDLE_MSG,
    KILL_SLAVE
};
class CLoginMsg {
public :
    enum {
        MSG_ID = SlaveAndMaster::LOGIN_MSG
    };
    uint32_t ip;
    uint16_t port;
};
class CLoginRspMsg {
public:
    enum {
        MSG_ID = SlaveAndMaster::LOGIN_MSG
    };
    uint32_t id;
};
class CIdleMsg {
public:
    enum {
        MSG_ID = IDLE_MSG
    };
    uint32_t id;
};
class CKillSlaveMsg {
public:
    enum {
        MSG_ID = KILL_SLAVE
    };
    char reserve;
};

enum ClientAndMaster {
    GET_SERVER_NUMBER=2000,
    CREATE_FLOW_JOB,
    START_FLOW_REQUEST,
    EDIT_SCHEDULER,
    IS_MASTER_IDLE,
    CLIENT_LOGIN,
    START_COFLOW_TEST,
    END_COFLOW_TEST,
    REPORT_FLOW_STATISTIC,
    KILL_CLIENT
};
class CGetServerNumberMsg {
public:
    enum {
        MSG_ID = GET_SERVER_NUMBER
    };
    uint32_t number;
};
class CCreateFlowJobMsg {
public:
    enum {
        MSG_ID = CREATE_FLOW_JOB
    };
    uint32_t targetServerId;
    uint32_t flowSize;
    uint32_t flowId;
    uint32_t coflowSize;
};
class CStartFlowRequestMsg {
public:
    enum {
        MSG_ID = START_FLOW_REQUEST
    };
    uint32_t targetServerId;
    uint32_t flowSize;
    uint32_t flowId;
    uint32_t ip;
    uint16_t port;
};
class CEditSchedulerMsg {
public:
    enum {
        MSG_ID = EDIT_SCHEDULER
    };
    bool hrrn;
};
class CCheckMasterIdleMsg {
public:
    enum {
        MSG_ID = IS_MASTER_IDLE
    };
    bool idle;
};
class CClientLoginMsg {
public:
    enum {
        MSG_ID = CLIENT_LOGIN
    };
    uint32_t userId;
};
class CStartCoflowTestMsg {
public:
    enum {
        MSG_ID = START_COFLOW_TEST
    };
    int scheduleAlgorithm;
};
class CEndCoflowTestMsg {
public:
    enum {
        MSG_ID = END_COFLOW_TEST
    };
    char reserve;
};
class CKillClientMsg {
public:
    enum {
        MSG_ID = KILL_CLIENT
    };
    char reserve;
};
class CReportFlowStatistic {
public:
    enum {
        MSG_ID = REPORT_FLOW_STATISTIC
    };
    uint32_t userId;
    uint32_t flowId;
    uint32_t flowSize;
    uint32_t targetServer;
    uint64_t SJFCreateTime;
    uint64_t SJFStartTime;
    uint64_t SJFEndTime;
    uint64_t HRRNCreateTime;
    uint64_t HRRNStartTime;
    uint64_t HRRNEndTime;
    uint64_t SCFCreateTime;
    uint64_t SCFStartTime;
    uint64_t SCFEndTime;
    uint64_t CHRRNCreateTime;
    uint64_t CHRRNStartTime;
    uint64_t CHRRNEndTime;
};

enum ClientAndSlave {
    GET_FLOW = 3000,
    PUSH_FLOW
};
class CGetFlowMsg {
public:
    enum {
        MSG_ID = GET_FLOW
    };
    uint32_t flowSize;
};
class CPushFlowMsg {
public:
    enum {
        MSG_ID = PUSH_FLOW
    };
    uint16_t totalPacketNum;
    uint16_t currentPacketNum;
    char data[1000];
};
#pragma pack()