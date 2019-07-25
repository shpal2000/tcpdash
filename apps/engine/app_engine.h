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


typedef void AppCtx_t;
typedef void AppConn_t;
typedef void AppConnCtx_t;

typedef struct AppMethods {

    AppCtx_t* (*OnAppInit) (JObject* appCfg, int appIndex);
    int (*OnAppLoop) (AppCtx_t* appCtx);
    void (*OnAppExit) (AppCtx_t* appCtx);
    void (*OnMinTick) (AppCtx_t* appCtx);

    void (*OnEstablish) (AppConn_t* appConn, AppConnCtx_t* appConnCtx);
    void (*OnEstablishErr) (AppConn_t* appConn, AppConnCtx_t* appConnCtx);

    void (*OnWriteNext) (AppConn_t* appConn, AppConnCtx_t* appConnCtx);
    void (*OnWriteStatus) (AppConn_t* appConn, AppConnCtx_t* appConnCtx, int bytesSent);

    void (*OnReadNext) (AppConn_t* appConn, AppConnCtx_t* appConnCtx);
    void (*OnReadStatus) (AppConn_t* appConn, AppConnCtx_t* appConnCtx, int bytesRcvd);

    void (*OnStatus) (AppConn_t* appConn, AppConnCtx_t* appConnCtx);
    void (*OnCleanup) (AppConn_t* appConn, AppConnCtx_t* appConnCtx);

} AppMethods_t;

typedef struct AppCtxW {

    char* appName;
    int appIndex;
    int appStatus;

    AppCtx_t* appCtx;

    AppMethods_t appMethods;

} AppCtxW_t;

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

int App_conn_new (AppConnCtx_t* appConnCtx
                    , SockAddr_t* localAddr
                    , SockAddr_t* remoteAddr
                    );


#define GetFreeSess(__app_ctx, __new_sess) \
{ \
    __new_sess = GetFromPool (__app_ctx->freeSessPool); \
    if (__new_sess) { \
        AddToPool (&__app_ctx->activeSessPool, __new_sess); \
        CleanupSess (__new_sess); \
    } \
} \

#define SetFreeSess(__new_sess) \
{ \
    RemoveFromPool (&__new_sess->appCtx->activeSessPool, __new_sess); \
    AddToPool (__new_sess->appCtx->freeSessPool, __new_sess); \
} \


#define CreateSessPools(__app_ctx,__sess_type) \
{ \
    InitPool(&__app_ctx->actSessPool); \
    CreatePool(&__app_ctx->freeSessPool, __app_ctx->maxActSess, __sess_type,InitSess); \
} \

#endif