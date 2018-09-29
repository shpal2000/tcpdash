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
#include <fcntl.h>
#include <time.h>


#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"
#include "tcp_client.h"

TcpClientApp_t* TheApp;

//hard coded
char* srcIp = "12.20.50.1";
char* dstIp = "12.20.60.1";
int dstPort = 8081;
struct sockaddr_in localAddr;
struct sockaddr_in remoteAddr;

TcpClientAppOptions_t* AppOptions;


//hard coded



void InitSession(TcpClientSession_t* newSess) {

    TcpClientConnection_t* newConn = &newSess->tcConn; 
    
    SSInit(newConn);

    newConn->socketFd = 0;
    newConn->isIpv6 = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->bytesSent = 0;
}

void InitSessionInitApp(TcpClientSession_t* newSess) {

    newSess->tcConn.tcSess = newSess;

    InitSession(newSess); 
}

void SetSessionAddress(TcpClientConnection_t* newConn
                        , int isIpv6
                        , struct sockaddr* localAddr
                        , struct sockaddr* remoteAddr) 
{
    newConn->isIpv6 = isIpv6;
    newConn->localAddress = localAddr;
    newConn->remoteAddress = remoteAddr;
}

TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* newSess 
        = GetAnySesionFromPool (TheApp->freeSessionPool);

    if (newSess != NULL) {
        AddToSessionPool (TheApp->activeSessionPool, newSess);
        InitSession(newSess);
    }

    return newSess;
}

void SetFreeSession(TcpClientSession_t* newSess) {
    
    RemoveFromSessionPool (TheApp->activeSessionPool, newSess);
    AddToSessionPool (TheApp->freeSessionPool, newSess);
}

void StoreErrSession(TcpClientSession_t* aSession) {
    if (TheApp->errorSessionCount < AppOptions->maxErrorSessions) {

        TcpClientSession_t* errSession 
                = &TheApp->errorSessionArr[TheApp->errorSessionCount];

        *errSession =  *aSession;

        TheApp->errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (TheApp->errorSessionCount) 
    {
        for (int i = 0; i < TheApp->errorSessionCount; i++) {

            TcpClientSession_t* newSess 
                    = &TheApp->errorSessionArr[i];

            TcpClientConnection_t* newConn 
                    = &newSess->tcConn;

            printf ("SS1 = %#018" PRIx64 
                    ", Err = %d" 
                    ", SysErr = %d\n"
                    , GetSS1(newConn)
                    , GetSSLastErr(newConn)
                    , GetErrno(newConn) ); 
        }
    }
}

void InitApp() {

    TheApp->freeSessionPool = AllocEmptySessionPool();
    TheApp->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < AppOptions->maxActiveSessions; i++) {

        TcpClientSession_t* aSession 
                    = AllocSession (TcpClientSession_t);

        InitSessionInitApp (aSession);

        AddToSessionPool (TheApp->freeSessionPool, aSession);
    }

    TheApp->errorSessionCount = 0;
    
    TheApp->errorSessionArr = CreateArray (TcpClientSession_t
                                , AppOptions->maxErrorSessions); 

    TheApp->EventArr = CreateEventArray(AppOptions->maxEvents);

    TheApp->appConnStats = CreateStruct0 (TcpClientConnStats_t);

    TheApp->appStats = CreateStruct0 (TcpClientAppStats_t);

    TheApp->appGroupConnStats = CreateArray (TcpClientConnStats_t, 1);

    TheApp->eventQId = CreateEventQ();

    TheApp->timerWheel = CreateTimerWheel();

    //malloc ???
    TheApp->sendBuffer =  malloc(AppOptions->csDataLen);
}

void DumpStats() {
    
    char statsString[120];
    
    sprintf (statsString, 
                        "%" PRIu64 "\n" 
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "\n"
        , GetSStats(TheApp->appConnStats, tcpConnInit)
        , GetSStats(TheApp->appConnStats, tcpConnInitSuccess)
        , GetSStats(TheApp->appConnStats, tcpConnInitFail)
        , GetSStats(TheApp->appConnStats, tcpConnInitFailEaddrNotAvail)
        , GetSStats(TheApp->appConnStats, tcpConnRegisterForWriteEventFail)
        );

    puts (statsString);
}

void CleanupApp() {

    DeleteEventQ(TheApp->eventQId);

    DeleteTimerWheel(TheApp->timerWheel);

    DumpStats();

    DumpErrSessions();
}

