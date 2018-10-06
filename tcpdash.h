#ifndef __TCP_DASH_H
#define __TCP_DASH_H

#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"

/*###############################################################
##################### TCPClientAPP Params #######################
###############################################################*/
typedef struct TcpClientAppConnStats {
    CommonConnStats_t connStats;
} TcpClientAppConnStats_t;

typedef struct TcpClientAppStats {
    uint64_t dbgNoFreeSession;
} TcpClientAppStats_t;

typedef struct TcpClientAppConnGroup {
    uint32_t clientAddrCount;
    struct sockaddr* clientAddrArr;
    struct sockaddr serverAddr;
    TcpClientAppConnStats_t cStats;
} TcpClientAppConnGroup_t;

typedef struct TcpClientAppInterface {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;

    TcpClientAppStats_t appStats;
    TcpClientAppConnStats_t appConnStats;

    uint32_t csGroupCount;
    TcpClientAppConnGroup_t* csGroupArr;
} TcpClientAppInterface_t;

void TcpClienAppRun(TcpClientAppInterface_t* appIface);

/*###############################################################
##################### TCPServerAPP Params #######################
###############################################################*/



#endif