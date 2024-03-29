#include "app_engine.h"

#include "tls_client.h"

static void OnInitSessState (TlsClientSess_t* appSess) {
    appSess->cConn = NULL;
}

static void OnInitConnState (TlsClientConn_t* appConn) {
    appConn->bytesRead = 0;
    appConn->bytesWritten = 0;
    appConn->writeBuffOffset = 0;
    appConn->isSslInit = 0;
    appConn->csGrp = NULL;
}

static int OnContinue (TlsClientCtx_t* appCtx) {

    if ( GetConnStats(&appCtx->allStats, tcpConnInitFail) 
                                            >= appCtx->maxErrSess ) {
        return APP_EXIT;
    }

    if ( (GetConnStats(&appCtx->allStats, tcpConnInit) 
                        == appCtx->maxSess) && App_zero_act_sess(appCtx) ) {
        return APP_EXIT;
    }

    return APP_CONTINUE;
}

static void InitSSL (TlsClientCtx_t* appCtx, TlsClientConn_t* appConn) {    
    SSL* ssl = SSL_new(appConn->csGrp->sslCtx);
    if (ssl) {
        appConn->isSslInit = 1;
        App_conn_set_ssl_as_client (appConn, ssl);        
    } else {
        //??? update stats; mark connection state why fail 
        App_conn_abort (appConn);
    }
}

static void CleanupSSL (TlsClientCtx_t* appCtx, TlsClientConn_t* appConn) {
    if (appConn->isSslInit) {
        SSL* ssl = App_conn_get_ssl (appConn);
        SSL_free (ssl);
    }
}

static void OnAppLoop (TlsClientCtx_t* appCtx, int newConnCount) {

    for (uint32_t connIndex = 0; connIndex < newConnCount; connIndex++) {
        uint64_t tcpConnInitCount = GetConnStats(&appCtx->allStats, tcpConnInit);
        if (tcpConnInitCount >= appCtx->maxSess) {
            break;
        }

        TlsClientGrp_t* csGrp = &appCtx->csGrpArr[appCtx->nextCsGrpIndex];
        appCtx->nextCsGrpIndex += 1;
        if (appCtx->nextCsGrpIndex == appCtx->csGrpCount) {
            appCtx->nextCsGrpIndex = 0;
        }

        SockAddrCtx_t* cAddrCtx = &csGrp->cAddrArr[csGrp->cAddrNextIndex];
        csGrp->cAddrNextIndex += 1;
        if (csGrp->cAddrNextIndex == csGrp->cAddrCount) {
            csGrp->cAddrNextIndex = 0;
        }

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
                                        , csGrp
                                        , &cAddrCtx->sockAddr
                                        , &cAddrCtx->portPool 
                                        , &csGrp->srvAddr
                                        , csGrp->statsArr 
                                        , csGrp->statsCount);
        if (appConn == NULL) {
            // handle error
            // log stats 
        } else {
            //log stats
        }
    }
}

static void SetStatsHelper (TlsClientStats_t* stats, JObject* jObj) {
    
    // JSET_MEMBER_INT (jObj, "TlsClientStats1", 1);
    // JSET_MEMBER_INT (jObj, "TlsClientStats2", 2);
    // JSET_MEMBER_INT (jObj, "TlsClientStats3", 3);
    // JSET_MEMBER_INT (jObj, "TlsClientStats4", 4);

    SetCommonAppStats (stats, jObj);
}

