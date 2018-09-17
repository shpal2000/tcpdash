#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpClientAppOptions {
    uint32_t maxEvents;
    uint32_t maxActiveSessions;
} TcpClientAppOptions_t;

typedef struct TcpClientStats {
    uint64_t connectionAttempt;
    uint64_t connectionSuccess;
    uint64_t connectionFail;
} TcpClientStats_t;

typedef struct TcpClientApp {
    TcpClientAppOptions_t appOptions;
    TcpClientStats_t appStats;
    GQueue* freeSessionPool;
    GQueue* activeSessionPool;
    struct epoll_event* EventArr;
    int eventQId;
} TcpClientApp_t;

typedef struct TcpClientSession{
    SessionState_t _sS; 
    
    int socketFd;
    int isIpv6;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
    int appState;
} TcpClientSession_t;

#endif