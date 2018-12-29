#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"
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
    uint64_t csDataLen;
    uint64_t scDataLen;
    enum ConnCloseMethod cCloseMethod;
    enum ConnCloseMethod sCloseMethod;
    enum ConnCloseType csCloseType;
    uint32_t csWeight;
    TcpClientServerStats_t cStats;
} TcpClientServerGroup_t;

typedef struct TcpClientServerI {
    uint32_t isRunning;
    uint32_t maxEvents;
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpClientServerGroup_t* csGroupArr;
    uint32_t nextCsGroupIndex;
    TcpClientServerStats_t gStats;
} TcpClientServerI_t;

void TcpClientRun(TcpClientServerI_t* appI);

#ifdef __APP__MAIN__

#define COMMON_READBUFF_MAXLEN      2048
#define COMMON_WRITEBUFF_MAXLEN     1048576

// --------TcpClient---------//

typedef struct TcpClientAppCtx {
    Pool_t* freeSessionPool;
    Pool_t activeSessionPool;
    char commonReadBuff[COMMON_READBUFF_MAXLEN];
    char commonWriteBuff[COMMON_WRITEBUFF_MAXLEN];
    TcpClientServerI_t* appI; 
} TcpClientAppCtx_t;

typedef struct TcpClientConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TcpClientConn_t;

typedef struct TcpClientSession {
    TcpClientAppCtx_t* appCtx;
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
#endif
#endif