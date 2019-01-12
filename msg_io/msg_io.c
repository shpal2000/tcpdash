#include <sys/mman.h>
#include "iovents.h"

#include "msg_io.h"

static void OnEstablish (struct IoVentConn* iovConn) {

    MsgIoSession_t* newSess 
        = (MsgIoSession_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {
    } else {
        setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ 15000 }, sizeof(int));
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

}

static void OnWriteNext (struct IoVentConn* iovConn) {

}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

}

static void OnCleanup (struct IoVentConn* iovConn) {

}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {

    return EmAppContinue;
}

static IoVentCtx_t* InitApp (TcpCsAppI_t* appI) {

    MsgIoCtx_t* appCtx = CreateStruct0 (MsgIoCtx_t);

    appCtx->appI = appI;

    CreatePool (&appCtx->freeSessionPool
                , appI->maxActSessions
                , MsgIoSession_t);

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

void TcpClientRun (AppI_t* appBase) {

    TcpCsAppI_t* appI = (TcpCsAppI_t*) appBase;

    IoVentCtx_t* iovCtx = InitApp (appI);

    double lastConnInitTime 
        = TimeElapsedIoVentCtx (iovCtx);

    while (1) {

        if ( ProcessIoVent (iovCtx) == 0 ) {
            break;
        }

        uint64_t tcpConnInitCount 
            = GetConnStats(&appI->gStats, tcpConnInit);

        int newConnectionInits 
            = (TimeElapsedIoVentCtx (iovCtx) - lastConnInitTime) 
                * appI->connPerSec;

        while (newConnectionInits > 0
                    && tcpConnInitCount < appI->maxSessions) {

            //new connection init
            TcpCsAppGroup_t* csGroup
                = &appI->csGroupArr[appI->nextCsGroupIndex];

            SockAddr_t* localAddress 
                = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

            SockAddr_t* remoteAddress 
                = &(csGroup->serverAddr);

            LocalPortPool_t* localPortPool 
                = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

            MsgIoSession_t* newSess = GetSession (iovCtx->appCtx);

            if (newSess == NULL) {

                IncConnStats2 ( &appI->gStats
                                , &csGroup->cStats
                                , appSessStructNotAvail );
                break;
            }

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
                                , newSess
                                , localAddress
                                , localPortPool
                                , remoteAddress
                                , &csGroup->cStats
                                , &appI->gStats);

            if (newConnInitErr) {
                FreeSession (newSess);
            }  

            newConnectionInits -= 1;

            lastConnInitTime = TimeElapsedIoVentCtx (iovCtx);

            tcpConnInitCount 
                = GetConnStats(&appI->gStats, tcpConnInit);
        }
    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);

    appI->ctrlInfo.isRunning = 0;
}

MsgIoChannel_t* MsgIoNew (SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mIoMethods) {

    return NULL;
}

void MsgIoDelete (MsgIoChannel_t* mIoConn) {

}

void MsgIoSend (MsgIoChannel_t* mIoConn
                            , char* msgBuff
                            , int msgOffset
                            , int msgLen) {

}