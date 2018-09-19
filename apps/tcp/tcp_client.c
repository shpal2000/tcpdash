#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
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
    
    SSInit(aSession);

    aSession->socketFd = 0;
    aSession->isIpv6 = 0;
    aSession->localAddress = NULL;
    aSession->remoteAddress = NULL;
}

void SetSessionAddress(TcpClientSession_t* aSession
                        , int isIpv6
                        , struct sockaddr* localAddr
                        , struct sockaddr* remoteAddr) {

    aSession->isIpv6 = isIpv6;
    aSession->localAddress = localAddr;
    aSession->remoteAddress = remoteAddr;
}

TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* aSession = GetAnySesionFromPool (theApp.freeSessionPool);
    if (aSession != NULL) {
        AddToSessionPool (theApp.activeSessionPool, aSession);
        InitSession(aSession);
    }
    return aSession;
}

void SetFreeSession(TcpClientSession_t* aSession) {
    
    RemoveFromSessionPool (theApp.activeSessionPool, aSession);
    AddToSessionPool (theApp.freeSessionPool, aSession);
}

void StoreErrSession(TcpClientSession_t* aSession) {
    if (theApp.errorSessionCount < theApp.appOptions.maxErrorSessions) {

        TcpClientSessionE_t* errSession = &theApp.errorSessionArr[theApp.errorSessionCount];

        CopySS (errSession, aSession);

        //copy additional info
        errSession->socketFd = aSession->socketFd;
        
        theApp.errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (theApp.errorSessionCount) {
        for (int i = 0; i < theApp.errorSessionCount; i++) {
            TcpClientSessionE_t* eS = &theApp.errorSessionArr[i];
            printf ("SS1 = %#018" PRIx64 ", Err = %d, SysErr = %d\n"
                    , GetSS1(eS), GetSSLastErr(eS), GetErrno(eS) ); 
        }
    }
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

    theApp.errorSessionCount = 0;
    theApp.errorSessionArr = CreateSessionArray (TcpClientSessionE_t
                                , theApp.appOptions.maxErrorSessions); 

    theApp.EventArr = CreateEventArray(theApp.appOptions.maxEvents);

    memset(&theApp.appStats, 0, sizeof (TcpClientStats_t));

    theApp.eventQId = CreateEventQ();
}

void CleanupApp() {
    TcpClientStats_t* appStats = &theApp.appStats;

    DeleteEventQ(theApp.eventQId);

    printf ("%" PRIu64 ", %" PRIu64 ", %" PRIu64 "\n"
                , appStats->connectionAttempt
                , appStats->connectionSuccess
                , appStats->connectionFail);

    DumpSessionStats(appStats);

    DumpErrSessions();
}

int main(int argc, char** argv)
{
    TcpClientAppOptions_t appOptions = {  .maxEvents = 1000
                                        , .maxActiveSessions = 1000
                                        , .maxErrorSessions = 20
                                        , .maxSessions = 1000 };
    InitApp(&appOptions);
    TcpClientStats_t* appStats = &theApp.appStats;

    while (true) {

        if ( (appStats->connectionFail == appOptions.maxErrorSessions) 
                || (appStats->connectionAttempt == appOptions.maxSessions 
                        && IsSessionPoolEmpty (theApp.activeSessionPool)) ){
            break;
        }

        if (appStats->connectionAttempt < appOptions.maxSessions) {
            
            TcpClientSession_t* newSession = GetFreeSession ();
            if (newSession == NULL) {
                appStats->dbgNoFreeSession++;      
            }else {
                //hard coded
                char* srcIp = "10.116.6.4";
                char* dstIp = "10.116.0.61";
                struct sockaddr_in localAddr;
                struct sockaddr_in remoteAddr;
                memset(&localAddr, 0, sizeof(localAddr));
                localAddr.sin_family = AF_INET;
                inet_pton(AF_INET, srcIp, &(localAddr.sin_addr));
                memset(&remoteAddr, 0, sizeof(remoteAddr));
                remoteAddr.sin_family = AF_INET;
                remoteAddr.sin_port = htons(80);
                inet_pton(AF_INET, dstIp, &(remoteAddr.sin_addr));
                //hard coded

                SetSessionAddress(newSession, 0
                                    , (struct sockaddr*) &localAddr
                                    , (struct sockaddr*) &remoteAddr);
                                    
                SetAppState (newSession, APP_STATE_CONNECTION_IN_PROGRESS);
                appStats->connectionAttempt++;
                newSession->socketFd = TcpNewConnection(newSession->isIpv6, 
                                                        newSession->localAddress, 
                                                        newSession->remoteAddress,
                                                        newSession,
                                                        appStats);
                if ( GetSSLastErr (newSession) ) {
                    appStats->connectionFail++;
                    StoreErrSession(newSession);
                    SetFreeSession (newSession);
                } else {
                    RegisterForWriteEvent(theApp.eventQId, newSession->socketFd, newSession);
                }
            }
        }

        int eCount = GetIOEvents(theApp.eventQId, theApp.EventArr
                                    , theApp.appOptions.maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientSession_t* eSession = 
                    (TcpClientSession_t*) GetIOEventData(theApp.EventArr[eIndex]);

                if (IsWriteEventSet(theApp.EventArr[eIndex])) {
                    if ( GetAppState(eSession) == APP_STATE_CONNECTION_IN_PROGRESS ) {
                        if ( IsNewTcpConnectionComplete(eSession->socketFd) ){
                            appStats->connectionSuccess += 1;
                            SetAppState (eSession, APP_STATE_CONNECTION_ESTABLISHED);
                            SetSS1(eSession, STATE_TCP_CONN_ESTABLISHED);
                            close(eSession->socketFd);
                            SetSS1(eSession, STATE_TCP_SOCK_FD_CLOSE);
                            SetFreeSession (eSession);
                            SetAppState (eSession, APP_STATE_CONNECTION_CLOSED);
                        }else{
                            appStats->connectionFail += 1;
                            close(eSession->socketFd);
                            SetSS1(eSession, STATE_TCP_SOCK_FD_CLOSE);
                            SetFreeSession (eSession);
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