#ifndef __TLS_CLIENT_APP_H
#define __TLS_CLIENT_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TlsClientStats {
    SockStats_t connStats;
} TlsClientStats_t;

typedef struct TlsClientGrp {
    const char* srvIp;
    uint32_t srvPort;

    SockAddr_t srvAddr;

    uint32_t cAddrCount;
    SockAddrCtx_t* cAddrArr;
    uint32_t cAddrNextIndex;

    TlsClientStats_t cStats;
} TlsClientGrp_t;

typedef struct TlsClientCtx {

    uint32_t connPerSec;
    uint32_t maxActSess;
    uint32_t maxErrSess;
    uint64_t maxSess;
    uint32_t csGrpCount;
    TlsClientGrp_t* csGrpArr;

    uint32_t nextCsGrpIndex;

    Pool_t* freeSessPool;
    Pool_t actSessPool;

    char commonReadBuff[COMMON_READBUFF_MAXLEN];
    char commonWriteBuff[COMMON_WRITEBUFF_MAXLEN];
} TlsClientCtx_t;

typedef struct TlsClientConn {
    AppConn_t* appConn;

    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;

    struct TlsClientSess* pSess; 
} TlsClientConn_t;

typedef struct TlsClientSess {
    TlsClientCtx_t* appCtx;
    TlsClientConn_t cConn;
} TlsClientSess_t;

#endif