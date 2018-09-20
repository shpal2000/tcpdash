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

void InitSession(TcpClientConnection_t* aSession) {
    
    SSInit(aSession);

    aSession->socketFd = 0;
    aSession->isIpv6 = 0;
    aSession->localAddress = NULL;
    aSession->remoteAddress = NULL;
}

void SetSessionAddress(TcpClientConnection_t* aSession
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

        TcpClientSession_t* errSession = &theApp.errorSessionArr[theApp.errorSessionCount];
        *errSession =  *aSession;
        theApp.errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (theApp.errorSessionCount) {
        for (int i = 0; i < theApp.errorSessionCount; i++) {
            TcpClientSession_t* eS = &theApp.errorSessionArr[i];
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
    theApp.errorSessionArr = CreateSessionArray (TcpClientSession_t
                                , theApp.appOptions.maxErrorSessions); 

    theApp.EventArr = CreateEventArray(theApp.appOptions.maxEvents);

    memset(&theApp.appConnStats, 0, sizeof (TcpClientConnStats_t));

    theApp.eventQId = CreateEventQ();
}

void CleanupApp() {
    //TcpClientConnStats_t* appConnStats = &theApp.appConnStats;
    CommonConnStats_t* appCommonConnStats = (CommonConnStats_t*) &theApp.appConnStats;

    DeleteEventQ(theApp.eventQId);

    printf ("%" PRIu64 ", %" PRIu64 ", %" PRIu64 "\n"
                , appCommonConnStats->tcpConnInit
                , appCommonConnStats->tcpConnInitSuccess
                , appCommonConnStats->tcpConnInitFail);

    DumpCommonConnStats(appCommonConnStats);

    DumpErrSessions();
}

int main(int argc, char** argv)
{
    TcpClientAppOptions_t appOptions = {  .maxEvents = 1000
                                        , .maxActiveSessions = 1000
                                        , .maxErrorSessions = 20
                                        , .maxSessions = 1000 };
    InitApp(&appOptions);
    TcpClientConnStats_t* appConnStats = &theApp.appConnStats;
    CommonConnStats_t* appCommonConnStats = (CommonConnStats_t*) &theApp.appConnStats;
    TcpClientAppStats_t* appStats = &theApp.appStats;

    while (true) {

        if ( (appCommonConnStats->tcpConnInitFail == appOptions.maxErrorSessions) 
                || (appCommonConnStats->tcpConnInit == appOptions.maxSessions 
                        && IsSessionPoolEmpty (theApp.activeSessionPool)) ){
            break;
        }

        if (appCommonConnStats->tcpConnInit < appOptions.maxSessions) {
            
            TcpClientConnection_t* newSession = GetFreeSession ();
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

                TcpClientConnStats_t* groupStats = &theApp.appGroupConnStats[0]; 
                newSession->groupStats = groupStats;

                SetAppState (newSession, APP_STATE_CONNECTION_IN_PROGRESS);
                IncSStats2(appConnStats, groupStats, tcpConnInit);

                newSession->socketFd = TcpNewConnection(newSession->isIpv6, 
                                                        newSession->localAddress, 
                                                        newSession->remoteAddress,
                                                        appConnStats,
                                                        groupStats,
                                                        newSession);
                if ( GetSSLastErr (newSession) ) {
                    appCommonConnStats->tcpConnInitFail++;
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

                TcpClientConnection_t* eSession = 
                    (TcpClientConnection_t*) GetIOEventData(theApp.EventArr[eIndex]);
                
                TcpClientConnStats_t* groupStats = eSession->groupStats;

                if (IsWriteEventSet(theApp.EventArr[eIndex])) {
                    if ( GetAppState(eSession) == APP_STATE_CONNECTION_IN_PROGRESS ) {
                        if ( IsNewTcpConnectionComplete(eSession->socketFd) ){
                            IncSStats2(appConnStats, groupStats, tcpConnInitSuccess);
                            SetAppState (eSession, APP_STATE_CONNECTION_ESTABLISHED);
                            SetSS1(eSession, STATE_TCP_CONN_ESTABLISHED);
                            close(eSession->socketFd);
                            SetSS1(eSession, STATE_TCP_SOCK_FD_CLOSE);
                            SetFreeSession (eSession);
                            SetAppState (eSession, APP_STATE_CONNECTION_CLOSED);
                        }else{
                            IncSStats2(appConnStats, groupStats, tcpConnInitFail);
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