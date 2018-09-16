#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"
#include "tcp_client.h"


void InitApp(TcpClientApp_t* theApp, TcpClientAppOptions_t* options) {

    theApp->appOptions = *options;

    theApp->freeSessionPool = AllocEmptySessionPool();
    theApp->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < theApp->appOptions.maxActiveSessions; i++) {
        TcpClientSession_t* aSession = AllocAppSession (TcpClientSession_t);
        InitAppSession (aSession);
        AddToSessionPool (theApp->freeSessionPool, aSession);
    }

    theApp->EventArray = CreateEventArray(theApp->appOptions.maxEvents);

    memset(&theApp->appStats, 0, sizeof (TcpClientStats_t)); 
}

void CleanupApp(TcpClientApp_t* theApp) {
    //todo
}

TcpClientSession_t* AllocSession(TcpClientApp_t* theApp) {

    TcpClientSession_t* aSession = GetAnySesionFromPool (theApp->freeSessionPool);
    AddToSessionPool (theApp->activeSessionPool, aSession);
    InitAppSession(aSession);
    return aSession;
}

void FreeSession(TcpClientApp_t* theApp
                                , TcpClientSession_t* aSession) {
    
    RemoveFromSessionPool (theApp->activeSessionPool, aSession);
    AddToSessionPool (theApp->freeSessionPool, aSession);
}

void InitAppSession(TcpClientSession_t* aSession) {
    
    TdSSInit(&aSession->sState);

    aSession->socketFd = 0;
    aSession->isIpv6 = 0;
    aSession->localAddress = NULL;
    aSession->remoteAddress = NULL;
    aSession->appState = APP_STATE_INIT;
}

int InitiateConnection(TcpClientSession_t* aSession) {

    aSession->socketFd = TcpNewConnection(aSession->isIpv6, 
                                            aSession->localAddress, 
                                            aSession->remoteAddress,
                                            &aSession->sState);

    return TdGetSSLastErr (&aSession->sState);    
}

int TcpClientAppRun() {
    
    TcpClientApp_t tcpClientApp;
    TcpClientAppOptions_t tcpClientAppOptions = { .maxEvents = 1000
                                                , .maxActiveSessions = 100 };
    InitApp(&tcpClientApp, &tcpClientAppOptions);
    TcpClientStats_t* tcpClientStats = &tcpClientApp.appStats;


    //hard coded
    struct sockaddr_in localAddr;
    struct sockaddr_in remoteAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "10.116.0.61", &(localAddr.sin_addr));
    memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(80);
    inet_pton(AF_INET, "172.217.6.78", &(remoteAddr.sin_addr));

    int epfd = epoll_create(1);

    while (tcpClientStats->connectionAttempt < tcpClientAppOptions.maxActiveSessions 
                || tcpClientStats->connectionSuccess + tcpClientStats->connectionFail < tcpClientAppOptions.maxActiveSessions) {


        if (tcpClientStats->connectionAttempt < tcpClientAppOptions.maxActiveSessions) {
            tcpClientStats->connectionAttempt += 1;
            TcpClientSession_t* newSession = AllocSession(&tcpClientApp);

            SetSessionAddress(newSession, 0
                                , (struct sockaddr*) &localAddr
                                , (struct sockaddr*) &remoteAddr);

            newSession->appState = APP_STATE_CONNECTION_IN_PROGRESS;
            
            if (InitiateConnection(newSession)){
                tcpClientStats->connectionFail += 1;
                FreeSession (&tcpClientApp, newSession);
            }else{
                struct epoll_event setEvent;
                setEvent.events = EPOLLOUT;
                setEvent.data.ptr = newSession;
                epoll_ctl(epfd, EPOLL_CTL_ADD, newSession->socketFd, &setEvent);
            }
        }

        int eventReadyCount = epoll_wait(epfd, tcpClientApp.EventArray
                                            , tcpClientApp.appOptions.maxEvents
                                            , 0);
        if (eventReadyCount > 0){
            for(int eventIndex = 0; eventIndex < eventReadyCount; eventIndex++) {

                struct epoll_event readyEvent = tcpClientApp.EventArray[eventIndex];
                TcpClientSession_t* eventSession = (TcpClientSession_t*) readyEvent.data.ptr;

                if (readyEvent.events && EPOLLOUT) {
                    if (eventSession->appState == APP_STATE_CONNECTION_IN_PROGRESS) {
                        int socketErr;
                        socklen_t socketErrBufLen = sizeof(int);
                        int retGetsockopt = getsockopt(eventSession->socketFd
                                                , SOL_SOCKET
                                                , SO_ERROR
                                                , &socketErr
                                                , &socketErrBufLen);
                        if ((retGetsockopt|socketErr) == 0){
                            tcpClientStats->connectionSuccess += 1;
                            eventSession->appState = APP_STATE_CONNECTION_ESTABLISHED;
                            TdSetSS1(&eventSession->sState
                                                , STATE_TCP_CONN_ESTABLISHED);
                        }else{
                           tcpClientStats->connectionFail += 1; 
                        }
                    }
                }

            }
        }
    }

    return 0;   
}

int main(int argc, char** argv)
{
    TcpClientAppRun();
    
    return 0;
}