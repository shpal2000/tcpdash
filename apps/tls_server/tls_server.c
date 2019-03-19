#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#include "tls_server.h"

static SSL_CTX* GSslContext = NULL;

static void InitConn (TlsServerConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->bytesRead = 0;
    tcpConn->bytesWritten = 0;
    tcpConn->writeBuffOffset = 0;
}

static TlsServerSession_t* GetSession (TlsServerCtx_t* appCtx) {

    TlsServerSession_t* newSess =         
        GetFromPool (appCtx->freeSessionPool);
    
    if (newSess) {

        AddToPool (&appCtx->activeSessionPool, newSess);

        newSess->appCtx = appCtx;

        InitConn (&newSess->tcpConn);
    }

    return newSess;
}

static void FreeSession (TlsServerSession_t* newSess) {

    RemoveFromPool (&newSess->appCtx->activeSessionPool, newSess);

    AddToPool (newSess->appCtx->freeSessionPool, newSess);
}

static void StartTlsServer (struct IoVentConn* iovConn) {

    TlsServerCtx_t* appCtx 
        = (TlsServerCtx_t*) iovConn->cInfo.appCtx;

    iovConn->cInfo.cSSL = SSL_new(GSslContext);
    
    if (iovConn->cInfo.cSSL) {
        setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ appCtx->appI->connLifetimeSec * 1000 }, sizeof(int));

        SslServerInit (iovConn, (SSL*) iovConn->cInfo.cSSL);
    }else  {
        AbortConnection (iovConn); // stats; state update
    }
}

