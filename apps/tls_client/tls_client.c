#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#include "tls_client.h"

static SSL_CTX* GSslContext = NULL;

static void InitConn (TlsClientConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->bytesRead = 0;
    tcpConn->bytesWritten = 0;
    tcpConn->writeBuffOffset = 0;
}

static TlsClientSession_t* GetSession (TlsClientCtx_t* appCtx) {

    TlsClientSession_t* newSess =         
        GetFromPool (appCtx->freeSessionPool);
    
    if (newSess) {

        AddToPool (&appCtx->activeSessionPool, newSess);

        newSess->appCtx = appCtx;

        InitConn (&newSess->tcpConn);
    }

    return newSess;
}

static void FreeSession (TlsClientSession_t* newSess) {

    RemoveFromPool (&newSess->appCtx->activeSessionPool, newSess);

    AddToPool (newSess->appCtx->freeSessionPool, newSess);
}

static void OnEstablish (struct IoVentConn* iovConn) { 

    TlsClientCtx_t* appCtx 
        = (TlsClientCtx_t*) iovConn->cInfo.appCtx;

    TlsClientSession_t* newSess 
        = (TlsClientSession_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {
        FreeSession (newSess);
    } else {

        iovConn->cInfo.cSSL = SSL_new(GSslContext);

        if (iovConn->cInfo.cSSL) {
            setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
            setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ appCtx->appI->connLifetimeSec * 1000 }, sizeof(int));

            SslClientInit (iovConn, (SSL*) iovConn->cInfo.cSSL);            
        } else {
            //??? update stats; mark connection state why fail 
            AbortConnection (iovConn);
        }   
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TlsClientCtx_t* appCtx 
        = (TlsClientCtx_t*) iovConn->cInfo.appCtx;

    ReadNextData (iovConn
                , appCtx->commonReadBuff
                , 0
                , COMMON_READBUFF_MAXLEN
                , 1);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    TlsClientSession_t* newSess 
        = (TlsClientSession_t*) iovConn->cInfo.sessionData;

    
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

    TlsClientCtx_t* appCtx 
        = (TlsClientCtx_t*) iovConn->cInfo.appCtx;

    TlsClientSession_t* newSess 
        = (TlsClientSession_t*) iovConn->cInfo.sessionData;

    TlsClientGroup_t* groupCtx 
        = (TlsClientGroup_t*) iovConn->cInfo.groupCtx;
  

    if (newSess->tcpConn.bytesWritten < groupCtx->csDataLen) {

        int nextChunkLen = 0;

        if ( (groupCtx->csDataLen - newSess->tcpConn.bytesWritten) 
                                > 1200 ) {

            nextChunkLen = 1200;

        } else {

            nextChunkLen 
                = (int) (groupCtx->csDataLen - newSess->tcpConn.bytesWritten);
        }

        WriteNextData (iovConn
                        , appCtx->commonWriteBuff
                        , 0
                        , nextChunkLen
                        , 0);
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    TlsClientSession_t* newSess 
        = (TlsClientSession_t*) iovConn->cInfo.sessionData;

    TlsClientGroup_t* groupCtx 
        = (TlsClientGroup_t*) iovConn->cInfo.groupCtx;


    if (bytesWritten > 0) {

        newSess->tcpConn.bytesWritten += bytesWritten;

        if (newSess->tcpConn.bytesWritten == groupCtx->csDataLen) {
            WriteClose (iovConn); // ??? close notify
        }
    } else {
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    TlsClientSession_t* newSess 
        = (TlsClientSession_t*) iovConn->cInfo.sessionData;

    if (iovConn->cInfo.cSSL) {
        SSL_free( (SSL*) iovConn->cInfo.cSSL);
    }

    FreeSession (newSess);
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {
    
    TlsClientCtx_t* appCtx = (TlsClientCtx_t*) appData;

    TlsClientI_t* appI = appCtx->appI;

    //exceeded error connections
    uint64_t tcpConnInitFailCount 
        = GetConnStats(&appI->gStats, tcpConnInitFail);

    if (tcpConnInitFailCount >= appI->maxErrSessions) {
        return EmAppExit; 
    }

    //achived desired total connections
    uint64_t tcpConnInitCount 
        = GetConnStats(&appI->gStats, tcpConnInit);

    if (tcpConnInitCount == appI->maxSessions) {
        return EmAppContinueZeroActive; 
    }

    //continue to run
    return EmAppContinue;
}

static void MsgIoOnOpen (MsgIoChannelId_t mioChannelId) {

    TlsClientCtx_t* appCtx = (TlsClientCtx_t*) MsgIoGetCtx (mioChannelId);

    appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_GET_CONFIG;

    MsgIoSend ( mioChannelId
                , appCtx->testId
                , strlen(appCtx->testId) );
}

static void MsgIoOnError (MsgIoChannelId_t mioChannelId) {

    TlsClientCtx_t* appCtx = (TlsClientCtx_t*) MsgIoGetCtx (mioChannelId);

    appCtx->nAdminChannelErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

static void MsgIoOnMsgRecv (MsgIoChannelId_t mioChannelId) {

    TlsClientCtx_t* appCtx = (TlsClientCtx_t*) MsgIoGetCtx (mioChannelId);

    char* msgData;
    int msgLen;
    MsgIoRecv (mioChannelId, &msgData, &msgLen);
    msgData[msgLen] = '\0';

    JNode* cfgNode;
    JObject* cfgObj;

    JGET_ROOT_NODE (msgData, &cfgNode, &cfgObj);

    if (cfgNode) {

        TlsClientI_t* appI = CreateStruct0 (TlsClientI_t);
        
        appCtx->appI = appI;

        JGET_MEMBER_INT (cfgObj, "connPerSec", &appI->connPerSec);
        JGET_MEMBER_INT (cfgObj, "maxActSessions", &appI->maxActSessions);
        JGET_MEMBER_INT (cfgObj, "maxErrSessions", &appI->maxErrSessions);
        JGET_MEMBER_INT (cfgObj, "maxSessions", &appI->maxSessions);

        JArray* csGroupArrJ;
        JGET_MEMBER_ARR (cfgObj, "csGroupArr", &csGroupArrJ);

        appI->csGroupCount = JGET_ARR_LEN (csGroupArrJ);
        appI->csGroupArr = CreateArray (TlsClientGroup_t, appI->csGroupCount);
        appI->nextCsGroupIndex = 0;

        for (int gIndex = 0; gIndex < appI->csGroupCount; gIndex++) {
            
            TlsClientGroup_t* csGroup = &appI->csGroupArr[gIndex];

            JObject* csGroupJ = JGET_ARR_ELEMENT_OBJ (csGroupArrJ, gIndex);

            JArray* clientAddrArrJ;
            JGET_MEMBER_ARR (csGroupJ, "clientAddrArr", &clientAddrArrJ);

            csGroup->clientAddrCount = JGET_ARR_LEN (clientAddrArrJ);

            csGroup->nextClientAddrIndex = 0;

            csGroup->clientAddrArr 
                = CreateArray (SockAddr_t, csGroup->clientAddrCount);

            csGroup->LocalPortPoolArr 
                = CreateArray (LocalPortPool_t, csGroup->clientAddrCount);

            for (int cIndex = 0
                    ; cIndex < csGroup->clientAddrCount
                    ; cIndex++) {
    
                struct sockaddr_in* localAddr 
                    = &(csGroup->clientAddrArr[cIndex].inAddr);

                memset(localAddr, 0, sizeof(SockAddr_t));
                localAddr->sin_family = AF_INET;
                inet_pton(AF_INET
                            , JGET_ARR_ELEMENT_STR (clientAddrArrJ, cIndex)
                            , &(localAddr->sin_addr));

                LocalPortPool_t* portQ = &csGroup->LocalPortPoolArr[cIndex];
                InitPortBindQ(portQ);
                for (int srcPort = 5000; srcPort <= 65000; srcPort++) {
                    SetPortToPool(portQ, htons(srcPort));
                }
            }

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
            JGET_MEMBER_INT (csGroupJ, "cCloseMethod", &csGroup->cCloseMethod);
            JGET_MEMBER_INT (csGroupJ, "csCloseType", &csGroup->csCloseType);
            JGET_MEMBER_INT (csGroupJ, "csWeight", &csGroup->csWeight);
        }

        JFREE_ROOT_NODE (cfgNode, cfgObj);

        appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;
    } else {
        appCtx->nAdminChannelErr = N_ADMIN_CHANNEL_ERROR_GET_CONFIG;
    }
}

static void MsgIoOnMsgSent (MsgIoChannelId_t mioChannelId) {

}

static TlsClientCtx_t* InitApp (char* nAdminTestId
                                , char* nAdminAddr
                                , int nAdminPort
                                , const char* testId) {

    int status = -1;

    signal(SIGPIPE, SIG_IGN);

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    GSslContext = SSL_CTX_new(SSLv23_client_method());

    SSL_CTX_set_verify(GSslContext
                            , SSL_VERIFY_NONE, 0);

    SSL_CTX_set_options(GSslContext
                            , SSL_OP_NO_SSLv2 
                            | SSL_OP_NO_SSLv3 
                            | SSL_OP_NO_COMPRESSION);
                            
    SSL_CTX_set_mode(GSslContext
                            , SSL_MODE_ENABLE_PARTIAL_WRITE);

    TlsClientCtx_t* appCtx = CreateStruct0 (TlsClientCtx_t);

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
                                    , TlsClientSession_t);

                        InitPool (&appCtx->activeSessionPool);
                    }

                    break;
                }
            }
        }

        if (appCtx->nAdminChannelId
                && appCtx->iovCtx
                && appCtx->freeSessionPool
                && GSslContext ) {
            status = 0;
        }
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

            DeleteStruct (TlsClientCtx_t, appCtx);

            appCtx = NULL;
        }
    }

    return appCtx;
}

