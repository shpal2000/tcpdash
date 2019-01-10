#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "platform/common.h"
#include "apps/common.h"

typedef struct TcpCsAppStats {
    SockStats_t connStats;
} TcpCsAppStats_t;

typedef struct TcpCsAppGroup {
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
    TcpCsAppStats_t cStats;
} TcpCsAppGroup_t;

typedef struct TcpCsAppI {
    uint32_t isRunning;
    uint32_t maxEvents;
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpCsAppGroup_t* csGroupArr;
    uint32_t nextCsGroupIndex;
    TcpCsAppStats_t gStats;
} TcpCsAppI_t;

void TcpClientRun(TcpCsAppI_t* appI);
void DumpTcpClientStats(TcpCsAppStats_t* appConnStats);

void TcpServerRun(TcpCsAppI_t* appI);
void DumpTcpServerStats(TcpCsAppStats_t* appConnStats);

#ifdef __APP__MAIN__

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
#endif