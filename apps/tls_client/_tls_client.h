#ifndef __TLS_CLIENT_APP_H
#define __TLS_CLIENT_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TlsClientStats {
    SockStats_t connStats;
} TlsClientStats_t;

typedef struct TlsClientGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;
    SockAddr_t serverAddr;
    uint64_t csDataLen;
    uint64_t scDataLen;
    uint64_t csStartTlsLen;
    uint64_t scStartTlsLen;
    TlsClientStats_t cStats;
} TlsClientGroup_t;

typedef struct TlsClientI {
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TlsClientGroup_t* csGroupArr;
    uint32_t nextCsGroupIndex;
    TlsClientStats_t gStats;
    int connLifetimeSec;
} TlsClientI_t;

typedef struct TlsClientCtx {
    const char* testId;

    Pool_t* freeSessionPool;
    Pool_t activeSessionPool;
    char commonReadBuff[COMMON_READBUFF_MAXLEN];
    char commonWriteBuff[COMMON_WRITEBUFF_MAXLEN];
    MsgIoChannelId_t nAdminChannelId;
    int nAdminChannelState;
    int nAdminChannelErr;
    char nAdminTestId[N_ADMIN_TEST_ID_MAX_LEN];
    SockAddr_t nAdminAddr;
    SockAddr_t nLocalAddr;
    IoVentCtx_t* iovCtx;
    TlsClientI_t* appI; 
} TlsClientCtx_t;

typedef struct TlsClientConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TlsClientConn_t;

typedef struct TlsClientSession {
    TlsClientCtx_t* appCtx;
    TlsClientConn_t tcpConn;
} TlsClientSession_t;

#endif