#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#include "tcpdash.h"

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

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
    TcpClientAppConnStats_t* groupConnStats;
} TcpClientSession_t;

typedef struct TcpClientApp {

    TcpClientAppConnStats_t* appGroupConnStats;

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