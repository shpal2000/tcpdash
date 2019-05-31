#include "app_engine.h"

void nAdmin_channel_open (MsgIoChannelId_t chanId) {

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

void nAdmin_channel_error (MsgIoChannelId_t chanId) {

    EngCtx_t* engCtx = (EngCtx_t*) MsgIoGetCtx (chanId);

    engCtx->chanErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

void nAdmin_channel_recv (MsgIoChannelId_t chanId) {

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

void nAdmin_channel_sent (MsgIoChannelId_t chanId) {
}

int nAdmin_channel_setup(EngCtx_t* engCtx) {

    int status = -1;
    MsgIoMethods_t mioMethods;

    SetSockAddress0 (&engCtx->nLocalAddr, 0); 
    SetSockAddress (&engCtx->nAdminAddr, engCtx->nAdminIp, engCtx->nAdminPort);

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
                engCtx->chanErr = N_ADMIN_CHANNEL_ERROR_GET_CONFIG;
            }
            if (engCtx->chanErr) {
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