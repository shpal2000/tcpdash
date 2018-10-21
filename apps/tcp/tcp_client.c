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

TcpClientApp_t* AppO;
TcpClientAppInterface_t* AppI;

void InitSession(TcpClientSession_t* newSess, int initApp) {

    TcpClientConnection_t* newConn = &newSess->tcConn; 
    
    if (initApp) {
        newSess->tcConn.tcSess = newSess;
    }

    CSInit(newConn);

    newConn->socketFd = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->bytesSent = 0;
}

TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* newSess 
        = GetSesionFromPool (AppO->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppO->activeSessionPool, newSess);
        InitSession(newSess, 0);
    }

    return newSess;
}

void SetFreeSession(TcpClientSession_t* newSess) {
    
    RemoveFromSessionPool (AppO->activeSessionPool, newSess);
    SetSessionToPool (AppO->freeSessionPool, newSess);
}

void StoreErrSession(TcpClientSession_t* aSession) {
    if (AppO->errorSessionCount < AppI->maxErrorSessions) {

        TcpClientSession_t* errSession 
                = &AppO->errorSessionArr[AppO->errorSessionCount];

        *errSession =  *aSession;

        errSession->tcConn.savedLocalPort 
            = ntohs(GetSockPort(errSession->tcConn.localAddress));

        errSession->tcConn.savedRemotePort 
            = ntohs(GetSockPort(errSession->tcConn.remoteAddress));

        AppO->errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (AppO->errorSessionCount) {

        char srcAddr[INET6_ADDRSTRLEN];
        char dstAddr[INET6_ADDRSTRLEN];

        for (int i = 0; i < AppO->errorSessionCount; i++) {

            TcpClientSession_t* newSess 
                    = &AppO->errorSessionArr[i];

            TcpClientConnection_t* newConn 
                    = &newSess->tcConn;

            AddressToString (newConn->localAddress, srcAddr);

            AddressToString (newConn->remoteAddress, dstAddr);

            printf ("SS1 = %#018" PRIx64 
                    ", Err = %d" 
                    ", SysErr = %d"
                    ", src = %s : %hu"
                    ", dst = %s : %hu\n"
                    , GetCS1(newConn)
                    , GetConnLastErr(newConn)
                    , GetErrno(newConn) 
                    , srcAddr, newConn->savedLocalPort
                    , dstAddr, newConn->savedRemotePort); 
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

void OnAssignSocketLocalPortError(TcpClientConnection_t* newConn){
    
    IncConnStats2(&AppI->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpLocalPortAssignFail);    

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
}

void OnConnectionInitError(TcpClientConnection_t* newConn){

    if (GetConnLastErr (newConn) 
            == TD_SOCKET_CONNECT_FAILED_IMMEDIATE
        && GetErrno(newConn) == EADDRNOTAVAIL) {

            IncConnStats2(&AppI->appConnStats
                        , newConn->tcSess->groupConnStats 
                        , tcpConnInitFailEaddrNotAvail);
    }

    IncConnStats2(&AppI->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnInitFail);

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    ReleasePort (newConn);
}

void OnConnectionEstablished(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnInitSuccess);

    SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISHED);

    SetCS1 (newConn, STATE_TCP_CONN_ESTABLISHED);
}

void OnConnectionEstablishError(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppI->appConnStats
        , newConn->tcSess->groupConnStats 
        , tcpConnInitFail);

    int unRegisterStatus = UnRegisterForWriteEvent(AppO->eventQ
                                                , newConn->socketFd
                                                , newConn);
    if (unRegisterStatus){
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnUnRegisterForWriteEventFail);        
    }

    close(newConn->socketFd);
    SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    if (unRegisterStatus == 0){
        ReleasePort (newConn);
    }
}

void OnRegisterForWriteEventError(TcpClientConnection_t* newConn){

    IncConnStats2(&AppI->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnRegisterForWriteEventFail);    

    close(newConn->socketFd);
    SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);
    
    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    ReleasePort (newConn);
}

void OnWriteError(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppI->appConnStats
        , newConn->tcSess->groupConnStats 
        , tcpWriteFail);

    int unRegisterStatus = UnRegisterForWriteEvent(AppO->eventQ
                                                , newConn->socketFd
                                                , newConn);
    if (unRegisterStatus){
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnUnRegisterForWriteEventFail);        
    }

    close(newConn->socketFd);
    SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    if (unRegisterStatus == 0){
        ReleasePort (newConn);
    }
}

void OnAppDataWriteComplete(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppI->appConnStats
        , newConn->tcSess->groupConnStats 
        , appDataWriteComplete);

    int unRegisterStatus = UnRegisterForWriteEvent(AppO->eventQ
                                                , newConn->socketFd
                                                , newConn);
    if (unRegisterStatus){
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnUnRegisterForWriteEventFail);        
    }

    close(newConn->socketFd);
    SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

    if (unRegisterStatus) {
        StoreErrSession (newConn->tcSess);
    }
    SetFreeSession (newConn->tcSess);
    if (unRegisterStatus == 0){
        ReleasePort (newConn);
    }
}

void InitApp() {

    AppO->freeSessionPool = AllocEmptySessionPool();
    AppO->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < AppI->maxActiveSessions; i++) {

        TcpClientSession_t* aSession 
            = AllocSession (TcpClientSession_t);

        InitSession (aSession, 1);

        SetSessionToPool (AppO->freeSessionPool, aSession);
    }

    AppO->errorSessionCount = 0;
    
    AppO->errorSessionArr = CreateArray (TcpClientSession_t
                                , AppI->maxErrorSessions); 

    AppO->EventArr = CreateEventArray(AppI->maxEvents);

    AppO->eventQ = CreateEventQ();

    AppO->timerWheel = CreateTimerWheel();

    AppO->sendBuffer =  TdMalloc(AppI->csDataLen);
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
        , GetConnStats(appConnStats, tcpConnInit)
        , GetConnStats(appConnStats, tcpConnInitSuccess)
        , GetConnStats(appConnStats, tcpConnInitFail)
        , GetConnStats(appConnStats, tcpConnInitFailEaddrNotAvail)
        , GetConnStats(appConnStats, tcpConnRegisterForWriteEventFail)
        );

    puts (statsString);
}

