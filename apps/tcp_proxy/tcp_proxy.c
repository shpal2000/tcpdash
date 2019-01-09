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
    tpConn->isActive = 1;

    InitPool (&tpConn->writeQ);
}

static void InitSession (TcpProxySession_t* newSess
                            , TcpProxyAppCtx_t* appCtx) {

    newSess->appCtx = appCtx;
    InitConn (&newSess->aConn);
    InitConn (&newSess->iConn);
}

static void OnEstablish (struct IoVentConn* iovConn) {
    
    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    if (iovConn->cInfo.sessionData == NULL) {
        // puts ("accepted");
        TcpProxySession_t* newSess 
            = GetFromPool (appCtx->freeSessionPool);
        if (newSess == NULL) {
            //todo; stats; close connection
        } else {
            AddToPool (&appCtx->activeSessionPool, newSess);
            //init session
            InitSession (newSess, appCtx);
            iovConn->cInfo.sessionData = newSess;

            // store client side of proxied connection
            newSess->aConn.iovConn = iovConn;

            setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ 15000 }, sizeof(int));

            TcpProxyServer_t* server 
                = (TcpProxyServer_t*) iovConn->cInfo.groupCtx;

            // uint16_t localPort;
            // GET_SOCK_PORT (&iovConn->remoteAddressAccept, &localPort);
            // iovConn->remoteAddressAccept = server->serverAddrL;
            // SET_SOCK_PORT (&iovConn->remoteAddressAccept, localPort);

            // int newConnInitErr
            //             = NewConnection (iovConn->cInfo.iovCtx
            //                 , server
            //                 , iovConn->cInfo.sessionData
            //                 //, &server->serverAddrL
            //                 , &iovConn->remoteAddressAccept 
            //                 , NULL
            //                 , &server->serverAddrR
            //                 , &appCtx->appI->gStats
            //                 , &server->cStats);

            int newConnInitErr
                        = NewConnection (iovConn->cInfo.iovCtx
                            , server
                            , iovConn->cInfo.sessionData
                            , &iovConn->remoteAddressAccept 
                            , NULL
                            , &iovConn->localAddressAccept
                            , &appCtx->appI->gStats
                            , &server->cStats);
            
            if (newConnInitErr) {
                //update stats
                newSess->iConn.isActive = 0;
                AbortConnection (newSess->aConn.iovConn);
            } else {
                DisableReadWriteNotification (newSess->aConn.iovConn);
            }
        }
    } else {
        // store server side of proxied connection
        // puts ("established");
        TcpProxySession_t* newSess 
            = (TcpProxySession_t*) iovConn->cInfo.sessionData;

        newSess->iConn.iovConn = iovConn;

        if ( IsConnErr (iovConn) ) {
            //update stats
            AbortConnection (newSess->aConn.iovConn);
        } else {
            EnableReadOnlyNotification (newSess->iConn.iovConn);
            EnableReadOnlyNotification (newSess->aConn.iovConn);
        }
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    // puts ("OnReadNext");

    TcpProxyAppCtx_t* appCtx 
        = (TcpProxyAppCtx_t*) iovConn->cInfo.appCtx;

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;


    TcpProxyConn_t* tpConn = NULL;
    TcpProxyConn_t* tpConnOther = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
        tpConnOther = &newSess->iConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
        tpConnOther = &newSess->aConn;
    }

    if (tpConn && tpConnOther && tpConnOther->iovConn) {

        RwBuff_t* readBuff = GetFromPool (appCtx->freeBuffPool);

        if (readBuff == NULL) {
            //todo
        } else {
            InitRwBuff (readBuff);
            tpConn->readBuff = readBuff;
            ReadNextData (tpConn->iovConn
                        , readBuff->dataBuff
                        , 0
                        , readBuff->buffLen);
        }
    }
}

static void OnReadStatus (struct IoVentConn* iovConn
                            , int bytesReceived) {

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
        if (bytesReceived > 0) { //data

            tpConn->readBuff->dataLen = bytesReceived;
            AddToPool (otherWriteQ, tpConn->readBuff);
            tpConn->readBuff = NULL;
            EnableWriteNotification (tpConnOther->iovConn);

        } else { // remote close

            int closeErr = bytesReceived;

            switch (closeErr) {
                case ON_CLOSE_ERROR_NONE:
                    WriteClose (tpConnOther->iovConn);
                    break;
                default:
                    AbortConnection (tpConnOther->iovConn);
                    break;
            }
        }
   }else {
       //update stats
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
    TcpProxyConn_t* tpConnOther = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
        tpConnOther = &newSess->iConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
        tpConnOther = &newSess->aConn;
    }

    if (tpConn) {

        if (bytesWritten > 0) {

            if (IsPoolEmpty (&tpConn->writeQ) ) {
                DisableWriteNotification (tpConn->iovConn);
            }      

            AddToPool (newSess->appCtx->freeBuffPool
                                , tpConn->writeBuff);

            tpConn->writeBuff = NULL;
            
        } else {

            int closeErr = bytesWritten;

            switch (closeErr) {
                case ON_CLOSE_ERROR_NONE:
                    WriteClose (tpConnOther->iovConn);
                    break;
                default:
                    AbortConnection (tpConnOther->iovConn);
                    break;
            }            
        }
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    // puts ("OnCleanup\n");

    TcpProxySession_t* newSess 
        = (TcpProxySession_t*) iovConn->cInfo.sessionData;

    TcpProxyConn_t* tpConn = NULL;
    TcpProxyConn_t* tpConnOther = NULL;

    if (newSess->aConn.iovConn == iovConn) {
        tpConn = &newSess->aConn;
        tpConnOther = &newSess->iConn;
    } else if (newSess->iConn.iovConn == iovConn) {
        tpConn = &newSess->iConn;
        tpConnOther = &newSess->aConn;
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

        tpConn->isActive = 0;

        if (tpConnOther->isActive == 0) {

            if (tpConnOther->readBuff) {
                AddToPool (newSess->appCtx->freeBuffPool
                                    , tpConnOther->readBuff);
                tpConnOther->readBuff = NULL;
            }

            if (tpConnOther->writeBuff) {
                AddToPool (newSess->appCtx->freeBuffPool
                                    , tpConnOther->writeBuff);
                tpConnOther->writeBuff = NULL;
            }

            while (1) {
                RwBuff_t* tmpBuff = GetFromPool (&tpConnOther->writeQ);
                if (tmpBuff == NULL) {
                    break;
                }
                AddToPool (newSess->appCtx->freeBuffPool, tmpBuff);
            }

            AddToPool (newSess->appCtx->freeSessionPool, newSess);
            RemoveFromPool (&newSess->appCtx->activeSessionPool, newSess);
            // printf ("Free Sessions = %d\n", GetPoolCount (newSess->appCtx->freeSessionPool) );
            // printf ("Free Buffs = %d\n", GetPoolCount (newSess->appCtx->freeBuffPool) );
        }

    } else {
        //update stats
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

    InitPool (&appCtx->activeSessionPool);

    return appCtx;
}

void TcpProxyRun (TcpProxyI_t* appI) {

    TcpProxyAppCtx_t* appCtx = CreateAppCtx (appI);

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
    
    IoVentCtx_t* iovCtx 
        = CreateIoVentCtx (iovMethods, iovOptions, appCtx);

    TcpProxyServer_t* server 
        = &appI->serverArr[0];

    InitServer(iovCtx
                , server
                , &server->serverAddrP
                , &appCtx->appI->gStats
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





