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

TcpClientApp_t theApp;

//hard coded
char* srcIp = "12.20.50.1";
char* dstIp = "12.20.60.1";
int dstPort = 8081;
struct sockaddr_in localAddr;
struct sockaddr_in remoteAddr;

TcpClientAppOptions_t appOptions = {  .maxEvents = 1000
                        , .maxActiveSessions = 25000
                        , .maxErrorSessions = 1000
                        , .maxSessions = 5
                        , .connectionPerSec = 2000
                        , .csDataLen = 1500};

int tcp_client_fifo;
char statsString[120];
int ignoreRet;
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
    if (theApp.errorSessionCount < appOptions.maxErrorSessions) {

        TcpClientSession_t* errSession 
                = &theApp.errorSessionArr[theApp.errorSessionCount];

        *errSession =  *aSession;

        theApp.errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (theApp.errorSessionCount) 
    {
        for (int i = 0; i < theApp.errorSessionCount; i++) {

            TcpClientSession_t* newSess 
                    = &theApp.errorSessionArr[i];

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

    theApp.freeSessionPool = AllocEmptySessionPool();
    theApp.activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < appOptions.maxActiveSessions; i++) {

        TcpClientSession_t* aSession 
                    = AllocSession (TcpClientSession_t);

        InitSessionInitApp (aSession);

        AddToSessionPool (theApp.freeSessionPool, aSession);
    }

    theApp.errorSessionCount = 0;
    
    theApp.errorSessionArr = CreateArray (TcpClientSession_t
                                , appOptions.maxErrorSessions); 

    theApp.EventArr = CreateEventArray(appOptions.maxEvents);

    theApp.appConnStats = CreateStruct0 (TcpClientConnStats_t);

    theApp.appStats = CreateStruct0 (TcpClientAppStats_t);

    theApp.appGroupConnStats = CreateArray (TcpClientConnStats_t, 1);

    theApp.eventQId = CreateEventQ();

    theApp.timerWheel = CreateTimerWheel();

    //malloc ???
    theApp.sendBuffer =  malloc(appOptions.csDataLen);
}

char* GetStatsString() {

    sprintf (statsString, 
                        "%" PRIu64 "\n" 
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "\n"
        , GetSStats(theApp.appConnStats, tcpConnInit)
        , GetSStats(theApp.appConnStats, tcpConnInitSuccess)
        , GetSStats(theApp.appConnStats, tcpConnInitFail)
        , GetSStats(theApp.appConnStats, tcpConnInitFailEaddrNotAvail)
        , GetSStats(theApp.appConnStats, tcpConnRegisterForWriteEventFail)
        );

    return statsString;
}

void CleanupApp() {
    DeleteEventQ(theApp.eventQId);

    DeleteTimerWheel(theApp.timerWheel);

    GetStatsString();

    puts(statsString);

    DumpErrSessions();
}

int main(int argc, char** argv)
{
    //hardcoded
    //tcp_client_fifo = open("/tmp/tcp_client", O_WRONLY);
    //hardcoded

    InitApp();
    TcpClientConnStats_t* appConnStats = theApp.appConnStats;
    TcpClientAppStats_t* appStats = theApp.appStats;

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
                    == appOptions.maxErrorSessions) 
                || (GetSStats(appConnStats, tcpConnInit) 
                    == appOptions.maxSessions 
                && IsSessionPoolEmpty (theApp.activeSessionPool)) ){
            break;
        }

        if (GetSStats(appConnStats, tcpConnInit) 
                < appOptions.maxSessions) {
 
            int connectionBurst 
                = (TimeElapsedTimerWheel(theApp.timerWheel) 
                    * appOptions.connectionPerSec) 
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
                                        = &theApp.appGroupConnStats[0]; 

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
                        if ( RegisterForWriteEvent(theApp.eventQId
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

        int eCount = GetIOEvents(theApp.eventQId, theApp.EventArr
                                    , appOptions.maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                            = (TcpClientConnection_t*)
                                GetIOEventData(theApp.EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                TcpClientConnStats_t* groupConnStats = newSess->groupConnStats;

                if (IsWriteEventSet(theApp.EventArr[eIndex])) {
                    if ( GetAppState(newConn) == APP_STATE_CONNECTION_IN_PROGRESS ) {

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
                                    appOptions.csDataLen ) {

                        int bytesToSend 
                            = appOptions.csDataLen - newConn->bytesSent;

                        const char* sendBuffer
                            = &theApp.sendBuffer[newConn->bytesSent];

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

                            if (newConn->bytesSent == appOptions.csDataLen) {
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

    return 0;
}