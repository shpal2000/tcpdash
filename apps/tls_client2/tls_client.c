#include "app_engine.h"

#include "tls_client.h"

static void CleanupSess (TlsClientSess_t* sess) {
    sess->cConn.appConn = NULL;
    sess->cConn.bytesRead = 0;
    sess->cConn.bytesWritten = 0;
    sess->cConn.writeBuffOffset = 0;
}

static void InitSess (TlsClientSess_t* sess) {
    sess->cConn.pSess = sess;
}

static AppCtx_t* OnAppInit (JObject* appJ, int appIndex) {

    TlsClientCtx_t* appCtx = CreateStruct0 (TlsClientCtx_t);

    if (appCtx) {
        JGET_MEMBER_INT (appJ, "connPerSec", &appCtx->connPerSec);
        JGET_MEMBER_INT (appJ, "maxActSess", &appCtx->maxActSess);
        JGET_MEMBER_INT (appJ, "maxErrSess", &appCtx->maxErrSess);
        JGET_MEMBER_INT (appJ, "maxSess", &appCtx->maxSess);

        CreateSessPools (appCtx, TlsClientSess_t);

        if (appCtx->freeSessPool == NULL) {
            // log
            return NULL;
        }

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

static int OnAppLoop (AppCtx_t* appCtx) {
    return 0;
}

static void OnAppExit (AppCtx_t* appCtx) {

}

static void OnMinTick (AppCtx_t* appCtx) {
    puts ("OnMinTick\n");
}

static void OnEstablish (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx) { 

}

static void OnEstablishErr (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx) {

}

static void OnWriteNext (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx) { 
}

static void OnWriteStatus (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx
                        , int bytesSent) { 
}

static void OnReadNext (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx) { 
}

static void OnReadStatus (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx
                        , int bytesRcvd) { 
}

static void OnStatus (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx) { 
}

static void OnCleanup (AppConn_t* appConn
                        , AppConnCtx_t* appConnCtx) { 
}

void TlsClient_get_methods (AppMethods_t* appMethods) {

    appMethods->OnAppInit = &OnAppInit;
    appMethods->OnAppLoop = &OnAppLoop;
    appMethods->OnAppExit = &OnAppExit;
    appMethods->OnMinTick = &OnMinTick;

    appMethods->OnEstablish = &OnEstablish;
    appMethods->OnEstablishErr = &OnEstablishErr;

    appMethods->OnWriteNext = &OnWriteNext;
    appMethods->OnWriteStatus = &OnWriteStatus;

    appMethods->OnReadNext = &OnReadNext;
    appMethods->OnReadStatus = &OnReadStatus;

    appMethods->OnStatus = &OnStatus;

    appMethods->OnCleanup = &OnCleanup;
}
