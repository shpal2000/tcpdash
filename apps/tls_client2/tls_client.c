#include "app_engine.h"

#include "tls_client.h"

static void FreeSession (TlsClientSession_t* newSess) {
}

static AppCtx_t* OnAppInit (JObject* appJ, int appIndex) {

    TlsClientI_t* appCtx = CreateStruct0 (TlsClientI_t);

    if (appCtx) {
        JGET_MEMBER_INT (appJ, "connPerSec", &appCtx->connPerSec);
        JGET_MEMBER_INT (appJ, "maxActSess", &appCtx->maxActSess);
        JGET_MEMBER_INT (appJ, "maxErrSess", &appCtx->maxErrSess);
        JGET_MEMBER_INT (appJ, "maxSess", &appCtx->maxSess);
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
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx) { 

}

static void OnEstablishErr (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx) {

    TlsClientSession_t* tcSess = (TlsClientSession_t*) appSess;
    FreeSession (tcSess);
}

static void OnWriteNext (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx) { 
}

static void OnWriteStatus (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx
                        , int bytesSent) { 
}

static void OnReadNext (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx) { 
}

static void OnReadStatus (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx
                        , int bytesReceived) { 
}

static void OnStatus (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx) { 
}

static void OnCleanup (AppConn_t* appConn
                        , AppSess_t* appSess
                        , AppCtx_t* appCtx) { 
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
