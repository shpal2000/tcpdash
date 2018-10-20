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

        errSession->tcConn.savedLocalPort 
            = ntohs(GetSockPort(errSession->tcConn.localAddress));

        errSession->tcConn.savedRemotePort 
            = ntohs(GetSockPort(errSession->tcConn.remoteAddress));

        AppObj->errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (AppObj->errorSessionCount) {

        char srcAddr[INET6_ADDRSTRLEN];
        char dstAddr[INET6_ADDRSTRLEN];

        for (int i = 0; i < AppObj->errorSessionCount; i++) {

            TcpClientSession_t* newSess 
                    = &AppObj->errorSessionArr[i];

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
    
    IncConnStats2(&AppIface->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpLocalPortAssignFail);    

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
}

void OnConnectionInitError(TcpClientConnection_t* newConn){

    if (GetConnLastErr (newConn) 
            == TD_SOCKET_CONNECT_FAILED_IMMEDIATE
        && GetErrno(newConn) == EADDRNOTAVAIL) {

            IncConnStats2(&AppIface->appConnStats
                        , newConn->tcSess->groupConnStats 
                        , tcpConnInitFailEaddrNotAvail);
    }

    IncConnStats2(&AppIface->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnInitFail);

    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    ReleasePort (newConn);
}

void OnConnectionEstablished(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppIface->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnInitSuccess);

    SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISHED);

    SetCS1 (newConn, STATE_TCP_CONN_ESTABLISHED);
}

void OnConnectionEstablishError(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppIface->appConnStats
        , newConn->tcSess->groupConnStats 
        , tcpConnInitFail);

    int unRegisterStatus = UnRegisterForWriteEvent(AppObj->eventQ
                                                , newConn->socketFd
                                                , newConn);
    if (unRegisterStatus){
        IncConnStats2(&AppIface->appConnStats
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

    IncConnStats2(&AppIface->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnRegisterForWriteEventFail);    

    close(newConn->socketFd);
    SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);
    
    StoreErrSession (newConn->tcSess);
    SetFreeSession (newConn->tcSess);
    ReleasePort (newConn);
}

void OnWriteError(TcpClientConnection_t* newConn) {

    IncConnStats2(&AppIface->appConnStats
        , newConn->tcSess->groupConnStats 
        , tcpWriteFail);

    int unRegisterStatus = UnRegisterForWriteEvent(AppObj->eventQ
                                                , newConn->socketFd
                                                , newConn);
    if (unRegisterStatus){
        IncConnStats2(&AppIface->appConnStats
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

    IncConnStats2(&AppIface->appConnStats
        , newConn->tcSess->groupConnStats 
        , appDataWriteComplete);

    int unRegisterStatus = UnRegisterForWriteEvent(AppObj->eventQ
                                                , newConn->socketFd
                                                , newConn);
    if (unRegisterStatus){
        IncConnStats2(&AppIface->appConnStats
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
        , GetConnStats(appConnStats, tcpConnInit)
        , GetConnStats(appConnStats, tcpConnInitSuccess)
        , GetConnStats(appConnStats, tcpConnInitFail)
        , GetConnStats(appConnStats, tcpConnInitFailEaddrNotAvail)
        , GetConnStats(appConnStats, tcpConnRegisterForWriteEventFail)
        );

    puts (statsString);
}

void CleanupApp() {

    DeleteEventQ(AppObj->eventQ);

    DeleteTimerWheel(AppObj->timerWheel);

    DumpTcpClientAppStats(&AppIface->appConnStats);

    DumpErrSessions();
}

TcpClientConnection_t* PrepNextConnection(TcpClientSession_t* newSess){

    TcpClientConnection_t* newConn = &newSess->tcConn; 

    TcpClientAppConnGroup_t* csGroup 
        = &AppIface->csGroupArr[AppIface->nextCsGroupIndex];

    newConn->localAddress 
        = (struct sockaddr*)&(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

    newConn->remoteAddress 
        = (struct sockaddr*)&(csGroup->serverAddr);

    newConn->tcSess->groupConnStats = &csGroup->cStats;

    newConn->localPortPool 
        = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

    AppIface->nextCsGroupIndex += 1;
    if (AppIface->nextCsGroupIndex == AppIface->csGroupCount){
        AppIface->nextCsGroupIndex = 0;
    }
     
    csGroup->nextClientAddrIndex += 1;
    if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
        csGroup->nextClientAddrIndex = 0;
    }

    return newConn;
}

int AppRunContinue() {
    TcpClientAppConnStats_t* appConnStats = &AppIface->appConnStats;

    if ( (GetConnStats(appConnStats, tcpConnInitFail) 
                == AppIface->maxErrorSessions) 
            || (GetConnStats(appConnStats, tcpConnInit) 
                == AppIface->maxSessions 
            && IsSessionPoolEmpty (AppObj->activeSessionPool)) ){
        return 0;
    }

    return 1;
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

    while (AppRunContinue()) {

        if (GetConnStats(appConnStats, tcpConnInit) 
                < AppIface->maxSessions) {
 
            int newConnectionInits 
                = ((TimeElapsedTimerWheel(AppObj->timerWheel)
                        - lastConnectionInitTime)
                    * AppIface->connectionPerSec);

            while (newConnectionInits > 0 
                    && GetConnStats(appConnStats, tcpConnInit) 
                        < AppIface->maxSessions) {

                newConnectionInits--;

                lastConnectionInitTime 
                        = TimeElapsedTimerWheel(AppObj->timerWheel);

                TcpClientSession_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    appStats->dbgNoFreeSession++;      
                }else {
                    
                    TcpClientConnection_t* newConn = PrepNextConnection(newSess);

                    if ( AssignSocketLocalPort(newConn->localAddress
                                                , newConn->localPortPool
                                                , newConn) ) {
                        OnAssignSocketLocalPortError(newConn);
                    }else{
                        SetAppState (newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                        IncConnStats2(&AppIface->appConnStats
                                    , newConn->tcSess->groupConnStats 
                                    , tcpConnInit);

                        newConn->socketFd 
                            = TcpNewConnection(newConn->localAddress
                                                , newConn->remoteAddress
                                                , &AppIface->appConnStats
                                                , newSess->groupConnStats
                                                , newConn);

                        if ( GetConnLastErr (newConn) ) {
                            OnConnectionInitError(newConn);
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
        }

        int eCount = GetIOEvents(AppObj->eventQ
                                    , AppObj->EventArr
                                    , AppIface->maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                    = (TcpClientConnection_t*) GetIOEventData(AppObj->EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                if (IsWriteEventSet(AppObj->EventArr[eIndex])) {
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

                        if (GetConnLastErr (newConn)) {
                            OnWriteError(newConn);
                        }else {
                            newConn->bytesSent += bytesSent;
                            if (newConn->bytesSent == AppIface->csDataLen) {
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