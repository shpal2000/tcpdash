#include <sys/mman.h>

#include "iovents.h"

#define __APP__MAIN__
#include "tcp_proxy.h"


static void InitRwBuff (RwBuff_t* newBuff) {

    newBuff->buffLen = RW_MAX_BUFF_LEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void InitSession (TcpProxyAppCtx_t* appCtx
                        , TcpProxySession_t* newSess) {

    newSess->initiatedConn = NULL;
    newSess->acceptedConn = NULL;
    newSess->appCtx = appCtx;

    newSess->aConnReadBuff = NULL;
    newSess->iConnReadBuff = NULL;
    
    InitPool (&newSess->aConnWriteBuffQ);
    InitPool (&newSess->iConnWriteBuffQ);
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
            newSess->acceptedConn = iovConn;
            
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
        extSess->initiatedConn = iovConn;
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    // puts ("OnWriteNext");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    
    if (newSess->acceptedConn == iovConn) {
        RwBuff_t* writeBuff = GetFromPool (&newSess->aConnWriteBuffQ);
        if (writeBuff) {
            newSess->aConnWriteBuff = writeBuff;
            WriteNextData (newSess->acceptedConn
                            ,  writeBuff->dataBuff
                            , 0
                            , writeBuff->dataLen
                            , 0);
            }
    } else if (newSess->initiatedConn == iovConn) {
        RwBuff_t* writeBuff = GetFromPool (&newSess->iConnWriteBuffQ);
        if (writeBuff) {
            newSess->iConnWriteBuff = writeBuff;
            WriteNextData (newSess->initiatedConn
                            ,  writeBuff->dataBuff
                            , 0
                            , writeBuff->dataLen
                            , 0);
            }
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                            , int bytesWritten
                            ) {
    puts ("OnWriteStatus");

    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->acceptedConn == iovConn) {
        AddToPool (appCtx->freeBuffPool, newSess->aConnWriteBuff);
        newSess->aConnWriteBuff = NULL;
    } else if (newSess->initiatedConn == iovConn) {
        AddToPool (appCtx->freeBuffPool, newSess->iConnWriteBuff);
        newSess->iConnWriteBuff = NULL;
    }
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
        
        if ( newSess->acceptedConn == iovConn ) {
            newSess->aConnReadBuff = readBuff;
            ReadNextData (newSess->acceptedConn
                    , readBuff->dataBuff
                    , 0
                    , readBuff->buffLen);

        } else if (newSess->initiatedConn == iovConn) {
            newSess->iConnReadBuff = readBuff;
            ReadNextData (newSess->initiatedConn
                    , readBuff->dataBuff
                    , 0
                    , readBuff->buffLen);

        }
    }
}

static void OnReadStatus (struct IoVentConn* iovConn
                            , int bytesReceived
                            ) {

    puts ("OnReadStatus");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    if (newSess->acceptedConn == iovConn) {
        newSess->aConnReadBuff->dataLen = bytesReceived;
        AddToPool (&newSess->iConnWriteBuffQ, newSess->aConnReadBuff);
        newSess->aConnReadBuff = NULL;
    } else if (newSess->initiatedConn == iovConn) {
        newSess->iConnReadBuff->dataLen = bytesReceived;
        AddToPool (&newSess->aConnWriteBuffQ, newSess->iConnReadBuff);
        newSess->iConnReadBuff = NULL;
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {
    puts ("OnCleanup\n");
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



