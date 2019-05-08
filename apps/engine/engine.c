#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#include "engine.h"

static void App_channel_open (MsgIoChannelId_t chanId) {

    AppCtx_t* appCtx = (AppCtx_t*) MsgIoGetCtx (chanId);

    if ( appCtx->chanState == N_ADMIN_CHANNEL_STATE_INIT ) {
        appCtx->chanState = N_ADMIN_CHANNEL_STATE_GET_CONFIG;
        MsgIoSend ( chanId
            , appCtx->testCfgId
            , strlen( appCtx->testCfgId) );
    } else if ( appCtx->chanState == N_ADMIN_CHANNEL_STATE_REINIT ) {
        appCtx->chanState = N_ADMIN_CHANNEL_STATE_ESTABLISHED;
    }
}

static void App_channel_error (MsgIoChannelId_t chanId) {

    AppCtx_t* appCtx = (AppCtx_t*) MsgIoGetCtx (chanId);

    appCtx->chanErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

static void App_channel_recv (MsgIoChannelId_t chanId) {

    AppCtx_t* appCtx = (AppCtx_t*) MsgIoGetCtx (chanId);

    if ( appCtx->chanState == N_ADMIN_CHANNEL_STATE_GET_CONFIG ) {
        char* cfgData;
        int cfgLen;
        MsgIoRecv (chanId, &cfgData, &cfgLen);
        cfgData [cfgLen] = '\0'

        void* appCfg  = appCtx->appMethods.OnParseCfg (cfgData);
        if (appCfg) {
            appCtx->chanState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;
        } else {
            appCtx->chanErr = N_ADMIN_CHANNEL_ERROR_PARSE_CONFIG;
        }
    } else {
        //??? other runtime data
    }
}

static void App_channel_sent (MsgIoChannelId_t chanId) {
}

static int App_channel_setup(AppCtx_t* appCtx) {

    int status = -1;
    MsgIoMethods_t mioMethods;

    mioMethods.OnOpen = &App_channel_open;
    mioMethods.OnError = &App_channel_error;
    mioMethods.OnMsgRecv = &App_channel_recv;
    mioMethods.OnMsgSent = &App_channel_sent;

    appCtx->chanId =  MsgIoNew (&appCtx->nLocalAddr
                        , &appCtx->nAdminAddr
                        , &mioMethods
                        , appCtx);

    if ( appCtx->chanId ) {
        appCtx->chanState = N_ADMIN_CHANNEL_STATE_INIT;
        while ( 1 ) {
            MsgIoProcess (appCtx->chanId);
            if ( MsgIoTimeElapsed (appCtx->chanId) > N_ADMIN_GET_CONFIG_MAX_TIME ) {
                appCtx->chanlErr = N_ADMIN_CHANNEL_ERROR_GET_CONFIG;
            }
            if (appCtx->chanlErr) {
                break;
            }
            if (appCtx->chanState == N_ADMIN_CHANNEL_STATE_RECV_CONFIG) {
                appCtx->chanState = N_ADMIN_CHANNEL_STATE_ESTABLISHED;
                status = 0;
                break;
            }
        }
    }
    
    return status;
}

static int App_library_init () {

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    return 0
}

static int App_main(char* nAdminIp
        , int nAdminPort
        , char* testCfgId
        , char* testRunId) {

    signal(SIGPIPE, SIG_IGN);

    AppCtx_t* appCtx = CreateStruct0 (AppCtx_t);

    appCtx->nAdminIp = nAdminIp;
    appCtx->nAdminPort = nAdminPort;
    appCtx->testCfgId = testCfgId;
    appCtx->testRunId = testRunId;
    
    if ( App_channel_setup (appCtx) ) {
        exit (-1); //???
    }

    if ( App_library_init () ) {
        exit (-1); //???
    }

    while (1) {
    }
}

int main(int argc, char** argv) {
    return App_main (argv[1]
                    , atoi(argv[2])
                    , argv[3]
                    , argv[4]); 
}