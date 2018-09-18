#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpClientAppOptions {
    uint32_t maxEvents;
    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
} TcpClientAppOptions_t;

typedef struct TcpClientStats {
    uint64_t connectionAttempt;
    uint64_t connectionSuccess;
    uint64_t connectionFail;
    uint64_t connectionProgress;
} TcpClientStats_t;

typedef struct TcpClientApp {
    TcpClientAppOptions_t appOptions;
    TcpClientStats_t appStats;
    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;
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