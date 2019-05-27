#include "_tls_client.h"

#include "engine.h"

static AppCtx_t* AppInit (int appId) {

    return NULL;
}

static int OnRunLoop (AppCtx_t* ) {

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

void TlsClient_get_methods (AppMethods_t* appMethods) {

    appMethods->OnEstablish = &OnEstablish;
    appMethods->OnEstablishErr = &OnEstablish;
    appMethods->OnWriteNext = &OnEstablish;

    appMethods->AppInit = & AppInit;

}
