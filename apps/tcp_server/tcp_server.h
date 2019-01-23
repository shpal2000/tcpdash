#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#define COMMON_READBUFF_MAXLEN      100000
#define COMMON_WRITEBUFF_MAXLEN     1048576

typedef struct TcpServerStats {
    SockStats_t connStats;
} TcpServerStats_t;

typedef struct TcpServerGroup {
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
    TcpServerStats_t cStats;
} TcpServerGroup_t;

typedef struct TcpServerI {
    uint32_t maxEvents;
    uint32_t connPerSec;
    uint32_t maxActSessions;
    uint32_t maxErrSessions;
    uint64_t maxSessions;
    uint32_t csGroupCount;
    TcpServerGroup_t* csGroupArr;
    uint32_t nextCsGroupIndex;
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

typedef struct TcpCsSession {
    TcpServerCtx_t* appCtx;
    TcpServerConn_t tcpConn;
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

void TcpClientRun(TcpServerI_t* appI);

void TcpServerRun(TcpServerI_t* appI);

#endif