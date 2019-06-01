#include "app_engine.h"
#include "app_methods.h"

int App_get_methods (AppCtxW_t* appCtxW) {

    int status = 0;

    char* appName = appCtxW->appName;
    AppMethods_t* appMethods = &appCtxW->appMethods; 

    if ( strcmp (appName, "TlsClient") == 0 ) {
        TlsClient_get_methods (appMethods);
    } else if ( strcmp (appName, "TlsServer") == 0 ) {
        // TlsServer_get_methods (appMethods);
    } else {
        status = -1;
    }

    return status; 
}

int App_parse_config (AppCtxW_t* appCtxW, JObject* appJ) {

    int status = 0;

    JGET_MEMBER_INT (appJ, "connPerSec", &appCtxW->connPerSec);
    JGET_MEMBER_INT (appJ, "maxActSess", &appCtxW->maxActSess);
    JGET_MEMBER_INT (appJ, "maxErrSess", &appCtxW->maxErrSess);
    JGET_MEMBER_INT (appJ, "maxSess", &appCtxW->maxSess);

    appCtxW->appCtx 
            = (*appCtxW->appMethods.OnAppInit) (appJ, appCtxW->appIndex);
    
    if (appCtxW->appCtx == NULL) {
        status = -1;
    }

    return status;
}
