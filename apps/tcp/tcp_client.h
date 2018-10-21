#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"

typedef struct TcpClientAppConnStats {
    CommonConnStats_t connStats;
} TcpClientAppConnStats_t;

typedef struct TcpClientAppStats {
    uint64_t dbgNoFreeSession;  
} TcpClientAppStats_t;

typedef struct TcpClientAppConnGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;  
    SockAddr_t serverAddr;
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
    uint32_t nextCsGroupIndex;
    TcpClientAppConnGroup_t* csGroupArr;
} TcpClientAppInterface_t;

void TcpClientAppRun(TcpClientAppInterface_t* appIface);

TcpClientAppInterface_t* CreateTcpClienAppInterface(int csGroupCount
                                            , int* clientAddrCounts);

void DeleteTcpClienAppInterface(TcpClientAppInterface_t* iFace);

void DumpTcpClientAppStats(TcpClientAppConnStats_t* appConnStats);

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpClientConnection{
    CommonConnState_t ccState; 
    int socketFd;
    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
    LocalPortPool_t* localPortPool;
    struct TcpClientSession* tcSess;
    uint32_t bytesSent;
} TcpClientConnection_t;

typedef struct TcpClientSession{
    TcpClientConnection_t tcConn; 
    TcpClientAppConnStats_t* groupConnStats;
} TcpClientSession_t;

typedef struct TcpClientApp {

    TcpClientAppConnStats_t* appGroupConnStats;

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    struct epoll_event* EventArr;
    int eventQ;

    uint32_t errorSessionCount;
    TcpClientSession_t* errorSessionArr;
    
    TimerWheel_t* timerWheel;
    
    char* sendBuffer; 
} TcpClientApp_t;

#endif