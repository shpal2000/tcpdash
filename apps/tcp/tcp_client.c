#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

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

    theApp->EventArr = CreateEventArray(theApp->appOptions.maxEvents);

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
    
    TcpClientApp_t theApp;
    TcpClientAppOptions_t appOptions = {  .maxEvents = 1000
                                        , .maxActiveSessions = 100 };
    InitApp(&theApp, &appOptions);
    TcpClientStats_t* appStats = &theApp.appStats;


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

    int eventQId = CreateEventQ();

    while (appStats->connectionAttempt < appOptions.maxActiveSessions 
            || appStats->connectionSuccess + appStats->connectionFail 
                    < appOptions.maxActiveSessions) {


        if (appStats->connectionAttempt < appOptions.maxActiveSessions) {
            appStats->connectionAttempt += 1;
            TcpClientSession_t* newSession = AllocSession(&theApp);

            SetSessionAddress(newSession, 0
                                , (struct sockaddr*) &localAddr
                                , (struct sockaddr*) &remoteAddr);

            newSession->appState = APP_STATE_CONNECTION_IN_PROGRESS;
            
            if (InitiateConnection(newSession)){
                appStats->connectionFail += 1;
                FreeSession (&theApp, newSession);
            }else{
                RegisterForWriteEvent(eventQId, newSession->socketFd, newSession);
            }
        }

        int eCount = GetIOEvents(eventQId, theApp.EventArr
                                , theApp.appOptions.maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientSession_t* eSession = 
                    (TcpClientSession_t*) GetIOEventData(theApp.EventArr[eIndex]);

                if (IsWriteEvent(theApp.EventArr[eIndex])) {
                    if (eSession->appState == APP_STATE_CONNECTION_IN_PROGRESS) {
                        if ( IsTcpConnectionComplete(eSession->socketFd) ){
                            appStats->connectionSuccess += 1;
                            eSession->appState = APP_STATE_CONNECTION_ESTABLISHED;
                            TdSetSS1(&eSession->sState, STATE_TCP_CONN_ESTABLISHED);
                        }else{
                            appStats->connectionFail += 1; 
                        }
                    }
                }
            }
        }
    }

    DeleteEventQ(eventQId);

    return 0;   
}

int main(int argc, char** argv)
{
    TcpClientAppRun();
    
    return 0;
}