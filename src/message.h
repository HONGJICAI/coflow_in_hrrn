#pragma once
#include <stdint.h>
#pragma pack(1)
enum SlaveAndMaster{
	PUSH_FILE_INFO = 1000,
    LOGIN_MSG,
    LOGIN_RSP_MSG,
    IDLE_MSG
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
    int id;
};
//class CPushFileInfoMsg {
//public:
//    enum {
//        MSG_ID = PUSH_FILE_INFO
//    };
//    uint32_t ip;
//    uint16_t port;
//    uint32_t len;
//    char filename[16];
//};
class CIdleMsg {
public:
    enum {
        MSG_ID = IDLE_MSG
    };
    int id;
};

enum ClientAndMaster {
    GET_FILE_LOCATION = 2000,
    GET_SERVER_NUMBER,
    CREATE_FLOW_JOB,
    START_FLOW_REQUEST,
    EDIT_SCHEDULER,
    IS_MASTER_IDLE
};
//class CGetFileLocationMsg {
//public:
//    enum {
//        MSG_ID = GET_FILE_LOCATION
//    };
//    uint32_t ip;
//    uint16_t port;
//    uint32_t len;
//    char filename[16];
//};
class CGetServerNumberMsg {
public:
    enum {
        MSG_ID = GET_SERVER_NUMBER
    };
    int number;
};
class CCreateFlowJobMsg {
public:
    enum {
        MSG_ID = CREATE_FLOW_JOB
    };
    int targetServerId;
    int flowSize;
    int flowId;
};
class CStartFlowRequestMsg {
public:
    enum {
        MSG_ID = START_FLOW_REQUEST
    };
    int targetServerId;
    int flowSize;
    int flowId;
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

enum ClientAndSlave {
    GET_FLOW = 3000,
    PUSH_FLOW
};
class CGetFlowMsg {
public:
    enum {
        MSG_ID = GET_FLOW
    };
    int flowSize;
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