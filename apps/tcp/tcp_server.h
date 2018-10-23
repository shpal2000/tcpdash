#ifndef __TCP_SERVER_APP_H
#define __TCP_SERVER_APP_H

#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"

typedef struct TcpServerConnStats {
    CommonConnStats_t connStats;
} TcpServerConnStats_t;

typedef struct TcpServerStats {
    uint64_t dbgNoFreeSession;  
} TcpServerStats_t;

typedef struct TcpServerConnGroup {
    SockAddr_t serverAddr;
    TcpServerConnStats_t cStats;
} TcpServerConnGroup_t;

typedef struct TcpServerInterface {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint32_t csDataLen;

    TcpServerStats_t appStats;
    TcpServerConnStats_t appConnStats;

    uint32_t csGroupCount;
    TcpServerConnGroup_t* csGroupArr;
} TcpServerInterface_t;

void TcpServerRun(TcpServerInterface_t* appIface);

TcpServerInterface_t* CreateTcpServerInterface(int csGroupCount);

void DeleteTcpServerInterface (TcpServerInterface_t* iFace);

void DumpTcpServerStats(TcpServerConnStats_t* appConnStats);

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
    SockAddr_t* localAddress;
    SockAddr_t remoteAddress;
    struct TcpServerSession* tcSess;
    uint32_t bytesReceived;
} TcpServerConnection_t;

typedef struct TcpServerSession{
    TcpServerConnection_t tcConn; 
    TcpServerConnStats_t* groupConnStats;
    int sessionType;
} TcpServerSession_t;

typedef struct TcpServer {

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    PollEvent_t* EventArr;
    int eventQ;

    uint32_t errorSessionCount;
    TcpServerSession_t* errorSessionArr;

    TcpServerSession_t* serverSessionArr;

    char* readBuffer;
} TcpServer_t;

#endif