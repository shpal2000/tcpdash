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
    SessionStats_t _sS;

    uint64_t connectionAttempt;
    uint64_t connectionSuccess;
    uint64_t connectionFail;
    uint64_t connectionProgress;

    uint64_t dbgNoFreeSession;
} TcpClientStats_t;

typedef struct TcpClientSession{
    SessionState_t _sS; 
    int socketFd;
    int isIpv6;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
} TcpClientSession_t;

typedef struct TcpClientSessionE{
    SessionState_t _sS; 
    int socketFd;
} TcpClientSessionE_t;

typedef struct TcpClientApp {
    TcpClientAppOptions_t appOptions;
    TcpClientStats_t appStats;
    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;
    struct epoll_event* EventArr;
    int eventQId;
    uint32_t errorSessionCount;
    TcpClientSessionE_t* errorSessionArr;
} TcpClientApp_t;

#endif