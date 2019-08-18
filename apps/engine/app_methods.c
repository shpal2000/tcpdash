#include "app_engine.h"


APP_DECLARE_METHODS (TlsClient);
APP_DECLARE_METHODS (TlsServer);

int App_get_methods (AppCtxW_t* appCtxW) {
    APP_GET_METHODS (appCtxW, TlsClient);
    APP_GET_METHODS (appCtxW, TlsServer);
    return -1; 
}

