#ifndef __TCP_PROXY_APP_H
#define __TCP_PROXY_APP_H

#include "platform/common.h"

typedef struct TcpProxyStats {
    SockStats_t connStats;

    //add application specific stats
} TcpProxyStats_t;

typedef struct TcpProxyServer {
    SockAddr_t serverAddrP;
    SockAddr_t serverAddrL;
    SockAddr_t serverAddrR;
    TcpProxyStats_t cStats;
} TcpProxyServer_t;

typedef struct TcpProxyI {

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;

    uint32_t serverCount;
    TcpProxyServer_t* serverArr;

    TcpProxyStats_t gStats;
} TcpProxyI_t;

void TcpProxyRun(TcpProxyI_t* appIface);

TcpProxyI_t* CreateTcpProxyInterface(int serverCount);

void DeleteTcpProxyInterface(TcpProxyI_t* iFace);

void DumpTcpProxyStats(TcpProxyStats_t* appConnStats);

#ifdef __APP__MAIN__

typedef struct TcpProxyAppCtx {
    Pool_t* freeSessionPool;
    Pool_t* freeBuffPool;
    TcpProxyI_t* appI; 
} TcpProxyAppCtx_t;

#define RW_MAX_BUFF_LEN 2048
typedef struct RwBuff {
    int buffLen;
    int buffOffset;
    int dataLen;
    char dataBuff[RW_MAX_BUFF_LEN];
} RwBuff_t;

typedef struct TcpProxySession {
    IoVentConn_t* acceptedConn;
    IoVentConn_t* initiatedConn;
    TcpProxyAppCtx_t* appCtx;
    Pool_t aConnWriteBuffQ;
    Pool_t iConnWriteBuffQ;
    RwBuff_t* aConnReadBuff;
    RwBuff_t* iConnReadBuff;
    RwBuff_t* aConnWriteBuff;
    RwBuff_t* iConnWriteBuff;
} TcpProxySession_t;


#endif //__APP__MAIN__

#endif
