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

TcpClientApp_t theApp;

void InitSession(TcpClientSession_t* aSession) {
    
    SSInit(aSession, TcpClientAppId);

    aSession->socketFd = 0;
    aSession->isIpv6 = 0;
    aSession->localAddress = NULL;
    aSession->remoteAddress = NULL;
    aSession->appState = APP_STATE_INIT;
}

void SetSessionAddress(TcpClientSession_t* aSession
                        , int isIpv6
                        , struct sockaddr* localAddr
                        , struct sockaddr* remoteAddr) {

    aSession->isIpv6 = isIpv6;
    aSession->localAddress = localAddr;
    aSession->remoteAddress = remoteAddr;
}

TcpClientSession_t* GetSession() {

    TcpClientSession_t* aSession = GetAnySesionFromPool (theApp.freeSessionPool);
    AddToSessionPool (theApp.activeSessionPool, aSession);
    InitSession(aSession);
    return aSession;
}

void ReturnSession(TcpClientSession_t* aSession) {
    
    RemoveFromSessionPool (theApp.activeSessionPool, aSession);
    AddToSessionPool (theApp.freeSessionPool, aSession);
}

int InitiateConnection(TcpClientSession_t* aSession) {

    aSession->socketFd = TcpNewConnection(aSession->isIpv6, 
                                            aSession->localAddress, 
                                            aSession->remoteAddress,
                                            aSession);

    return GetSSLastErr (aSession);    
}

void InitApp(TcpClientAppOptions_t* options) {

    theApp.appOptions = *options;

    theApp.freeSessionPool = AllocEmptySessionPool();
    theApp.activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < theApp.appOptions.maxActiveSessions; i++) {
        TcpClientSession_t* aSession = AllocSession (TcpClientSession_t);
        InitSession (aSession);
        AddToSessionPool (theApp.freeSessionPool, aSession);
    }

    theApp.EventArr = CreateEventArray(theApp.appOptions.maxEvents);

    memset(&theApp.appStats, 0, sizeof (TcpClientStats_t));

    theApp.eventQId = CreateEventQ();
}

void CleanupApp() {
    DeleteEventQ(theApp.eventQId);
}

int main(int argc, char** argv)
{
    TcpClientAppOptions_t appOptions = {  .maxEvents = 1000
                                        , .maxActiveSessions = 100 };
    InitApp(&appOptions);
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

    while (appStats->connectionAttempt < appOptions.maxActiveSessions 
            || appStats->connectionSuccess + appStats->connectionFail 
                    < appOptions.maxActiveSessions) {


        if (appStats->connectionAttempt < appOptions.maxActiveSessions) {
            appStats->connectionAttempt += 1;
            TcpClientSession_t* newSession = GetSession ();

            SetSessionAddress(newSession, 0
                                , (struct sockaddr*) &localAddr
                                , (struct sockaddr*) &remoteAddr);

            newSession->appState = APP_STATE_CONNECTION_IN_PROGRESS;
            
            if (InitiateConnection(newSession)){
                appStats->connectionFail += 1;
                ReturnSession (newSession);
            }else{
                RegisterForWriteEvent(theApp.eventQId, newSession->socketFd, newSession);
            }
        }

        int eCount = GetIOEvents(theApp.eventQId, theApp.EventArr
                                    , theApp.appOptions.maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientSession_t* eSession = 
                    (TcpClientSession_t*) GetIOEventData(theApp.EventArr[eIndex]);

                if (IsWriteEventBitSet(theApp.EventArr[eIndex])) {
                    if (eSession->appState == APP_STATE_CONNECTION_IN_PROGRESS) {
                        if ( IsNewTcpConnectionComplete(eSession->socketFd) ){
                            appStats->connectionSuccess += 1;
                            eSession->appState = APP_STATE_CONNECTION_ESTABLISHED;
                            SetSS1(eSession, STATE_TCP_CONN_ESTABLISHED);
                        }else{
                            appStats->connectionFail += 1;
                            SetSS1(eSession, STATE_TCP_SOCK_FD_CLOSE);
                            //to capture the error ?
                        }
                    }
                }
            }
        }
    }

    CleanupApp();

    return 0;
}