#ifndef __TCP_CLIENT_SERVER_APP_H

#include "app/common.h"
#include "platform/common.h"

typedef struct TcpClientServerStats {
    SockStats_t connStats;
} TcpClientServerStats_t;

typedef struct TcpClientServerGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;
    SockAddr_t serverAddr;
    uint32_t csDataLen;
    uint32_t scDataLen;
    enum ConnCloseMethod cCloseMethod;
    enum ConnCloseMethod sCloseMethod;
    enum ConnCloseType csCloseType;
    uint32_t grWeight;
    TcpClientServerStats_t grStats;
} TcpClientServerGroup_t;

typedef struct TcpClientServerI {
    uint32_t isRunning;
    uint32_t maxEvents;
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpProxyServer_t* csGroupArr;
    TcpClientServerStats_t glStats;
} TcpClientServerI_t;

void TcpClientRun(TcpClientServerI_t* appIface);

TcpClientServerI_t* CreateTcpClientServerInterface(int csGroupCount);

void DeleteTcpClientServerInterface(TcpClientServerI_t* iFace);

void DumpTcpClientStats(TcpClientServerI_t* appConnStats);

#ifdef __APP__MAIN__

#define RW_MAX_BUFF_LEN 2048
typedef struct RwBuff {
    int buffLen;
    int buffOffset;
    int dataLen;
    char dataBuff[RW_MAX_BUFF_LEN];
} RwBuff_t;
#endif

// --------TcpClient---------//

typedef struct TcpClientAppCtx {
    Pool_t* freeSessionPool;
    Pool_t* freeBuffPool;
    Pool_t activeSessionPool;
    TcpClientServerI_t* appI; 
} TcpClientAppCtx_t;

typedef struct TcpClientConn {
    IoVentConn_t* iovConn;
    RwBuff_t* readBuff;
    RwBuff_t* writeBuff;
} TcpClientConn_t;

typedef struct TcpClientSession {
    TcpClientServerAppCtx_t* appCtx;
    TcpClientConn_t tcpConn;
} TcpClientSession_t;

// --------TcpServer---------//

// typedef struct TcpServerAppCtx {
//     Pool_t* freeSessionPool;
//     Pool_t* freeBuffPool;
//     Pool_t activeSessionPool;
//     TcpClientServerI_t* appI; 
// } TcpServerAppCtx_t;

// typedef struct TcpServerConn {
//     IoVentConn_t* iovConn;
//     RwBuff_t* readBuff;
//     RwBuff_t* writeBuff;
// } TcpServerConn_t;

// typedef struct TcpServerSession {
//     TcpServerAppCtx* appCtx;
//     TcpServerConn_t aConn;
// } TcpServerSession_t;

#define __TCP_CLIENT_SERVER_APP_H
#endif