static int OnAppStats (TlsClientCtx_t* appCtx, JObject* jAppObj) {
    
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

static void OnAppExit (TlsClientCtx_t* appCtx) {

}

static void OnSecTick (TlsClientCtx_t* appCtx) {
    
    SetCommonAppStatsRate (&appCtx->allStats);

    for (int grpI=0; grpI < appCtx->csGrpCount; grpI++) {
        SetCommonAppStatsRate (&appCtx->csGrpArr[grpI].grpStats);
    }
}

static void OnMinTick (TlsClientCtx_t* appCtx) {
}

static void OnEstablish (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn
                        , TlsClientGrp_t* csGrp) {
    appConn->csGrp = csGrp;
    // log stats
}

static void OnEstablishErr (TlsClientCtx_t* appCtx
                            , TlsClientConn_t* appConn
                            , TlsClientGrp_t* csGrp) {
    appConn->csGrp = csGrp;
    // log stats
}

static void OnWriteNext (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) { 
    if (appConn->bytesWritten < appConn->csGrp->csStartTlsLen) {
        int nextChunkLen = 0;
        if ( (appConn->csGrp->csStartTlsLen - appConn->bytesWritten) > 1200 ) {
            nextChunkLen = 1200;
        } else {
            nextChunkLen 
                = (int) (appConn->csGrp->csStartTlsLen - appConn->bytesWritten);
        }
        App_conn_write_next (appConn
                        , appCtx->appWrBuff
                        , 0
                        , nextChunkLen
                        , 1);
    } else {
        if (appConn->isSslInit == 0) {
            if ( appConn->bytesWritten == appConn->csGrp->csStartTlsLen
                    && appConn->bytesRead == appConn->csGrp->scStartTlsLen ) {
                InitSSL (appCtx, appConn);
            }
        } else {
            if (appConn->bytesWritten < appConn->csGrp->csDataLen) {
                int nextChunkLen = 0;
                if ( (appConn->csGrp->csDataLen - appConn->bytesWritten) > 1200 ) {
                    nextChunkLen = 1200;
                } else {
                    nextChunkLen 
                        = (int) (appConn->csGrp->csDataLen - appConn->bytesWritten);
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

static void OnWriteStatus (TlsClientCtx_t* appCtx
                            , TlsClientConn_t* appConn
                            , int bytesSent) {
    appConn->bytesWritten += bytesSent;
    if (appConn->bytesWritten == appConn->csGrp->csDataLen) {
        App_conn_write_close (appConn, 0);
    }
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

static void OnReadStatus (TlsClientCtx_t* appCtx
                            , TlsClientConn_t* appConn
                            , int bytesRcvd) {
    appConn->bytesRead += bytesRcvd;
}

static void OnRemoteClose (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) {
}

static void OnRemoteCloseErr (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) {
}

static void OnStatus (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) { 
}

static void OnCleanup (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) {
    CleanupSSL(appCtx, appConn);
    App_conn_release (appConn, 1);
}

static int OnAppInit (TlsClientCtx_t* appCtx, JObject* appJ) {

    JGET_MEMBER_INT (appJ, "connPerSec", &appCtx->connPerSec);
    JGET_MEMBER_INT (appJ, "maxActSess", &appCtx->maxActSess);
    JGET_MEMBER_INT (appJ, "maxErrSess", &appCtx->maxErrSess);
    JGET_MEMBER_INT (appJ, "maxSess", &appCtx->maxSess);

    APP_SET_CONN_SESS_LIMITS (appCtx
                                , appCtx->connPerSec
                                , appCtx->maxActSess
                                , appCtx->maxErrSess
                                , TLS_CLIENT_MAX_ACT_CONN_PER_SESSION);

    JArray* csGrpArrJ;
    JGET_MEMBER_ARR (appJ, "csGrpArr", &csGrpArrJ);
    appCtx->csGrpCount = JGET_ARR_LEN (csGrpArrJ);
    appCtx->csGrpArr = CreateArray0 (TlsClientGrp_t, appCtx->csGrpCount);
    
    if (appCtx->csGrpArr) {
        for (int csGrpIndex = 0; csGrpIndex < appCtx->csGrpCount; csGrpIndex++) {
            TlsClientGrp_t* csGrp = &appCtx->csGrpArr[csGrpIndex];
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

            //client address array
            JGET_MEMBER_STR (csGrpJ, "cAddrBase", &csGrp->cAddrBase);
            JGET_MEMBER_INT (csGrpJ, "cAddrCount", &csGrp->cAddrCount);
            csGrp->cAddrArr = CreateArray0 (SockAddrCtx_t, csGrp->cAddrCount);

            if (csGrp->cAddrArr) {
                char* lastIpAddr = csGrp->cAddrBase;
                for (int cAddrIndex = 0; cAddrIndex < csGrp->cAddrCount; cAddrIndex++) {
                    SockAddrCtx_t* cAddrCtx = &csGrp->cAddrArr[cAddrIndex]; 

                    
                    if (cAddrIndex == 0) {
                        GetNextIpStr (lastIpAddr, 0, cAddrCtx->cIp);
                    } else {
                        GetNextIpStr (lastIpAddr, 1, cAddrCtx->cIp);
                    }
                    lastIpAddr = cAddrCtx->cIp; 

                    JGET_MEMBER_INT (csGrpJ, "cPortB", &cAddrCtx->cPortB);
                    JGET_MEMBER_INT (csGrpJ, "cPortE", &cAddrCtx->cPortE);

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
                return -1;
            }
        }
    } else {
        // log
        return -1;
    }

    return 0;
}

APP_REGISTER_METHODS (TlsClient);
