#ifndef __TCP_PROXY_APP_H
#define __TCP_PROXY_APP_H

#include "msg_io/msg_io.h"
#include "nadmin/nadmin.h"

#define TP_CONN_TYPE_ACCEPTED       1
#define TP_CONN_TYPE_INITIATED      2

typedef struct TcpProxyStats {
    SockStats_t connStats;
} TcpProxyStats_t;

typedef struct TcpProxyGroup {
    uint32_t keepSourcePort;
    SockAddr_t serverAddrP;
    TcpProxyStats_t cStats;
} TcpProxyGroup_t;

typedef struct TcpProxyI {
    uint32_t maxActSessions;
    uint32_t maxErrSessions;

    uint32_t csGroupCount;
    TcpProxyGroup_t* csGroupArr;

    TcpProxyStats_t gStats;
} TcpProxyI_t;

typedef struct TcpProxyCtx {
    Pool_t* freeSessionPool;
    Pool_t* freeBuffPool;
    TcpProxyI_t* appI; 
    Pool_t activeSessionPool;
    MsgIoChannelId_t nAdminChannelId;
    int nAdminChannelState;
    int nAdminChannelErr;
    char nAdminTestId[N_ADMIN_TEST_ID_MAX_LEN];
    SockAddr_t nAdminAddr;
    SockAddr_t nLocalAddr;
    IoVentCtx_t* iovCtx;
} TcpProxyCtx_t;

#define RW_MAX_BUFF_LEN 2048
typedef struct RwBuff {
    int buffLen;
    int buffOffset;
    int dataLen;
    char dataBuff[RW_MAX_BUFF_LEN];
} RwBuff_t;

typedef struct TcpProxyConn {
    IoVentConn_t* iovConn;
    RwBuff_t* readBuff;
    RwBuff_t* writeBuff;
    Pool_t writeQ;
    int isActive;
} TcpProxyConn_t;

typedef struct TcpProxySession {
    TcpProxyCtx_t* appCtx;
    TcpProxyConn_t aConn;
    TcpProxyConn_t iConn;
} TcpProxySession_t;


#endif 

