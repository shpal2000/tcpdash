#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#define APP_STATE_INIT                               0
#define APP_STATE_CONNECTION_IN_PROGRESS             1
#define APP_STATE_CONNECTION_ESTABLISHED             2
#define APP_STATE_CONNECTION_CLOSED                  3

typedef struct TcpClientAppOptions {
    int maxEvents;
    int maxActiveSessions;
} TcpClientAppOptions;

typedef struct TcpClientApp {
    struct TcpClientAppOptions appOptions;
    GQueue* freeSessionPool;
    GQueue* activeSessionPool;
    struct epoll_event* EventArray;
} TcpClientApp;

typedef struct TcpClientSession{
    int socketFd;
    int isIpv6;
    struct sockaddr* localAddress;
    struct sockaddr* remoteAddress;
    struct TDSessionState sState;
    int appState;
} TcpClientSession;

void AppInit(TcpClientApp* theApp, TcpClientAppOptions* options);
void AppCleanup(TcpClientApp* theApp);

TcpClientSession* GetFromFreeSessionPool(TcpClientApp* theApp);
void ReturnToFreeSessionPool(TcpClientApp* theApp, TcpClientSession* aSession);

void SessionInit(TcpClientSession* aSession);

inline static void SetSessionAddress(TcpClientSession* aSession
                                    , int isIpv6
                                    , struct sockaddr* localAddr
                                    , struct sockaddr* remoteAddr) {

    aSession->isIpv6 = isIpv6;
    aSession->localAddress = localAddr;
    aSession->remoteAddress = remoteAddr;
}

int InitiateConnection(TcpClientSession* aSession);

#endif