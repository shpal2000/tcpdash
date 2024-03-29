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

#define MAX_START_APP_MSG_LENGTH            1024

typedef void AppConn_t;
typedef void AppSess_t;
typedef void AppCtx_t;
typedef void SrvCtx_t;
typedef void AppConnCtx_t;
typedef void AppStats_t;

typedef struct AppConnBase {
    IoVentConn_t* ioVentConn;
    AppSess_t* appSess;
    int isSrv;
    AppConnCtx_t* appConnCtx;
}AppConnBase_t;

typedef struct AppSessBase {
    AppCtx_t* appCtx;
}AppSessBase_t;

typedef struct AppStatsBase {
    SockStats_t connStats;
}AppStatsBase_t;

//App Function Ptrs
typedef int (*OnAppInit_t) (AppCtx_t*, JObject*);
typedef void (*OnAppLoop_t) (AppCtx_t*, int);
typedef void (*OnAppExit_t) (AppCtx_t*);
typedef int (*OnContinue_t) (AppCtx_t*);
typedef void (*OnSecTick_t) (AppCtx_t*);
typedef void (*OnMinTick_t) (AppCtx_t*);
typedef int (*OnAppStats_t) (AppCtx_t*, JObject*);

// Socket event Function Ptrs
typedef void (*OnEstablish_t) (AppCtx_t*, AppConn_t*, AppConnCtx_t*);
typedef void (*OnEstablishErr_t) (AppCtx_t*, AppConn_t*, AppConnCtx_t*);
typedef void (*OnWriteNext_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnWriteStatus_t) (AppCtx_t*, AppConn_t*, int);
typedef void (*OnReadNext_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnReadStatus_t) (AppCtx_t*, AppConn_t*, int);
typedef void (*OnRemoteClose_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnRemoteCloseErr_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnStatus_t) (AppCtx_t*, AppConn_t*);
typedef void (*OnCleanup_t) (AppCtx_t*, AppConn_t*);

// AppCtx Function Ptrs
typedef AppCtx_t* (*OnCreateAppCtx_t) ();

// Session Function Ptrs
typedef AppSess_t* (*OnCreateSess_t) ();
typedef void (*OnInitSessState_t) (AppSess_t*);
typedef void (*OnDeleteSess_t) (AppSess_t*);

// Connection Function Ptrs
typedef AppConn_t* (*OnCreateConn_t) ();
typedef void (*OnInitConnState_t) (AppConn_t*);
typedef void (*OnDeleteConn_t) (AppConn_t*);

typedef struct AppMethods {

    OnAppInit_t OnAppInit;
    OnAppLoop_t OnAppLoop;
    OnAppExit_t OnAppExit;
    OnContinue_t OnContinue;
    OnMinTick_t OnSecTick;
    OnMinTick_t OnMinTick;
    OnAppStats_t OnAppStats;

    OnEstablish_t OnEstablish;
    OnEstablishErr_t OnEstablishErr;
    OnWriteNext_t OnWriteNext; 
    OnWriteStatus_t OnWriteStatus;
    OnReadNext_t OnReadNext;
    OnReadStatus_t OnReadStatus;
    OnRemoteClose_t OnRemoteClose;
    OnRemoteCloseErr_t OnRemoteCloseErr;
    OnStatus_t OnStatus;
    OnCleanup_t OnCleanup;

    OnCreateAppCtx_t OnCreateAppCtx;
    OnCreateSess_t OnCreateSess;
    OnInitSessState_t OnInitSessState;
    OnDeleteSess_t OnDeleteSess;

    OnCreateConn_t OnCreateConn;
    OnInitConnState_t OnInitConnState;
    OnDeleteConn_t OnDeleteConn;

} AppMethods_t;

typedef struct AppCtxW {

    char* appName;
    int appIndex;
    int appStatus;

    AppCtx_t* appCtx;

    AppMethods_t appMethods;

    uint32_t connPerSec;
    uint32_t maxActSess;
    uint32_t maxErrSess;
    uint32_t maxActConn;
    uint32_t maxErrConn;
    
    Pool_t freeSessPool;
    Pool_t actSessPool;

    Pool_t freeConnPool;
    Pool_t actConnPool;

    //for rate control
    double lastConnInitTime;
    uint64_t connInitCount;

    struct EngCtx* engCtx;

} AppCtxW_t;

typedef struct AppCtxBase {
    AppCtxW_t* appCtxW; 
}AppCtxBase_t;

typedef struct EngCtx {

    char* nAdminIp;
    int nAdminPort;
    char* cfgId;
    char* cfgSelect;
    char* docName;

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
    int isSecTick;

    uint32_t maxActConn;
    uint32_t maxErrConn;

} EngCtx_t;

