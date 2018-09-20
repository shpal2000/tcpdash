#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpServerAppOptions {
    uint32_t maxEvents;
    uint32_t maxErrorSessions;
    uint32_t maxActiveSessions;
    uint32_t listenQueueLen;
} TcpServerAppOptions_t;

typedef struct TcpServerStats {
    SessionStats_t _sS;
    uint64_t connectionSuccess;
    uint64_t dbgNoFreeSession;
} TcpServerStats_t;

typedef struct TcpServerSession{
    SessionState_t _sS; 
    int socketFd;
    int isIpv6;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
} TcpServerSession_t;

typedef struct TcpServerSessionE{
    SessionState_t _sS; 
    int socketFd;
} TcpServerSessionE_t;

typedef struct TcpServerApp {
    TcpServerAppOptions_t appOptions;
    TcpServerStats_t appStats;
    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;
    struct epoll_event* EventArr;
    int eventQId;
    uint32_t errorSessionCount;
    TcpServerSessionE_t* errorSessionArr;
} TcpServerApp_t;

#endif