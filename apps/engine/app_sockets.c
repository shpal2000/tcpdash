#include "app_engine.h"

AppConn_t* App_conn_session_child (AppCtx_t* appCtx
                    , AppSess_t* appSess
                    , AppConnCtx_t* appConnCtx
                    , SockAddr_t* localAddr
                    , LocalPortPool_t* localPortPool
                    , SockAddr_t* remoteAddr
                    , SockStats_t** statsArr
                    , int statsCount) {
    AppConn_t* appConn = NULL;
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
            appConn = NULL;
        } else {
            ((AppConnBase_t*)appConn)->appConnCtx = appConnCtx;
        }
    } else {
        //stats no free connection resource
    }
    return appConn;
}

AppConn_t* App_conn_session_new (AppCtx_t* appCtx
                    , AppConnCtx_t* appConnCtx
                    , SockAddr_t* localAddr
                    , LocalPortPool_t* localPortPool
                    , SockAddr_t* remoteAddr
                    , SockStats_t** statsArr
                    , int statsCount) {
    AppSess_t* appSess = NULL;
    AppConn_t* appConn = NULL;
    GetSession(appCtx, &appSess);
    if (appSess) {
        appConn = App_conn_session_child (appCtx
                                        , appSess
                                        , appConnCtx
                                        , localAddr
                                        , localPortPool
                                        , remoteAddr
                                        , statsArr
                                        , statsCount);
        if ( appConn == NULL ) {
            // stats
            // handle error ??? free resources
            FreeSession (appSess);
        } else {
            // stats
        }
    } else {
        // stats no free session resurce
    }

    return appConn;
}

void App_server_init (AppCtx_t* appCtx
                    , AppConnCtx_t* appConnCtx
                    , SockAddr_t* localAddress
                    , SockStats_t** statsArr
                    , int statsCount) {
    AppSess_t* appSess = NULL;
    AppConn_t* appConn = NULL;
    GetSession(appCtx, &appSess);
    if (appSess) {
        GetConnection(appSess, &appConn); 
        if ( appConn == NULL ) {
            // stats
            // handle error ??? free resources
            FreeSession (appSess);
        } else {
            ((AppConnBase_t*)appConn)->isSrv = 1;
            ((AppConnBase_t*)appConn)->appConnCtx = appConnCtx;
            InitServer((((AppCtxBase_t*)appCtx)->appCtxW)->engCtx->iovCtx
                , appConn
                , localAddress
                , statsArr
                , statsCount);
        }
    } else {
        // stats no free session resurce
        // handle error ???
    }
}
