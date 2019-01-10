#ifndef __TCP_PROXY_APP_H
#define __TCP_PROXY_APP_H

#include "apps/common.h"

#define TP_CONN_TYPE_ACCEPTED       1
#define TP_CONN_TYPE_INITIATED      2

typedef struct TcpProxyAppCtx {
    Pool_t* freeSessionPool;
    Pool_t* freeBuffPool;
    TcpProxyAppI_t* appI; 
    Pool_t activeSessionPool;
} TcpProxyAppCtx_t;

#define RW_MAX_BUFF_LEN 2048
typedef struct RwBuff {
    int buffLen;
    int buffOffset;
    int dataLen;
    char dataBuff[RW_MAX_BUFF_LEN];
} RwBuff_t;

typedef struct TcpProxyAppConn {
    IoVentConn_t* iovConn;
    RwBuff_t* readBuff;
    RwBuff_t* writeBuff;
    Pool_t writeQ;
    int isActive;
} TcpProxyAppConn_t;

typedef struct TcpProxyAppSession {
    TcpProxyAppCtx_t* appCtx;
    TcpProxyAppConn_t aConn;
    TcpProxyAppConn_t iConn;
} TcpProxyAppSession_t;


#endif 