void CleanupApp() {

    DeleteEventQ(AppO->eventQ);

    DeleteTimerWheel(AppO->timerWheel);

    DumpTcpClientAppStats(&AppI->appConnStats);

    DumpErrSessions();
}

TcpClientConnection_t* PrepNextConnection(TcpClientSession_t* newSess){

    TcpClientConnection_t* newConn = &newSess->tcConn; 

    TcpClientAppConnGroup_t* csGroup 
        = &AppI->csGroupArr[AppI->nextCsGroupIndex];

    newConn->localAddress 
        = (struct sockaddr*)&(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

    newConn->remoteAddress 
        = (struct sockaddr*)&(csGroup->serverAddr);

    newConn->tcSess->groupConnStats = &csGroup->cStats;

    newConn->localPortPool 
        = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

    AppI->nextCsGroupIndex += 1;
    if (AppI->nextCsGroupIndex == AppI->csGroupCount){
        AppI->nextCsGroupIndex = 0;
    }
     
    csGroup->nextClientAddrIndex += 1;
    if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
        csGroup->nextClientAddrIndex = 0;
    }

    return newConn;
}

int AppRunContinue() {
    TcpClientAppConnStats_t* appConnStats = &AppI->appConnStats;

    if ( (GetConnStats(appConnStats, tcpConnInitFail) 
                == AppI->maxErrorSessions) 
            || (GetConnStats(appConnStats, tcpConnInit) 
                == AppI->maxSessions 
            && IsSessionPoolEmpty (AppO->activeSessionPool)) ){
        return 0;
    }

    return 1;
}

void TcpClientAppRun(TcpClientAppInterface_t* appIface)
{
    AppO = CreateStruct0 (TcpClientApp_t);
    AppI = appIface;
    InitApp();

    time_t epochSinceSeconds = time(NULL);
    printf("%ld - -\n", epochSinceSeconds);

    double lastConnectionInitTime = TimeElapsedTimerWheel(AppO->timerWheel);

    while (AppRunContinue()) {

        if (GetConnStats(&AppI->appConnStats, tcpConnInit) 
                < AppI->maxSessions) {
 
            int newConnectionInits 
                = ((TimeElapsedTimerWheel(AppO->timerWheel)
                        - lastConnectionInitTime)
                    * AppI->connectionPerSec);

            while (newConnectionInits > 0 
                    && GetConnStats(&AppI->appConnStats, tcpConnInit) 
                        < AppI->maxSessions) {

                newConnectionInits--;

                lastConnectionInitTime 
                        = TimeElapsedTimerWheel(AppO->timerWheel);

                TcpClientSession_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    AppI->appStats.dbgNoFreeSession++;      
                }else {
                    
                    TcpClientConnection_t* newConn = PrepNextConnection(newSess);

                    if ( AssignSocketLocalPort(newConn->localAddress
                                                , newConn->localPortPool
                                                , newConn) ) {
                        OnAssignSocketLocalPortError(newConn);
                    }else{
                        SetAppState (newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                        IncConnStats2(&AppI->appConnStats
                                    , newConn->tcSess->groupConnStats 
                                    , tcpConnInit);

                        newConn->socketFd 
                            = TcpNewConnection(newConn->localAddress
                                                , newConn->remoteAddress
                                                , &AppI->appConnStats
                                                , newSess->groupConnStats
                                                , newConn);

                        if ( GetConnLastErr (newConn) ) {
                            OnConnectionInitError(newConn);
                        } else {
                            if ( RegisterForWriteEvent(AppO->eventQ
                                                    , newConn->socketFd
                                                    , newConn)){
                                OnRegisterForWriteEventError(newConn);
                            }
                        }
                    }
                }
            }
        }

        int eCount = GetIOEvents(AppO->eventQ
                                    , AppO->EventArr
                                    , AppI->maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                    = (TcpClientConnection_t*) GetIOEventData(AppO->EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                if (IsWriteEventSet(AppO->EventArr[eIndex])) {
                    if ( GetAppState(newConn) 
                            == APP_STATE_CONNECTION_IN_PROGRESS ) {

                        if ( IsNewTcpConnectionComplete(newConn->socketFd) ){
                            OnConnectionEstablished(newConn);
                        }else{
                            OnConnectionEstablishError(newConn);
                        }
                    } else if ( GetAppState(newConn) == 
                                     APP_STATE_CONNECTION_ESTABLISHED 
                                && newConn->bytesSent < 
                                    AppI->csDataLen ) {

                        int bytesToSend 
                            = AppI->csDataLen - newConn->bytesSent;

                        const char* sendBuffer
                            = &AppO->sendBuffer[newConn->bytesSent];

                        int bytesSent 
                            = TcpWrite (newConn->socketFd
                                        , sendBuffer
                                        , bytesToSend
                                        , &AppI->appConnStats
                                        , newSess->groupConnStats
                                        , newConn);

                        if (GetConnLastErr (newConn)) {
                            OnWriteError(newConn);
                        }else {
                            newConn->bytesSent += bytesSent;
                            if (newConn->bytesSent == AppI->csDataLen) {
                                OnAppDataWriteComplete(newConn);
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
    
    AppI->isRunning = 0;
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