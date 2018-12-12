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

typedef struct TcpProxyConn {
    IoVentConn_t* iovConn;
    RwBuff_t* readBuff;
    RwBuff_t* writeBuff;
    Pool_t writeQ;
} TcpProxyConn_t;

typedef struct TcpProxySession {
    TcpProxyAppCtx_t* appCtx;
    TcpProxyConn_t aConn;
    TcpProxyConn_t iConn;
} TcpProxySession_t;


#endif //__APP__MAIN__

#endif
