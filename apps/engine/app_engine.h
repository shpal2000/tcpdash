#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H

#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#define APP_ENGINE_MAX_ACTIVE_CONNECTION    100000
#define APP_ENGINE_MAX_ERROR_CONNECTION     100000
#define APP_ENGINE_MAX_POLL_TIMEOUT         100

#define APP_STATUS_UNDEF                    0
#define APP_STATUS_INIT                     1
#define APP_STATUS_RUNNING                  2
#define APP_STATUS_EXIT                     100
#define APP_STATUS_EXIT_WITH_ERROR          101

typedef void AppConn_t;
typedef void AppSess_t;
typedef void AppCtx_t;

typedef struct AppConnBase {
    IoVentConn_t* ioVentConn;
    AppSess_t* appSess; 
}AppConnBase_t;

typedef struct AppSessBase {
    AppCtx_t* appCtx;
}AppSessBase_t;

//App Function Ptrs
typedef AppCtx_t* (*OnAppInit_t) (JObject*);
typedef int (*OnAppLoop_t) (AppCtx_t*);
typedef void (*OnAppExit_t) (AppCtx_t*);
typedef void (*OnMinTick_t) (AppCtx_t*);

// Socket event Function Ptrs
typedef void (*OnEstablish_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnEstablishErr_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnWriteNext_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnWriteStatus_t) (AppCtx_t*, AppConn_t*, int);
typedef void (*OnReadNext_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnReadStatus_t) (AppCtx_t*, AppConn_t*, int);
typedef void (*OnStatus_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnCleanup_t) (AppCtx_t*, AppConn_t*);

// Session Function Ptrs
typedef AppSess_t* (*OnCreateSess_t) ();
typedef void (*OnInitSess_t) (AppSess_t*);
typedef void (*OnDeleteSess_t) (AppSess_t*);

// Connection Function Ptrs
typedef AppConn_t* (*OnCreateConn_t) ();
typedef void (*OnInitConn_t) (AppConn_t*);
typedef void (*OnDeleteConn_t) (AppConn_t*);

// Get uint32_t application attribute Ptrs
typedef uint32_t (*GetAppAttribUint32_t) (AppCtx_t*);

typedef struct AppMethods {

    OnAppInit_t OnAppInit;
    OnAppLoop_t OnAppLoop;
    OnAppExit_t OnAppExit;
    OnMinTick_t OnMinTick;

    OnEstablish_t OnEstablish;
    OnEstablishErr_t OnEstablishErr;
    OnWriteNext_t OnWriteNext; 
    OnWriteStatus_t OnWriteStatus;
    OnReadNext_t OnReadNext;
    OnReadStatus_t OnReadStatus;
    OnStatus_t OnStatus;
    OnCleanup_t OnCleanup;

    OnCreateSess_t OnCreateSess;
    OnInitSess_t OnInitSess;
    OnDeleteSess_t OnDeleteSess;

    OnCreateConn_t OnCreateConn;
    OnInitConn_t OnInitConn;
    OnDeleteConn_t OnDeleteConn;

    GetAppAttribUint32_t GetMaxActSess;
    GetAppAttribUint32_t GetMaxActConn;

    GetAppAttribUint32_t GetMaxErrSess;
    GetAppAttribUint32_t GetMaxErrConn;

    GetAppAttribUint32_t GetConnPerSec;
} AppMethods_t;

typedef struct AppCtxW {

    char* appName;
    int appStatus;

    AppCtx_t* appCtx;

    AppMethods_t appMethods;

    Pool_t freeSessPool;
    Pool_t actSessPool;

    Pool_t freeConnPool;
    Pool_t actConnPool;

    //for rate control
    uint32_t connPerSec;
    double lastConnInitTime;
    uint64_t connInitCount;
    int nextConnInits; 

    struct EngCtx* engCtx;

} AppCtxW_t;

typedef struct AppCtxBase {
    AppCtxW_t* appCtxW; 
}AppCtxBase_t;

typedef struct EngCtx {

    char* nAdminIp;
    int nAdminPort;
    char* testCfgId;
    char* testCfgSelect;
    char* testRunId;
    char* cfgData;

    SockAddr_t nAdminAddr;
    SockAddr_t nLocalAddr;
    MsgIoChannelId_t chanId;
    int chanState;
    int chanErr;

    IoVentCtx_t* iovCtx;

    int appCount;
    AppCtxW_t* appCtxWArr;

    double lastTickTime;
    uint64_t tickCount;
    int tick5Count;
    int tick60Count;
    int isMinTick;

    uint32_t maxActConn;
    uint32_t maxErrConn;

} EngCtx_t;

typedef struct SockAddrCtx {
    char* cIp;
    SockAddr_t sockAddr;
    LocalPortPool_t portPool;
    uint32_t cPortB;
    uint32_t cPortE;
} SockAddrCtx_t;

int nAdmin_channel_setup(EngCtx_t* engCtx);

int App_get_methods (AppCtxW_t* appCtxW);

int App_conn_session_new (AppCtx_t* appCtx
                            , SockAddr_t* localAddr
                            , LocalPortPool_t* localPortPool
                            , SockAddr_t* remoteAddr
                            , SockStats_t* statsArr
                            , int statsCount);

