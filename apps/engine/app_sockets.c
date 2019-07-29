#include "app_engine.h"

// int App_conn_new (AppCtx_t* appCtx
//                     , AppConn_t* appConn
//                     , SockAddr_t* localAddr
//                     , SockAddr_t* remoteAddr) {
//     AppConnBase_t* appConnBase = (AppConnBase_t*) appConn;
//     appConnBase->appCtx = appCtx;
//     // appConnBase->ioVentConn = ???; 
//     // return NewConnection ();
//     return 0;
// }

int App_conn_session_child (AppCtx_t* appCtx
                    , AppSess_t* appSess
                    , AppConn_t** appConn
                    , SockAddr_t* localAddr
                    , SockAddr_t* remoteAddr) {
    int isErr = -1;
    // GetConnection(appSess, appConn);
    // if (*appConn) {
    //     if ( NewConnection (...) ) {
    //         // stats
    //         // handle error ??? free resources
    //         FreeConnetion(*appConn);
    //     } else {
    //         isErr = 0;    
    //     }
    // } else {
    //     //stats no free connection resource
    // }
    return isErr;
}

int App_conn_session_new (AppCtx_t* appCtx
                    , AppSess_t** appSess 
                    , AppConn_t** appConn
                    , SockAddr_t* localAddr
                    , SockAddr_t* remoteAddr) {
    int isErr = -1;
    // GetSession(appCtx, appSess);
    // if (*appSess) {
    //     if ( App_conn_session_child (appCtx, *appSess, appConn, ...) ) {
    //         // stats
    //         // handle error ??? free resources
    //         FreeSession (*appSess);
    //     } else {
    //         isErr = 0;    
    //     }
    // } else {
    //     // stats no free session resurce
    // }

    return isErr;
}

