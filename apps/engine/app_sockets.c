#include "app_engine.h"

int App_conn_session_child (AppCtx_t* appCtx
                    , AppSess_t* appSess
                    , SockAddr_t* localAddr
                    , LocalPortPool_t* localPortPool
                    , SockAddr_t* remoteAddr
                    , SockStats_t** statsArr
                    , int statsCount) {
    int isErr = -1;
    AppConn_t* appConn;
    GetConnection(appSess, &appConn);
    if (appConn) {
        AppCtxW_t* appCtxW = ((AppCtxBase_t*)appCtx)->appCtxW; 
        IoVentCtx_t* iovCtx = appCtxW->engCtx->iovCtx;
        appCtxW->connInitCount++;
        appCtxW->lastConnInitTime = TimeElapsedIoVentCtx (iovCtx);
        if ( NewConnection (iovCtx
                            , appConn
                            , localAddr
                            , localPortPool
                            , remoteAddr
                            , 0
                            , statsArr
                            , statsCount) ) {
            // stats
            // handle error ??? free resources
            FreeConnetion(appConn);
        } else {
            isErr = 0;    
        }
    } else {
        //stats no free connection resource
    }
    return isErr;
}

int App_conn_session_new (AppCtx_t* appCtx
                    , SockAddr_t* localAddr
                    , LocalPortPool_t* localPortPool
                    , SockAddr_t* remoteAddr
                    , SockStats_t** statsArr
                    , int statsCount) {
    int isErr = -1;
    AppSess_t* appSess;
    GetSession(appCtx, &appSess);
    if (appSess) {
        if ( App_conn_session_child (appCtx
                                        , appSess
                                        , localAddr
                                        , localPortPool
                                        , remoteAddr
                                        , statsArr
                                        , statsCount) ) {
            // stats
            // handle error ??? free resources
            FreeSession (appSess);
        } else {
            isErr = 0;    
        }
    } else {
        // stats no free session resurce
    }

    return isErr;
}

