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
#include <sys/mman.h>

#include "tcp_client.h"

TcpClientApp_t* AppObj;
TcpClientAppInterface_t* AppIface;

#define IncConnStats(__session,__stats) IncSStats2(&AppIface->appConnStats,__session->groupConnStats,__stats) 


void InitSession(TcpClientSession_t* newSess) {

    TcpClientConnection_t* newConn = &newSess->tcConn; 
    
    SSInit(newConn);

    newConn->socketFd = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->bytesSent = 0;
}

void InitSessionInitApp(TcpClientSession_t* newSess) {

    newSess->tcConn.tcSess = newSess;

    InitSession(newSess); 
}

TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* newSess 
        = GetSesionFromPool (AppObj->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppObj->activeSessionPool, newSess);
        InitSession(newSess);
    }

    return newSess;
}

void SetFreeSession(TcpClientSession_t* newSess) {
    
    RemoveFromSessionPool (AppObj->activeSessionPool, newSess);
    SetSessionToPool (AppObj->freeSessionPool, newSess);
}

void StoreErrSession(TcpClientSession_t* aSession) {
    if (AppObj->errorSessionCount < AppIface->maxErrorSessions) {

        TcpClientSession_t* errSession 
                = &AppObj->errorSessionArr[AppObj->errorSessionCount];

        *errSession =  *aSession;

        AppObj->errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (AppObj->errorSessionCount) 
    {
        for (int i = 0; i < AppObj->errorSessionCount; i++) {

            TcpClientSession_t* newSess 
                    = &AppObj->errorSessionArr[i];

            TcpClientConnection_t* newConn 
                    = &newSess->tcConn;

            char srcAddr[INET6_ADDRSTRLEN];
            char dstAddr[INET6_ADDRSTRLEN];
            uint16_t localPort, remotePort;
            if (IsIpv6(newConn->localAddress)){
                struct sockaddr_in6* localAddress 
                    = (struct sockaddr_in6*)newConn->localAddress;
                struct sockaddr_in6* remoteAddress 
                    = (struct sockaddr_in6*)newConn->remoteAddress;
                inet_ntop(AF_INET6, &(localAddress->sin6_addr), srcAddr, INET6_ADDRSTRLEN);
                inet_ntop(AF_INET6, &(remoteAddress->sin6_addr), dstAddr, INET6_ADDRSTRLEN);
                localPort = ntohs(localAddress->sin6_port);
                remotePort = ntohs(remoteAddress->sin6_port);
            } else {
                struct sockaddr_in* localAddress 
                    = (struct sockaddr_in*)newConn->localAddress;
                struct sockaddr_in* remoteAddress 
                    = (struct sockaddr_in*)newConn->remoteAddress;
                inet_ntop(AF_INET, &(localAddress->sin_addr), srcAddr, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &(remoteAddress->sin_addr), dstAddr, INET_ADDRSTRLEN);
                localPort = ntohs(localAddress->sin_port);
                remotePort = ntohs(remoteAddress->sin_port);
            }

            printf ("SS1 = %#018" PRIx64 
                    ", Err = %d" 
                    ", SysErr = %d"
                    ", src = %s : %hu"
                    ", dst = %s : %hu\n"
                    , GetSS1(newConn)
                    , GetSSLastErr(newConn)
                    , GetErrno(newConn) 
                    , srcAddr, localPort
                    , dstAddr, remotePort); 
        }
    }
}

void ReleasePort(TcpClientConnection_t* newConn){
    if (IsIpv6(newConn->localAddress)) {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in6*)newConn->localAddress)->sin6_port);
    } else {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in*)newConn->localAddress)->sin_port);
    }
}

void OnSessionError(TcpClientSession_t* newSess){
    StoreErrSession (newSess);
    SetFreeSession (newSess);
    ReleasePort (&newSess->tcConn);
}

void OnConnectionError(TcpClientConnection_t* newConn){

    if (GetSSLastErr (newConn) 
            == TD_SOCKET_CONNECT_FAILED_IMMEDIATE
        && GetErrno(newConn) == EADDRNOTAVAIL) {

            IncSStats2(&AppIface->appConnStats
                        , newConn->tcSess->groupConnStats 
                        , tcpConnInitFailEaddrNotAvail);
    }

    IncSStats2(&AppIface->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnInitFail);

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    ReleasePort (newConn);
}

