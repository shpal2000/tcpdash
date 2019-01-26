#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

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
                            , TcpProxyCtx_t* appCtx) {

    newSess->appCtx = appCtx;
    InitConn (&newSess->aConn);
    InitConn (&newSess->iConn);
}

static void OnEstablish (struct IoVentConn* iovConn) {
    
    TcpProxyCtx_t* appCtx 
        = (TcpProxyCtx_t*) iovConn->cInfo.appCtx;

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

            TcpProxyGroup_t* server 
                = (TcpProxyGroup_t*) iovConn->cInfo.groupCtx;

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

    TcpProxyCtx_t* appCtx 
        = (TcpProxyCtx_t*) iovConn->cInfo.appCtx;

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
                        , readBuff->buffLen
                        , 1);
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

static int OnContinue (void* appData) {

    return EmAppContinue;
}

static void MsgIoOnOpen (MsgIoChannelId_t mioChannelId) {

    TcpProxyCtx_t* appCtx = (TcpProxyCtx_t*) MsgIoGetCtx (mioChannelId);

    int csGroupCount = 1;

    TcpProxyI_t* appI = CreateStruct0 (TcpProxyI_t); 

    appI->csGroupCount = csGroupCount;

    appI->csGroupArr = CreateArray (TcpProxyGroup_t, appI->csGroupCount);

    appI->maxActSessions = 100000;

    appI->maxErrSessions = 100000;

    TcpProxyGroup_t* server 
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

    appCtx->appI = appI;

    appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;  
}

static void MsgIoOnError (MsgIoChannelId_t mioChannelId) {

    TcpProxyCtx_t* appCtx = (TcpProxyCtx_t*) MsgIoGetCtx (mioChannelId);

    appCtx->nAdminChannelErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

static void MsgIoOnMsgRecv (MsgIoChannelId_t mioChannelId) {

}

static void MsgIoOnMsgSent (MsgIoChannelId_t mioChannelId) {

}

static TcpProxyCtx_t* InitApp (char* nAdminTestId
                                , char* nAdminAddr
                                , int nAdminPort) {

    int status = -1;

    TcpProxyCtx_t* appCtx = CreateStruct0 (TcpProxyCtx_t);

    if (appCtx) {
        
        appCtx->nAdminChannelErr = 0;

        strcpy (appCtx->nAdminTestId, nAdminTestId);

        SetSockAddress (&appCtx->nAdminAddr, nAdminAddr, nAdminPort);

        SetSockAddress0 (&appCtx->nLocalAddr, 0); 

        MsgIoMethods_t mioMethods;
        mioMethods.OnOpen = &MsgIoOnOpen;
        mioMethods.OnError = &MsgIoOnError;
        mioMethods.OnMsgRecv = &MsgIoOnMsgRecv;
        mioMethods.OnMsgSent = &MsgIoOnMsgSent;

        appCtx->nAdminChannelId =  MsgIoNew (&appCtx->nLocalAddr
                                                , &appCtx->nAdminAddr
                                                , &mioMethods
                                                , appCtx);
        
        if (appCtx->nAdminChannelId) {

            appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_INIT;

            while ( 1 ) {

                MsgIoProcess (appCtx->nAdminChannelId);

                if (MsgIoTimeElapsed (appCtx->nAdminChannelId) 
                                    > N_ADMIN_GET_CONFIG_MAX_TIME) {
                    
                    appCtx->nAdminChannelErr 
                        = N_ADMIN_CHANNEL_ERROR_GET_CONFIG_TIMEOUT;
                }

                if (appCtx->nAdminChannelErr) {
                    break;
                }

                if (appCtx->nAdminChannelState == N_ADMIN_CHANNEL_STATE_RECV_CONFIG) {

                    IoVentMethods_t iovMethods;
                    iovMethods.OnEstablish = &OnEstablish;
                    iovMethods.OnWriteNext = &OnWriteNext;
                    iovMethods.OnWriteStatus = &OnWriteStatus;
                    iovMethods.OnReadNext = &OnReadNext;
                    iovMethods.OnReadStatus = &OnReadStatus;
                    iovMethods.OnCleanup = &OnCleanup;
                    iovMethods.OnStatus = &OnStatus;
                    iovMethods.OnContinue = &OnContinue;

                    IoVentOptions_t iovOptions;
                    iovOptions.maxActiveConnections = appCtx->appI->maxActSessions * 2;
                    iovOptions.maxErrorConnections = appCtx->appI->maxErrSessions * 2;
                    iovOptions.maxEvents = 0;
                    iovOptions.eventPTO = DEFAULT_MAX_POLL_TIMEOUT;
                     
                    appCtx->iovCtx 
                        = CreateIoVentCtx (&iovMethods, &iovOptions, appCtx);

                    if (appCtx->iovCtx) {

                        CreatePool (&appCtx->freeSessionPool
                                    , appCtx->appI->maxActSessions
                                    , TcpProxySession_t);

                        CreatePool (&appCtx->freeBuffPool
                                    , appCtx->appI->maxActSessions * 16
                                    , RwBuff_t);

                        InitPool (&appCtx->activeSessionPool);
                    }

                    break;
                }
            }
        }

        if (appCtx->nAdminChannelId
                && appCtx->iovCtx
                && appCtx->freeSessionPool
                && appCtx->freeBuffPool) {
            status = 0;
        }

        if (status) {

            if (appCtx) {

                if (appCtx->nAdminChannelId) {
                    MsgIoDelete (appCtx->nAdminChannelId);
                }

                if (appCtx->iovCtx) {
                    DeleteIoVentCtx (appCtx->iovCtx);
                }

                if (appCtx->freeSessionPool) {
                    //??? clean up pool
                }

                if (appCtx->freeBuffPool) {
                    //??? clean up pool
                }

                DeleteStruct (TcpProxyCtx_t, appCtx);

                appCtx = NULL;
            }
        }        
    }
                
    return appCtx;
}

int main (int argc, char** argv) {

    TcpProxyCtx_t* appCtx = InitApp ( argv[1], argv[2], atoi(argv[3]) );

    if (appCtx == NULL) {
        exit (-1); //???
    }

    TcpProxyI_t* appI = appCtx->appI;

    IoVentCtx_t* iovCtx = appCtx->iovCtx;
    
    TcpProxyGroup_t* server 
        = &appI->csGroupArr[0];

    InitServer(iovCtx
                , server
                , &server->serverAddrP
                , &appCtx->appI->gStats
                , &server->cStats);

    double lastMsgIoTime
        = MsgIoTimeElapsed (appCtx->nAdminChannelId);

    char statsString[256];

    while (1) {

        MsgIoProcess (appCtx->nAdminChannelId);

        if ( (MsgIoTimeElapsed (appCtx->nAdminChannelId) - lastMsgIoTime) >= 2 ) {

            lastMsgIoTime = MsgIoTimeElapsed (appCtx->nAdminChannelId);

            sprintf (statsString, 
                        "TP : Accept = %" PRIu64 "; " 
                        "Establish = %" PRIu64
                        "\n"
                        , GetConnStats(&appI->gStats, tcpAcceptSuccess)
                        , GetConnStats(&appI->gStats, tcpConnInitSuccess)
                        );

            MsgIoSend (appCtx->nAdminChannelId
                        , statsString
                        , strlen(statsString));
        }

        ProcessIoVent (iovCtx);
    }

    DumpErrConnections (iovCtx);

    MsgIoDelete (appCtx->nAdminChannelId);

    DeleteIoVentCtx (iovCtx);

    return 0;
}

