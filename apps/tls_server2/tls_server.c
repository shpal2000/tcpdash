#include "app_engine.h"

#include "tls_server.h"

static void OnInitSessState (TlsServerSess_t* appSess) {
    appSess->cConn = NULL;
}

static void OnInitConnState (TlsServerConn_t* appConn) {
    appConn->bytesRead = 0;
    appConn->bytesWritten = 0;
    appConn->writeBuffOffset = 0;
    appConn->isSslInit = 0;
    appConn->csGrp = NULL;
}

static int OnContinue (TlsServerCtx_t* appCtx) {
    return APP_CONTINUE;
}

static void InitSSL (TlsServerCtx_t* appCtx, TlsServerConn_t* appConn) {

    SSL* ssl = SSL_new(appConn->csGrp->sslCtx);

    if (ssl) {
        appConn->isSslInit = 1;
        App_conn_set_ssl_as_server (appConn, ssl);           
    } else {
        //??? update stats; mark connection state why fail 
        App_conn_abort (appConn);
    }
}

static void CleanupSSL (TlsServerCtx_t* appCtx, TlsServerConn_t* appConn) {
    if (appConn->isSslInit) {
        SSL* ssl = App_conn_get_ssl (appConn);
        SSL_free (ssl);
    }
}

static void OnAppLoop (TlsServerCtx_t* appCtx, int newConnCount) {
            int __ret = 0;

    TlsServerGrp_t* csGrp = &appCtx->csGrpArr[0]; //todo
    if (csGrp->srvStarted == 0) {
        csGrp->srvStarted = 1;

        csGrp->sslCtx = SSL_CTX_new(SSLv23_server_method());

        if (csGrp->sslCtx) {
            SSL_CTX_set_verify(csGrp->sslCtx
                                    , SSL_VERIFY_NONE, 0);

            SSL_CTX_set_options(csGrp->sslCtx
                                    , SSL_OP_NO_SSLv2 
                                    | SSL_OP_NO_SSLv3 
                                    | SSL_OP_NO_COMPRESSION);
                                    
            SSL_CTX_set_mode(csGrp->sslCtx
                                    , SSL_MODE_ENABLE_PARTIAL_WRITE);

            // SSL_CTX_set_session_cache_mode(csGrp->sslCtx
            //                         , SSL_SESS_CACHE_OFF);                

            __ret += SSL_CTX_use_certificate_file(csGrp->sslCtx
                    , "/usr/local/tcpdash_bin/ss_rsa2048_a_tcpdash_com.crt"
                    , SSL_FILETYPE_PEM);

            __ret += SSL_CTX_use_PrivateKey_file(csGrp->sslCtx
                    , "/usr/local/tcpdash_bin/ss_rsa2048_a_tcpdash_com.key"
                    , SSL_FILETYPE_PEM);
        } else {
            //log
        }

        App_server_init(appCtx
                , csGrp
                , &csGrp->srvAddr
                , csGrp->statsArr
                , csGrp->statsCount);
    }

}

static void SetStatsHelper (TlsServerStats_t* stats, JObject* jObj) {

    // JSET_MEMBER_INT (jObj, "TlsServerStats1", 1);
    // JSET_MEMBER_INT (jObj, "TlsServerStats2", 2);
    // JSET_MEMBER_INT (jObj, "TlsServerStats3", 3);
    // JSET_MEMBER_INT (jObj, "TlsServerStats4", 4);

    // SetCommonAppStats (stats, jObj);
}

static int OnAppStats (TlsServerCtx_t* appCtx, JObject* jAppObj) {

    int status = 0;

    JObject* jObjSummary;
    JSET_MEMBER_OBJ (jAppObj, "Summary", &jObjSummary);
    if (jObjSummary) {
        SetStatsHelper (&appCtx->allStats, jObjSummary);
        JArray* jArrGrps;
        JSET_MEMBER_ARR (jAppObj, "CsGroups", &jArrGrps);
        if (jArrGrps) {
            for (int grpI=0; grpI < appCtx->csGrpCount; grpI++) {
                JObject* jObjGrp;
                JADD_ARR_ELEMENT_OBJ (jArrGrps, &jObjGrp);
                if (jObjGrp) {
                    SetStatsHelper (&appCtx->csGrpArr[grpI].grpStats, jObjGrp);
                } else {
                    status = -1;
                }
            }
        } else {
            status = -1;
        }
    } else {
        status = -1;
    }

    return status;
}

static void OnAppExit (TlsServerCtx_t* appCtx) {

}

static void OnSecTick (TlsServerCtx_t* appCtx) {

    SetCommonAppStatsRate (&appCtx->allStats);

    for (int grpI=0; grpI < appCtx->csGrpCount; grpI++) {
        SetCommonAppStatsRate (&appCtx->csGrpArr[grpI].grpStats);
    }
}

static void OnMinTick (TlsServerCtx_t* appCtx) {
}

static void OnEstablish (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn
                        , TlsServerGrp_t* csGrp) {
    appConn->csGrp = csGrp;
    // log stats
}

static void OnEstablishErr (TlsServerCtx_t* appCtx
                            , TlsServerConn_t* appConn
                            , TlsServerGrp_t* csGrp) {
    appConn->csGrp = csGrp;
    // log stats
}

