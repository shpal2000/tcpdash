#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#include "engine.h"

static void nAdmin_channel_open (MsgIoChannelId_t chanId) {

    EngCtx_t* engCtx = (EngCtx_t*) MsgIoGetCtx (chanId);

    if ( engCtx->chanState == N_ADMIN_CHANNEL_STATE_INIT ) {
        engCtx->chanState = N_ADMIN_CHANNEL_STATE_GET_CONFIG;
        MsgIoSend ( chanId
            , engCtx->testCfgId
            , strlen( engCtx->testCfgId) );
    } else if ( engCtx->chanState == N_ADMIN_CHANNEL_STATE_REINIT ) {
        engCtx->chanState = N_ADMIN_CHANNEL_STATE_ESTABLISHED;
    }
}

static void nAdmin_channel_error (MsgIoChannelId_t chanId) {

    EngCtx_t* engCtx = (EngCtx_t*) MsgIoGetCtx (chanId);

    engCtx->chanErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

static void nAdmin_channel_recv (MsgIoChannelId_t chanId) {

    EngCtx_t* engCtx = (EngCtx_t*) MsgIoGetCtx (chanId);

    if ( engCtx->chanState == N_ADMIN_CHANNEL_STATE_GET_CONFIG ) {
        char* cfgData;
        int cfgLen;
        MsgIoRecv (chanId, &cfgData, &cfgLen);

        engCtx->cfgData = AllocMem ( cfgLen + 1 );
        if (engCtx->cfgData == NULL) {
            engCtx->chanErr = N_ADMIN_CHANNEL_ERROR_MEM_CONFIG;
        } else {
            memcpy (engCtx->cfgData, cfgData, cfgLen);
            engCtx->cfgData[cfgLen] = '\0';
            engCtx->chanState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;
        }
    } else {
        //??? other runtime data
    }
}

static void nAdmin_channel_sent (MsgIoChannelId_t chanId) {
}

static int nAdmin_channel_setup(EngCtx_t* engCtx) {

    int status = -1;
    MsgIoMethods_t mioMethods;

    mioMethods.OnOpen = &nAdmin_channel_open;
    mioMethods.OnError = &nAdmin_channel_error;
    mioMethods.OnMsgRecv = &nAdmin_channel_recv;
    mioMethods.OnMsgSent = &nAdmin_channel_sent;

    engCtx->chanId =  MsgIoNew (&engCtx->nLocalAddr
                        , &engCtx->nAdminAddr
                        , &mioMethods
                        , engCtx);

    if ( engCtx->chanId ) {
        engCtx->chanState = N_ADMIN_CHANNEL_STATE_INIT;
        while ( 1 ) {
            MsgIoProcess (engCtx->chanId);
            if ( MsgIoTimeElapsed (engCtx->chanId) > N_ADMIN_GET_CONFIG_MAX_TIME ) {
                engCtx->chanlErr = N_ADMIN_CHANNEL_ERROR_GET_CONFIG;
            }
            if (engCtx->chanlErr) {
                break;
            }
            if (engCtx->chanState == N_ADMIN_CHANNEL_STATE_RECV_CONFIG) {
                engCtx->chanState = N_ADMIN_CHANNEL_STATE_ESTABLISHED;
                status = 0;
                break;
            }
        }
    }
    
    return status;
}

static int App_get_methods (AppCtx_t* appCtx) {

    int status = 0;

    char* appName = appCtx->appName;
    AppMethods_t* appMethods = &appCtx->appMethods; 

    if ( strcmp (appName, "TlsClient") == 0 ) {
        TlsClient_get_methods (appMethods);
    } else if ( strcmp (appName, "TlsServer") == 0 ) {
        TlsServer_get_methods (appMethods);
    } else {
        status = -1;
    }

    return status; 
}

static int App_ctx_setup (EngCtx_t* engCtx) {
    
    int status = 0;

    engCtx->appCount = 0;
    //todo parse config and find app count

    engCtx->appCtxArr = CreateArray0 (AppCtx_t, engCtx->appCount);
    if (engCtx->appCtxArr == NULL) {
        status = -1; //??? log
    } else {
        for (int i = 0; i < engCtx->appCount; i++) {
            AppCtx_t* appCtx = engCtx->appCtxArr[i]; 
            if ( App_get_methods (appCtx) 
                    || (*appCtx->appMethods.AppInit)(appCtx)) {
                status = -1; //??? log
                break;
            }
        }
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
    iovMethods.OnStatus = &OnStatus;
    iovMethods.OnContinue = &OnContinue;

    IoVentOptions_t iovOptions;
    iovOptions.maxActiveConnections = 1;
    ovOptions.maxErrorConnections = 1;
    iovOptions.maxEvents = 0;
    iovOptions.eventPTO = DEFAULT_MAX_POLL_TIMEOUT;

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

    return 0
}

static void Engine_post_stats (EngCtx_t* engCtx) {

    char statsString[256];

    engCtx->lastStatsPost = MsgIoTimeElapsed (engCtx->chanId);

    // statsString = 
    MsgIoSend (engCtx->chanId
                , statsString
                , strlen(statsString) )
}

static int Engine_loop (EngCtx_t* engCtx) {

    int status = 0;    

    while (1) {
        MsgIoProcess (engCtx->chanId);

        ProcessIoVent (engCtx->iovCtx);

        if ( (MsgIoTimeElapsed (engCtx->chanId) - engCtx->lastStatsPost) >= 2 ) { 
            Engine_post_stats (engCtx);
        }

        for (int i = 0; i < engCtx->appCount; i++) {
            AppCtx_t* appCtx = engCtx->appCtxArr[i];
            (*appCtx->appMethods.OnRunLoop)(appCtx);
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

int Conn_new (AppCtx_t* appCtx
                        , AppSess_t* appSess
                        , SockAddr_t* localAddr
                        , SockAddr_t* remoteAddr
                        , int connLifetime
                        ) {

    return NewConnection ();
}