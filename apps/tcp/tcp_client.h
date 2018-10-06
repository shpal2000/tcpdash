#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#include "tcp_apps.h"

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpClientConnStats {
    CommonConnStats_t connStats;
} TcpClientConnStats_t;

typedef struct TcpClientAppStats {
    uint64_t dbgNoFreeSession;
} TcpClientAppStats_t;

typedef struct TcpClientConnection{
    CommonConnState_t ccState; 
    int socketFd;
    int isIpv6;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
    struct TcpClientSession* tcSess;
    uint32_t bytesSent;
} TcpClientConnection_t;

typedef struct TcpClientSession{
    TcpClientConnection_t tcConn; 
    TcpClientConnStats_t* groupConnStats;
} TcpClientSession_t;

typedef struct TcpClientApp {

    TcpClientAppStats_t* appStats;
    TcpClientConnStats_t* appConnStats;
    TcpClientConnStats_t* appGroupConnStats;

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    struct epoll_event* EventArr;
    int eventQId;
    uint32_t errorSessionCount;
    TcpClientSession_t* errorSessionArr;
    TimerWheel_t* timerWheel;
    char* sendBuffer; 
} TcpClientApp_t;

#endif