#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H

#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

typedef IoVentConn_t AppConn_t;

typedef void AppSess_t; 

typedef struct AppMethods {

    void (*OnEstablish) (AppConn_t* appConn, AppSess_t* appSess);

    void (*OnEstablishErr) (AppConn_t* appConn, AppSess_t* appSess);

    void (*OnWriteNext) (AppConn_t* appConn, AppSess_t* appSess);

    void (*OnReadNext) (AppConn_t* appConn, AppSess_t* appSess);

    void (*OnCleanup) (AppConn_t* appConn, AppSess_t* appSess);

    void (*OnStatus) (AppConn_t* appConn, AppSess_t* appSess);

    void (*OnWriteStatus) (AppConn_t* appConn
                        , AppSess_t* appSess
                        , int bytesSent);

    void (*OnReadStatus) (AppConn_t* appConn
                        , AppSess_t* appSess
                        , int bytesReceived);

    int (*OnRunLoop) (void* appCtx);

} AppMethods_t;

typedef struct AppCtx {

    int appTypeId;
    void* appCfgData;
    void* appUserData;

    Pool_t* freeSPool;
    Pool_t actSPool;

    AppMethods_t appMethods;
    int appId;
    char* appName;

} AppCtx_t;

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
    AppCtx_t* appCtxArr;

    double lastStatsPost;

} EngCtx_t;

#endif