#ifndef __TLS_CLIENT_APP_H
#define __TLS_CLIENT_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TlsClientStats {
    __APPSTATS_BASE__;

} TlsClientStats_t;

typedef struct TlsClientGrp {
    TlsClientStats_t grpStats;

    SockStats_t* statsArr[2];
    int statsCount;

    char* srvIp;
    uint32_t srvPort;
    SockAddr_t srvAddr;

    uint32_t cAddrCount;
    SockAddrCtx_t* cAddrArr;
    uint32_t cAddrNextIndex;

    uint64_t csDataLen;
    uint64_t scDataLen;
    uint64_t csStartTlsLen;
    uint64_t scStartTlsLen;    

    SSL_CTX* sslCtx;
} TlsClientGrp_t;

typedef struct TlsClientCtx {
    __APPCTX_BASE__;

    TlsClientStats_t allStats;

    uint32_t connPerSec;
    uint32_t maxActSess;
    uint32_t maxErrSess;
    uint64_t maxSess;
    uint32_t csGrpCount;
    TlsClientGrp_t* csGrpArr;

    uint32_t nextCsGrpIndex;

    char appRdBuff[COMMON_READBUFF_MAXLEN];
    char appWrBuff[COMMON_WRITEBUFF_MAXLEN];
} TlsClientCtx_t;

typedef struct TlsClientConn {
    __APPCONN_BASE__;

    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;

    uint32_t isSslInit; 

    TlsClientGrp_t* csGrp;
} TlsClientConn_t;

typedef struct TlsClientSess {
    __APPSESS_BASE__;

    TlsClientConn_t* cConn;
} TlsClientSess_t;

#endif