void OnRegisterForWriteEventError(TcpClientConnection_t* newConn){
    SaveErrno(newConn);

    IncSStats2(&AppIface->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnRegisterForWriteEventFail);    

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    ReleasePort (newConn);
}

void InitApp() {

    AppObj->freeSessionPool = AllocEmptySessionPool();
    AppObj->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < AppIface->maxActiveSessions; i++) {

        TcpClientSession_t* aSession 
                    = AllocSession (TcpClientSession_t);

        InitSessionInitApp (aSession);

        SetSessionToPool (AppObj->freeSessionPool, aSession);
    }

    AppObj->errorSessionCount = 0;
    
    AppObj->errorSessionArr = CreateArray (TcpClientSession_t
                                , AppIface->maxErrorSessions); 

    AppObj->EventArr = CreateEventArray(AppIface->maxEvents);

    AppObj->eventQ = CreateEventQ();

    AppObj->timerWheel = CreateTimerWheel();

    AppObj->sendBuffer =  TdMalloc(AppIface->csDataLen);
}

void DumpTcpClientAppStats(TcpClientAppConnStats_t* appConnStats) {
    
    char statsString[120];

    sprintf (statsString, 
                        "%" PRIu64 "\n" 
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "\n"
        , GetSStats(appConnStats, tcpConnInit)
        , GetSStats(appConnStats, tcpConnInitSuccess)
        , GetSStats(appConnStats, tcpConnInitFail)
        , GetSStats(appConnStats, tcpConnInitFailEaddrNotAvail)
        , GetSStats(appConnStats, tcpConnRegisterForWriteEventFail)
        );

    puts (statsString);
}

void CleanupApp() {

    DeleteEventQ(AppObj->eventQ);

    DeleteTimerWheel(AppObj->timerWheel);

    DumpTcpClientAppStats(&AppIface->appConnStats);

    DumpErrSessions();
}

int PrepConnection(TcpClientConnection_t* newConn){
    
    int status = 0;

    TcpClientAppConnGroup_t* csGroup 
        = &AppIface->csGroupArr[AppIface->nextCsGroupIndex];

    newConn->localAddress 
        = (struct sockaddr*)&(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

    newConn->remoteAddress 
        = (struct sockaddr*)&(csGroup->serverAddr);

    newConn->tcSess->groupConnStats = &csGroup->cStats;

    newConn->localPortPool 
        = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

    int nextSrcPort = GetPortFromPool(newConn->localPortPool);
    if (nextSrcPort) {
        SetSockPort(newConn->localAddress, nextSrcPort);
    }else{
        status = -1;
    }

    AppIface->nextCsGroupIndex += 1;
    if (AppIface->nextCsGroupIndex == AppIface->csGroupCount){
        AppIface->nextCsGroupIndex = 0;
    }
     
    csGroup->nextClientAddrIndex += 1;
    if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
        csGroup->nextClientAddrIndex = 0;
    }

    return status;
}

