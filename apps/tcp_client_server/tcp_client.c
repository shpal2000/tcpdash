#include <sys/mman.h>
#include "iovents.h"

#define __APP__MAIN__
#include "tcp_client_server.h"


static void InitRwBuff (RwBuff_t* newBuff) {

    newBuff->buffLen = RW_MAX_BUFF_LEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void InitConn (TcpClientConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->readBuff = NULL;
    tcpConn->writeBuff = NULL;
}

static void InitSession (TcpClientSession_t* newSess
                            TcpClientAppCtx_t* appCtx ) {

    newSess->appCtx = appCtx;
    InitConn (&newSess->tcpConn);
}

static void OnEstablish (struct IoVentConn* iovConn) {
}

static void OnWriteNext (struct IoVentConn* iovConn) {
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {
}

static void OnReadNext (struct IoVentConn* iovConn) {
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesReceived) {

}

static void OnCleanup (struct IoVentConn* iovConn) {
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* iovData) {
    
    TcpClientAppCtx_t* appCtx = (TcpClientAppCtx_t*) iovData;

    TcpClientServerI_t* appI = appCtx->appI;

    //exceeded error connections
    uint32_t tcpConnInitFailCount 
        = GetConnStats(appI->gStats, tcpConnInitFail);

    if (tcpConnInitFailCount >= appI->maxErrorSessions) {
        return EmAppExit; 
    }

    //achived desired total connections
    uint32_t tcpConnInitCount 
        = GetConnStats(appI->gStats, tcpConnInit);

    if (tcpConnInitCount == appI->maxSessions) {
        return EmAppContinueZeroActive; 
    }

    
    //continue to run
    return EmAppContinue;
}

static TcpClientAppCtx_t* CreateAppCtx (TcpClientServerI_t* appI) {

    TcpClientAppCtx_t* appCtx = CreateStruct0 (TcpClientAppCtx_t);

    appCtx->appI = appI;

    CreatePool (&appCtx->freeSessionPool
                , appI->maxActiveSessions
                , TcpProxySession_t);

    CreatePool (&appCtx->freeBuffPool
                , appI->maxActiveSessions * 16
                , RwBuff_t);

    InitPool (&appCtx->activeSessionPool);

    return appCtx;
}

void TcpClientRun (TcpClientServerI_t* appI) {

    TcpClientAppCtx_t* appCtx = CreateAppCtx (appI);

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);
    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnWriteStatus = &OnWriteStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadStatus = &OnReadStatus;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;
    iovMethods->OnContinue = &OnContinue;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);
    iovOptions->maxActiveConnections = appI->maxActiveSessions;
    iovOptions->maxErrorConnections = appI->maxErrorSessions;

    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);

    double lastConnInitTime = TimeElapsedIoVentCtx (iovCtx);

    while (1) {

        if ( ProcessIoVent (iovCtx) == 0 ) {
            break;
        }
        
    }

    DeleteIoVentCtx (iovCtx);
}

TcpClientServerI_t* CreateTcpClientServerInterface(int csGroupCount
                                            , int* clientAddrCounts) {

    TcpClientServerI_t* appI 
        = (TcpClientServerI_t*) mmap(NULL
            , sizeof (TcpClientServerI_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    appI->csGroupCount = csGroupCount;
    appI->csGroupArr 
        = (TcpClientServerGroup_t*) mmap(NULL
            , sizeof (TcpClientServerGroup_t) * appI->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    
    appI->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < appI->csGroupCount; gIndex++) {
        TcpClientServerGroup_t* csGroup = &appI->csGroupArr[gIndex];
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

        csGroup->cCloseMethod = EmTcpFIN; 
        csGroup->sCloseMethod = EmTcpFIN;
        csGroup->csCloseType = EmDataFinish;
        csGroup->csWeight = 1; 
    }

    return appI;
}

void DeleteTcpClientServerInterface (TcpClientServerI_t* appI){
    // todo
}

void DumpTcpClientStats() {
    puts ("todo: DumpTcpClientStats\n");
}