int main(int argc, char** argv) {

    TlsClientCtx_t* appCtx 
        = InitApp ( argv[1], argv[2], atoi(argv[3]), argv[4] );
    
    if (appCtx == NULL) {
        exit (-1); //???
    }

    TlsClientI_t* appI = appCtx->appI;

    IoVentCtx_t* iovCtx = appCtx->iovCtx;

    double lastConnInitTime 
        = TimeElapsedIoVentCtx (iovCtx);

    double lastMsgIoTime
        = MsgIoTimeElapsed (appCtx->nAdminChannelId);

    char statsString[256];

    while (1) {

        MsgIoProcess (appCtx->nAdminChannelId);

        if ( (MsgIoTimeElapsed (appCtx->nAdminChannelId) - lastMsgIoTime) >= 2 ) {

            lastMsgIoTime = MsgIoTimeElapsed (appCtx->nAdminChannelId);

            sprintf (statsString, 
                        "TC : Init = %" PRIu64 "; " 
                        "Succ = %" PRIu64 "; "
                        "Fail = %" PRIu64
                        "\n"
                        , GetConnStats(&appI->gStats, tcpConnInit)
                        , GetConnStats(&appI->gStats, tcpConnInitSuccess)
                        , GetConnStats(&appI->gStats, tcpConnInitFail)
                        );

            MsgIoSend (appCtx->nAdminChannelId
                        , statsString
                        , strlen(statsString) );

        }
        
        if ( ProcessIoVent (iovCtx) == 0 ) {
            break;
        }

        uint64_t tcpConnInitCount 
            = GetConnStats(&appI->gStats, tcpConnInit);

        int newConnectionInits 
            = (TimeElapsedIoVentCtx (iovCtx) - lastConnInitTime) 
                * appI->connPerSec;

        while (newConnectionInits > 0
                    && tcpConnInitCount < appI->maxSessions) {

            //new connection init
            TlsClientGroup_t* csGroup
                = &appI->csGroupArr[appI->nextCsGroupIndex];

            SockAddr_t* localAddress 
                = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

            SockAddr_t* remoteAddress 
                = &(csGroup->serverAddr);

            LocalPortPool_t* localPortPool 
                = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

            TlsClientSession_t* newSess = GetSession (iovCtx->appCtx);

            if (newSess == NULL) {

                IncConnStats2 ( &appI->gStats
                                , &csGroup->cStats
                                , appSessStructNotAvail );
                break;
            }

            appI->nextCsGroupIndex += 1;
            if (appI->nextCsGroupIndex == appI->csGroupCount){
                appI->nextCsGroupIndex = 0;
            }
            
            csGroup->nextClientAddrIndex += 1;
            if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
                csGroup->nextClientAddrIndex = 0;
            }

            int newConnInitErr = 
                NewConnection (iovCtx
                                , csGroup
                                , newSess
                                , localAddress
                                , localPortPool
                                , remoteAddress
                                , 10 //??? hardcoded
                                , &csGroup->cStats
                                , &appI->gStats);

            if (newConnInitErr) {
                FreeSession (newSess);
            }  

            newConnectionInits -= 1;

            lastConnInitTime = TimeElapsedIoVentCtx (iovCtx);

            tcpConnInitCount 
                = GetConnStats(&appI->gStats, tcpConnInit);
        }
    }

    DumpErrConnections (iovCtx);

    MsgIoDelete (appCtx->nAdminChannelId);

    DeleteIoVentCtx (iovCtx);

    return 0;
}

