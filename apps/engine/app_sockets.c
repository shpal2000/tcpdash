#include "app_engine.h"

#define GetConnection(__appsess,__appconn) \
{ \
    if (__appsess) { \
        AppCtx_t* __appctx = ((AppSessBase_t*)__appsess)->appCtx; \
        AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
        *(__appconn) = GetFromPool (&__appctx_w->freeConnPool); \
        if (*(__appconn)) { \
            (*__appctx_w->appMethods.OnInitConn) (*(__appconn)); \
            ((AppConnBase_t*)(__appconn))->appSess = __appsess; \
        } \
    } else { \
        *(__appconn) = NULL; \
    } \
} 

#define FreeConnetion(__appconn) \
{ \
    AppSessBase_t* __appSess = ((AppConnBase_t*)__appconn)->appSess; \
    AppCtxBase_t* __appctx = (AppCtxBase_t*) __appSess->appCtx; \
    AppCtxW_t* __appctx_w = __appctx->appCtxW; \
    RemoveFromPool (&__appctx_w->actConnPool, __appconn); \
    AddToPool (&__appctx_w->freeConnPool, __appconn); \
}

#define GetSession(__appctx,__appsess) \
{ \
    AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
    *(__appsess) = GetFromPool (&__appctx_w->freeSessPool); \
    if (*(__appsess)) { \
        (*__appctx_w->appMethods.OnInitSess) (*(__appsess)); \
    } \
} 

#define FreeSession(__appsess) \
{ \
    AppCtx_t* __appctx = ((AppSessBase_t*)__appsess)->appCtx; \
    AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
    RemoveFromPool (&__appctx_w->actSessPool, __appsess); \
    AddToPool (&__appctx_w->freeSessPool, __appsess); \
} 

#define FreeParentSession(__conn) FreeSession((((AppConnBase_t*)(__conn))->appSess))

AppConn_t* App_conn_session_child (AppCtx_t* appCtx
                    , AppSess_t* appSess
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
        }
    } else {
        //stats no free connection resource
    }
    return appConn;
}

AppConn_t* App_conn_session_new (AppCtx_t* appCtx
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

void App_conn_release (AppConn_t* appConn
                        , int toFreeSess) {
    if (toFreeSess) {
        FreeParentSession(appConn);
    }
    FreeConnetion(appConn);
}
