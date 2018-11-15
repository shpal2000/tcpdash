#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#define TCP_CLIENT_MAIN
#include "tcp_client.h"

static TcAppRun_t* AppO;
static TcAppInt_t* AppI;

static TcMethods_t* TcMethods;

static void InitSession(TcSess_t* newSess
                        , int initApp) {

    TcConn_t* newConn = &newSess->tcConn; 
    
    if (initApp) {
        newSess->tcConn.tcSess = newSess;
    }

    CSInit(newConn);

    newConn->socketFd = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->bytesSent = 0;

    if (initApp == 0) {
        newConn->cSSL = SSL_new(AppO->sslContext);
    }   
    // (*TcMethods->InitSession)(newSess);
}

static TcSess_t* GetFreeSession() {

    TcSess_t* newSess 
        = GetSesionFromPool (AppO->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppO->activeSessionPool, newSess);
        InitSession(newSess, 0);
    }

    return newSess;
}

static void SetFreeSession(TcSess_t* newSess) {
    
    RemoveFromSessionPool (AppO->activeSessionPool, newSess);
    SetSessionToPool (AppO->freeSessionPool, newSess);

    TcConn_t* newConn = &newSess->tcConn;
    SSL_free(newConn->cSSL);
}

static void StoreErrSession(TcSess_t* aSession) {

    if (AppO->errorSessionCount < AppI->maxErrorSessions) {

        TcSess_t* errSession 
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

            TcSess_t* newSess 
                    = &AppO->errorSessionArr[i];

            TcConn_t* newConn 
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

static void ReleasePort(TcConn_t* newConn) {

    if (IsIpv6(newConn->localAddress)) {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in6*)newConn->localAddress)->sin6_port);
    } else {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in*)newConn->localAddress)->sin_port);
    }
}

static void CloseConnection(TcConn_t* newConn) {

    if ( IsSetCES(newConn, STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {

        StopPollReadWriteEvent(AppO->eventQ
                                , newConn->socketFd
                                , &AppI->appConnStats
                                , newConn->tcSess->groupConnStats
                                , newConn);
    }

    TcpClose(newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess);
    }

    SetFreeSession (newConn->tcSess);

    if ( IsSetCES(newConn, STATE_TCP_SOCK_FD_CLOSE_FAIL
                            | STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {
        ReleasePort(newConn);
    }
}

static void OnTcpConnectionCompletion (TcConn_t* newConn) {

    VerifyTcpConnectionEstablished (newConn->socketFd
                                    , &AppI->appConnStats
                                    , newConn->tcSess->groupConnStats
                                    , newConn);
    
    if ( GetCES(newConn) ) {
        SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISH_FAILED);
        CloseConnection(newConn);
    } else {
        SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISHED);

        PollReadWriteEvent(AppO->eventQ
                            , newConn->socketFd
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        } else {
            SSLConnectInit (newConn->cSSL
                            , newConn->socketFd
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);
        }
    }
}

static void OnWriteNextData (TcConn_t* newConn) {
    
    if ( GetAppState(newConn) == APP_STATE_CONNECTION_ESTABLISHED) {
        SSLConnectContinue (newConn->cSSL
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);
        if (IsSetCS1(newConn, STATE_SSL_CONN_ESTABLISHED)) {
            SetAppState (newConn
                        , APP_STATE_SSL_CONNECTION_ESTABLISHED);
        }
    }

    if ( GetAppState(newConn) == APP_STATE_SSL_CONNECTION_ESTABLISHED ) {
        TcSess_t* newSess = newConn->tcSess;

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
                CloseConnection(newConn);
            } else {

                newConn->bytesSent += bytesSent;

                if (newConn->bytesSent == AppI->csDataLen) {
                    CloseConnection(newConn);
                }
            }
        }
    }
}

static void OnReadNextData (TcConn_t* newConn) {
    if ( GetAppState(newConn) == APP_STATE_CONNECTION_ESTABLISHED) {
        SSLConnectContinue (newConn->cSSL
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);
        if (IsSetCS1(newConn, STATE_SSL_CONN_ESTABLISHED)) {
            SetAppState (newConn
                        , APP_STATE_SSL_CONNECTION_ESTABLISHED);
        }
    }
}

void DumpTcpClientStats(TcConnStats_t* appConnStats) {
    
    char statsString[120];

    sprintf (statsString, 
                        "%" PRIu64 "\n" 
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "\n"
        , GetConnStats(appConnStats, tcpConnInit)
        , GetConnStats(appConnStats, tcpConnInitSuccess)
        , GetConnStats(appConnStats, tcpConnInitFail)
        , GetConnStats(appConnStats, tcpConnInitFailImmediateOther)
        , GetConnStats(appConnStats, tcpConnInitFailImmediateEaddrNotAvail)
        , GetConnStats(appConnStats, tcpPollRegUnregFail)
        , GetConnStats(appConnStats, dummyCount)
        );

    puts (statsString);
}

