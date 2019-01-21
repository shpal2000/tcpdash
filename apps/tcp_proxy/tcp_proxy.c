#include <sys/mman.h>
#include "iovents.h"

#include "tcp_proxy.h"

static void InitRwBuff (RwBuff_t* newBuff) {

    newBuff->buffLen = RW_MAX_BUFF_LEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void InitConn (TcpProxyAppConn_t * tpConn) {

    tpConn->iovConn = NULL;
    tpConn->readBuff = NULL;
    tpConn->writeBuff = NULL;
    tpConn->isActive = 1;

    InitPool (&tpConn->writeQ);
}

static void InitSession (TcpProxyAppSession_t* newSess
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
        TcpProxyAppSession_t* newSess 
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

            TcpProxyAppGroup_t* server 
                = (TcpProxyAppGroup_t*) iovConn->cInfo.groupCtx;

            if (server->keepSourcePort == 0) {
                SET_SOCK_PORT (&iovConn->remoteAddressAccept, 0);
            }

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
        TcpProxyAppSession_t* newSess 
            = (TcpProxyAppSession_t*) iovConn->cInfo.sessionData;

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

    TcpProxyAppSession_t* newSess 
        = (TcpProxyAppSession_t*) iovConn->cInfo.sessionData;


    TcpProxyAppConn_t* tpConn = NULL;
    TcpProxyAppConn_t* tpConnOther = NULL;

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
                        , readBuff->buffLen
                        , 1);
        }
    }
}

static void OnReadStatus (struct IoVentConn* iovConn
                            , int bytesReceived) {

    // puts ("OnReadStatus");

    TcpProxyAppSession_t* newSess 
        = (TcpProxyAppSession_t*) iovConn->cInfo.sessionData;

    TcpProxyAppConn_t* tpConn = NULL;
    TcpProxyAppConn_t* tpConnOther = NULL;
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

    TcpProxyAppSession_t* newSess 
        = (TcpProxyAppSession_t*) iovConn->cInfo.sessionData;
    
    TcpProxyAppConn_t* tpConn = NULL;

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

    TcpProxyAppSession_t* newSess 
        = (TcpProxyAppSession_t*) iovConn->cInfo.sessionData;

    TcpProxyAppConn_t* tpConn = NULL;
    TcpProxyAppConn_t* tpConnOther = NULL;

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

    TcpProxyAppSession_t* newSess 
        = (TcpProxyAppSession_t*) iovConn->cInfo.sessionData;

    TcpProxyAppConn_t* tpConn = NULL;
    TcpProxyAppConn_t* tpConnOther = NULL;

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

static TcpProxyAppCtx_t* CreateAppCtx (TcpProxyAppI_t* appI) {

    TcpProxyAppCtx_t* appCtx = CreateStruct0 (TcpProxyAppCtx_t);

    appCtx->appI = appI;
                
    CreatePool (&appCtx->freeSessionPool
                , appI->maxActiveSessions
                , TcpProxyAppSession_t);

    CreatePool (&appCtx->freeBuffPool
                , appI->maxActiveSessions * 16
                , RwBuff_t);

    InitPool (&appCtx->activeSessionPool);

    return appCtx;
}

static void DumpTcpProxyStats (TcpProxyAppCtx_t* appCtx, TcpProxyAppI_t* appI) {

    TcpProxyAppStats_t* appConnStats = &appI->gStats; 

    char statsString[120];

    sprintf (statsString, 
                        "%" PRIu64 " : " 
                        "%" PRIu64 " : "
                        "%" PRIu64 " : "
                        "%d" " : "
                        "%d"
                        "\n"
        , GetConnStats(appConnStats, tcpAcceptSuccess)
        , GetConnStats(appConnStats, tcpConnInit)
        , GetConnStats(appConnStats, tcpConnInitSuccess)
        , GetPoolCount(appCtx->freeSessionPool)
        , GetPoolCount(appCtx->freeBuffPool)
        );

    puts (statsString);
}

void TcpProxyRun (AppI_t* appBase) {

    TcpProxyAppI_t* appI = (TcpProxyAppI_t*) appBase;

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

    TcpProxyAppGroup_t* server 
        = &appI->csGroupArr[0];

    InitServer(iovCtx
                , server
                , &server->serverAddrP
                , &appCtx->appI->gStats
                , &server->cStats);

    DumpTcpProxyStats (appCtx, appI);

    while (1) {

        ProcessIoVent (iovCtx);

        DumpTcpProxyStats (appCtx, appI);
    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);
}

int main (int argc, char** argv) {

    int csGroupCount = 1;

    TcpProxyAppI_t* appI 
        = (TcpProxyAppI_t*) mmap(NULL
            , sizeof (TcpProxyAppI_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    appI->csGroupCount = csGroupCount;
    appI->csGroupArr 
        = (TcpProxyAppGroup_t*) mmap(NULL
            , sizeof (TcpProxyAppGroup_t) * appI->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    appI->maxActiveSessions = 100000;
    appI->maxErrorSessions = 100000;

    TcpProxyAppGroup_t* server 
        = &appI->csGroupArr[0];

    server->keepSourcePort = 1;

    // proxy address
    struct sockaddr_in* serverAddrP 
        = &(server->serverAddrP.inAddr);
    memset(serverAddrP, 0, sizeof(SockAddr_t));
    serverAddrP->sin_family = AF_INET;
    inet_pton(AF_INET
                , "0.0.0.0"
                , &(serverAddrP->sin_addr));
    serverAddrP->sin_port = htons(883);

    TcpProxyRun ( (AppI_t*) appI);
}

