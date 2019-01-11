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

/////////////////////////////////TcpProxyApp////////////////////////////// 

typedef struct TcpProxyAppStats {
    SockStats_t connStats;
} TcpProxyAppStats_t;

typedef struct TcpProxyAppGroup {
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

void TcpClientRun(AppI_t* appBase);
void DumpTcpClientStats(AppI_t* appBase);

void TcpServerRun(AppI_t* appBase);
void DumpTcpServerStats(AppI_t* appBase);

///////////////////////////////////////////////////////////////////////////

#endif