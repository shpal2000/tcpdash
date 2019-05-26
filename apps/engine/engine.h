#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H

#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

typedef IoVentConn_t AppConn_t;

typedef void AppSess_t;

typedef void AppCtx_t;

typedef struct AppMethods {

    void (*OnEstablish) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnEstablishErr) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnWriteNext) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnReadNext) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnCleanup) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnStatus) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx);

    void (*OnWriteStatus) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx
                            , int bytesSent);

    void (*OnReadStatus) (AppConn_t* appConn
                            , AppSess_t* appSess
                            , AppCtx_t* appCtx
                            , int bytesReceived);

    AppCtx_t* (*AppInit) ();

    int (*OnRunLoop) (AppCtx_t* appCtx);

} AppMethods_t;

typedef struct AppCtxW {

    AppCtx_t* appCtx;

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

#endif