void TcpClientRun(TcpClientAppOptions_t* appOptions)
{
    TheApp = CreateStruct0 (TcpClientApp_t);
    AppOptions = appOptions;

    InitApp();
    TcpClientConnStats_t* appConnStats = TheApp->appConnStats;
    TcpClientAppStats_t* appStats = TheApp->appStats;

    //hard coded
    int srcPort = 10000;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    inet_pton(AF_INET, srcIp, &(localAddr.sin_addr));
    localAddr.sin_port = htons(srcPort);
    memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(dstPort);
    inet_pton(AF_INET, dstIp, &(remoteAddr.sin_addr));
    //hard coded 

    time_t epochSinceSeconds = time(NULL);
    printf("%ld - -\n", epochSinceSeconds);

    while (true) {

        if ( (GetSStats(appConnStats, tcpConnInitFail) 
                    == AppOptions->maxErrorSessions) 
                || (GetSStats(appConnStats, tcpConnInit) 
                    == AppOptions->maxSessions 
                && IsSessionPoolEmpty (TheApp->activeSessionPool)) ){
            break;
        }

        if (GetSStats(appConnStats, tcpConnInit) 
                < AppOptions->maxSessions) {
 
            int connectionBurst 
                = (TimeElapsedTimerWheel(TheApp->timerWheel) 
                    * AppOptions->connectionPerSec) 
                    - GetSStats(appConnStats, tcpConnInit);

            while (connectionBurst-- > 0) {
                
                TcpClientSession_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    appStats->dbgNoFreeSession++;      
                }else {

                    TcpClientConnection_t* newConn = &newSess->tcConn;

                    SetSessionAddress(newConn, 0
                                , (struct sockaddr*) &localAddr
                                , (struct sockaddr*) &remoteAddr);

                    TcpClientConnStats_t* groupConnStats 
                                        = &TheApp->appGroupConnStats[0]; 

                    newSess->groupConnStats = groupConnStats;

                    SetAppState (newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                    //hard coded
                    localAddr.sin_port = htons(srcPort++);
                    if (srcPort == 60000) {
                        srcPort = 10000;
                    }

                    //hard coded 

                    IncSStats2(appConnStats
                                , groupConnStats
                                , tcpConnInit);

                    newConn->socketFd 
                        = TcpNewConnection(newConn->isIpv6, 
                                        newConn->localAddress, 
                                        newConn->remoteAddress,
                                        appConnStats,
                                        groupConnStats,
                                        newConn);


                    if ( GetSSLastErr (newConn) ) {
                        if (GetSSLastErr (newConn) 
                                == TD_SOCKET_CONNECT_FAILED_IMMEDIATE
                            && GetErrno(newConn) == EADDRNOTAVAIL) {
                                IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitFailEaddrNotAvail);
                        }
                        IncSStats2(appConnStats
                                    , groupConnStats
                                    , tcpConnInitFail);
                        StoreErrSession(newSess);
                        SetFreeSession (newSess);
                    } else {
                        if ( RegisterForWriteEvent(TheApp->eventQId
                                        , newConn->socketFd
                                        , newConn)){

                            SaveErrno(newConn);                
                            IncSStats2(appConnStats
                                    , groupConnStats
                                    , tcpConnRegisterForWriteEventFail);
                            StoreErrSession(newSess);
                            SetFreeSession (newSess);
                        }
                    }
                }
            }
        }

        int eCount = GetIOEvents(TheApp->eventQId, TheApp->EventArr
                                    , AppOptions->maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                            = (TcpClientConnection_t*)
                                GetIOEventData(TheApp->EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                TcpClientConnStats_t* groupConnStats = newSess->groupConnStats;

                if (IsWriteEventSet(TheApp->EventArr[eIndex])) {
                    if ( GetAppState(newConn) 
                            == APP_STATE_CONNECTION_IN_PROGRESS ) {

                        if ( IsNewTcpConnectionComplete(newConn->socketFd) ){
                            
                            IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitSuccess);

                            SetAppState (newConn
                                        , APP_STATE_CONNECTION_ESTABLISHED);

                            SetSS1(newConn, STATE_TCP_CONN_ESTABLISHED);
                        }else{
                            IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitFail);

                            close(newConn->socketFd);

                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            SetFreeSession (newSess);
                            //to capture the error ?
                        }
                    } else if ( GetAppState(newConn) == 
                                     APP_STATE_CONNECTION_ESTABLISHED 
                                && newConn->bytesSent < 
                                    AppOptions->csDataLen ) {

                        int bytesToSend 
                            = AppOptions->csDataLen - newConn->bytesSent;

                        const char* sendBuffer
                            = &TheApp->sendBuffer[newConn->bytesSent];

                        int bytesSent 
                            = TcpWrite (newConn->socketFd
                                        , sendBuffer
                                        , bytesToSend
                                        , appConnStats
                                        , groupConnStats
                                        , newConn);

                        if (GetSSLastErr (newConn)) {
                            close(newConn->socketFd);

                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            StoreErrSession(newSess);

                            SetFreeSession (newSess);
                        }else {
                            newConn->bytesSent += bytesSent;

                            if (newConn->bytesSent == AppOptions->csDataLen) {
                                close(newConn->socketFd);

                                SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                                SetFreeSession (newSess);
                            }
                        }
                    }
                }
            }
        }
    }

    epochSinceSeconds = time(NULL);
    printf("-- %ld\n", epochSinceSeconds);

    CleanupApp();
}