typedef struct SockAddrCtx {
    char cIp [INET6_ADDRSTRLEN];
    SockAddr_t sockAddr;
    LocalPortPool_t portPool;
    uint32_t cPortB;
    uint32_t cPortE;
} SockAddrCtx_t;

int nAdmin_channel_setup(EngCtx_t* engCtx);

int App_get_methods (AppCtxW_t* appCtxW);

AppConn_t* App_conn_session_new (AppCtx_t* appCtx
                            , AppConnCtx_t* appConnCtx
                            , SockAddr_t* localAddr
                            , LocalPortPool_t* localPortPool
                            , SockAddr_t* remoteAddr
                            , SockStats_t** statsArr
                            , int statsCount);

AppConn_t* App_conn_session_child (AppCtx_t* appCtx
                            , AppSess_t* appSess
                            , AppConnCtx_t* appConnCtx
                            , SockAddr_t* localAddr
                            , LocalPortPool_t* localPortPool
                            , SockAddr_t* remoteAddr
                            , SockStats_t** statsArr
                            , int statsCount);

#define APP_IOVENT_CONN(__appConn) (((AppConnBase_t*)__appConn)->ioVentConn)

#define App_conn_abort(__appConn) AbortConnection (APP_IOVENT_CONN(__appConn))

#define App_conn_read_next(__appConn,__rbuf,__roff,__rlen,__rpart) \
ReadNextData(APP_IOVENT_CONN(__appConn),__rbuf,__roff,__rlen,__rpart)

#define App_conn_write_next(__appConn,__wbuf,__woff,__wlen,__wpart) \
WriteNextData(APP_IOVENT_CONN(__appConn),__wbuf,__woff,__wlen,__wpart)

#define App_conn_write_close(__appConn,__ssl_close_notify) \
WriteClose (APP_IOVENT_CONN(__appConn))

#define App_conn_set_ssl_as_client(__appConn,__ssl) \
InitSslConnection(APP_IOVENT_CONN(__appConn), __ssl, 1);

#define App_conn_set_ssl_as_server(__appConn,__ssl) \
InitSslConnection(APP_IOVENT_CONN(__appConn), __ssl, 0)

#define App_conn_get_ssl(__appConn) IOV_GET_SSL(APP_IOVENT_CONN(__appConn))

void App_server_init (AppCtx_t* appCtx
                    , AppConnCtx_t* appConnCtx
                    , SockAddr_t* localAddress
                    , SockStats_t** statsArr
                    , int statsCount);

#define GetAppIndex(__appctx) (((AppCtxBase_t*)__appctx)->appCtxW->appIndex)

#define GetAppStatsIdBuff(__appctx) \
(((AppCtxBase_t*)__appctx)->appCtxW->appStatsIdBuff)