void TcpClienAppRun(TcpClientAppInterface_t* appIface)
{
    AppObj = CreateStruct0 (TcpClientApp_t);
    AppIface = appIface;

    InitApp();
    TcpClientAppConnStats_t* appConnStats = &AppIface->appConnStats;
    TcpClientAppStats_t* appStats = &AppIface->appStats;

    time_t epochSinceSeconds = time(NULL);
    printf("%ld - -\n", epochSinceSeconds);

    double lastConnectionInitTime = TimeElapsedTimerWheel(AppObj->timerWheel);

    while (true) {

        if ( (GetSStats(appConnStats, tcpConnInitFail) 
                    == AppIface->maxErrorSessions) 
                || (GetSStats(appConnStats, tcpConnInit) 
                    == AppIface->maxSessions 
                && IsSessionPoolEmpty (AppObj->activeSessionPool)) ){
            break;
        }

        if (GetSStats(appConnStats, tcpConnInit) 
                < AppIface->maxSessions) {
 
            int newConnectionInits 
                = ((TimeElapsedTimerWheel(AppObj->timerWheel)
                        - lastConnectionInitTime)
                    * AppIface->connectionPerSec);

            while (newConnectionInits > 0 
                    && GetSStats(appConnStats, tcpConnInit) 
                        < AppIface->maxSessions) {

                newConnectionInits--;

                lastConnectionInitTime 
                        = TimeElapsedTimerWheel(AppObj->timerWheel);

                TcpClientSession_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    appStats->dbgNoFreeSession++;      
                }else {

                    TcpClientConnection_t* newConn = &newSess->tcConn;

                    PrepConnection(newConn);
                    // ??? error handling 

                    SetAppState (newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                    IncSStats2(&AppIface->appConnStats
                                , newConn->tcSess->groupConnStats 
                                , tcpConnInit);

                    newConn->socketFd 
                        = TcpNewConnection(newConn->localAddress
                                            , newConn->remoteAddress
                                            , &AppIface->appConnStats
                                            , newSess->groupConnStats
                                            , newConn);

                    if ( GetSSLastErr (newConn) ) {
                        OnConnectionError(newConn);
                    } else {
                        if ( RegisterForWriteEvent(AppObj->eventQ
                                                    , newConn->socketFd
                                                    , newConn)){
                            OnRegisterForWriteEventError(newConn);
                        }
                    }
                }
            }
        }

        int eCount = GetIOEvents(AppObj->eventQ, AppObj->EventArr
                                    , AppIface->maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                            = (TcpClientConnection_t*)
                                GetIOEventData(AppObj->EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                if (IsWriteEventSet(AppObj->EventArr[eIndex])) {
                    if ( GetAppState(newConn) 
                            == APP_STATE_CONNECTION_IN_PROGRESS ) {

                        if ( IsNewTcpConnectionComplete(newConn->socketFd) ){
                            
                            IncConnStats(newSess, tcpConnInitSuccess);

                            SetAppState (newConn
                                        , APP_STATE_CONNECTION_ESTABLISHED);

                            SetSS1(newConn, STATE_TCP_CONN_ESTABLISHED);
                        }else{
                            IncConnStats(newSess, tcpConnInitFail);
                            UnRegisterForEvent(AppObj->eventQ
                                            , newConn->socketFd
                                            , newConn);
                            close(newConn->socketFd);
                            ReleasePort (newConn);

                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            SetFreeSession (newSess);
                            // ??? error handling
                        }
                    } else if ( GetAppState(newConn) == 
                                     APP_STATE_CONNECTION_ESTABLISHED 
                                && newConn->bytesSent < 
                                    AppIface->csDataLen ) {

                        int bytesToSend 
                            = AppIface->csDataLen - newConn->bytesSent;

                        const char* sendBuffer
                            = &AppObj->sendBuffer[newConn->bytesSent];

                        int bytesSent 
                            = TcpWrite (newConn->socketFd
                                        , sendBuffer
                                        , bytesToSend
                                        , &AppIface->appConnStats
                                        , newSess->groupConnStats
                                        , newConn);

                        if (GetSSLastErr (newConn)) {
                            UnRegisterForEvent(AppObj->eventQ, newConn->socketFd, newConn);
                            close(newConn->socketFd);
                            ReleasePort (newConn);

                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            StoreErrSession(newSess);

                            SetFreeSession (newSess);
                        }else {
                            newConn->bytesSent += bytesSent;

                            if (newConn->bytesSent == AppIface->csDataLen) {
                                UnRegisterForEvent(AppObj->eventQ, newConn->socketFd, newConn);
                                close(newConn->socketFd);
                                ReleasePort (newConn);

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
    
    AppIface->isRunning = 0;
}

TcpClientAppInterface_t* CreateTcpClienAppInterface(int csGroupCount
                                            , int* clientAddrCounts)
{
    TcpClientAppInterface_t* iFace 
        = (TcpClientAppInterface_t*) mmap(NULL
            , sizeof (TcpClientAppInterface_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TcpClientAppConnGroup_t*) mmap(NULL
            , sizeof (TcpClientAppConnGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    iFace->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < iFace->csGroupCount; gIndex++) {
        TcpClientAppConnGroup_t* csGroup = &iFace->csGroupArr[gIndex];
        csGroup->clientAddrCount = clientAddrCounts[gIndex];
        csGroup->nextClientAddrIndex = 0;
        csGroup->clientAddrArr
            = (SockAddr_t*) mmap(NULL
                , sizeof (SockAddr_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        csGroup->LocalPortPoolArr 
            = (LocalPortPool_t*) mmap(NULL
                , sizeof (LocalPortPool_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        for (int cIndex = 0
                ; cIndex < csGroup->clientAddrCount
                ; cIndex++) {
            InitPortBindQ(&csGroup->LocalPortPoolArr[cIndex]);
        }
    }

    return iFace;
}

void DeleteTcpClienAppInterface (TcpClientAppInterface_t* iFace)
{
    //todo
}