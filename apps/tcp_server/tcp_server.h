#ifndef __TCP_SERVER_APP_H
#define __TCP_SERVER_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TcpServerStats {
    SockStats_t connStats;
} TcpServerStats_t;

typedef struct TcpServerGroup {
    SockAddr_t serverAddr;
    uint64_t csDataLen;
    uint64_t scDataLen;
    enum ConnCloseMethod sCloseMethod;
    enum ConnCloseType csCloseType;
    TcpServerStats_t cStats;
} TcpServerGroup_t;

typedef struct TcpServerI {
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint32_t csGroupCount;
    TcpServerGroup_t* csGroupArr;
    TcpServerStats_t gStats;
} TcpServerI_t;

typedef struct TcpServerCtx {
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
    TcpServerI_t* appI; 
} TcpServerCtx_t;

typedef struct TcpServerConn {
    IoVentConn_t* iovConn;
    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;
} TcpServerConn_t;

typedef struct TcpServerSession {
    TcpServerCtx_t* appCtx;
    TcpServerConn_t tcpConn;
} TcpServerSession_t;

#endif