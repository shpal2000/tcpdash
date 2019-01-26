#ifndef __TCP_CLIENT_APP_H
#define __TCP_CLIENT_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TcpClientStats {
    SockStats_t connStats;
} TcpClientStats_t;

typedef struct TcpClientGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;
    SockAddr_t serverAddr;
    uint64_t csDataLen;
    uint64_t scDataLen;
    enum ConnCloseMethod cCloseMethod;
    enum ConnCloseType csCloseType;
    uint32_t csWeight;
    TcpClientStats_t cStats;
} TcpClientGroup_t;

typedef struct TcpClientI {
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpClientGroup_t* csGroupArr;
    uint32_t nextCsGroupIndex;
    TcpClientStats_t gStats;
} TcpClientI_t;

typedef struct TcpClientCtx {
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
    TcpClientI_t* appI; 
} TcpClientCtx_t;

typedef struct TcpClientConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TcpClientConn_t;

typedef struct TcpClientSession {
    TcpClientCtx_t* appCtx;
    TcpClientConn_t tcpConn;
} TcpClientSession_t;

#endif