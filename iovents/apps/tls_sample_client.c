
#include <sys/mman.h>

#include "iovents.h"
#include "tls_sample_client.h"


static void OnEstablish (IoVentConn_t* iovConn) {

}

static void OnWriteNext (IoVentConn_t* iovConn) {
    
}

static void OnReadNext (IoVentConn_t* iovConn) {
    
}

static void OnCleanup (IoVentConn_t* iovConn) {
    
}

static void OnStatus (IoVentConn_t* iovConn) {

    //todo for more advavanced control
    switch (iovConn->statusId) {
        default:
            break;
    }

}

void TlsSampleClientRun (TlsSampleClient_t* appI) {

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);
    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);
    iovOptions->maxActiveConnections = appI->maxActiveSessions;
    iovOptions->maxErrorConnections = appI->maxErrorSessions;
    iovOptions->maxEvents = appI->maxEvents; 
    
    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);

    TimerWheel_t* timerWheel = CreateTimerWheel();
    double lastConnectionInitTime = TimeElapsedTimerWheel(timerWheel);
    int activeConnections = 0;
    TlsSampleClientStats_t* appConnStats = &appI->appConnStats;
    // TlsSampleClientAppStats_t* appStats = &appI->appStats;

    while (1) {

        activeConnections = ProcessIoVent (iovCtx);

        if ( (GetConnStats(appConnStats, tcpConnInitFail) 
                    >= appI->maxErrorSessions) 
                || (GetConnStats(appConnStats, tcpConnInit) 
                    == appI->maxSessions 
                    && activeConnections == 0) ) {
            break;
        }

        if (GetConnStats(appConnStats, tcpConnInit) < appI->maxSessions) {

            int newConnectionInits 
                = ((TimeElapsedTimerWheel(timerWheel)
                        - lastConnectionInitTime)
                    * appI->connectionPerSec);
                
            while ( (newConnectionInits > 0) 
                    && (GetConnStats(appConnStats, tcpConnInit) 
                        < appI->maxSessions) ) {

                newConnectionInits--;

                lastConnectionInitTime = TimeElapsedTimerWheel(timerWheel);

                TlsSampleClientGroup_t* csGroup 
                    = &appI->csGroupArr[appI->nextCsGroupIndex];

                SockAddr_t* localAddress 
                    = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

                SockAddr_t* remoteAddress 
                    = &(csGroup->serverAddr);

                TlsSampleClientStats_t* groupConnStats = &csGroup->cStats;

                LocalPortPool_t* localPortPool 
                    = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

                appI->nextCsGroupIndex += 1;
                if (appI->nextCsGroupIndex == appI->csGroupCount){
                    appI->nextCsGroupIndex = 0;
                }
                
                csGroup->nextClientAddrIndex += 1;
                if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
                    csGroup->nextClientAddrIndex = 0;
                }

                NewConnection (iovCtx
                                , localAddress
                                , localPortPool
                                , remoteAddress
                                , appConnStats
                                , groupConnStats);
            }
        }
    }

    DeleteIoVentCtx (iovCtx);
    DeleteTimerWheel(timerWheel);
}

TlsSampleClient_t* CreateTlsSampleClientInterface(int csGroupCount
                                            , int* clientAddrCounts) {

    TlsSampleClient_t* iFace 
        = (TlsSampleClient_t*) mmap(NULL
            , sizeof (TlsSampleClient_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TlsSampleClientGroup_t*) mmap(NULL
            , sizeof (TlsSampleClientGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    iFace->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < iFace->csGroupCount; gIndex++) {
        TlsSampleClientGroup_t* csGroup = &iFace->csGroupArr[gIndex];
        csGroup->clientAddrCount = clientAddrCounts[gIndex];
        csGroup->nextClientAddrIndex = 0;
        csGroup->clientAddrArr
            = (SockAddr_t*) mmap(NULL
                , sizeof (SockAddr_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        csGroup->LocalPortPoolArr 
            = (LocalPortPool_t*) mmap(NULL
                , sizeof (LocalPortPool_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        for (int cIndex = 0
                ; cIndex < csGroup->clientAddrCount
                ; cIndex++) {
            InitPortBindQ(&csGroup->LocalPortPoolArr[cIndex]);
        }
    }

    return iFace;
}

void DeleteTlsSampleClientInterface (TlsSampleClient_t* iFace){
    //todo
}




