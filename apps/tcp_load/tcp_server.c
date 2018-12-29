#include <sys/mman.h>
#include "iovents.h"

#define __APP__MAIN__
#include "tcp_load.h"


static void InitConn (TcpCSConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->bytesRead = 0;
    tcpConn->bytesWritten = 0;
    tcpConn->writeBuffOffset = 0;
}

static TcpCSSession_t* GetSession (TcpCSAppCtx_t* appCtx) {

    TcpCSSession_t* newSess =         
        GetFromPool (appCtx->freeSessionPool);
    
    if (newSess) {

        AddToPool (&appCtx->activeSessionPool, newSess);

        newSess->appCtx = appCtx;

        InitConn (&newSess->tcpConn);
    }

    return newSess;
}

static void FreeSession (TcpCSSession_t* newSess) {

    RemoveFromPool (&newSess->appCtx->activeSessionPool, newSess);

    AddToPool (newSess->appCtx->freeSessionPool, newSess);
}

static void OnEstablish (struct IoVentConn* iovConn) {

    TcpCSAppCtx_t* appCtx 
        = (TcpCSAppCtx_t*) iovConn->cInfo.appCtx;

    TcpCSGroup_t* groupCtx 
        = (TcpCSGroup_t*) iovConn->cInfo.groupCtx;

    TcpCSSession_t* newSess = GetSession (appCtx);

    if (newSess == NULL) {

        IncConnStats2 ( &appCtx->appI->gStats
                        , &groupCtx->cStats
                        , appSessStructNotAvail );

        AbortConnection (iovConn);

    } else {

        iovConn->cInfo.sessionData = newSess;

        setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ 15000 }, sizeof(int));
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TcpCSAppCtx_t* appCtx 
        = (TcpCSAppCtx_t*) iovConn->cInfo.appCtx;

    ReadNextData (iovConn
                , appCtx->commonReadBuff
                , 0
                , COMMON_READBUFF_MAXLEN);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    TcpCSSession_t* newSess 
        = (TcpCSSession_t*) iovConn->cInfo.sessionData;

    
    if (bytesRead > 0) {

        newSess->tcpConn.bytesRead += bytesRead;
    
    } else {

        int closeErr = bytesRead;

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
        }
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    TcpCSAppCtx_t* appCtx 
        = (TcpCSAppCtx_t*) iovConn->cInfo.appCtx;

    TcpCSSession_t* newSess 
        = (TcpCSSession_t*) iovConn->cInfo.sessionData;

    TcpCSGroup_t* groupCtx 
        = (TcpCSGroup_t*) iovConn->cInfo.groupCtx;
  

    if (newSess->tcpConn.bytesWritten < groupCtx->scDataLen) {

        int nextChunkLen = 0;

        if ( (groupCtx->scDataLen - newSess->tcpConn.bytesWritten) 
                                > COMMON_WRITEBUFF_MAXLEN ) {

            nextChunkLen = COMMON_WRITEBUFF_MAXLEN;

        } else {

            nextChunkLen 
                = (int) (groupCtx->scDataLen - newSess->tcpConn.bytesWritten);
        }

        WriteNextData (iovConn
                        , appCtx->commonWriteBuff
                        , 0
                        , nextChunkLen
                        , 0);
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    TcpCSSession_t* newSess 
        = (TcpCSSession_t*) iovConn->cInfo.sessionData;

    TcpCSGroup_t* groupCtx 
        = (TcpCSGroup_t*) iovConn->cInfo.groupCtx;


    if (bytesWritten > 0) {

        newSess->tcpConn.bytesWritten += bytesWritten;

        if (newSess->tcpConn.bytesWritten == groupCtx->scDataLen) {
            WriteClose (iovConn);
        }
    } else {
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    TcpCSSession_t* newSess 
        = (TcpCSSession_t*) iovConn->cInfo.sessionData;

    FreeSession (newSess);
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {

    return EmAppContinue;
}

static IoVentCtx_t* InitApp (TcpCSI_t* appI) {

    TcpCSAppCtx_t* appCtx = CreateStruct0 (TcpCSAppCtx_t);

    appCtx->appI = appI;

    CreatePool (&appCtx->freeSessionPool
                , appI->maxActSessions
                , TcpCSSession_t);

    InitPool (&appCtx->activeSessionPool);

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
    iovOptions->maxActiveConnections = appCtx->appI->maxActSessions;
    iovOptions->maxErrorConnections = appCtx->appI->maxErrSessions;

    IoVentCtx_t* iovCtx 
        = CreateIoVentCtx (iovMethods, iovOptions, appCtx);

    return iovCtx;
}

void TcpServerRun (TcpCSI_t* appI) {

    IoVentCtx_t* iovCtx = InitApp (appI);

    for (int i = 0; i < appI->csGroupCount; i++) {

        TcpCSGroup_t* csGroup 
            = &appI->csGroupArr[i];

        SockAddr_t* localAddress 
             = &(csGroup->serverAddr);

        InitServer(iovCtx
                    , csGroup
                    , localAddress
                    , &appI->gStats
                    , &csGroup->cStats);
    }

    while (1) {

        if ( ProcessIoVent (iovCtx) == 0 ) {
            break;
        }
    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);

    appI->isRunning = 0;
}


