#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"

typedef struct MsgIoCtx {
    Pool_t* freeSessionPool;
    Pool_t activeSessionPool;
    TcpCsAppI_t* appI; 
} MsgIoCtx_t;

typedef struct MsgIoConn {
    IoVentConn_t* iovConn;
} MsgIoConn_t;

typedef struct MsgIoSession {
    MsgIoCtx_t* appCtx;
    MsgIoConn_t tcpConn;
} MsgIoSession_t;

#endif

