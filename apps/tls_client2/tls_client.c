#include "app_engine.h"

#include "tls_client.h"

static TlsClientSess_t* OnCreateSess () {
    return CreateStruct0 (TlsClientSess_t);
}

static void OnInitSess (TlsClientSess_t* appSess) {
    appSess->cConn = NULL;
}

static void OnDeleteSess (TlsClientSess_t* appSess) {
    DeleteStruct (TlsClientSess_t, appSess);
}

static TlsClientConn_t* OnCreateConn () {
    return CreateStruct0 (TlsClientConn_t);
}

static void OnInitConn (TlsClientConn_t* appConn) {
    appConn->bytesRead = 0;
    appConn->bytesWritten = 0;
    appConn->writeBuffOffset = 0;
    appConn->isSslInit = 0;
    appConn->csGrp = NULL;
}

static void OnDeleteConn (TlsClientConn_t* appConn) {
    DeleteStruct (TlsClientConn_t, appConn);
}

static uint32_t GetMaxActSess (TlsClientCtx_t* appCtx) {
    return appCtx->maxActSess;
}

static uint32_t GetMaxActConn (TlsClientCtx_t* appCtx) {
    // 1 connection per session
    return appCtx->maxActSess * 1;
}

static uint32_t GetMaxErrSess (TlsClientCtx_t* appCtx) {
    return appCtx->maxErrSess;
}

static uint32_t GetMaxErrConn (TlsClientCtx_t* appCtx) {
    // 1 connection per session
    return appCtx->maxErrSess * 1;
}

static uint32_t GetConnPerSec (TlsClientCtx_t* appCtx) {
    return appCtx->connPerSec;
}

static int OnContinue (TlsClientCtx_t* appCtx) {

    if ( GetConnStats(&appCtx->allStats, tcpConnInitFail) 
                                            >= appCtx->maxErrSess ) {
        return 0;
    }

    if ( (GetConnStats(&appCtx->allStats, tcpConnInit) 
                        == appCtx->maxSess) && App_zero_act_sess(appCtx) ) {
        return 0;
    }

    return 1;
}

static void StartTls (TlsClientCtx_t* appCtx, TlsClientConn_t* appConn) {

    SSL* sslCtx = SSL_new(appConn->csGrp->sslCtx);

    if (sslCtx) {
        appConn->isSslInit = 1;
        App_ssl_client_init (appConn, sslCtx);            
    } else {
        //??? update stats; mark connection state why fail 
        App_conn_abort (appConn);
    }
}

static void OnAppLoop (TlsClientCtx_t* appCtx, int newConnCount) {

    for (uint32_t connIndex = 0; connIndex < newConnCount; connIndex++) {
        TlsClientGrp_t* csGrp = &appCtx->csGrpArr[0]; //todo
        SockAddr_t* localAddr = &csGrp->cAddrArr[0].sockAddr;
        SockAddr_t* remoteAddr = &csGrp->srvAddr;
        LocalPortPool_t* localPortPool = &csGrp->cAddrArr[0].portPool;
        int statsCount = 2;
        SockStats_t* statsArr[] = { (SockStats_t*) &appCtx->allStats
                                    , (SockStats_t*) &csGrp->grpStats};

        if (csGrp->sslCtx == NULL) {
            csGrp->sslCtx = SSL_CTX_new(SSLv23_client_method());

            if (csGrp->sslCtx) {
                SSL_CTX_set_verify(csGrp->sslCtx
                                        , SSL_VERIFY_NONE, 0);

                SSL_CTX_set_options(csGrp->sslCtx
                                        , SSL_OP_NO_SSLv2 
                                        | SSL_OP_NO_SSLv3 
                                        | SSL_OP_NO_COMPRESSION);
                                        
                SSL_CTX_set_mode(csGrp->sslCtx
                                        , SSL_MODE_ENABLE_PARTIAL_WRITE);

                SSL_CTX_set_session_cache_mode(csGrp->sslCtx
                                        , SSL_SESS_CACHE_OFF);                
            } else {
                //log
                continue;
            }
        }

        TlsClientConn_t* appConn 
            = (TlsClientConn_t*) App_conn_session_new (appCtx
                                        , localAddr
                                        , localPortPool 
                                        , remoteAddr
                                        , statsArr 
                                        , statsCount);
        if (appConn == NULL) {
            // handle error
            // log stats 
        } else {
            //log stats
            appConn->csGrp = csGrp; 
        }
    }
}

static void OnAppExit (TlsClientCtx_t* appCtx) {

}

static void OnMinTick (TlsClientCtx_t* appCtx) {
    puts ("OnMinTick\n");
}

static void OnEstablish (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) {

    // App_conn_abort (appCtx, appConn);

}

