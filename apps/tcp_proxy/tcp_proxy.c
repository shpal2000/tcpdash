#include <sys/mman.h>

#include "iovents.h"

#define __APP__MAIN__
#include "tcp_proxy.h"


static void InitRwBuff (RwBuff_t* newBuff) {

    newBuff->buffLen = RW_MAX_BUFF_LEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void InitConn (TcpProxyConn_t * tpConn) {

    tpConn->iovConn = NULL;
    tpConn->readBuff = NULL;
    tpConn->writeBuff = NULL;
    InitPool (&tpConn->writeQ);
}

static void InitSession (TcpProxyAppCtx_t* appCtx
                        , TcpProxySession_t* newSess) {

    newSess->appCtx = appCtx;
    InitConn (&newSess->aConn);
    InitConn (&newSess->iConn);
}

static void OnEstablish (struct IoVentConn* iovConn) {
    
    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    if (iovConn->cInfo.sessionData == NULL) {
        puts ("accepted");
        TcpProxySession_t* newSess 
            = GetFromPool (appCtx->freeSessionPool);
        if (newSess == NULL) {
            //todo; stats; close connection
        } else {
            //init session
            InitSession (appCtx, newSess);
            iovConn->cInfo.sessionData = newSess;

            // store client side of proxied connection
            newSess->aConn.iovConn = iovConn;
            
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
        puts ("established");
        TcpProxySession_t* extSess 
            = (TcpProxySession_t*) iovConn->cInfo.sessionData;
        extSess->iConn.iovConn = iovConn;
    }
}

static void OnWriteNextHelper (struct IoVentConn* iovConn
                                , Pool_t* writeQ
                                , RwBuff_t** writeBufStateAddr ) {

    RwBuff_t* tmpBuff = GetFromPool (writeQ);
    if (tmpBuff) {
        *writeBufStateAddr = tmpBuff;
        WriteNextData (iovConn
                        ,  tmpBuff->dataBuff
                        , 0
                        , tmpBuff->dataLen
                        , 0);
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    // puts ("OnWriteNext");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    
    if (newSess->aConn.iovConn == iovConn) {

        OnWriteNextHelper (iovConn
                            , &newSess->aConnWriteBuffQ
                            , &newSess->aConnWriteBuff);

    } else if (newSess->iConn.iovConn == iovConn) {

        OnWriteNextHelper (iovConn
                            , &newSess->iConnWriteBuffQ
                            , &newSess->iConnWriteBuff);
    }
}

static void OnWriteStatusHelper (Pool_t* freeBuffPool
                                ,  RwBuff_t** writeBuf) {
    AddToPool (freeBuffPool, *writeBuf);
    *writeBuf = NULL;
}

static void OnWriteStatus (struct IoVentConn* iovConn
                            , int bytesWritten
                            ) {
    puts ("OnWriteStatus");

    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->aConn.iovConn == iovConn) {
        OnWriteStatusHelper (appCtx->freeBuffPool, &newSess->aConnWriteBuff);
    } else if (newSess->iConn.iovConn == iovConn) {
        OnWriteStatusHelper (appCtx->freeBuffPool, &newSess->iConnWriteBuff);
    }
}

static void OnReadNextHelper (struct IoVentConn* iovConn
                                , RwBuff_t* nextReadBuff
                                , RwBuff_t** readBuffStateAddr) {

    *readBuffStateAddr = nextReadBuff;
    ReadNextData (iovConn
            , nextReadBuff->dataBuff
            , 0
            , nextReadBuff->buffLen);
}

static void OnReadNext (struct IoVentConn* iovConn) {

    // puts ("OnReadNext");

    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    RwBuff_t* readBuff = GetFromPool (appCtx->freeBuffPool);

    if (readBuff == NULL) {
        //todo
    } else { 
        InitRwBuff (readBuff);
        
        if ( newSess->aConn.iovConn == iovConn ) {
            OnReadNextHelper (iovConn
                                , readBuff
                                , &newSess->aConnReadBuff);
        } else if (newSess->iConn.iovConn == iovConn) {
            OnReadNextHelper (iovConn
                                , readBuff
                                , &newSess->iConnReadBuff);
        }
    }
}

static void OnReadStatusHelper (Pool_t* otherWriteQ
                                , RwBuff_t** readBuffStateAddr
                                , int bytesReceived) {
    (*readBuffStateAddr)->dataLen = bytesReceived;
    AddToPool (otherWriteQ, (*readBuffStateAddr));
    *readBuffStateAddr = NULL;
}

static void OnReadStatus (struct IoVentConn* iovConn
                            , int bytesReceived
                            ) {

    puts ("OnReadStatus");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->aConn.iovConn == iovConn) {
        OnReadStatusHelper (&newSess->iConnWriteBuffQ
                            , &newSess->aConnReadBuff
                            , bytesReceived);
    } else if (newSess->iConn.iovConn == iovConn) {
        OnReadStatusHelper (&newSess->aConnWriteBuffQ
                            , &newSess->iConnReadBuff
                            , bytesReceived);
    }
}

static void OnCleanupHelper (Pool_t* freeBuffPool
                                , Pool_t* writeQ
                                , RwBuff_t** readBufStateAddr
                                , RwBuff_t** writeBufStateAddr
                                ) {

    if (*readBufStateAddr) {
        AddToPool (freeBuffPool, *readBufStateAddr);
        *readBufStateAddr = NULL;
    }

    if (*writeBufStateAddr) {
        AddToPool (freeBuffPool, *writeBufStateAddr);
        *writeBufStateAddr = NULL;
    }

    RwBuff_t* tmpBuff = GetFromPool (writeQ); 
    while (tmpBuff) {
        AddToPool (freeBuffPool, tmpBuff);
        tmpBuff = GetFromPool (writeQ);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    puts ("OnCleanup\n");

    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->aConn.iovConn == iovConn) {
        OnCleanupHelper (appCtx->freeBuffPool
                            , &newSess->aConnWriteBuffQ
                            , &newSess->aConnReadBuff
                            , &newSess->aConnWriteBuff);
    } else if (newSess->iConn.iovConn == iovConn) {
        OnCleanupHelper (appCtx->freeBuffPool
                            , &newSess->iConnWriteBuffQ
                            , &newSess->iConnReadBuff
                            , &newSess->iConnWriteBuff);
    }
}

static void OnCloseHelper (struct IoVentConn* iovConn) {

    //change this to iovent API
    SetCS1(iovConn, STATE_NO_MORE_WRITE_DATA 
                                    | STATE_TCP_TO_SEND_FIN);
}

static void OnClose (struct IoVentConn* iovConn) {
    puts ("OnOnClose\n");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->aConn.iovConn == iovConn) {
        if (newSess->iConn.iovConn) {
            OnCloseHelper (newSess->iConn.iovConn);
        }
    } else if (newSess->iConn.iovConn == iovConn) {
        if (newSess->aConn.iovConn) {
            OnCloseHelper (newSess->aConn.iovConn);
        }
    }
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static TcpProxyAppCtx_t* CreateAppCtx (TcpProxyI_t* appI) {

    TcpProxyAppCtx_t* appCtx = CreateStruct0 (TcpProxyAppCtx_t);

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

    TcpProxyAppCtx_t* TcpProxyAppCtx = CreateAppCtx (appI);

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);

    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnWriteStatus = &OnWriteStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadStatus = &OnReadStatus;
    iovMethods->OnClose = &OnClose;
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
                , TcpProxyAppCtx
                , &server->serverAddrP
                , &TcpProxyAppCtx->appI->gStats
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



