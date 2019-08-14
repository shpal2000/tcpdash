#include "app_engine.h"

static void OnEstablish (struct IoVentConn* iovConn) {
    AppConnBase_t* appConn = (AppConnBase_t*) IOV_GET_CONN_CTX (iovConn);
    AppSessBase_t* appSess = (AppSessBase_t*) appConn->appSess;
    AppCtxBase_t* appCtx = (AppCtxBase_t*) appSess->appCtx;

    if (appConn->isSrv) { //server connection
        AppSess_t* newAppSess = NULL;
        AppConn_t* newAppConn = NULL;
        GetSession(appCtx, &newAppSess);
        if (newAppSess) {
            GetConnection(newAppSess, &newAppConn);
            if (newAppConn) {
                IOV_SET_CONN_CTX (iovConn, newAppConn);
                ((AppConnBase_t*)newAppConn)->ioVentConn = iovConn;
                ((AppConnBase_t*)newAppConn)->appConnCtx = appConn->appConnCtx;
                (*appCtx->appCtxW->appMethods.OnEstablish) (appCtx
                                    , newAppConn
                                    , ((AppConnBase_t*)newAppConn)->appConnCtx);
            } else {
                FreeSession (newAppSess);
                AbortConnection (iovConn);
                //??? error handling 
            }
        } else {
            AbortConnection (iovConn);
            //??? error handling
        }
    }
    else { //client connection
        appConn->ioVentConn = iovConn;
        if ( IsConnErr (iovConn) ) {
            (*appCtx->appCtxW->appMethods.OnEstablishErr) (appCtx
                                                    , appConn
                                                    , appConn->appConnCtx);
        } else {
            (*appCtx->appCtxW->appMethods.OnEstablish) (appCtx
                                                    , appConn
                                                    , appConn->appConnCtx);
        }
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {
    AppConnBase_t* appConn = (AppConnBase_t*) iovConn->cInfo.connCtx;
    AppSessBase_t* appSess = (AppSessBase_t*) appConn->appSess;
    AppCtxBase_t* appCtx = (AppCtxBase_t*) appSess->appCtx;
    (*appCtx->appCtxW->appMethods.OnReadNext) (appCtx, appConn);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {
    AppConnBase_t* appConn = (AppConnBase_t*) iovConn->cInfo.connCtx;
    AppSessBase_t* appSess = (AppSessBase_t*) appConn->appSess;
    AppCtxBase_t* appCtx = (AppCtxBase_t*) appSess->appCtx;

    if (bytesRead > 0) {
        (*appCtx->appCtxW->appMethods.OnReadStatus) (appCtx, appConn, bytesRead);
    } else {
        int closeErr = bytesRead;
        if (closeErr != ON_CLOSE_ERROR_NONE) { 
            (*appCtx->appCtxW->appMethods.OnRemoteCloseErr) (appCtx, appConn);
            AbortConnection (iovConn);
        } else {
            (*appCtx->appCtxW->appMethods.OnRemoteClose) (appCtx, appConn);
        }
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {
    AppConnBase_t* appConn = (AppConnBase_t*) iovConn->cInfo.connCtx;
    AppSessBase_t* appSess = (AppSessBase_t*) appConn->appSess;
    AppCtxBase_t* appCtx = (AppCtxBase_t*) appSess->appCtx;
    (*appCtx->appCtxW->appMethods.OnWriteNext) (appCtx, appConn);
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {
    AppConnBase_t* appConn = (AppConnBase_t*) iovConn->cInfo.connCtx;
    AppSessBase_t* appSess = (AppSessBase_t*) appConn->appSess;
    AppCtxBase_t* appCtx = (AppCtxBase_t*) appSess->appCtx;

    if (bytesWritten > 0) {
        (*appCtx->appCtxW->appMethods.OnWriteStatus) (appCtx, appConn, bytesWritten);
    } else {
        (*appCtx->appCtxW->appMethods.OnRemoteCloseErr) (appCtx, appConn);
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {
    AppConnBase_t* appConn = (AppConnBase_t*) iovConn->cInfo.connCtx;
    AppSessBase_t* appSess = (AppSessBase_t*) appConn->appSess;
    AppCtxBase_t* appCtx = (AppCtxBase_t*) appSess->appCtx;

    if (appConn->isSrv == 0) {
        (*appCtx->appCtxW->appMethods.OnCleanup) (appCtx, appConn);
    }
}

int App_alloc_resources (AppCtx_t* appCtx) {

    int status = 0;
    AppCtxW_t* appCtxW = ((AppCtxBase_t*)appCtx)->appCtxW;

    InitPool (&appCtxW->actSessPool);
    InitPool (&appCtxW->freeSessPool);
    InitPool (&appCtxW->actConnPool);
    InitPool (&appCtxW->freeConnPool);

    // ??? uint32_t maxErrSess = (*appCtxW->appMethods.OnGetMaxErrSess)(appCtx); 
    for (uint32_t i = 0; i < appCtxW->maxActSess; i++) {
        AppSess_t* newSess = (*appCtxW->appMethods.OnCreateSess)();
        if (newSess) {
            AppSessBase_t* newSessBase = (AppSessBase_t*) newSess; 
            newSessBase->appCtx = appCtx; 
            AddToPool (&appCtxW->freeSessPool, newSess);
        } else {
            while (1) {
                AppSess_t* oldSess = GetFromPool (&appCtxW->freeSessPool);
                if (oldSess == NULL) {
                    break;
                }
                (*appCtxW->appMethods.OnDeleteSess)(oldSess);
            }
            status = -1;
            break; 
        }
    }

    if (status == 0) {
        for (uint32_t i = 0; i < appCtxW->maxActConn; i++) {
            AppConn_t* newConn = (*appCtxW->appMethods.OnCreateConn)();
            if (newConn) {
                AddToPool (&appCtxW->freeConnPool, newConn);
            } else {
                while (1) {
                    AppConn_t* oldConn = GetFromPool (&appCtxW->freeConnPool);
                    if (oldConn == NULL) {
                        break;
                    }
                    (*appCtxW->appMethods.OnDeleteConn)(oldConn);
                }
                status = -1;
                break; 
            }
        }
    }

    return status;
}

static int App_ctx_setup (EngCtx_t* engCtx) {
    
    int status = 0;
    engCtx->appCount = 0;

    JNode* cfgNode = NULL;
    JObject* cfgObjAll = NULL;
    JObject* cfgObj = NULL;

    puts(engCtx->cfgData);
    JGET_ROOT_NODE (engCtx->cfgData, &cfgNode, &cfgObjAll);
    if (cfgObjAll) {
        JGET_MEMBER_OBJ (cfgObjAll, engCtx->testCfgSelect, &cfgObj);
        if (cfgObj) {
            JArray* appArrJ;
            JGET_MEMBER_ARR (cfgObj, "appList", &appArrJ);
            engCtx->appCount = JGET_ARR_LEN (appArrJ);
            if (engCtx->appCount) {
                engCtx->appCtxWArr = CreateArray0 (AppCtxW_t, engCtx->appCount);
                if (engCtx->appCtxWArr == NULL) {
                    status = -1; //??? log
                } else {
                    for (int appIndex = 0; appIndex < engCtx->appCount; appIndex++) {
                        AppCtxW_t* appCtxW = &engCtx->appCtxWArr[appIndex];
                        JObject* appJ = JGET_ARR_ELEMENT_OBJ (appArrJ, appIndex);
                        appCtxW->engCtx = engCtx;
                        appCtxW->appStatus = APP_STATUS_INIT;
                        JGET_MEMBER_STR (appJ, "appName", &appCtxW->appName);
                        if ( App_get_methods (appCtxW) ) {
                            status = -1; //??? log
                            break;
                        }
                        
                        AppCtx_t* appCtx = (*appCtxW->appMethods.OnAppInit) (appJ);
                        if (appCtx) {
                            appCtxW->appCtx = appCtx; 
                            ((AppCtxBase_t*)appCtx)->appCtxW = appCtxW;
                            if (App_alloc_resources (appCtx)) {
                                status = -1; //??? log
                                break;
                            } else {
                                engCtx->maxActConn += appCtxW->maxActConn;
                                engCtx->maxErrConn += appCtxW->maxErrConn;
                                appCtxW->appStatus = APP_STATUS_RUNNING;
                            }
                        } else {
                            status = -1; //??? log
                            break;
                        }
                    }
                }
            } else {
                status = -1; //??? log
            }
        } else {
            status = -1; //??? log    
        }
    } else {
            status = -1; //??? log
    }

    return status;
}

static int IoVent_ctx_setup (EngCtx_t* engCtx) {

    int status = 0;

    IoVentMethods_t iovMethods;
    iovMethods.OnEstablish = &OnEstablish;
    iovMethods.OnWriteNext = &OnWriteNext;
    iovMethods.OnWriteStatus = &OnWriteStatus;
    iovMethods.OnReadNext = &OnReadNext;
    iovMethods.OnReadStatus = &OnReadStatus;
    iovMethods.OnCleanup = &OnCleanup;
    iovMethods.OnStatus = NULL;
    iovMethods.OnContinue = NULL;

    IoVentOptions_t iovOptions;
    iovOptions.maxActiveConnections = engCtx->maxActConn;
    iovOptions.maxErrorConnections = engCtx->maxErrConn;
    iovOptions.eventPTO = APP_ENGINE_MAX_POLL_TIMEOUT;
    iovOptions.maxEvents = 0;

    engCtx->iovCtx = CreateIoVentCtx (&iovMethods, &iovOptions, engCtx);
    
    if (engCtx->iovCtx == NULL) {
        status = -1;
    }

    return status;
}

static int App_library_init (EngCtx_t* engCtx) {

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    return 0;
}

static void Engine_post_1sec_tick (EngCtx_t* engCtx) {

    // statsString = 
    MsgIoSend (engCtx->chanId
                , "1"
                , strlen("1") );
}

static void Engine_post_5sec_tick (EngCtx_t* engCtx) {

    // statsString = 
    MsgIoSend (engCtx->chanId
                , "5"
                , strlen("5") );
}

static void Engine_post_60sec_tick (EngCtx_t* engCtx) {
    engCtx->isMinTick = 1;
}

static void Engine_tick (EngCtx_t* engCtx) {
    engCtx->lastTickTime = MsgIoTimeElapsed (engCtx->chanId);
    engCtx->tickCount += 1;

    Engine_post_1sec_tick (engCtx);

    engCtx->tick5Count++;
    if (engCtx->tick5Count == 5) {
        engCtx->tick5Count = 0;
        Engine_post_5sec_tick (engCtx);
    }

    engCtx->tick60Count++;
    if (engCtx->tick60Count == 60) {
        engCtx->tick60Count = 0;
        Engine_post_60sec_tick (engCtx);
    }
}

static int Engine_loop (EngCtx_t* engCtx) {

    int status = 0;

    for (int i = 0; i < engCtx->appCount; i++) {
        AppCtxW_t* appCtxW = &engCtx->appCtxWArr[i];
        if ( appCtxW->appStatus == APP_STATUS_RUNNING ) {
            appCtxW->lastConnInitTime = TimeElapsedIoVentCtx (engCtx->iovCtx);
        }
    }

    while (1) {
        MsgIoProcess (engCtx->chanId);

        ProcessIoVent (engCtx->iovCtx);

        if ( (MsgIoTimeElapsed (engCtx->chanId) - engCtx->lastTickTime) >= 1 ) {
            Engine_tick (engCtx);
        }

        int appRunning = 0;
        for (int i = 0; i < engCtx->appCount; i++) {
            AppCtxW_t* appCtxW = &engCtx->appCtxWArr[i];
            if ( appCtxW->appStatus == APP_STATUS_RUNNING ) {
                if (engCtx->isMinTick) {
                    (*appCtxW->appMethods.OnMinTick)(appCtxW->appCtx);
                }
                if ( (*appCtxW->appMethods.OnContinue)(appCtxW->appCtx) ) {
                    double timeDelta = TimeElapsedIoVentCtx (engCtx->iovCtx) 
                                                - appCtxW->lastConnInitTime;
                    int newConnCount = timeDelta * appCtxW->connPerSec;
                    (*appCtxW->appMethods.OnAppLoop)(appCtxW->appCtx
                                                            , newConnCount);
                    appRunning = 1;
                } else {
                    appCtxW->appStatus = APP_STATUS_EXIT;
                }
            }
        }
        engCtx->isMinTick = 0;

        if (appRunning == 0) {
            break;
        }
    }

    for (int i = 0; i < engCtx->appCount; i++) {
        AppCtxW_t* appCtxW = &engCtx->appCtxWArr[i];
        if ( appCtxW->appStatus >= APP_STATUS_EXIT ) {
            (*appCtxW->appMethods.OnAppExit)(appCtxW->appCtx);
        }
    }

    return status;
}

int main(int argc, char** argv) {

    signal(SIGPIPE, SIG_IGN);

    EngCtx_t* engCtx = CreateStruct0 (EngCtx_t);

    engCtx->nAdminIp = argv[1];
    engCtx->nAdminPort = atoi(argv[2]);
    engCtx->testCfgId = argv[3];
    engCtx->testCfgSelect = argv[4];
    engCtx->testRunId = argv[5];
    
    if ( nAdmin_channel_setup (engCtx) ) {
        exit (-1); //???
    }

    if ( App_ctx_setup (engCtx) ) {
        exit (-1); //???
    }

    if ( IoVent_ctx_setup (engCtx) ) {
        exit (-1); //??? 
    }

    if ( App_library_init (engCtx) ) {
        exit (-1); //???
    }

    Engine_loop (engCtx);
}

