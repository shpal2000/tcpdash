#include "app_engine.h"


APP_DECLARE_METHODS (TlsClient);

int App_get_methods (AppCtxW_t* appCtxW) {

    APP_GET_METHODS (appCtxW, TlsClient);

    return -1; 
}

