#ifndef __TLS_SAMPLE_SERVER_APP_H
#define __TLS_SAMPLE_SERVER_APP_H

#include "platform/common.h"

typedef struct TlsSampleServerStats {
    SockStats_t connStats;
} TlsSampleServerStats_t;

typedef struct TlsSampleServerAppStats {
    uint64_t dbgNoFreeSession;  
} TlsSampleServerAppStats_t;

typedef struct TlsSampleServerGroup {
    uint32_t clientAddrCount;
    SockAddr_t* clientAddrArr;
    uint32_t nextClientAddrIndex;
    LocalPortPool_t* LocalPortPoolArr;  
    SockAddr_t serverAddr;
    TlsSampleServerStats_t cStats;
} TlsSampleServerGroup_t;

typedef struct TlsSampleServer {
    uint32_t isRunning;
    uint32_t maxEvents;

    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;
    uint32_t scDataLen;

    TlsSampleServerAppStats_t appStats;
    TlsSampleServerStats_t appConnStats;

    uint32_t csGroupCount;
    uint32_t nextCsGroupIndex;
    TlsSampleServerGroup_t* csGroupArr;
      
} TlsSampleServer_t;

void TlsSampleServerRun(TlsSampleServer_t* appIface);

TlsSampleServer_t* CreateTlsSampleServerInterface(int csGroupCount);

void DeleteTlsSampleServerInterface(TlsSampleServer_t* iFace);

void DumpTlsSampleServerStats(TlsSampleServerStats_t* appConnStats);

// non-interface types
typedef struct TlsSampleServerCtx {
    char* sendBuffer;
    int csDataLen;
    char* receiveBuffer;
    int scDataLen;
} TlsSampleServerCtx_t;

typedef struct TlsSampleServerConnData {
    int bytesSent;
} TlsSampleServerConnData_t;

#endif
