#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"
#include "platform/common.h"

typedef struct TcpCSStats {
    SockStats_t connStats;
} TcpCSStats_t;

typedef struct TcpCSGroup {
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
    TcpCSStats_t cStats;
} TcpCSGroup_t;

typedef struct TcpCSI {
    uint32_t isRunning;
    uint32_t maxEvents;
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpCSGroup_t* csGroupArr;
    uint32_t nextCsGroupIndex;
    TcpCSStats_t gStats;
} TcpCSI_t;

void TcpClientRun(TcpCSI_t* appI);
void TcpServerRun(TcpCSI_t* appI);

#ifdef __APP__MAIN__

#define COMMON_READBUFF_MAXLEN      2048
#define COMMON_WRITEBUFF_MAXLEN     1048576

// --------TcpClient---------//

typedef struct TcpCSAppCtx {
    Pool_t* freeSessionPool;
    Pool_t activeSessionPool;
    char commonReadBuff[COMMON_READBUFF_MAXLEN];
    char commonWriteBuff[COMMON_WRITEBUFF_MAXLEN];
    TcpCSI_t* appI; 
} TcpCSAppCtx_t;

typedef struct TcpCSConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TcpCSConn_t;

typedef struct TcpCSSession {
    TcpCSAppCtx_t* appCtx;
    TcpCSConn_t tcpConn;
} TcpCSSession_t;

#endif
#endif