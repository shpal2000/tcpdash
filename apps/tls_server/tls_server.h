#ifndef __TLS_SERVER_APP_H
#define __TLS_SERVER_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TlsServerStats {
    SockStats_t connStats;
} TlsServerStats_t;

typedef struct TlsServerGroup {
    SockAddr_t serverAddr;
    uint64_t csDataLen;
    uint64_t scDataLen;
    TlsServerStats_t cStats;
} TlsServerGroup_t;

typedef struct TlsServerI {
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint32_t csGroupCount;
    TlsServerGroup_t* csGroupArr;
    TlsServerStats_t gStats;
    int connLifetimeSec;
} TlsServerI_t;

typedef struct TlsServerCtx {
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
    TlsServerI_t* appI; 
} TlsServerCtx_t;

typedef struct TlsServerConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TlsServerConn_t;

typedef struct TlsServerSession {
    TlsServerCtx_t* appCtx;
    TlsServerConn_t tcpConn;
} TlsServerSession_t;

#endif