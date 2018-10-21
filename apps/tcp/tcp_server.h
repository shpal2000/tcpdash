#ifndef __TCP_SERVER_APP_H
#define __TCP_SERVER_APP_H

#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"

typedef struct TcpServerAppConnStats {
    CommonConnStats_t connStats;
} TcpServerAppConnStats_t;

typedef struct TcpServerAppStats {
    uint64_t dbgNoFreeSession;  
} TcpServerAppStats_t;

typedef struct TcpServerAppConnGroup {
    SockAddr_t serverAddr;
    TcpServerAppConnStats_t cStats;
} TcpServerAppConnGroup_t;

typedef struct TcpServerAppInterface {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;

    TcpServerAppStats_t appStats;
    TcpServerAppConnStats_t appConnStats;

    uint32_t csGroupCount;
    TcpServerAppConnGroup_t* csGroupArr;
} TcpServerAppInterface_t;

void TcpServerAppRun(TcpServertAppInterface_t* appIface);

TcpServerAppInterface_t* CreateTcpServerAppInterface(int csGroupCount);

void DumpTcpServerAppStats(TcpServerAppConnStats_t* appConnStats);

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

#define SESSION_TYPE_LISTENER                       1
#define SESSION_TYPE_CONNECTION                     2

typedef struct TcpServerConnection{
    CommonConnState_t ccState; 
    int socketFd;
    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
    struct TcpServerSession* tcSess;
    uint32_t bytesReceived;
} TcpServerConnection_t;

typedef struct TcpServerSession{
    TcpServerConnection_t tcConn; 
    TcpServerAppConnStats_t* groupConnStats;
    int sessionType;
} TcpServerSession_t;

typedef struct TcpServerApp {

    TcpServerAppConnStats_t* appGroupConnStats;

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    struct epoll_event* EventArr;
    int eventQ;

    uint32_t errorSessionCount;
    TcpServerSession_t* errorSessionArr;

    TcpServerSession_t* serverSessionArr;

    char* readBuffer;
} TcpServerApp_t;

#endif