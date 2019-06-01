#include "app_engine.h"
#include "app_methods.h"


static void OnEstablish (struct IoVentConn* iovConn) {

    AppCtxW_t* appCtxW = (AppCtxW_t*) iovConn->cInfo.appCtx;
    AppCtx_t* appCtx = appCtxW->appCtx; 
    AppSess_t* appSess = (AppSess_t*) iovConn->cInfo.sessionData;

    if ( IsConnErr (iovConn) ) {
        (*appCtxW->appMethods.OnEstablishErr) (iovConn, appSess, appCtx);
    } else {
        (*appCtxW->appMethods.OnEstablish) (iovConn, appSess, appCtx);
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

}

static void OnWriteNext (struct IoVentConn* iovConn) {

}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

}

static void OnCleanup (struct IoVentConn* iovConn) {

}

static int App_ctx_setup (EngCtx_t* engCtx) {
    
    int status = 0;
    engCtx->appCount = 0;

    JNode* cfgNode;
    JObject* cfgObj;

    JGET_ROOT_NODE (engCtx->cfgData, &cfgNode, &cfgObj);
    if (cfgNode) {
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
                    appCtxW->appIndex = appIndex;
                    appCtxW->appStatus = APP_STATUS_INIT;
                    JGET_MEMBER_STR (appJ, "appName", &appCtxW->appName);
                    if ( App_get_methods (appCtxW) ) {
                        status = -1; //??? log
                        break;
                    }
                    if ( App_parse_config (appCtxW, appJ) ){
                        status = -1; //??? log
                        break;
                    }
                    appCtxW->appStatus = APP_STATUS_RUNNING;
                }
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
    iovOptions.maxActiveConnections = APP_ENGINE_MAX_ACTIVE_CONNECTION;
    iovOptions.maxErrorConnections = APP_ENGINE_MAX_ERROR_CONNECTION;
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

static void Engine_post_stats (EngCtx_t* engCtx) {

    // char statsString[256];

    engCtx->lastStatsPost = MsgIoTimeElapsed (engCtx->chanId);

    // statsString = 
    MsgIoSend (engCtx->chanId
                , "1234"
                , strlen("1234") );
}

static int Engine_loop (EngCtx_t* engCtx) {

    int status = 0;    

    while (1) {
        MsgIoProcess (engCtx->chanId);

        ProcessIoVent (engCtx->iovCtx);

        if ( (MsgIoTimeElapsed (engCtx->chanId) - engCtx->lastStatsPost) >= 2 ) { 
            Engine_post_stats (engCtx);
        }

        int appRunning = 0;
        for (int i = 0; i < engCtx->appCount; i++) {
            AppCtxW_t* appCtxW = &engCtx->appCtxWArr[i];
            if ( appCtxW->appStatus == APP_STATUS_RUNNING ) {
                int loopStatus = (*appCtxW->appMethods.OnAppLoop)(appCtxW->appCtx);
                if (loopStatus){
                    if (loopStatus > 0) {
                        appCtxW->appStatus = APP_STATUS_EXIT;
                    } else {
                        appCtxW->appStatus = APP_STATUS_EXIT_WITH_ERROR;
                    }
                } else {
                    appRunning = 1;
                }
            }
        }

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
    engCtx->testRunId = argv[4];
    
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

