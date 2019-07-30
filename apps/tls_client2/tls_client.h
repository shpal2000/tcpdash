#ifndef __TLS_CLIENT_APP_H
#define __TLS_CLIENT_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TlsClientStats {
    SockStats_t connStats;
} TlsClientStats_t;

typedef struct TlsClientGrp {
    char* srvIp;
    uint32_t srvPort;

    SockAddr_t srvAddr;

    uint32_t cAddrCount;
    SockAddrCtx_t* cAddrArr;
    uint32_t cAddrNextIndex;

    TlsClientStats_t cStats;
} TlsClientGrp_t;

typedef struct TlsClientCtx {
    __APPCTX_BASE__;

    uint32_t connPerSec;
    uint32_t maxActSess;
    uint32_t maxErrSess;
    uint64_t maxSess;
    uint32_t csGrpCount;
    TlsClientGrp_t* csGrpArr;

    uint32_t nextCsGrpIndex;

    char commonReadBuff[COMMON_READBUFF_MAXLEN];
    char commonWriteBuff[COMMON_WRITEBUFF_MAXLEN];
} TlsClientCtx_t;

typedef struct TlsClientConn {
    __APPCONN_BASE__;

    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;

} TlsClientConn_t;

typedef struct TlsClientSess {
    __APPSESS_BASE__;

    TlsClientConn_t* cConn;
} TlsClientSess_t;

#endif