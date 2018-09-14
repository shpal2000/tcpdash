#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <gmodule.h>

#include "sessions/sstates.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"
#include "tcp_client.h"


void AppInit(TcpClientApp* theApp, TcpClientAppOptions* options) {

    theApp->appOptions = *options;

    theApp->freeSessionPool = g_queue_new();
    theApp->activeSessionPool = g_queue_new();

    for (int i = 0; i < theApp->appOptions.maxActiveSessions; i++) {
        TcpClientSession* aSession = g_slice_new (TcpClientSession);
        SessionInit(aSession);
        g_queue_push_tail (theApp->freeSessionPool, aSession);
    }

    theApp->EventArray = g_new (struct epoll_event, theApp->appOptions.maxEvents);
}

void AppCleanup(struct TcpClientApp* theApp) {
    //todo
}

TcpClientSession* GetFromFreeSessionPool(TcpClientApp* theApp) {

    struct TcpClientSession* aSession = g_queue_pop_head (theApp->freeSessionPool);
    SessionInit(aSession);
    g_queue_push_tail (theApp->activeSessionPool, aSession);
    return aSession;
}

void ReturnToFreeSessionPool(TcpClientApp* theApp
                                , TcpClientSession* aSession) {
    
    g_queue_remove (theApp->activeSessionPool, aSession);
    g_queue_push_tail (theApp->freeSessionPool, aSession);
}

void SessionInit(TcpClientSession* aSession) {
    
    TDSessionSateInit(&aSession->sState);

    aSession->socketFd = 0;
    aSession->isIpv6 = 0;
    aSession->localAddress = NULL;
    aSession->remoteAddress = NULL;
    aSession->appState = APP_STATE_INIT;
}

int InitiateConnection(TcpClientSession* aSession) {

    aSession->socketFd = TcpNewConnection(aSession->isIpv6, 
                                            aSession->localAddress, 
                                            aSession->remoteAddress,
                                            &aSession->sState);

    return TDGetSessionStateLastErr (&aSession->sState);    
}

int TcpClientAppRun() {
    
    TcpClientApp tcpClientApp;
    TcpClientAppOptions tcpClientAppOptions = { .maxEvents = 1000
                                                , .maxActiveSessions = 100 };
    AppInit(&tcpClientApp, &tcpClientAppOptions);


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

    int connectionAttempt = 0;
    int connectionSuccess = 0;
    int connectionFail = 0;
    int epfd = epoll_create(1);

    while (connectionAttempt < tcpClientAppOptions.maxActiveSessions 
                || connectionSuccess + connectionFail < tcpClientAppOptions.maxActiveSessions) {


        if (connectionAttempt < tcpClientAppOptions.maxActiveSessions) {
            connectionAttempt += 1;
            struct TcpClientSession* newSession = GetFromFreeSessionPool(&tcpClientApp);

            SetSessionAddress(newSession, 0
                                , (struct sockaddr*) &localAddr
                                , (struct sockaddr*) &remoteAddr);

            newSession->appState = APP_STATE_CONNECTION_IN_PROGRESS;
            
            if (InitiateConnection(newSession)){
                connectionFail += 1;
                ReturnToFreeSessionPool (&tcpClientApp, newSession);
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
                TcpClientSession* eventSession = (TcpClientSession*) readyEvent.data.ptr;

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
                            connectionSuccess += 1;
                            eventSession->appState = APP_STATE_CONNECTION_ESTABLISHED;
                            TDSetSessionState1(&eventSession->sState
                                                , STATE_TCP_CONN_ESTABLISHED);
                        }else{
                           connectionFail += 1; 
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