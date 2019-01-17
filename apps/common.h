#ifndef __APP_COMMON_H
#define __APP_COMMON_H

#include "platform/common.h"

enum ConnCloseMethod {EmTcpFIN
                    , EmTcpRST
                    , EmCloseNotify};

enum ConnCloseType {EmClientClose
                    , EmServerClose
                    , EmDataFinish};

typedef struct AppI {
    uint32_t isRunning;
} AppI_t;

typedef void* MsgIoChannelId_t;

typedef struct MsgIoDataBuff {
    char* data;
    int len;
} MsgIoDataBuff_t;

typedef struct MsgIoChannelStats {
    SockStats_t connStats;
} MsgIoChannelStats_t;

typedef struct MsgIoMethods {

    void (*OnOpen) (MsgIoChannelId_t mioChanelId); 

    void (*OnError) (MsgIoChannelId_t mioChanelId);

    void (*OnMsgRecv) (MsgIoChannelId_t mioChanelId);

    void (*OnMsgSent) (MsgIoChannelId_t mioChanelId);

} MsgIoMethods_t;

MsgIoChannelId_t MsgIoNew (SockAddr_t* localAddress
                            , SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mioMethods);
void MsgIoDelete (MsgIoChannelId_t mioChanelId);
MsgIoDataBuff_t* MsgIoGetRecvBuff (MsgIoChannelId_t mioChanelId);
MsgIoDataBuff_t* MsgIoGetSendBuff (MsgIoChannelId_t mioChanelId);
void MsgIoSendNextInit (MsgIoChannelId_t mioChanelId);
void MsgIoProcess (MsgIoChannelId_t mioChannelId);
/////////////////////////////////TcpProxyApp////////////////////////////// 

typedef struct TcpProxyAppStats {
    SockStats_t connStats;
} TcpProxyAppStats_t;

typedef struct TcpProxyAppGroup {
    uint32_t keepSourcePort;
    SockAddr_t serverAddrP;
    TcpProxyAppStats_t cStats;
} TcpProxyAppGroup_t;

typedef struct TcpProxyAppI {
    AppI_t ctrlInfo;
     
    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;

    uint32_t csGroupCount;
    TcpProxyAppGroup_t* csGroupArr;

    TcpProxyAppStats_t gStats;
} TcpProxyAppI_t;

void TcpProxyRun(AppI_t* appBase);

void DumpTcpProxyStats(AppI_t* appBase);

////////////////////////////////////TcpCsApp///////////////////////////////

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
    AppI_t ctrlInfo;

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

void TcpClientRun(AppI_t* appBase);
void DumpTcpClientStats(AppI_t* appBase);

void TcpServerRun(AppI_t* appBase);
void DumpTcpServerStats(AppI_t* appBase);

///////////////////////////////////////////////////////////////////////////

#endif