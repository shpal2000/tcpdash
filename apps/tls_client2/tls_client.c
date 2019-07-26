#include "app_engine.h"

#include "tls_client.h"

static AppSess_t* OnCreateSess () {
    TlsClientSess_t* tlscSess = CreateStruct0 (TlsClientSess_t);
    SetParentSession (&tlscSess->cConn, tlscSess);
    return tlscSess;
}

static void OnInitSess (AppSess_t* appSess) {
    TlsClientSess_t* tlscSess = appSess;
    tlscSess->cConn.bytesRead = 0;
    tlscSess->cConn.bytesWritten = 0;
    tlscSess->cConn.writeBuffOffset = 0;
}


static void OnDeleteSess (AppSess_t* appSess) {
    DeleteStruct (TlsClientSess_t, appSess);
}

static AppCtx_t* OnAppInit (JObject* appJ) {

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

static uint32_t GetMaxActSess (AppCtx_t* appCtx) {
    return ((TlsClientCtx_t*)appCtx)->maxActSess;
}

static uint32_t GetMaxErrSess (AppCtx_t* appCtx) {
    return ((TlsClientCtx_t*)appCtx)->maxErrSess;
}

static int OnAppLoop (AppCtx_t* appCtx) {
    return 0;
}

static void OnAppExit (AppCtx_t* appCtx) {

}

static void OnMinTick (AppCtx_t* appCtx) {
    puts ("OnMinTick\n");
}

static void OnEstablish (AppCtx_t* appCtx
                        , AppConn_t* appConn) { 

}

static void OnEstablishErr (AppCtx_t* appCtx
                            , AppConn_t* appConn) {

}

static void OnWriteNext (AppCtx_t* appCtx
                        , AppConn_t* appConn) { 
}

static void OnWriteStatus (AppCtx_t* appCtx
                            , AppConn_t* appConn
                            , int bytesSent) { 
}

static void OnReadNext (AppCtx_t* appCtx
                        , AppConn_t* appConn) { 
}

static void OnReadStatus (AppCtx_t* appCtx
                            , AppConn_t* appConn
                            , int bytesRcvd) { 
}

static void OnStatus (AppCtx_t* appCtx
                        , AppConn_t* appConn) { 
}

static void OnCleanup (AppCtx_t* appCtx
                        , AppConn_t* appConn) { 
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

    appMethods->OnCreateSess = &OnCreateSess;
    appMethods->OnInitSess = &OnInitSess;
    appMethods->OnDeleteSess = &OnDeleteSess;

    appMethods->GetMaxActSess = &GetMaxActSess;
    appMethods->GetMaxErrSess = &GetMaxErrSess;

}
