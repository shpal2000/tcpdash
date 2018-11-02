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

static TcpClient_t* AppO;
static TcpClientInterface_t* AppI;

static void InitSession(TcpClientSession_t* newSess, int initApp) {

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

static TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* newSess 
        = GetSesionFromPool (AppO->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppO->activeSessionPool, newSess);
        InitSession(newSess, 0);
    }

    return newSess;
}

static void SetFreeSession(TcpClientSession_t* newSess) {
    
    RemoveFromSessionPool (AppO->activeSessionPool, newSess);
    SetSessionToPool (AppO->freeSessionPool, newSess);
}

static void StoreErrSession(TcpClientSession_t* aSession) {
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
                    ", ES = %#018" PRIx64 
                    ", SysErr = %d"
                    ", SockErr = %d"
                    ", src = %s : %hu"
                    ", dst = %s : %hu\n"
                    , GetCS1(newConn)
                    , GetCES(newConn)
                    , GetSysErrno(newConn) 
                    , GetSockErrno(newConn) 
                    , srcAddr, newConn->savedLocalPort
                    , dstAddr, newConn->savedRemotePort); 
        }
    }
}

static void ReleasePort(TcpClientConnection_t* newConn){
    if (IsIpv6(newConn->localAddress)) {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in6*)newConn->localAddress)->sin6_port);
    } else {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in*)newConn->localAddress)->sin_port);
    }
}

static void RegisterForReadEventHelper(TcpClientConnection_t* newConn) {

    RegisterForReadEvent(AppO->eventQ
                            , newConn->socketFd
                            , newConn);

    if ( GetCES(newConn) & STATE_TCP_SOCK_READ_REG_FAIL) {
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnRegisterForReadEventFail);
    } 
}

static void RegisterForWriteEventHelper(TcpClientConnection_t* newConn) {

    RegisterForWriteEvent(AppO->eventQ
                            , newConn->socketFd
                            , newConn);

    if ( GetCES(newConn) & STATE_TCP_SOCK_WRITE_REG_FAIL) {
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnRegisterForWriteEventFail);
    } 
}

static void UnRegisterForReadWriteEventHelper(TcpClientConnection_t* newConn) {

    UnRegisterForReadWriteEvent(AppO->eventQ
                            , newConn->socketFd
                            , newConn);

    if ( GetCES(newConn) & STATE_TCP_SOCK_READ_WRITE_UNREG_FAIL) {
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnUnRegisterForReadWriteEventFail);
    } 
}

static void UnRegisterForReadEventHelper(TcpClientConnection_t* newConn) {

    UnRegisterForReadEvent(AppO->eventQ
                            , newConn->socketFd
                            , newConn);

    if ( GetCES(newConn) & STATE_TCP_SOCK_READ_UNREG_FAIL) {
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnUnRegisterForReadEventFail);
    } 
}

static void UnRegisterForWriteEventHelper(TcpClientConnection_t* newConn) {

    UnRegisterForWriteEvent(AppO->eventQ
                            , newConn->socketFd
                            , newConn);

    if ( GetCES(newConn) & STATE_TCP_SOCK_WRITE_UNREG_FAIL) {
        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnUnRegisterForWriteEventFail);
    } 
}

static void CloseConnection(TcpClientConnection_t* newConn) {

    if ( (GetCS1(newConn) & STATE_TCP_REGISTER_READ) 
            && (GetCS1(newConn) & STATE_TCP_REGISTER_WRITE) ) {

        UnRegisterForReadWriteEventHelper(newConn);    

    } else if ( GetCS1(newConn) & STATE_TCP_REGISTER_READ ) {

        UnRegisterForReadEventHelper(newConn);    

    } else if ( GetCS1(newConn) & STATE_TCP_REGISTER_WRITE ) {

        UnRegisterForWriteEventHelper(newConn);
    }

    TcpClose(newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess);
    }

    SetFreeSession (newConn->tcSess);

    if ( (GetCES(newConn) 
            & ( STATE_TCP_SOCK_FD_CLOSE_FAIL
                | STATE_TCP_SOCK_READ_UNREG_FAIL 
                | STATE_TCP_SOCK_WRITE_UNREG_FAIL 
                | STATE_TCP_SOCK_READ_WRITE_UNREG_FAIL)) == 0) {

        ReleasePort(newConn);
    }
}

static void OnTcpConnectionCompletion (TcpClientConnection_t* newConn) {

    VerifyTcpConnectionEstablished (newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {

        IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpConnInitFail);

    } else {

        IncConnStats2(&AppI->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnInitSuccess);

        SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISHED);

        RegisterForReadEventHelper(newConn);
    }

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    }
}

static void OnWriteNextData (TcpClientConnection_t* newConn) {
    
    TcpClientSession_t* newSess = newConn->tcSess;

    if (newConn->bytesSent < AppI->csDataLen ) {

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

        if ( GetCES(newConn) ) {

            IncConnStats2(&AppI->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpWriteFail);

        } else {

            newConn->bytesSent += bytesSent;

            if (newConn->bytesSent == AppI->csDataLen) {
                UnRegisterForWriteEventHelper(newConn);
            }
        }
    }

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    }
}

