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
    AppCtx_t* appCtx;
    AppSess_t* appSess; 
}AppConnBase_t;

typedef struct AppSessBase {
    AppCtx_t* appCtx;
}AppSessBase_t;

typedef struct AppMethods {

    AppCtx_t* (*OnAppInit) (JObject* appCfg);
    int (*OnAppLoop) (AppCtx_t* appCtx);
    void (*OnAppExit) (AppCtx_t* appCtx);
    void (*OnMinTick) (AppCtx_t* appCtx);

    void (*OnEstablish) (AppCtx_t* appCtx
                        , AppConn_t* appConn);

    void (*OnEstablishErr) (AppCtx_t* appCtx
                        , AppConn_t* appConn);

    void (*OnWriteNext) (AppCtx_t* appCtx
                        , AppConn_t* appConn);

    void (*OnWriteStatus) (AppCtx_t* appCtx
                        , AppConn_t* appConn
                        , int bytesSent);

    void (*OnReadNext) (AppCtx_t* appCtx
                        , AppConn_t* appConn);

    void (*OnReadStatus) (AppCtx_t* appCtx
                        , AppConn_t* appConn
                        , int bytesRcvd);

    void (*OnStatus) (AppCtx_t* appCtx
                        , AppConn_t* appConn);

    void (*OnCleanup) (AppCtx_t* appCtx
                        , AppConn_t* appConn);

    AppSess_t* (*OnCreateSess) ();

    void (*OnInitSess) (AppSess_t* appSess);

    void (*OnDeleteSess) (AppSess_t* appSess);

    uint32_t (*GetMaxActSess) (AppCtx_t* appCtx);

    uint32_t (*GetMaxErrSess) (AppCtx_t* appCtx);

} AppMethods_t;

typedef struct AppCtxW {

    char* appName;
    int appIndex;
    int appStatus;

    AppCtx_t* appCtx;

    AppMethods_t appMethods;

    Pool_t freeSessPool;
    Pool_t actSessPool;

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

} EngCtx_t;

typedef struct SockAddrCtx {
    const char* cIp;
    SockAddr_t sockAddr;
    LocalPortPool_t portPool;
    uint32_t cPortB;
    uint32_t cPortE;
} SockAddrCtx_t;

int nAdmin_channel_setup(EngCtx_t* engCtx);

int App_get_methods (AppCtxW_t* appCtxW);

int App_conn_new (AppCtx_t* appCtx
                    , AppConn_t* appConn
                    , SockAddr_t* localAddr
                    , SockAddr_t* remoteAddr);

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


#endif