static void InitApp() {
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    AppO->sslContext = SSL_CTX_new(SSLv23_client_method());
    SSL_CTX_set_verify(AppO->sslContext, SSL_VERIFY_NONE, 0);
    SSL_CTX_set_options(AppO->sslContext
                            , SSL_OP_NO_SSLv2 
                            | SSL_OP_NO_SSLv3 
                            | SSL_OP_NO_COMPRESSION);

    AppO->freeSessionPool = AllocEmptySessionPool();
    AppO->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < AppI->maxActiveSessions; i++) {

        TcSess_t* aSession 
            = AllocSession (TcSess_t);

        InitSession (aSession, 1);

        SetSessionToPool (AppO->freeSessionPool, aSession);
    }

    AppO->errorSessionCount = 0;
    
    AppO->errorSessionArr = CreateArray (TcSess_t
                                , AppI->maxErrorSessions); 

    AppO->EventArr = CreateArray(PollEvent_t
                                , AppI->maxEvents);

    AppO->eventQ = CreateEventQ();

    AppO->timerWheel = CreateTimerWheel();

    AppO->sendBuffer =  TdMalloc(AppI->csDataLen);
    AppO->readBuffer =  TdMalloc(AppI->scDataLen);

    AppO->eventPTO = 0;

}

static void CleanupApp() {

    DeleteEventQ(AppO->eventQ);

    DeleteTimerWheel(AppO->timerWheel);

    DumpTcpClientStats(&AppI->appConnStats);

    DumpErrSessions();

    EVP_cleanup();
}

static void InitConnection(TcConn_t* newConn){

    TcSess_t* newSess = newConn->tcSess;

    TcGroup_t* csGroup 
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
                        , &AppI->appConnStats
                        , newConn->tcSess->groupConnStats
                        , newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess);
        SetFreeSession (newConn->tcSess);

    } else {

        newConn->socketFd 
            = TcpNewConnection(newConn->localAddress
                    , newConn->remoteAddress
                    , &AppI->appConnStats
                    , newSess->groupConnStats
                    , newConn);

        if ( GetCES(newConn) ) {
            StoreErrSession (newConn->tcSess);
            SetFreeSession (newConn->tcSess);
            ReleasePort (newConn);
        } else {
            PollWriteEventOnly(AppO->eventQ
                                , newConn->socketFd
                                , &AppI->appConnStats
                                , newConn->tcSess->groupConnStats
                                , newConn);

            if ( GetCES(newConn) ) {
                CloseConnection(newConn);
            }
        }
    }
}

static int AppRunContinue() {

    TcConnStats_t* appConnStats = &AppI->appConnStats;

    if ( (GetConnStats(appConnStats, tcpConnInitFail) 
                >= AppI->maxErrorSessions) 
            || (GetConnStats(appConnStats, tcpConnInit) 
                == AppI->maxSessions 
            && IsSessionPoolEmpty (AppO->activeSessionPool)) ){
        return 0;
    }

    return 1;
}

static void AppInitSession(void* newSessParam) {
    TcAppSess_t* newSess = (TcAppSess_t*) newSessParam;
    newSess++;
    IncConnStats(&AppI->appConnStats, dummyCount); 
}

void TcpClientRun(TcAppInt_t* appIface) {

    TcMethods = CreateStruct0 (TcMethods_t);
    TcMethods->InitSession = &AppInitSession;

    AppO = CreateStruct0 (TcAppRun_t);
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

                TcSess_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    AppI->appStats.dbgNoFreeSession++;      
                }else {

                    TcConn_t* newConn = &newSess->tcConn;

                    SetAppState(newConn, APP_STATE_CONNECTION_IN_PROGRESS);

                    InitConnection(newConn);
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

                TcConn_t* newConn 
                    = (TcConn_t*) 
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
                    OnReadNextData (newConn);
                }
            }
        }else{
            if (AppO->eventPTO < MAX_POLL_TIMEOUT) {
                AppO->eventPTO++;
            }
        }
    }

    epochSinceSeconds = time(NULL);
    printf("-- %ld\n", epochSinceSeconds);

    CleanupApp();
    
    AppI->isRunning = 0;
}

TcAppInt_t* CreateTcpClientInterface(int csGroupCount
                                            , int* clientAddrCounts) {

    TcAppInt_t* iFace 
        = (TcAppInt_t*) mmap(NULL
            , sizeof (TcAppInt_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TcGroup_t*) mmap(NULL
            , sizeof (TcGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    iFace->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < iFace->csGroupCount; gIndex++) {
        TcGroup_t* csGroup = &iFace->csGroupArr[gIndex];
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

void DeleteTcpClientInterface (TcAppInt_t* iFace){
    //todo
}