static void InitApp() {

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

    AppO->EventArr = CreateArray(PollEvent_t
                                , AppI->maxEvents);

    AppO->eventQ = CreateEventQ();

    AppO->timerWheel = CreateTimerWheel();

    AppO->sendBuffer =  TdMalloc(AppI->csDataLen);
    AppO->readBuffer =  TdMalloc(AppI->scDataLen);
}

void DumpTcpClientStats(TcpClientConnStats_t* appConnStats) {
    
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

static void CleanupApp() {

    DeleteEventQ(AppO->eventQ);

    DeleteTimerWheel(AppO->timerWheel);

    DumpTcpClientStats(&AppI->appConnStats);

    DumpErrSessions();
}

static void InitNewConnection(TcpClientConnection_t* newConn){

    TcpClientSession_t* newSess = newConn->tcSess;

    TcpClientConnGroup_t* csGroup 
        = &AppI->csGroupArr[AppI->nextCsGroupIndex];

    newConn->localAddress 
        = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

    newConn->remoteAddress 
        = &(csGroup->serverAddr);

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

    AssignSocketLocalPort(newConn->localAddress
                        , newConn->localPortPool
                        , newConn);

    if ( GetCES(newConn) ) {

        IncConnStats2(&AppI->appConnStats
                    , newConn->tcSess->groupConnStats 
                    , tcpLocalPortAssignFail);    

        StoreErrSession (newConn->tcSess);
        SetFreeSession (newConn->tcSess);

    } else {

        IncConnStats2(&AppI->appConnStats
                    , newConn->tcSess->groupConnStats 
                    , tcpConnInit);

        newConn->socketFd 
            = TcpNewConnection(newConn->localAddress
                    , newConn->remoteAddress
                    , &AppI->appConnStats
                    , newSess->groupConnStats
                    , newConn);

        if ( GetCES(newConn) ) {

            if ( IsSetCES(newConn, STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE) 
                    && (GetSysErrno(newConn) == EADDRNOTAVAIL) ) {

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
        } else {
            RegisterForWriteEventHelper(newConn);

            if ( GetCES(newConn) ) {
               CloseConnection(newConn);
            }
        }
    }
}

static int AppRunContinue() {

    TcpClientConnStats_t* appConnStats = &AppI->appConnStats;

    if ( (GetConnStats(appConnStats, tcpConnInitFail) 
                >= AppI->maxErrorSessions) 
            || (GetConnStats(appConnStats, tcpConnInit) 
                == AppI->maxSessions 
            && IsSessionPoolEmpty (AppO->activeSessionPool)) ){
        return 0;
    }

    return 1;
}

void TcpClientRun(TcpClientInterface_t* appIface){
    AppO = CreateStruct0 (TcpClient_t);
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

            while ( (newConnectionInits > 0) 
                    && (GetConnStats(&AppI->appConnStats, tcpConnInit) 
                        < AppI->maxSessions) ) {

                newConnectionInits--;

                lastConnectionInitTime 
                        = TimeElapsedTimerWheel(AppO->timerWheel);

                TcpClientSession_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    AppI->appStats.dbgNoFreeSession++;      
                }else {

                    TcpClientConnection_t* newConn = &newSess->tcConn;

                    SetAppState(newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                    InitNewConnection(newConn);
                }
            }
        }

        int eCount = GetIOEvents(AppO->eventQ
                                    , AppO->EventArr
                                    , AppI->maxEvents
                                    , AppO->eventPTO);
        if (eCount > 0) {

            AppO->eventPTO = 0;

            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                    = (TcpClientConnection_t*) 
                        GetIOEventData(AppO->EventArr[eIndex]);

                //handle write event
                if (IsWriteEventSet(AppO->EventArr[eIndex]) 
                                    && !IsFdClosed(newConn) ) {
                    
                    if ( GetAppState(newConn) 
                            == APP_STATE_CONNECTION_IN_PROGRESS ) {

                        OnTcpConnectionCompletion(newConn);

                    } else if ( GetAppState(newConn) 
                            == APP_STATE_CONNECTION_ESTABLISHED ) {

                        OnWriteNextData (newConn);   
                    }
                }

                //handle read event
                if (IsReadEventSet(AppO->EventArr[eIndex])
                                    && !IsFdClosed(newConn) ) {
                }
            }
        }else{
            AppO->eventPTO++;
        }
    }

    epochSinceSeconds = time(NULL);
    printf("-- %ld\n", epochSinceSeconds);

    CleanupApp();
    
    AppI->isRunning = 0;
}

TcpClientInterface_t* CreateTcpClientInterface(int csGroupCount
                                            , int* clientAddrCounts){
    TcpClientInterface_t* iFace 
        = (TcpClientInterface_t*) mmap(NULL
            , sizeof (TcpClientInterface_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TcpClientConnGroup_t*) mmap(NULL
            , sizeof (TcpClientConnGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    iFace->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < iFace->csGroupCount; gIndex++) {
        TcpClientConnGroup_t* csGroup = &iFace->csGroupArr[gIndex];
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

void DeleteTcpClientInterface (TcpClientInterface_t* iFace){
    //todo
}