#ifndef __TCP_CLIENT_SERVER_APP_H

#include "app/common.h"
#include "platform/common.h"

typedef struct TcpClientServerStats {
    SockStats_t connStats;
} TcpClientServerStats_t;

typedef struct TcpClientServerGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;
    SockAddr_t serverAddr;
    uint32_t csDataLen;
    uint32_t scDataLen;
    enum ConnCloseMethod cCloseMethod;
    enum ConnCloseMethod sCloseMethod;
    enum ConnCloseType connCloseType;
    uint32_t cWeight;
    TcpClientServerStats_t cStats;
} TcpClientServerGroup_t;

typedef struct TcpClientServerI {
    uint32_t isRunning;
    uint32_t maxEvents;
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpProxyServer_t* csGroupArr;
    TcpClientServerStats_t gStats;
} TcpClientServerI_t;



#define __TCP_CLIENT_SERVER_APP_H
#endif