static void OnEstablishErr (TlsClientCtx_t* appCtx
                            , TlsClientConn_t* appConn) {
    
}

static void OnWriteNext (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) { 
}

static void OnWriteStatus (TlsClientCtx_t* appCtx
                            , TlsClientConn_t* appConn
                            , int bytesSent) { 
}

static void OnReadNext (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) {
    if ( appConn->bytesRead < appConn->csGrp->scStartTlsLen ) {
        App_conn_read_next (appConn
                        , appCtx->appRdBuff
                        , 0
                        , appConn->csGrp->scStartTlsLen - appConn->bytesRead
                        , 1);
    } else {
        if (appConn->isSslInit == 0) {
            if ( appConn->bytesWritten == appConn->csGrp->csStartTlsLen
                    && appConn->bytesRead == appConn->csGrp->scStartTlsLen ) {
                StartTls (appCtx, appConn);
            }
        } else {
            App_conn_read_next (appConn
                        , appCtx->appRdBuff
                        , 0
                        , COMMON_READBUFF_MAXLEN
                        , 1);
        }
    }
}

static void OnReadStatus (TlsClientCtx_t* appCtx
                            , TlsClientConn_t* appConn
                            , int bytesRcvd) { 
}

static void OnStatus (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) { 
}

static void OnCleanup (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) {
    App_conn_release (appConn, 1);
}

static TlsClientCtx_t* OnAppInit (JObject* appJ) {

    TlsClientCtx_t* appCtx = CreateStruct0 (TlsClientCtx_t);

    if (appCtx) {
        JGET_MEMBER_INT (appJ, "connPerSec", &appCtx->connPerSec);
        JGET_MEMBER_INT (appJ, "maxActSess", &appCtx->maxActSess);
        JGET_MEMBER_INT (appJ, "maxErrSess", &appCtx->maxErrSess);
        JGET_MEMBER_INT (appJ, "maxSess", &appCtx->maxSess);

        JArray* csGrpArrJ;
        JGET_MEMBER_ARR (appJ, "csGrpArr", &csGrpArrJ);
        appCtx->csGrpCount = JGET_ARR_LEN (csGrpArrJ);
        appCtx->csGrpArr = CreateArray0 (TlsClientGrp_t, appCtx->csGrpCount);
        
        if (appCtx->csGrpArr) {
            for (int csGrpIndex = 0; csGrpIndex < appCtx->csGrpCount; csGrpIndex++) {
                TlsClientGrp_t* csGrp = &appCtx->csGrpArr[csGrpIndex];
                
                JObject* csGrpJ = JGET_ARR_ELEMENT_OBJ (csGrpArrJ, csGrpIndex);

                //server address
                JGET_MEMBER_STR (csGrpJ, "srvIp", &csGrp->srvIp);
                JGET_MEMBER_INT (csGrpJ, "srvPort", &csGrp->srvPort);
                SetSockAddress (&csGrp->srvAddr, csGrp->srvIp, csGrp->srvPort);

                //client address array
                JArray *cAddrArrJ;
                JGET_MEMBER_ARR (csGrpJ, "cAddrArr", &cAddrArrJ);
                csGrp->cAddrCount = JGET_ARR_LEN (cAddrArrJ);
                csGrp->cAddrArr = CreateArray0 (SockAddrCtx_t, csGrp->cAddrCount);

                if (csGrp->cAddrArr) { 
                    for (int cAddrIndex = 0; cAddrIndex < csGrp->cAddrCount; cAddrIndex++) {
                        SockAddrCtx_t* cAddrCtx = &csGrp->cAddrArr[cAddrIndex]; 

                        JObject* cAddrJ = JGET_ARR_ELEMENT_OBJ (cAddrArrJ, cAddrIndex);

                        JGET_MEMBER_STR (cAddrJ, "cIp", &cAddrCtx->cIp);
                        JGET_MEMBER_INT (cAddrJ, "cPortB", &cAddrCtx->cPortB);
                        JGET_MEMBER_INT (cAddrJ, "cPortE", &cAddrCtx->cPortE);
                        SetSockAddress (&cAddrCtx->sockAddr, cAddrCtx->cIp, 0);
                        InitPool( &cAddrCtx->portPool );
                        for (int srcPort = cAddrCtx->cPortB; 
                                                        srcPort <= cAddrCtx->cPortE; 
                                                        srcPort++) {
                            SetPortToPool(&cAddrCtx->portPool, htons(srcPort));
                        }
                    }
                } else {
                    // log
                    return NULL;
                }
            }
        } else {
            // log
            return NULL;
        }
    } else {
        // log
        return NULL;
    }

    return appCtx;
}

APP_REGISTER_METHODS (TlsClient);
