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
} TcpClientApp_t;

typedef struct TcpClientSession{
    int socketFd;
    int isIpv6;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
    TdSS_t sState;
    int appState;
} TcpClientSession_t;

void InitApp(TcpClientApp_t* theApp, TcpClientAppOptions_t* options);
void CleanupApp(TcpClientApp_t* theApp);

TcpClientSession_t* GetFromFreeSessionPool(TcpClientApp_t* theApp);
void ReturnToFreeSessionPool(TcpClientApp_t* theApp, TcpClientSession_t* aSession);

void InitSession(TcpClientSession_t* aSession);

inline static void SetSessionAddress(TcpClientSession_t* aSession
                                    , int isIpv6
                                    , struct sockaddr* localAddr
                                    , struct sockaddr* remoteAddr) {

    aSession->isIpv6 = isIpv6;
    aSession->localAddress = localAddr;
    aSession->remoteAddress = remoteAddr;
}

int InitiateConnection(TcpClientSession_t* aSession);

#endif