static void OnWriteNext (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn) { 
    if (appConn->bytesWritten < appConn->csGrp->scStartTlsLen) {
        int nextChunkLen = 0;
        if ( (appConn->csGrp->scStartTlsLen - appConn->bytesWritten) > 1200 ) {
            nextChunkLen = 1200;
        } else {
            nextChunkLen 
                = (int) (appConn->csGrp->scStartTlsLen - appConn->bytesWritten);
        }
        App_conn_write_next (appConn
                        , appCtx->appWrBuff
                        , 0
                        , nextChunkLen
                        , 1);
    } else {
        if (appConn->isSslInit == 0) {
            if ( appConn->bytesWritten == appConn->csGrp->scStartTlsLen
                    && appConn->bytesRead == appConn->csGrp->csStartTlsLen ) {
                InitSSL (appCtx, appConn);
            }
        } else {
            if (appConn->bytesWritten < appConn->csGrp->scDataLen) {
                int nextChunkLen = 0;
                if ( (appConn->csGrp->scDataLen - appConn->bytesWritten) > 1200 ) {
                    nextChunkLen = 1200;
                } else {
                    nextChunkLen 
                        = (int) (appConn->csGrp->scDataLen - appConn->bytesWritten);
                }
                App_conn_write_next (appConn
                                , appCtx->appWrBuff
                                , 0
                                , nextChunkLen
                                , 1);
            }
        }
    }
}

static void OnWriteStatus (TlsServerCtx_t* appCtx
                            , TlsServerConn_t* appConn
                            , int bytesSent) {
    appConn->bytesWritten += bytesSent;
    if (appConn->bytesWritten == appConn->csGrp->scDataLen) {
        App_conn_write_close (appConn, 0);
    }
}

static void OnReadNext (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn) {
    if ( appConn->bytesRead < appConn->csGrp->csStartTlsLen ) {
        App_conn_read_next (appConn
                        , appCtx->appRdBuff
                        , 0
                        , appConn->csGrp->csStartTlsLen - appConn->bytesRead
                        , 1);
    } else {
        if (appConn->isSslInit == 0) {
            if ( appConn->bytesWritten == appConn->csGrp->scStartTlsLen
                    && appConn->bytesRead == appConn->csGrp->csStartTlsLen ) {
                InitSSL (appCtx, appConn);
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

static void OnReadStatus (TlsServerCtx_t* appCtx
                            , TlsServerConn_t* appConn
                            , int bytesRcvd) {
    appConn->bytesRead += bytesRcvd;
}

static void OnRemoteClose (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn) {
}

static void OnRemoteCloseErr (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn) {
}

static void OnStatus (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn) { 
}

static void OnCleanup (TlsServerCtx_t* appCtx
                        , TlsServerConn_t* appConn) {
    CleanupSSL (appCtx, appConn);
    App_conn_release (appConn, 1);
}

static int OnAppInit (TlsServerCtx_t* appCtx, JObject* appJ) {

    JGET_MEMBER_INT (appJ, "maxActSess", &appCtx->maxActSess);
    JGET_MEMBER_INT (appJ, "maxErrSess", &appCtx->maxErrSess);

    APP_SET_CONN_SESS_LIMITS (appCtx
                                , 0
                                , appCtx->maxActSess
                                , appCtx->maxErrSess
                                , TLS_SERVER_MAX_ACT_CONN_PER_SESSION);

    JArray* csGrpArrJ;
    JGET_MEMBER_ARR (appJ, "csGrpArr", &csGrpArrJ);
    appCtx->csGrpCount = JGET_ARR_LEN (csGrpArrJ);
    appCtx->csGrpArr = CreateArray0 (TlsServerGrp_t, appCtx->csGrpCount);
    
    if (appCtx->csGrpArr) {
        for (int csGrpIndex = 0; csGrpIndex < appCtx->csGrpCount; csGrpIndex++) {
            TlsServerGrp_t* csGrp = &appCtx->csGrpArr[csGrpIndex];
            csGrp->statsCount = 2;
            csGrp->statsArr [0] = (SockStats_t*) &appCtx->allStats;
            csGrp->statsArr [1] = (SockStats_t*) &csGrp->grpStats;
            
            JObject* csGrpJ = JGET_ARR_ELEMENT_OBJ (csGrpArrJ, csGrpIndex);

            //data lens
            JGET_MEMBER_INT (csGrpJ, "csDataLen", &csGrp->csDataLen);
            JGET_MEMBER_INT (csGrpJ, "scDataLen", &csGrp->scDataLen);
            JGET_MEMBER_INT (csGrpJ, "csStartTlsLen", &csGrp->csStartTlsLen);
            JGET_MEMBER_INT (csGrpJ, "scStartTlsLen", &csGrp->scStartTlsLen);

            //server address
            JGET_MEMBER_STR (csGrpJ, "srvIp", &csGrp->srvIp);
            JGET_MEMBER_INT (csGrpJ, "srvPort", &csGrp->srvPort);
            SetSockAddress (&csGrp->srvAddr, csGrp->srvIp, csGrp->srvPort);
        }
    } else {
        // log
        return -1;
    }
    
    return 0;
}

APP_REGISTER_METHODS (TlsServer);