#define GetConnection(__appsess,__appconn) \
{ \
    if (__appsess) { \
        AppCtx_t* __appctx = ((AppSessBase_t*)__appsess)->appCtx; \
        AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
        *(__appconn) = GetFromPool (&__appctx_w->freeConnPool); \
        if (*(__appconn)) { \
            AddToPool (&__appctx_w->actConnPool,*(__appconn)); \
            (*__appctx_w->appMethods.OnInitConnState) (*(__appconn)); \
            ((AppConnBase_t*)(*(__appconn)))->appSess = __appsess; \
            ((AppConnBase_t*)(*(__appconn)))->isSrv = 0; \
            ((AppConnBase_t*)(*(__appconn)))->appConnCtx = NULL; \
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
        AddToPool (&__appctx_w->actSessPool,*(__appsess)); \
        (*__appctx_w->appMethods.OnInitSessState) (*(__appsess)); \
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

#define App_zero_act_sess(__appctx) IsPoolEmpty((&((AppCtxBase_t*)__appctx)->appCtxW->actSessPool)) 

#define App_conn_release(__appconn,__free_parent_sess) \
{ \
    if (__free_parent_sess) { \
        FreeParentSession(__appconn); \
    } \
    FreeConnetion(__appconn); \
}

#define __APPCTX_BASE__ AppCtxBase_t appCtxBase;
#define __APPCONN_BASE__ AppConnBase_t appConnBase;
#define __APPSESS_BASE__ AppSessBase_t appSessBase;
#define __APPSTATS_BASE__ AppStatsBase_t appStatsBase;


#define APP_DECLARE_METHODS(__app_name) \
void __app_name (AppMethods_t* __app_methods);

#define APP_REGISTER_METHODS(__app_name) \
static __app_name##Ctx_t* OnCreateAppCtx () { \
    return CreateStruct0 (__app_name##Ctx_t); \
} \
static __app_name##Sess_t* OnCreateSess () { \
    return CreateStruct0 (__app_name##Sess_t); \
} \
\
static void OnDeleteSess (__app_name##Sess_t* appSess) { \
    DeleteStruct (__app_name##Sess_t, appSess); \
} \
\
static __app_name##Conn_t* OnCreateConn () { \
    return CreateStruct0 (__app_name##Conn_t); \
} \
\
static void OnDeleteConn (__app_name##Conn_t* appConn) { \
    DeleteStruct (__app_name##Conn_t, appConn); \
} \
void __app_name (AppMethods_t* __app_methods) \
{ \
    __app_methods->OnAppInit = (OnAppInit_t) &OnAppInit; \
    __app_methods->OnAppLoop = (OnAppLoop_t) &OnAppLoop; \
    __app_methods->OnAppExit = (OnAppExit_t) &OnAppExit; \
    __app_methods->OnContinue = (OnContinue_t) &OnContinue; \
    __app_methods->OnSecTick = (OnMinTick_t) &OnSecTick; \
    __app_methods->OnMinTick = (OnMinTick_t) &OnMinTick; \
    __app_methods->OnAppStats = (OnAppStats_t) &OnAppStats; \
\
    __app_methods->OnEstablish = (OnEstablish_t) &OnEstablish; \
    __app_methods->OnEstablishErr = (OnEstablishErr_t) &OnEstablishErr; \
    __app_methods->OnWriteNext = (OnWriteNext_t) &OnWriteNext; \
    __app_methods->OnWriteStatus = (OnWriteStatus_t) &OnWriteStatus; \
    __app_methods->OnReadNext = (OnReadNext_t) &OnReadNext; \
    __app_methods->OnReadStatus = (OnReadStatus_t) &OnReadStatus; \
    __app_methods->OnRemoteClose = (OnRemoteClose_t) &OnRemoteClose; \
    __app_methods->OnRemoteCloseErr = (OnRemoteCloseErr_t) &OnRemoteCloseErr; \
    __app_methods->OnStatus = (OnStatus_t) &OnStatus; \
    __app_methods->OnCleanup = (OnCleanup_t) &OnCleanup; \
\
    __app_methods->OnCreateAppCtx = (OnCreateAppCtx_t) &OnCreateAppCtx; \
\
    __app_methods->OnCreateSess = (OnCreateSess_t) &OnCreateSess; \
    __app_methods->OnInitSessState = (OnInitSessState_t) &OnInitSessState; \
    __app_methods->OnDeleteSess = (OnDeleteSess_t) &OnDeleteSess; \
\
    __app_methods->OnCreateConn = (OnCreateConn_t) &OnCreateConn; \
    __app_methods->OnInitConnState = (OnInitConnState_t) &OnInitConnState; \
    __app_methods->OnDeleteConn = (OnDeleteConn_t) &OnDeleteConn; \
}

#define APP_GET_METHODS(__appctx_w, __app_name) \
{ \
    if ( strcmp (__appctx_w->appName, #__app_name) == 0 ) { \
        __app_name (&__appctx_w->appMethods); \
        return 0; \
    } \
}

#define APP_GET_NEW_CONN_COUNT(__appctx) ((AppCtxBase_t*)appCtx)->appCtxW->nextConnInits

#define APP_SET_CONN_SESS_LIMITS(__appctx,__cps,__act_s,__err_s,__c_per_s) \
{ \
    AppCtxW_t* __appctx_w = ((AppCtxBase_t*)__appctx)->appCtxW; \
    __appctx_w->connPerSec = __cps; \
    __appctx_w->maxActSess = __act_s; \
    __appctx_w->maxErrSess = __err_s; \
    __appctx_w->maxActConn = (__act_s) * (__c_per_s); \
    __appctx_w->maxErrConn = (__err_s) * (__c_per_s); \
}

#define APP_CONTINUE 1
#define APP_EXIT 0

void SetCommonAppStats (AppStats_t* aStats
                        , JObject* jObj);

void SetCommonAppStatsRate (AppStats_t* aStats);


#define SetAppStartMsg(__msg,__mId,__cfgId,__cfgSelect,__docName) { \
sprintf (__msg, \
"{ \
    \"MessageType\" : \"AppStart\", \
    \"MessageId\" : %d, \
    \"Message\" : { \
        \"cfgId\" : \"%s\", \
        \"cfgSelect\" : \"%s\", \
        \"docName\" : \"%s\" \
    } \
}", \
__mId,__cfgId,__cfgSelect,__docName); \
}


#endif