#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#include "platform/common.h"

typedef struct TcConnStats {
    SockStats_t connStats;
} TcConnStats_t;

typedef struct TcAppStats {
    uint64_t dbgNoFreeSession;  
} TcAppStats_t;

typedef struct TcGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;  
    SockAddr_t serverAddr;
    TcConnStats_t cStats;
} TcGroup_t;

typedef struct TcAppInt {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;
    uint32_t scDataLen;

    TcAppStats_t appStats;
    TcConnStats_t appConnStats;

    uint32_t csGroupCount;
    uint32_t nextCsGroupIndex;
    TcGroup_t* csGroupArr;
} TcAppInt_t;

void TcpClientRun(TcAppInt_t* appIface);

TcAppInt_t* CreateTcpClientInterface(int csGroupCount
                                            , int* clientAddrCounts);

void DeleteTcpClientInterface(TcAppInt_t* iFace);

void DumpTcpClientStats(TcConnStats_t* appConnStats);



#ifdef TCP_CLIENT_MAIN

typedef struct TcMethods {
    void (*InitSession) (void*);
} TcMethods_t;

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_ESTABLISH_FAILED        3
#define APP_STATE_CONNECTION_CLOSED                  4

typedef struct TcConn {
    SockState_t ccState; 
    int socketFd;
    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    SockAddr_t* localAddress;
    SockAddr_t* remoteAddress;
    LocalPortPool_t* localPortPool;
    struct TcSess* tcSess;
    uint32_t bytesSent;
} TcConn_t;

typedef struct TcAppConn {
    TcConn_t __tcConn;
} TcAppConn_t;


typedef struct TcSess {
    TcConn_t tcConn; 
    TcConnStats_t* groupConnStats;
} TcSess_t;

typedef struct TcAppSess {
    TcSess_t __tcSes;
} TcAppSess_t;

typedef struct TcpClient {

    TcConnStats_t* appGroupConnStats;

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    PollEvent_t* EventArr;
    int eventQ;
    int eventPTO;

    uint32_t errorSessionCount;
    TcSess_t* errorSessionArr;
    
    TimerWheel_t* timerWheel;
    
    char* sendBuffer; 
    char* readBuffer; 
} TcAppRun_t;

#endif

#endif