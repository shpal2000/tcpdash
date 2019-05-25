#include "_tls_client.h"

#include "engine.h"

static void OnEstablish (AppConn_t* appConn, AppSess_t* appSess) { 

}

static void OnEstablishErr (AppConn_t* appConn, AppSess_t* appSess) {
    TlsClientSession_t* tcSess = (TlsClientSession_t*) appSess;
    FreeSession (tcSess);
}

static void OnWriteNext (AppConn_t* appConn, AppSess_t* appSess) { 
}

void TlsClient_get_methods (AppMethods_t* appMethods) {

    appMethods->OnEstablish = &OnEstablish;
    appMethods->OnEstablishErr = &OnEstablish;
    appMethods->OnWriteNext = &OnEstablish;

}
