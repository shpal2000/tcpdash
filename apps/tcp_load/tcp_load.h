#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

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
    AppI_t ctrlInfo; //??? todo

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

typedef struct TcpCsAppCtx {
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






/*
{
    "maxSessions" : 0, 

    "maxActSessions" : 0,
    
    "maxErrSessions" : 0,

    "csGroupList" : [
        {
            "clientAddrList" : [
                {
                    "addr" : ""
                    "mask" : ""
                },
                {
                    "addr" : ""
                    "mask" : ""
                }
            ],

            "serverAddr" : "",

            "serverPort" : 0,
        }, 
        {
            "clientAddrList" : [
                {
                    "addr" : ""
                    "mask" : ""
                },
                {
                    "addr" : ""
                    "mask" : ""
                }
            ],

            "serverAddr" : "",

            "serverPort" : 0,
        }        
    ]
}
*/

void TcpClientRun(TcpCsAppI_t* appI);

void TcpServerRun(TcpCsAppI_t* appI);

#endif