static void OnEstablish (struct IoVentConn* iovConn) {

    TlsServerCtx_t* appCtx 
        = (TlsServerCtx_t*) iovConn->cInfo.appCtx;

    TlsServerGroup_t* groupCtx 
        = (TlsServerGroup_t*) iovConn->cInfo.groupCtx;

    TlsServerSession_t* newSess = GetSession (appCtx);

    if (newSess == NULL) {

        IncConnStats2 ( &appCtx->appI->gStats
                        , &groupCtx->cStats
                        , appSessStructNotAvail );

        AbortConnection (iovConn);

    } else {
        iovConn->cInfo.sessionData = newSess;
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TlsServerCtx_t* appCtx 
        = (TlsServerCtx_t*) iovConn->cInfo.appCtx;

    TlsServerSession_t* newSess 
        = (TlsServerSession_t*) iovConn->cInfo.sessionData;

    TlsServerGroup_t* groupCtx
        = (TlsServerGroup_t*) iovConn->cInfo.groupCtx;

    if ( newSess->tcpConn.bytesRead < groupCtx->csStartTlsLen ) {
        ReadNextData (iovConn
                    , appCtx->commonReadBuff
                    , 0
                    , groupCtx->csStartTlsLen - newSess->tcpConn.bytesRead
                    , 1);
    } else {
        if ( iovConn->cInfo.cSSL == NULL ) {
            if ( newSess->tcpConn.bytesWritten == groupCtx->scStartTlsLen
                    && newSess->tcpConn.bytesRead == groupCtx->csStartTlsLen ) {
                StartTlsServer (iovConn);
            }
        } else {
            ReadNextData (iovConn
                        , appCtx->commonReadBuff
                        , 0
                        , COMMON_READBUFF_MAXLEN
                        , 1);
        }
    }
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    TlsServerSession_t* newSess 
        = (TlsServerSession_t*) iovConn->cInfo.sessionData;

    
    if (bytesRead > 0) {

        newSess->tcpConn.bytesRead += bytesRead;
    
    } else {

        int closeErr = bytesRead;

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
        }
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    TlsServerCtx_t* appCtx 
        = (TlsServerCtx_t*) iovConn->cInfo.appCtx;

    TlsServerSession_t* newSess 
        = (TlsServerSession_t*) iovConn->cInfo.sessionData;

    TlsServerGroup_t* groupCtx 
        = (TlsServerGroup_t*) iovConn->cInfo.groupCtx;
  
    if (newSess->tcpConn.bytesWritten < groupCtx->scStartTlsLen) {

        int nextChunkLen = 0;

        if ( (groupCtx->scStartTlsLen - newSess->tcpConn.bytesWritten) > 1200 ) {
            nextChunkLen = 1200;
        } else {
            nextChunkLen 
                = (int) (groupCtx->scStartTlsLen - newSess->tcpConn.bytesWritten);
        }

        WriteNextData (iovConn
                        , appCtx->commonWriteBuff
                        , 0
                        , nextChunkLen
                        , 1);
    } else {
        if ( iovConn->cInfo.cSSL == NULL ) {
            if ( newSess->tcpConn.bytesWritten == groupCtx->scStartTlsLen
                    && newSess->tcpConn.bytesRead == groupCtx->csStartTlsLen ) {
                StartTlsServer (iovConn);
            }
        } else {
            if ( newSess->tcpConn.bytesWritten < groupCtx->scDataLen ) {

                int nextChunkLen = 0;
                if ( (groupCtx->scDataLen - newSess->tcpConn.bytesWritten) 
                                                                    > 1200 ) {
                    nextChunkLen = 1200;
                } else {
                    nextChunkLen 
                        = (int) (groupCtx->scDataLen - newSess->tcpConn.bytesWritten);
                }

                WriteNextData (iovConn
                                , appCtx->commonWriteBuff
                                , 0
                                , nextChunkLen
                                , 1);
            }
        }
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    TlsServerSession_t* newSess 
        = (TlsServerSession_t*) iovConn->cInfo.sessionData;

    TlsServerGroup_t* groupCtx 
        = (TlsServerGroup_t*) iovConn->cInfo.groupCtx;


    if (bytesWritten > 0) {

        newSess->tcpConn.bytesWritten += bytesWritten;

        if (newSess->tcpConn.bytesWritten == groupCtx->scDataLen) {
            WriteClose (iovConn);
        }
    } else {
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    TlsServerSession_t* newSess 
        = (TlsServerSession_t*) iovConn->cInfo.sessionData;

    if (iovConn->cInfo.cSSL) {
        SSL_free( (SSL*) iovConn->cInfo.cSSL);
    }

    FreeSession (newSess);
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {

    return EmAppContinue;
}

static void MsgIoOnOpen (MsgIoChannelId_t mioChannelId) {

    TlsServerCtx_t* appCtx = (TlsServerCtx_t*) MsgIoGetCtx (mioChannelId);

    appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_GET_CONFIG;

    MsgIoSend (mioChannelId
                , appCtx->testId
                , strlen( appCtx->testId) );
}

static void MsgIoOnError (MsgIoChannelId_t mioChannelId) {

    TlsServerCtx_t* appCtx = (TlsServerCtx_t*) MsgIoGetCtx (mioChannelId);

    appCtx->nAdminChannelErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

static void MsgIoOnMsgRecv (MsgIoChannelId_t mioChannelId) {

    TlsServerCtx_t* appCtx = (TlsServerCtx_t*) MsgIoGetCtx (mioChannelId);

    char* msgData;
    int msgLen;
    MsgIoRecv (mioChannelId, &msgData, &msgLen);
    msgData[msgLen] = '\0';

    JNode* cfgNode;
    JObject* cfgObj;

    JGET_ROOT_NODE (msgData, &cfgNode, &cfgObj);

    if (cfgNode) {

        TlsServerI_t* appI = CreateStruct0 (TlsServerI_t);
        
        appCtx->appI = appI;

        JGET_MEMBER_INT (cfgObj, "maxActSessions", &appI->maxActSessions);
        JGET_MEMBER_INT (cfgObj, "maxErrSessions", &appI->maxErrSessions);

        JArray* csGroupArrJ;
        JGET_MEMBER_ARR (cfgObj, "csGroupArr", &csGroupArrJ);

        appI->csGroupCount = JGET_ARR_LEN (csGroupArrJ);
        appI->csGroupArr = CreateArray (TlsServerGroup_t, appI->csGroupCount);

        for (int gIndex = 0; gIndex < appI->csGroupCount; gIndex++) {
            
            TlsServerGroup_t* csGroup = &appI->csGroupArr[gIndex];

            JObject* csGroupJ = JGET_ARR_ELEMENT_OBJ (csGroupArrJ, gIndex);

            const char* serverAddrJ;
            JGET_MEMBER_STR (csGroupJ, "serverAddr", &serverAddrJ);

            struct sockaddr_in* remoteAddr = &(csGroup->serverAddr.inAddr);
            memset(remoteAddr, 0, sizeof(SockAddr_t));
            remoteAddr->sin_family = AF_INET;
            inet_pton(AF_INET
                        , serverAddrJ
                        , &(remoteAddr->sin_addr));
            remoteAddr->sin_port = htons(443);

            JGET_MEMBER_INT (csGroupJ, "csDataLen", &csGroup->csDataLen);
            JGET_MEMBER_INT (csGroupJ, "scDataLen", &csGroup->scDataLen);
            JGET_MEMBER_INT (csGroupJ, "csStartTlsLen", &csGroup->csStartTlsLen);
            JGET_MEMBER_INT (csGroupJ, "scStartTlsLen", &csGroup->scStartTlsLen);
        }

        JFREE_ROOT_NODE (cfgNode, cfgObj);

        appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;
    } else {
        appCtx->nAdminChannelErr = N_ADMIN_CHANNEL_ERROR_GET_CONFIG;
    }
}

static void MsgIoOnMsgSent (MsgIoChannelId_t mioChannelId) {

}

static TlsServerCtx_t* InitApp (char* nAdminTestId
                                , char* nAdminAddr
                                , int nAdminPort
                                , const char* testId) {
    
    int status = -1;

    signal(SIGPIPE, SIG_IGN);

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    GSslContext = SSL_CTX_new(SSLv23_server_method());

    SSL_CTX_set_verify(GSslContext
                            , SSL_VERIFY_NONE, 0);

    SSL_CTX_set_options(GSslContext
                            , SSL_OP_NO_SSLv2 
                            | SSL_OP_NO_SSLv3 
                            | SSL_OP_NO_COMPRESSION);
                            
    SSL_CTX_set_mode(GSslContext
                            , SSL_MODE_ENABLE_PARTIAL_WRITE);


    // SSL_CTX_use_certificate_file(GSslContext
    //         , "/root/autssl/certdepo/ca1/ica2/ica3/usrcerts3/server1.example.com.cert"
    //         , SSL_FILETYPE_PEM);

    // SSL_CTX_use_PrivateKey_file(GSslContext
    //         , "/root/autssl/certdepo/ca1/ica2/ica3/usrcerts3/server1.example.com.key"
    //         , SSL_FILETYPE_PEM);

    SSL_CTX_use_certificate_file(GSslContext
            , "/root/autssl/certdepo/ca1/ica2/ica3/ica4/usrcerts/server_chain_001.autssl.qa.gigamon.com.cert"
            , SSL_FILETYPE_PEM);

    SSL_CTX_use_PrivateKey_file(GSslContext
            , "/root/autssl/certdepo/ca1/ica2/ica3/ica4/usrcerts/server_chain_001.autssl.qa.gigamon.com.key"
            , SSL_FILETYPE_PEM);

    TlsServerCtx_t* appCtx = CreateStruct0 (TlsServerCtx_t);

    appCtx->testId = testId;

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
                    iovOptions.maxActiveConnections = appCtx->appI->maxActSessions;
                    iovOptions.maxErrorConnections = appCtx->appI->maxErrSessions;
                    iovOptions.maxEvents = 0;
                    iovOptions.eventPTO = DEFAULT_MAX_POLL_TIMEOUT;
                    
                    appCtx->iovCtx 
                        = CreateIoVentCtx (&iovMethods, &iovOptions, appCtx);

                    if (appCtx->iovCtx) {

                        CreatePool (&appCtx->freeSessionPool
                                    , appCtx->appI->maxActSessions
                                    , TlsServerSession_t);

                        InitPool (&appCtx->activeSessionPool);
                    }

                    break;
                }
            }
        }

        if (appCtx->nAdminChannelId
                && appCtx->iovCtx
                && appCtx->freeSessionPool
                && GSslContext) {
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

                DeleteStruct (TlsServerCtx_t, appCtx);

                appCtx = NULL;
            }
        }        
    }

    return appCtx;
}

