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

void InitSession(TcpClientSession_t* newSess) {

    TcpClientConnection_t* newConn = &newSess->tcConn; 
    
    SSInit(newConn);

    newConn->socketFd = 0;
    newConn->isIpv6 = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    
    newConn->tcSess = newSess;
}

void SetSessionAddress(TcpClientConnection_t* newConn
                        , int isIpv6
                        , struct sockaddr* localAddr
                        , struct sockaddr* remoteAddr) {

    newConn->isIpv6 = isIpv6;
    newConn->localAddress = localAddr;
    newConn->remoteAddress = remoteAddr;
}

TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* newSess 
        = GetAnySesionFromPool (theApp.freeSessionPool);

    if (newSess != NULL) {
        AddToSessionPool (theApp.activeSessionPool, newSess);
        InitSession(newSess);
    }

    return newSess;
}

void SetFreeSession(TcpClientSession_t* newSess) {
    
    RemoveFromSessionPool (theApp.activeSessionPool, newSess);
    AddToSessionPool (theApp.freeSessionPool, newSess);
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
            TcpClientSession_t* newSess = &theApp.errorSessionArr[i];
            TcpClientConnection_t* newConn = &newSess->tcConn;

            printf ("SS1 = %#018" PRIx64 ", Err = %d, SysErr = %d\n"
                , GetSS1(newConn), GetSSLastErr(newConn), GetErrno(newConn) ); 
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

    DumpSStats(appCommonConnStats);

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
    TcpClientAppStats_t* appStats = &theApp.appStats;

    while (true) {

        if ( (GetSStats(appConnStats, tcpConnInitFail) == appOptions.maxErrorSessions) 
                || (GetSStats(appConnStats, tcpConnInit) == appOptions.maxSessions 
                        && IsSessionPoolEmpty (theApp.activeSessionPool)) ){
            break;
        }

        if (GetSStats(appConnStats, tcpConnInit) < appOptions.maxSessions) {
            
            TcpClientSession_t* newSess = GetFreeSession ();
            if (newSess == NULL) {
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

                TcpClientConnection_t* newConn = &newSess->tcConn;

                SetSessionAddress(newConn, 0
                                    , (struct sockaddr*) &localAddr
                                    , (struct sockaddr*) &remoteAddr);

                TcpClientConnStats_t* groupConnStats = &theApp.appGroupConnStats[0]; 
                newSess->groupConnStats = groupConnStats;

                SetAppState (newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                IncSStats2(appConnStats, groupConnStats, tcpConnInit);

                newConn->socketFd = TcpNewConnection(newConn->isIpv6, 
                                                        newConn->localAddress, 
                                                        newConn->remoteAddress,
                                                        appConnStats,
                                                        groupConnStats,
                                                        newConn);
                if ( GetSSLastErr (newConn) ) {
                    IncSStats2(appConnStats, groupConnStats, tcpConnInitFail);
                    StoreErrSession(newSess);
                    SetFreeSession (newSess);
                } else {
                    RegisterForWriteEvent(theApp.eventQId, newConn->socketFd, newConn);
                }
            }
        }

        int eCount = GetIOEvents(theApp.eventQId, theApp.EventArr
                                    , theApp.appOptions.maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                    = (TcpClientConnection_t*) GetIOEventData(theApp.EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                TcpClientConnStats_t* groupConnStats = newSess->groupConnStats;

                if (IsWriteEventSet(theApp.EventArr[eIndex])) {
                    if ( GetAppState(newConn) == APP_STATE_CONNECTION_IN_PROGRESS ) {
                        if ( IsNewTcpConnectionComplete(newConn->socketFd) ){
                            IncSStats2(appConnStats, groupConnStats, tcpConnInitSuccess);
                            SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISHED);
                            SetSS1(newConn, STATE_TCP_CONN_ESTABLISHED);
                            close(newConn->socketFd);
                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);
                            SetAppState (newConn, APP_STATE_CONNECTION_CLOSED);

                            SetFreeSession (newSess);
                        }else{
                            IncSStats2(appConnStats, groupConnStats, tcpConnInitFail);
                            close(newConn->socketFd);
                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            SetFreeSession (newSess);
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