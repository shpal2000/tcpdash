#include <sys/mman.h>

#include "iovents.h"

#define __APP__MAIN__
#include "tcp_proxy.h"



static void InitSession (TcpProxyCtx_t* tcpProxyCtx
                        , TcpProxySession_t* newSess) {

    newSess->initConn = NULL;
    newSess->acceptConn = NULL;
    newSess->appCtx = tcpProxyCtx;
}

static void OnEstablish (void* appCtx
                        , void* groupCtx
                        , struct IoVentCtx* iovCtx
                        , struct IoVentConn* iovConn) {
    
    TcpProxyCtx_t* tcpProxyCtx = (TcpProxyCtx_t*) appCtx;

    if (iovConn->sessionData == NULL) {
        TcpProxySession_t* newSess 
            = GetFromPool (tcpProxyCtx->freeSessionPool);
        if (newSess == NULL) {
            //todo; stats; close connection
        } else {
            //init session
            InitSession (tcpProxyCtx ,newSess);
            iovConn->sessionData = newSess;

            // store client side of proxied connection
            newSess->acceptConn = iovConn;
            
            // init server side of proxied connection
            TcpProxyServer_t* server = (TcpProxyServer_t*) groupCtx;
            NewConnection (iovCtx
                            , server
                            , tcpProxyCtx
                            , &iovConn->remoteAddressAccept
                            , NULL
                            , &server->serverAddrR
                            , &tcpProxyCtx->appI->gStats
                            , &server->cStats);
        }
    } else {
        // store server side of proxied connection
        TcpProxySession_t* extSess 
            = (TcpProxySession_t*) iovConn->sessionData;
        extSess->initConn = iovConn;
    }
}

static void OnWriteNext (void* appCtx
                            , void* groupCtx
                            , struct IoVentCtx* iovCtx
                            , struct IoVentConn* iovConn) {
}

static void OnWriteNextStatus (void* appCtx
                                , void* groupCtx
                                , struct IoVentCtx* iovCtx
                                , struct IoVentConn* iovConn
                                , char* writeBuffer
                                , int writeBuffOffset
                                , int writeDataLen 
                                , int bytesWritten
                                ) {
}

static void OnReadNext (void* appCtx
                                , void* groupCtx
                                , struct IoVentCtx* iovCtx
                                , struct IoVentConn* iovConn) {

}

static void OnReadNextStatus (void* appCtx
                                , void* groupCtx
                                , struct IoVentCtx* iovCtx
                                , struct IoVentConn* iovConn
                                , char* readBuffer
                                , int readBuffOffset
                                , int readDataLen 
                                , int bytesReceived
                                ) {

}

static void OnCleanup (void* appCtx
                                , void* groupCtx
                                , struct IoVentCtx* iovCtx
                                , struct IoVentConn* iovConn) {
    puts ("OnCleanup\n");
}

static void OnStatus (void* appCtx
                                , void* groupCtx
                                , struct IoVentCtx* iovCtx
                                , struct IoVentConn* iovConn) {
}

static TcpProxyCtx_t* CreateAppCtx (TcpProxyI_t* appI) {

    TcpProxyCtx_t* appCtx = CreateStruct0 (TcpProxyCtx_t);

    appCtx->appI = appI;

    CreatePool (&appCtx->freeSessionPool
                , appI->maxActiveSessions
                , TcpProxySession_t);
     
    return appCtx;
}


void TcpProxyRun (TcpProxyI_t* appI) {

    TcpProxyCtx_t* tcpProxyCtx = CreateAppCtx (appI);

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);

    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnWriteNextStatus = &OnWriteNextStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadNextStatus = &OnReadNextStatus;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);

    iovOptions->maxActiveConnections = appI->maxActiveSessions * 2;
    iovOptions->maxErrorConnections = appI->maxErrorSessions * 2;
    
    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);

    TcpProxyServer_t* server 
        = &appI->serverArr[0];

    InitServer(iovCtx
                , server
                , tcpProxyCtx
                , &server->serverAddrL
                , &tcpProxyCtx->appI->gStats
                , &server->cStats);

    while (1) {

        ProcessIoVent (iovCtx);

    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);
}

TcpProxyI_t* CreateTcpProxyInterface(int serverCount) {

    TcpProxyI_t* iFace 
        = (TcpProxyI_t*) mmap(NULL
            , sizeof (TcpProxyI_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->serverCount = serverCount;
    iFace->serverArr 
        = (TcpProxyServer_t*) mmap(NULL
            , sizeof (TcpProxyServer_t) * iFace->serverCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    return iFace;
}

void DeleteTcpProxyInterface (TcpProxyI_t* iFace){
    //todo
}

void DumpTcpProxyStats(TcpProxyStats_t* appConnStats) {
    
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



