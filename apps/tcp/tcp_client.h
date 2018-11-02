#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"

typedef struct TcpClientConnStats {
    CommonConnStats_t connStats;
} TcpClientConnStats_t;

typedef struct TcpClientStats {
    uint64_t dbgNoFreeSession;  
} TcpClientStats_t;

typedef struct TcpClientConnGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;  
    SockAddr_t serverAddr;
    TcpClientConnStats_t cStats;
} TcpClientConnGroup_t;

typedef struct TcpClientInterface {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;
    uint32_t scDataLen;

    TcpClientStats_t appStats;
    TcpClientConnStats_t appConnStats;

    uint32_t csGroupCount;
    uint32_t nextCsGroupIndex;
    TcpClientConnGroup_t* csGroupArr;
} TcpClientInterface_t;

void TcpClientRun(TcpClientInterface_t* appIface);

TcpClientInterface_t* CreateTcpClientInterface(int csGroupCount
                                            , int* clientAddrCounts);

void DeleteTcpClientInterface(TcpClientInterface_t* iFace);

void DumpTcpClientStats(TcpClientConnStats_t* appConnStats);

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

#define MAX_POLL_TIMEOUT 100

typedef struct TcpClientConnection{
    CommonConnState_t ccState; 
    int socketFd;
    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    SockAddr_t* localAddress;
    SockAddr_t* remoteAddress;
    LocalPortPool_t* localPortPool;
    struct TcpClientSession* tcSess;
    uint32_t bytesSent;
} TcpClientConnection_t;

typedef struct TcpClientSession{
    TcpClientConnection_t tcConn; 
    TcpClientConnStats_t* groupConnStats;
} TcpClientSession_t;

typedef struct TcpClient {

    TcpClientConnStats_t* appGroupConnStats;

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    PollEvent_t* EventArr;
    int eventQ;
    int eventPTO;

    uint32_t errorSessionCount;
    TcpClientSession_t* errorSessionArr;
    
    TimerWheel_t* timerWheel;
    
    char* sendBuffer; 
    char* readBuffer; 
} TcpClient_t;

#endif