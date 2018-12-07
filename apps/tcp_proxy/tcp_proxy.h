#ifndef __TCP_PROXY_APP_H
#define __TCP_PROXY_APP_H

#include "platform/common.h"

typedef struct TcpProxyStats {
    SockStats_t connStats;

    //add application specific stats
} TcpProxyStats_t;

typedef struct TcpProxyServer {
    SockAddr_t serverAddr;
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

typedef struct TcpProxyCtx {

} TcpProxyCtx_t;

typedef struct TcpProxySession {
    IoVentConn_t* serverConn;
    IoVentConn_t* clientConn;
} TcpProxySession_t;

#endif //__APP__MAIN__

#endif
