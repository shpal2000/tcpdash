#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H

#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#define APP_ENGINE_MAX_ACTIVE_CONNECTION    100000
#define APP_ENGINE_MAX_ERROR_CONNECTION     100000
#define APP_ENGINE_MAX_POLL_TIMEOUT         100

#define APP_STATUS_INIT_FAIL                1
#define APP_STATUS_RUNNING                  2
#define APP_STATUS_EXIT                     100
#define APP_STATUS_EXIT_WITH_ERROR          101

typedef void AppConn_t;
typedef void AppSess_t;
typedef void AppCtx_t;

typedef struct AppMethods {

    AppCtx_t* (*OnAppInit) (JObject* appCfg, int appId);
    int (*OnAppLoop) (AppCtx_t* appCtx);
    int (*OnAppExit) (AppCtx_t* appCtx);

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

    AppCtx_t* appCtx;
    int appStatus;

    int appTypeId;
    void* appCfgData;
    void* appUserData;

    AppMethods_t appMethods;
    int appId;
    char* appName;

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

    double lastStatsPost;

} EngCtx_t;

int App_conn_new (int appId
                        , AppSess_t* appSess
                        , SockAddr_t* localAddr
                        , SockAddr_t* remoteAddr
                        );
#endif