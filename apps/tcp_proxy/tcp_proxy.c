#include <sys/mman.h>

#include "iovents.h"

#define __APP__MAIN__
#include "tcp_proxy.h"



static void InitSession (TcpProxyCtx_t* tcpProxyCtx
                        , TcpProxySession_t* newSess) {

    newSess->initiatedConn = NULL;
    newSess->acceptedConn = NULL;
    newSess->appCtx = tcpProxyCtx;
    newSess->aConnRBuff = NULL;
    newSess->iConnRBuff = NULL;
}

static void InitRwBuff (RwBuff_t* newBuff) {

    newBuff->buffLen = RW_MAX_BUFF_LEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void OnEstablish (struct IoVentConn* iovConn) {
    
    TcpProxyCtx_t* appCtx 
        = (TcpProxyCtx_t*) iovConn->cInfo.appCtx;

    if (iovConn->cInfo.sessionData == NULL) {
        TcpProxySession_t* newSess 
            = GetFromPool (appCtx->freeSessionPool);
        if (newSess == NULL) {
            //todo; stats; close connection
        } else {
            //init session
            InitSession (appCtx, newSess);
            iovConn->cInfo.sessionData = newSess;

            // store client side of proxied connection
            newSess->acceptedConn = iovConn;
            puts("accepted");
            
            // init server side of proxied connection
            TcpProxyServer_t* server 
                = (TcpProxyServer_t*) iovConn->cInfo.groupCtx;
            NewConnection (iovConn->cInfo.iovCtx
                            , server
                            , appCtx
                            , iovConn->cInfo.sessionData
                            , &server->serverAddrL//&iovConn->remoteAddressAccept
                            , NULL
                            , &server->serverAddrR
                            , &appCtx->appI->gStats
                            , &server->cStats);
        }
    } else {
        // store server side of proxied connection
        TcpProxySession_t* extSess 
            = (TcpProxySession_t*) iovConn->cInfo.sessionData;
        extSess->initiatedConn = iovConn;
                    puts("established");

    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess == NULL
            || (newSess->acceptedConn == iovConn 
                                && (newSess->iConnRBuff == NULL
                                    || newSess->iConnRBuff->dataLen == 0) ) 
            || (newSess->initiatedConn == iovConn 
                                && (newSess->aConnRBuff == NULL
                                    || newSess->aConnRBuff->dataLen == 0) ) 
                                    ) {
        return;
    }

    RwBuff_t* rwBuff;

    if (newSess->acceptedConn == iovConn) {
        rwBuff = newSess->iConnRBuff;
    } else {
        rwBuff = newSess->aConnRBuff;
    }

    WriteNextData (iovConn
                , rwBuff->dataBuff
                , 0
                , rwBuff->dataLen);
}

static void OnWriteStatus (struct IoVentConn* iovConn
                            , int bytesWritten
                            ) {

    TcpProxyCtx_t* appCtx 
        = (TcpProxyCtx_t*) iovConn->cInfo.appCtx;

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->acceptedConn == iovConn) {
        newSess->iConnRBuff->buffOffset += bytesWritten;

        if (newSess->iConnRBuff->buffOffset
                == newSess->iConnRBuff->dataLen) {
            AddToPool (appCtx->freeBuffPool, newSess->iConnRBuff);
            newSess->iConnRBuff = NULL;
        }
    } else {
        newSess->aConnRBuff->buffOffset += bytesWritten;

        if (newSess->aConnRBuff->buffOffset
                == newSess->aConnRBuff->dataLen) {
            AddToPool (appCtx->freeBuffPool, newSess->aConnRBuff);
            newSess->aConnRBuff = NULL;
        }
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess == NULL
            || (newSess->acceptedConn == iovConn && newSess->aConnRBuff) 
            || (newSess->initiatedConn == iovConn && newSess->iConnRBuff) ) {
        return;
    }

    TcpProxyCtx_t* appCtx 
        = (TcpProxyCtx_t*) iovConn->cInfo.appCtx;
    
    RwBuff_t* rwBuff =
        GetFromPool (appCtx->freeBuffPool);

    if (rwBuff == NULL) {
        //todo; stats; close connection
    } else {
        InitRwBuff (rwBuff);
        if (newSess->acceptedConn == iovConn) {
            newSess->aConnRBuff = rwBuff;
        } else {
            newSess->iConnRBuff = rwBuff;
        }

        ReadNextData (iovConn
                , rwBuff->dataBuff
                , 0
                , rwBuff->buffLen);
    }
}

static void OnReadStatus (struct IoVentConn* iovConn
                            , int bytesReceived
                            ) {

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->acceptedConn == iovConn) {
        newSess->aConnRBuff->dataLen = bytesReceived;
    } else {
        newSess->iConnRBuff->dataLen = bytesReceived;
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {
    puts ("OnCleanup\n");
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static TcpProxyCtx_t* CreateAppCtx (TcpProxyI_t* appI) {

    TcpProxyCtx_t* appCtx = CreateStruct0 (TcpProxyCtx_t);

    appCtx->appI = appI;

    CreatePool (&appCtx->freeSessionPool
                , appI->maxActiveSessions
                , TcpProxySession_t);

    CreatePool (&appCtx->freeBuffPool
                , appI->maxActiveSessions * 16
                , RwBuff_t);

    return appCtx;
}


void TcpProxyRun (TcpProxyI_t* appI) {

    TcpProxyCtx_t* tcpProxyCtx = CreateAppCtx (appI);

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);

    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnWriteStatus = &OnWriteStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadStatus = &OnReadStatus;
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
                , &server->serverAddrP
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



