#include "app_engine.h"

int App_conn_new (AppCtx_t* appCtx
                    , AppConn_t* appConn
                    , SockAddr_t* localAddr
                    , SockAddr_t* remoteAddr) {
    AppConnBase_t* appConnBase = (AppConnBase_t*) appConn;
    appConnBase->appCtx = appCtx;
    // appConnBase->ioVentConn = ???; 
    // return NewConnection ();
    return 0;
}