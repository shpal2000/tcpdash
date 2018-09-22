#ifndef __TCP_SERVER_APP_H
#define __TCP_SERVER_APP_H

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpServerAppOptions {
    uint32_t maxEvents;
    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
} TcpServerAppOptions_t;

typedef struct TcpServerConnStats {
    CommonConnStats_t connStats;
} TcpServerConnStats_t;

typedef struct TcpServerAppStats {
    uint64_t dbgNoFreeSession;
} TcpServerAppStats_t;

typedef struct TcpServerConnection{
    CommonConnState_t ccState; 
    int socketFd;
    struct TcpServerSession* tcSess;
} TcpServerConnection_t;

typedef struct TcpServerSession{
    TcpServerConnection_t tcConn; 
    TcpServerConnStats_t* groupConnStats;
} TcpServerSession_t;

typedef struct TcpServerApp {
    TcpServerAppOptions_t appOptions;
    
    TcpServerAppStats_t appStats;
    TcpServerConnStats_t appConnStats;
    TcpServerConnStats_t appGroupConnStats[1];

    SessionPool_t* freeSessionPool;
    SessionPool_t* activeSessionPool;

    struct epoll_event* EventArr;
    int eventQId;
    uint32_t errorSessionCount;
    TcpServerSession_t* errorSessionArr;
} TcpServerApp_t;

typedef struct TcpServerListener{
    CommonConnState_t ccState; 
    int socketFd;
} TcpServerListener_t;

#endif