int App_conn_session_child (AppCtx_t* appCtx
                            , AppSess_t* appSess
                            , SockAddr_t* localAddr
                            , LocalPortPool_t* localPortPool
                            , SockAddr_t* remoteAddr
                            , SockStats_t* statsArr
                            , int statsCount);

#define GetSession(__appctx,__appsess) \
{ \
    AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
    *(__appsess) = GetFromPool (&__appctx_w->freeSessPool); \
    if (*(__appsess)) { \
        (*__appctx_w->appMethods.OnInitSess) (*(__appsess)); \
    } \
} \

#define FreeSession(__appsess) \
{ \
    AppCtx_t* __appctx = ((AppSessBase_t*)__appsess)->appCtx; \
    AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
    RemoveFromPool (&__appctx_w->actSessPool, __appsess); \
    AddToPool (&__appctx_w->freeSessPool, __appsess); \
} \

#define SetParentSession(__conn,__sess) ((AppConnBase_t*)(__conn))->appSess = __sess 

#define GetParentSession(__conn) ((AppConnBase_t*)(__conn))->appSess 

#define GetConnection(__appsess,__appconn) \
{ \
    if (__appsess) { \
        AppCtx_t* __appctx = ((AppSessBase_t*)__appsess)->appCtx; \
        AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
        *(__appconn) = GetFromPool (&__appctx_w->freeConnPool); \
        if (*(__appconn)) { \
            (*__appctx_w->appMethods.OnInitConn) (*(__appconn)); \
            SetParentSession(__appconn,__appsess); \
        } \
    } else { \
        *(__appconn) = NULL; \
    } \
} \

#define GetConnectionX(__appctx,__appsess,__appconn) { \
    GetSession (__appctx, __appsess); \
    GetConnection (*(__appsess), __appconn); \
}

#define FreeConnetion(__appconn) \
{ \
    AppSessBase_t* __appSess = ((AppConnBase_t*)__appconn)->appSess; \
    AppCtxBase_t* __appctx = (AppCtxBase_t*) __appSess->appCtx; \
    AppCtxW_t* __appctx_w = __appctx->appCtxW; \
    RemoveFromPool (&__appctx_w->actConnPool, __appconn); \
    AddToPool (&__appctx_w->freeConnPool, __appconn); \
} \

#define __APPCTX_BASE__ AppCtxBase_t appCtxBase;
#define __APPCONN_BASE__ AppConnBase_t appConnBase;
#define __APPSESS_BASE__ AppSessBase_t appSessBase;


#define APP_DECLARE_METHODS(__app_name) \
void __app_name (AppMethods_t* __app_methods);

#define APP_REGISTER_METHODS(__app_name) \
void __app_name (AppMethods_t* __app_methods) \
{ \
    __app_methods->OnAppInit = (OnAppInit_t) &OnAppInit; \
    __app_methods->OnAppLoop = (OnAppLoop_t) &OnAppLoop; \
    __app_methods->OnAppExit = (OnAppExit_t) &OnAppExit; \
    __app_methods->OnMinTick = (OnMinTick_t) &OnMinTick; \
\
    __app_methods->OnEstablish = (OnEstablish_t) &OnEstablish; \
    __app_methods->OnEstablishErr = (OnEstablishErr_t) &OnEstablishErr; \
    __app_methods->OnWriteNext = (OnWriteNext_t) &OnWriteNext; \
    __app_methods->OnWriteStatus = (OnWriteStatus_t) &OnWriteStatus; \
    __app_methods->OnReadNext = (OnReadNext_t) &OnReadNext; \
    __app_methods->OnReadStatus = (OnReadStatus_t) &OnReadStatus; \
    __app_methods->OnStatus = (OnStatus_t) &OnStatus; \
    __app_methods->OnCleanup = (OnCleanup_t) &OnCleanup; \
\
    __app_methods->OnCreateSess = (OnCreateSess_t) &OnCreateSess; \
    __app_methods->OnInitSess = (OnInitSess_t) &OnInitSess; \
    __app_methods->OnDeleteSess = (OnDeleteSess_t) &OnDeleteSess; \
\
    __app_methods->OnCreateConn = (OnCreateConn_t) &OnCreateConn; \
    __app_methods->OnInitConn = (OnInitConn_t) &OnInitConn; \
    __app_methods->OnDeleteConn = (OnDeleteConn_t) &OnDeleteConn; \
\
    __app_methods->GetMaxActSess = (GetAppAttribUint32_t) &GetMaxActSess; \
    __app_methods->GetMaxActConn = (GetAppAttribUint32_t) &GetMaxActConn; \
\
    __app_methods->GetMaxErrSess = (GetAppAttribUint32_t) &GetMaxErrSess; \
    __app_methods->GetMaxErrConn = (GetAppAttribUint32_t) &GetMaxErrConn; \
\
    __app_methods->GetConnPerSec = (GetAppAttribUint32_t) &GetConnPerSec; \
}

#define APP_GET_METHODS(__appctx_w, __app_name) \
{ \
    if ( strcmp (__appctx_w->appName, #__app_name) == 0 ) { \
        __app_name (&__appctx_w->appMethods); \
        return 0; \
    } \
}

#define APP_GET_CONN_INITS(__appctx) ((AppCtxBase_t*)appCtx)->appCtxW->nextConnInits 
#endif