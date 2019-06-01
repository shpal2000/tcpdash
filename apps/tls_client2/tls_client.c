#include "app_engine.h"

#include "tls_client.h"

static void FreeSession (TlsClientSession_t* newSess) {
}

static AppCtx_t* OnAppInit (JObject* appCfg, int appIndex) {

    return CreateStruct0 (TlsClientI_t);
}

static int OnAppLoop (AppCtx_t* appCtx) {
    return 0;
}

static int OnAppExit (AppCtx_t* appCtx) {

    return 0;
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

    appMethods->OnEstablish = &OnEstablish;
    appMethods->OnEstablishErr = &OnEstablishErr;

    appMethods->OnWriteNext = &OnWriteNext;
    appMethods->OnWriteStatus = &OnWriteStatus;

    appMethods->OnReadNext = &OnReadNext;
    appMethods->OnReadStatus = &OnReadStatus;

    appMethods->OnStatus = &OnStatus;

    appMethods->OnCleanup = &OnCleanup;
}
