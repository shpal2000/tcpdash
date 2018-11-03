#ifndef __TCP_SERVER_APP_H
#define __TCP_SERVER_APP_H

#include "socks/libsock.h"
#include "logging/logs.h"

typedef struct TsConnStats {
    ConnStats_t connStats;
} TsConnStats_t;

typedef struct TsAppStats {
    uint64_t dbgNoFreeSession;  
} TsAppStats_t;

typedef struct TsGroup {
    SockAddr_t serverAddr;
    TsConnStats_t cStats;
} TsGroup_t;

typedef struct TsAppInt {
    uint32_t isRunning;
    uint32_t maxEvents;
    uint32_t listenQLen;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint32_t csDataLen;
    uint32_t scDataLen;

    TsAppStats_t appStats;
    TsConnStats_t appConnStats;

    uint32_t csGroupCount;
    TsGroup_t* csGroupArr;
} TsAppInt_t;

void TcpServerRun(TsAppInt_t* appIface);

TsAppInt_t* CreateTcpServerInterface(int csGroupCount);

void DeleteTcpServerInterface (TsAppInt_t* iFace);

void DumpTcpServerStats(TsConnStats_t* appConnStats);

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

#define SESSION_TYPE_LISTENER                       1
#define SESSION_TYPE_CONNECTION                     2

typedef struct TcpServerConnection{
    ConnState_t ccState; 
    int socketFd;
    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    SockAddr_t* localAddress;
    SockAddr_t remoteAddress;
    struct TsSess* tcSess;
    uint32_t bytesReceived;
} TsConn_t;

typedef struct TsSess {
    TsConn_t tcConn; 
    TsConnStats_t* groupConnStats;
    int sessionType;
} TsSess_t;

typedef struct TsAppRun {

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    PollEvent_t* EventArr;
    int eventQ;

    uint32_t errorSessionCount;
    TsSess_t* errorSessionArr;

    TsSess_t* serverSessionArr;

    char* sendBuffer;
    char* readBuffer;
} TsAppRun_t;

#endif