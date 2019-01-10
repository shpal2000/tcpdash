#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"

#define COMMON_READBUFF_MAXLEN      2048
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TcpCsAppCtx {
    Pool_t* freeSessionPool;
    Pool_t activeSessionPool;
    char commonReadBuff[COMMON_READBUFF_MAXLEN];
    char commonWriteBuff[COMMON_WRITEBUFF_MAXLEN];
    TcpCsAppI_t* appI; 
} TcpCsAppCtx_t;

typedef struct TcpCsAppConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TcpCsAppConn_t;

typedef struct TcpCsSession {
    TcpCsAppCtx_t* appCtx;
    TcpCsAppConn_t tcpConn;
} TcpCsSession_t;

#endif