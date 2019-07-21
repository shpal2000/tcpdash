#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H

#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#define APP_ENGINE_MAX_ACTIVE_CONNECTION    100000
#define APP_ENGINE_MAX_ERROR_CONNECTION     100000
#define APP_ENGINE_MAX_POLL_TIMEOUT         100

#define APP_STATUS_INIT                     0
#define APP_STATUS_RUNNING                  1
#define APP_STATUS_EXIT                     100
#define APP_STATUS_EXIT_WITH_ERROR          101

typedef void AppConn_t;
typedef void AppSess_t;
typedef void AppCtx_t;

typedef struct AppMethods {

    AppCtx_t* (*OnAppInit) (JObject* appCfg, int appIndex);
    int (*OnAppLoop) (AppCtx_t* appCtx);
    void (*OnAppExit) (AppCtx_t* appCtx);
    void (*OnMinTick) (AppCtx_t* appCtx);

    void (*OnEstablish) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnEstablishErr) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnWriteNext) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnWriteStatus) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx
                            , int bytesSent);

    void (*OnReadNext) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnReadStatus) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx
                            , int bytesReceived);

    void (*OnStatus) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnCleanup) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);
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

int nAdmin_channel_setup(EngCtx_t* engCtx);
int App_get_methods (AppCtxW_t* appCtxW);
int App_parse_config (AppCtxW_t* appCtxW, JObject* appJ);

int App_conn_new (int appId
                        , AppSess_t* appSess
                        , SockAddr_t* localAddr
                        , SockAddr_t* remoteAddr
                        );
#endif