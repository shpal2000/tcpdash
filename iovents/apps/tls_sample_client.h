#ifndef __TLS_SAMPLE_CLIENT_APP_H
#define __TLS_SAMPLE_CLIENT_APP_H

#include "platform/common.h"

typedef struct TlsSampleClientStats {
    SockStats_t connStats;
} TlsSampleClientStats_t;

typedef struct TlsSampleClientAppStats {
    uint64_t dbgNoFreeSession;  
} TlsSampleClientAppStats_t;

typedef struct TlsSampleClientGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;  
    SockAddr_t serverAddr;
    TlsSampleClientStats_t cStats;
} TlsSampleClientGroup_t;

typedef struct TlsSampleClient {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;
    uint32_t scDataLen;

    TlsSampleClientAppStats_t appStats;
    TlsSampleClientStats_t appConnStats;

    uint32_t csGroupCount;
    uint32_t nextCsGroupIndex;
    TlsSampleClientGroup_t* csGroupArr;
      
} TlsSampleClient_t;

void TlsSampleClientRun(TlsSampleClient_t* appIface);

TlsSampleClient_t* CreateTlsSampleClientInterface(int csGroupCount
                                            , int* clientAddrCounts);

void DeleteTlsSampleClientInterface(TlsSampleClient_t* iFace);

void DumpTlsSampleClientStats(TlsSampleClientStats_t* appConnStats);

// non-interface types
typedef struct TlsSampleClientCtx {
    char* sendBuffer;
    int csDataLen;
} TlsSampleClientCtx_t;

typedef struct TlsSampleClientConnData {
    int bytesSent;
} TlsSampleClientConnData_t;

#endif
