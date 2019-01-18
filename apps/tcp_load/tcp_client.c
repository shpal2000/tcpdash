#include <sys/mman.h>
#include "iovents.h"

#include "tcp_load.h"

static void InitConn (TcpCsAppConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->bytesRead = 0;
    tcpConn->bytesWritten = 0;
    tcpConn->writeBuffOffset = 0;
}

static TcpCsSession_t* GetSession (TcpCsAppCtx_t* appCtx) {

    TcpCsSession_t* newSess =         
        GetFromPool (appCtx->freeSessionPool);
    
    if (newSess) {

        AddToPool (&appCtx->activeSessionPool, newSess);

        newSess->appCtx = appCtx;

        InitConn (&newSess->tcpConn);
    }

    return newSess;
}

static void FreeSession (TcpCsSession_t* newSess) {

    RemoveFromPool (&newSess->appCtx->activeSessionPool, newSess);

    AddToPool (newSess->appCtx->freeSessionPool, newSess);
}

static void OnEstablish (struct IoVentConn* iovConn) {

    TcpCsSession_t* newSess 
        = (TcpCsSession_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {
        FreeSession (newSess);
    } else {
        setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ 15000 }, sizeof(int));
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TcpCsAppCtx_t* appCtx 
        = (TcpCsAppCtx_t*) iovConn->cInfo.appCtx;

    ReadNextData (iovConn
                , appCtx->commonReadBuff
                , 0
                , COMMON_READBUFF_MAXLEN
                , 1);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    TcpCsSession_t* newSess 
        = (TcpCsSession_t*) iovConn->cInfo.sessionData;

    
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

    TcpCsAppCtx_t* appCtx 
        = (TcpCsAppCtx_t*) iovConn->cInfo.appCtx;

    TcpCsSession_t* newSess 
        = (TcpCsSession_t*) iovConn->cInfo.sessionData;

    TcpCsAppGroup_t* groupCtx 
        = (TcpCsAppGroup_t*) iovConn->cInfo.groupCtx;
  

    if (newSess->tcpConn.bytesWritten < groupCtx->csDataLen) {

        int nextChunkLen = 0;

        if ( (groupCtx->csDataLen - newSess->tcpConn.bytesWritten) 
                                > COMMON_WRITEBUFF_MAXLEN ) {

            nextChunkLen = COMMON_WRITEBUFF_MAXLEN;

        } else {

            nextChunkLen 
                = (int) (groupCtx->csDataLen - newSess->tcpConn.bytesWritten);
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

    TcpCsSession_t* newSess 
        = (TcpCsSession_t*) iovConn->cInfo.sessionData;

    TcpCsAppGroup_t* groupCtx 
        = (TcpCsAppGroup_t*) iovConn->cInfo.groupCtx;


    if (bytesWritten > 0) {

        newSess->tcpConn.bytesWritten += bytesWritten;

        if (newSess->tcpConn.bytesWritten == groupCtx->csDataLen) {
            WriteClose (iovConn);
        }
    } else {
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    TcpCsSession_t* newSess 
        = (TcpCsSession_t*) iovConn->cInfo.sessionData;

    FreeSession (newSess);
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {
    
    TcpCsAppCtx_t* appCtx = (TcpCsAppCtx_t*) appData;

    TcpCsAppI_t* appI = appCtx->appI;

    //exceeded error connections
    uint64_t tcpConnInitFailCount 
        = GetConnStats(&appI->gStats, tcpConnInitFail);

    if (tcpConnInitFailCount >= appI->maxErrSessions) {
        return EmAppExit; 
    }

    //achived desired total connections
    uint64_t tcpConnInitCount 
        = GetConnStats(&appI->gStats, tcpConnInit);

    if (tcpConnInitCount == appI->maxSessions) {
        return EmAppContinueZeroActive; 
    }

    //continue to run
    return EmAppContinue;
}

static void MsgIoOnOpen (MsgIoChannelId_t mioChanelId) {

    MsgIoDataBuff_t* sndBuff = MsgIoGetSendBuff (mioChanelId);

    char* send = "";
    sndBuff->data = send;
    sndBuff->len = strlen(send);

    MsgIoSendNextInit (mioChanelId); 
}

static void MsgIoOnError (MsgIoChannelId_t mioChanelId) {

}

static void MsgIoOnMsgRecv (MsgIoChannelId_t mioChanelId) {

    MsgIoDataBuff_t* rcvBuff = MsgIoGetRecvBuff (mioChanelId);
    rcvBuff->data[rcvBuff->len] = '\0';
    puts (rcvBuff->data);
    puts("\n\n\n\n\n\n\n\n\n\n\n");

    MsgIoGetCtx (mioChanelId);
}

static void MsgIoOnMsgSent (MsgIoChannelId_t mioChanelId) {

}

static IoVentCtx_t* InitApp (TcpCsAppI_t* appI) {

    TcpCsAppCtx_t* appCtx = CreateStruct0 (TcpCsAppCtx_t);

    appCtx->appI = appI;

    CreatePool (&appCtx->freeSessionPool
                , appI->maxActSessions
                , TcpCsSession_t);

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

int main(int argc, char** argv) {

    char* testId = argv[1];
    char* msgIoAddress = argv[2];
    int msgIoPort = atoi(argv[3]);
    
    SockAddr_t msgIoLocalAddr;    
    memset(&msgIoLocalAddr, 0, sizeof(SockAddr_t));

    SockAddr_t msgIoRemoteAddr;
    struct sockaddr_in* remoteAddrIn = &msgIoRemoteAddr.inAddr;
    inet_pton(AF_INET
                , msgIoAddress
                , &(remoteAddrIn->sin_addr));

    remoteAddrIn->sin_port = htons(msgIoPort);

    MsgIoMethods_t mioMethods;
    mioMethods.OnOpen = &MsgIoOnOpen;
    mioMethods.OnError = &MsgIoOnError;
    mioMethods.OnMsgRecv = &MsgIoOnMsgRecv;
    mioMethods.OnMsgSent = &MsgIoOnMsgSent;

    MsgIoChannelId_t channelId 
            =  MsgIoNew (&msgIoLocalAddr
                        , &msgIoRemoteAddr
                        , &mioMethods);

    // TcpCsAppI_t* appI = NULL;

    // IoVentCtx_t* iovCtx = InitApp (appI);

    // double lastConnInitTime 
    //     = TimeElapsedIoVentCtx (iovCtx);

    while (1) {

        MsgIoProcess (channelId);
        
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

            TcpCsSession_t* newSess = GetSession (iovCtx->appCtx);

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

    MsgIoDelete (channelId);

    DeleteIoVentCtx (iovCtx);

    return 0;
}