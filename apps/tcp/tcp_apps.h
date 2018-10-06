#ifndef __TCP_APPS_H
#define __TCP_APPS_H

typedef struct TcpClientAppConnGroup {
    uint32_t clientAddrCount;
    struct sockaddr* clientAddArr;
    struct sockaddr serverAddr;
} TcpClientAppConnGroup_t;

typedef struct TcpClientAppOptions {
    u_int32_t isRunning;
    uint32_t maxEvents;
    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;
    uint32_t csGroupCount;
    TcpClientAppConnGroup_t csGroupArr[1];
} TcpClientAppOptions_t;

void TcpClientRun(TcpClientAppOptions_t* appOptions);

#endif