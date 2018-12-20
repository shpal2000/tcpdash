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

   DisableWriteNotification (iovConn);

    if (iovConn->cInfo.sessionData == NULL) {
        // puts ("accepted");
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
        // puts ("established");
        TcpProxySession_t* extSess 
            = (TcpProxySession_t*) iovConn->cInfo.sessionData;

        extSess->iConn.iovConn = iovConn;
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

        TcpProxyConn_t* tpConn = NULL;

        if (newSess->aConn.iovConn == iovConn) {
            tpConn = &newSess->aConn;
        } else if (newSess->iConn.iovConn == iovConn) {
            tpConn = &newSess->iConn;
        }

        if (tpConn) {
            tpConn->readBuff = readBuff;
            ReadNextData (tpConn->iovConn
                        , readBuff->dataBuff
                        , 0
                        , readBuff->buffLen);
        }
    }
}

static void OnReadStatus (struct IoVentConn* iovConn
                            , int bytesReceived
                            ) {

    // puts ("OnReadStatus");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    TcpProxyConn_t* tpConn = NULL;
    TcpProxyConn_t* tpConnOther = NULL;
    Pool_t* otherWriteQ = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
        tpConnOther = &newSess->iConn;
        otherWriteQ = &newSess->iConn.writeQ;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
        tpConnOther = &newSess->aConn;
        otherWriteQ = &newSess->aConn.writeQ;
    }

    if (tpConn) {
        tpConn->readBuff->dataLen = bytesReceived;
        AddToPool (otherWriteQ, tpConn->readBuff);
        tpConn->readBuff = NULL;
        if (tpConnOther && tpConnOther->iovConn) {
            EnableWriteNotification (tpConnOther->iovConn);
        }
   }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    // puts ("OnWriteNext");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;
    
    TcpProxyConn_t* tpConn = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
    }

    if (tpConn) {
        RwBuff_t* tmpBuff = GetFromPool (&tpConn->writeQ);
        if (tmpBuff) {
            tpConn->writeBuff = tmpBuff;
            WriteNextData (tpConn->iovConn
                            , tmpBuff->dataBuff
                            , 0
                            , tmpBuff->dataLen
                            , 0);
        }
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                            , int bytesWritten
                            ) {
    // puts ("OnWriteStatus");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    TcpProxyConn_t* tpConn = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
    }

    if (tpConn) {

        if (IsPoolEmpty (&tpConn->writeQ) ) {
            DisableWriteNotification (tpConn->iovConn);
        }      

        AddToPool (newSess->appCtx->freeBuffPool
                            , tpConn->writeBuff);

        tpConn->writeBuff = NULL;
    }
}

static void OnClose (struct IoVentConn* iovConn
                                , int iovConnErr) {
    
    // puts ("OnOnClose\n");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    TcpProxyConn_t* tpConnOther = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConnOther = &newSess->iConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConnOther = &newSess->aConn;
    }

    if (tpConnOther && tpConnOther->iovConn) {

        switch (iovConnErr) {

            case ON_CLOSE_ERROR_NONE:
                MarkEof (tpConnOther->iovConn, MARK_EOF_WITH_TCP_FIN);
                break;

            case ON_CLOSE_ERROR_TCP_RESET:
                MarkEof (tpConnOther->iovConn, MARK_EOF_WITH_TCP_RST);
                break;

            case ON_CLOSE_ERROR_TCP_TIMEOUT:
                MarkEof (tpConnOther->iovConn, MARK_EOF_WITH_TCP_RST);
                break;

            default:
                break;
        }
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    // puts ("OnCleanup\n");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    TcpProxyConn_t* tpConn = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
    }

    if (tpConn) {
        if (tpConn->readBuff) {
            AddToPool (newSess->appCtx->freeBuffPool
                                , tpConn->readBuff);
            tpConn->readBuff = NULL;
        }

        if (tpConn->writeBuff) {
            AddToPool (newSess->appCtx->freeBuffPool
                                , tpConn->writeBuff);
            tpConn->writeBuff = NULL;
        }

         
        while (1) {
            RwBuff_t* tmpBuff = GetFromPool (&tpConn->writeQ);
            if (tmpBuff == NULL) {
                break;
            }
            AddToPool (newSess->appCtx->freeBuffPool, tmpBuff);
        }
    }

    int aConnClosed = 0;
    if (newSess->aConn.iovConn) {
        if ( IsSetCS1 (newSess->aConn.iovConn, STATE_TCP_SOCK_FD_CLOSE) ) {
            aConnClosed = 1;
        }
    } else {
        aConnClosed = 1;
    }

    int iConnClosed = 0;
    if (newSess->iConn.iovConn) {
        if ( IsSetCS1 (newSess->iConn.iovConn, STATE_TCP_SOCK_FD_CLOSE) ) {
            iConnClosed = 1;
        }
    } else {
        iConnClosed = 1;
    }

    if (aConnClosed && iConnClosed) {
        // todo; update stats
        puts ("Session freeed\n");
        AddToPool (newSess->appCtx->freeSessionPool, newSess);
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





