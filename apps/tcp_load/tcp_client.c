#include <sys/mman.h>
#include "iovents.h"

#define __APP__MAIN__
#include "tcp_load.h"


static void InitConn (TcpClientConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->bytesRead = 0;
    tcpConn->bytesWritten = 0;
    tcpConn->writeBuffOffset = 0;
}

static TcpClientSession_t* GetSession (TcpClientAppCtx_t* appCtx) {

    TcpClientSession_t* newSess =         
        GetFromPool (appCtx->freeSessionPool);
    
    if (newSess) {

        AddToPool (&appCtx->activeSessionPool, newSess);

        newSess->appCtx = appCtx;

        InitConn (&newSess->tcpConn);
    }

    return newSess;
}

static void FreeSession (TcpClientSession_t* newSess
                            TcpClientAppCtx_t* appCtx ) {

    RemoveFromPool (&appCtx->activeSessionPool, newSess);

    AddToPool (&appCtx->freeSessionPool, newSess);
}

static void OnEstablish (struct IoVentConn* iovConn) {

    TcpClientAppCtx_t* appCtx 
        = (TcpClientAppCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {
        FreeSession (newSess, appCtx);
    } else {
        setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ 15000 }, sizeof(int));
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TcpClientAppCtx_t* appCtx 
        = (TcpClientAppCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    ReadNextData (iovConn
                , appCtx->commonReadBuff
                , 0
                , COMMON_READBUFF_MAXLEN);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesReceived) {

    TcpClientAppCtx_t* appCtx 
        = (TcpClientAppCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    
    if (bytesReceived > 0) {

        iovConn->bytesRead += bytesReceived;
    
    } else {

        int closeErr = bytesReceived;

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
        }
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    TcpClientAppCtx_t* appCtx 
        = (TcpClientAppCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    TcpClientServerGroup_t* groupCtx 
        = (TcpClientServerGroup_t*) iovConn->cInfo.groupCtx;
  

    if (iovConn->bytesWritten < groupCtx->csDataLen) {

        int nextChunkLen = 0;

        if ( (groupCtx->csDataLen - iovConn->bytesWritten) 
                                > COMMON_WRITEBUFF_MAXLEN ) {

            nextChunkLen = COMMON_WRITEBUFF_MAXLEN;

        } else {

            nextChunkLen = (int) (groupCtx->csDataLen - iovConn->bytesWritten);
        }

        WriteNextData (iovConn
                        , appCtx->commonWriteBuff
                        , 0
                        , tmpBuff->nextChunkLen
                        , 0);
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    TcpClientAppCtx_t* appCtx 
        = (TcpClientAppCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    TcpClientServerGroup_t* groupCtx 
        = (TcpClientServerGroup_t*) iovConn->cInfo.groupCtx;


    if (bytesWritten > 0) {

        iovConn->bytesWritten += bytesWritten;

        if (iovConn->bytesWritten == groupCtx->csDataLen) {
            WriteClose (iovConn);
        }
    } else {
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    TcpClientAppCtx_t* appCtx 
        = (TcpClientAppCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    FreeSession (newSess, appCtx);
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {
    
    TcpClientAppCtx_t* appCtx = (TcpClientAppCtx_t*) appData;

    TcpClientServerI_t* appI = appCtx->appI;

    //exceeded error connections
    uint32_t tcpConnInitFailCount 
        = GetConnStats(appI->gStats, tcpConnInitFail);

    if (tcpConnInitFailCount >= appI->maxErrorSessions) {
        return EmAppExit; 
    }

    //achived desired total connections
    uint64_t tcpConnInitCount 
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

        uint64_t tcpConnInitCount 
            = GetConnStats(appI->gStats, tcpConnInit);

        int newConnectionInits 
            = (TimeElapsedIoVentCtx (iovCtx) - lastConnInitTime) 
                * appI->connectionPerSec;

        while (newConnectionInits > 0
                    && tcpConnInitCount < appI->maxSessions) {

            TcpClientSession_t* newSess = GetSession (appCtx);

            if (newSess == NULL) {

                IncConnStats2 ( &appI->gStats
                                , &csGroup->cStats
                                , appSessStructNotAvail );
                break;
            }

            //new connection init
            TcpClientServerGroup_t* csGroup
                = &appI->csGroupArr[appI->nextCsGroupIndex];

            SockAddr_t* localAddress 
                = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

            SockAddr_t* remoteAddress 
                = &(csGroup->serverAddr);

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

            int newConnInitErr = 
                NewConnection (iovCtx
                                , csGroup
                                , appCtx
                                , newSess
                                , localAddress
                                , localPortPool
                                , remoteAddress
                                , &csGroup->cStats
                                , &appI->gStats);

            if (newConnInitErr) {
                FreeSession (newSess, appCtx);
            }  

            newConnectionInits -= 1;

            lastConnInitTime = TimeElapsedIoVentCtx (iovCtx);

            tcpConnInitCount 
                = GetConnStats(appI->gStats, tcpConnInit);
        }
    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);

    appI->isRunning = 0;
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