int main (int argc, char** argv) {

    TlsServerCtx_t* appCtx 
        = InitApp ( argv[1], argv[2], atoi(argv[3]), argv[4] );

    if (appCtx == NULL) {
        exit (-1); //???
    }

    TlsServerI_t* appI = appCtx->appI;

    IoVentCtx_t* iovCtx = appCtx->iovCtx;

    for (int i = 0; i < appI->csGroupCount; i++) {

        TlsServerGroup_t* csGroup 
            = &appI->csGroupArr[i];

        SockAddr_t* localAddress 
             = &(csGroup->serverAddr);

        InitServer(iovCtx
                    , csGroup
                    , localAddress
                    , &appI->gStats
                    , &csGroup->cStats);
    }

    double lastMsgIoTime
        = MsgIoTimeElapsed (appCtx->nAdminChannelId);

    char statsString[256];

    while (1) {

        MsgIoProcess (appCtx->nAdminChannelId);

        if ( (MsgIoTimeElapsed (appCtx->nAdminChannelId) - lastMsgIoTime) >= 2 ) {

            lastMsgIoTime = MsgIoTimeElapsed (appCtx->nAdminChannelId);

            sprintf (statsString, 
                        "TS : Succ = %" PRIu64 "; " 
                        "Fail = %" PRIu64 "; "
                        "\n"
                        , GetConnStats(&appI->gStats, tcpAcceptSuccess)
                        , GetConnStats(&appI->gStats, tcpAcceptFail)
                        );

            MsgIoSend (appCtx->nAdminChannelId
                        , statsString
                        , strlen(statsString));

        }
    
        if ( ProcessIoVent (iovCtx) == 0 ) {
            break;
        }
    }

    DumpErrConnections (iovCtx);

    MsgIoDelete (appCtx->nAdminChannelId);

    DeleteIoVentCtx (iovCtx);

    return 0;
}


