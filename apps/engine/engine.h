#ifndef __APP_ENGINE_H
#define __APP_ENGINE_H


typedef struct AppMethods {

    void (*OnEstablish) (AppConn_t* iovConn);

    void (*OnWriteNext) (AppConn_t* iovConn);

    void (*OnReadNext) (AppConn_t* iovConn);

    void (*OnCleanup) (AppConn_t* iovConn);

    void (*OnStatus) (AppConn_t* iovConn);

    void (*OnWriteStatus) (AppConn_t* iovConn
                        , int bytesSent);

    void (*OnReadStatus) (AppConn_t* iovConn
                        , int bytesReceived);

    void (*InitConn) (void* connData);

    void (*OnParseCfg) (char* cfgData);
    
    int (*OnRunLoop) (void* appCtx);

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

} AppCtx_t;

typedef struct EngCtx {

    char* nAdminIp;
    int nAdminPort;
    char* testCfgId;
    char* testRunId;

    MsgIoChannelId_t chanId;
    int chanState;
    int chanErr;

    AppCtx_t* iovCtx;

    AppCtx_t* appCtxArr;

} EngCtx_t;

typedef struct AppStats {
    SockStats_t connStats;
} AppStats_t;

typedef struct AppConn {
    IoVentConn_t* ioVConn;
    void* appConnCtx;
} AppConn_t

typedef struct AppSess {
    IoVentConn_t ioVConn;
} AppSess_t

#endif