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

static int OnAppLoop (TlsClientCtx_t* appCtx) {
    TlsClientSess_t* appSess;
    TlsClientConn_t* appConn;

    GetConnectionX (appCtx, &appSess, &appConn);
    if (appConn) {
        appSess->cConn = appConn; 
        // int connInitErr = App_conn_new (appCtx
        //                                 , appConn
        //                                 , lAddr
        //                                 , rAddr);
    } else {
        //log
    }
    
    return 0;
}

static void OnAppExit (TlsClientCtx_t* appCtx) {

}

static void OnMinTick (TlsClientCtx_t* appCtx) {
    puts ("OnMinTick\n");
}

static void OnEstablish (TlsClientCtx_t* appCtx
                        , TlsClientConn_t* appConn) { 

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
