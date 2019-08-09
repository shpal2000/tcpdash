#ifndef __TLS_SERVER_APP_H
#define __TLS_SERVER_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TlsServerStats {
    __APPSTATS_BASE__;

} TlsServerStats_t;

typedef struct TlsServerGrp {
    TlsServerStats_t grpStats;

    SockStats_t* statsArr[2];
    int statsCount;

    char* srvIp;
    uint32_t srvPort;
    SockAddr_t srvAddr;
    int srvStarted;

    uint64_t csDataLen;
    uint64_t scDataLen;
    uint64_t csStartTlsLen;
    uint64_t scStartTlsLen;    

    SSL_CTX* sslCtx;
} TlsServerGrp_t;

typedef struct TlsServerCtx {
    __APPCTX_BASE__;

    TlsServerStats_t allStats;

    uint32_t maxActSess;
    uint32_t maxErrSess;
    uint32_t csGrpCount;
    TlsServerGrp_t* csGrpArr;

    char appRdBuff[COMMON_READBUFF_MAXLEN];
    char appWrBuff[COMMON_WRITEBUFF_MAXLEN];
} TlsServerCtx_t;

typedef struct TlsServerConn {
    __APPCONN_BASE__;

    uint64_t bytesRead;
    uint64_t bytesWritten;
    uint32_t writeBuffOffset;

    uint32_t isSslInit; 

    TlsServerGrp_t* csGrp;
} TlsServerConn_t;

typedef struct TlsServerSess {
    __APPSESS_BASE__;

    TlsServerConn_t* cConn;
} TlsServerSess_t;

#endif