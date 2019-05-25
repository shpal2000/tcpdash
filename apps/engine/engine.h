#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H

typedef struct AppConn {
    IoVentConn_t iovConn;
} AppConn_t;

typedef struct AppMethods {

    void (*OnEstablish) (AppConn_t* appConn, void* appSess);

    void (*OnEstablishErr) (AppConn_t* appConn, void* appSess);

    void (*OnWriteNext) (AppConn_t* appConn, void* appSess);

    void (*OnReadNext) (AppConn_t* appConn, void* appSess);

    void (*OnCleanup) (AppConn_t* appConn, void* appSess);

    void (*OnStatus) (AppConn_t* appConn, void* appSess);

    void (*OnWriteStatus) (AppConn_t* appConn
                        , void* appSess
                        , int bytesSent);

    void (*OnReadStatus) (AppConn_t* appConn
                        , void* appSess
                        , int bytesReceived);

    void (*InitConn) (void* connData);

    void (*OnParseCfg) (char* cfgData);
    
    int (*OnRunLoop) (void* appCtx);

    int (*AppInit) (AppCtx_t* appCtx);

    

} AppMethods_t;


typedef struct AppStats {
    SockStats_t connStats;
} AppStats_t;

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

    MsgIoChannelId_t chanId;
    int chanState;
    int chanErr;

    IoVentCtx_t* iovCtx;

    int appCount;
    AppCtx_t* appCtxArr;

    double lastStatsPost;

} EngCtx_t;

typedef struct AppStats {
    SockStats_t connStats;
} AppStats_t;

#endif