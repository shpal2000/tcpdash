#include <sys/mman.h>
#include "iovents.h"

#include "msg_io.h"

static void OnEstablish (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {
        mioChanel->ioChannelMethods.OnError( (MsgIoChannelId_t) mioChanel );
    } else {
        mioChanel->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_LENGTH;
        mioChanel->currReadDataOff = 0;
        mioChanel->currReadDataLen = 0;

        mioChanel->ioChannelMethods.OnOpen( (MsgIoChannelId_t) mioChanel );
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    int nextReadLen;

    if (onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {
        nextReadLen = MSG_IO_READ_WRITE_BUFF_MAXLEN - mioChanel->currReadDataLen; 
    } else {
        nextReadLen = mioChanel->totalReadDataLen - mioChanel->currReadDataLen; 
    }

    ReadNextData (iovConn
                , mioChanel->readDataBuff
                , mioChanel->currReadDataOff
                , nextReadLen);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
    
    if (bytesRead > 0) {

        mioChanel->currReadDataLen += bytesRead;

        mioChanel->currReadDataOff += bytesRead;

        if ( onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH ) {

            if ( mioChanel->currReadDataLen >= MSG_IO_MESSAGEL_LENGTH_BYTES ) {
                
            }
        }

        if (mioChanel->currReadDataLen >= MSG_IO_MESSAGEL_LENGTH_BYTES) {

            if ()
        }


        if (onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

            if (mioChanel->currReadDataLen >= MSG_IO_MESSAGEL_LENGTH_BYTES) {

                onMsgState->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_DATA; 
            }

        } else {
            
        }

    } else {

        int closeErr = bytesRead;   

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
        } else {
            WriteClose (iovConn);
        }
    }
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

static void MsgIoCleanup (MsgIoChannel_t* mioChannel) {

    if (mioChannel->iovCtx) {

        DeleteIoVentCtx (mioChannel->iovCtx);
    }

    DeleteStruct (MsgIoChannel_t, mioChannel);
}

MsgIoChannelId_t MsgIoNew (SockAddr_t* localAddress
                            , SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mIoMethods) {

    int status = -1;

    MsgIoChannel_t* mioChannel
        = CreateStruct0 (MsgIoChannel_t);

    if (mioChannel) {

        ioChannelMethods

        IoVentMethods_t iovMethods = {0};
        IoVentOptions_t iovOptions = {0};

        iovMethods.OnEstablish = &OnEstablish;
        iovMethods.OnWriteNext = &OnWriteNext;
        iovMethods.OnWriteStatus = &OnWriteStatus;
        iovMethods.OnReadNext = &OnReadNext;
        iovMethods.OnReadStatus = &OnReadStatus;
        iovMethods.OnCleanup = &OnCleanup;
        iovMethods.OnStatus = &OnStatus;
        iovMethods.OnContinue = &OnContinue;

        iovOptions.maxActiveConnections = 1;
        iovOptions.maxErrorConnections = 1;
        iovOptions.maxEvents = 1;

        mioChannel->iovCtx 
            = CreateIoVentCtx (iovMethods, iovOptions, NULL);

        if (mioChannel->iovCtx) {

            int newConnInitErr = 
                NewConnection (iovCtx
                                , NULL
                                , mioChannel
                                , localAddress
                                , NULL
                                , remoteAddress
                                , &mioChannel->cStats
                                , &mioChannel->gStats);

            if (newConnInitErr == 0) {
                status = 0;
            }           
        }
    }

    if (status == -1) {

        if (mioChannel) {

            MsgIoCleanup (mioChannel);

            mioChannel = NULL;
        }
    }

    return (MsgIoChannelId_t) mioChannel;  
}

void MsgIoProcess (MsgIoChannelId_t mioChanelId) {
    
    MsgIoChannel_t* mioChanel = (MsgIoChannel_t*) mioChanelId;

    ProcessIoVent (mioChanel->iovCtx);
}

void MsgIoDelete (MsgIoChannelId_t mioChanelId) {

    MsgIoChannel_t* mioChanel = (MsgIoChannel_t*) mioChanelId;

    MsgIoCleanup (mioChanel);
}

void MsgIoSend (MsgIoChannelId_t mioChanelId
                            , char* msgBuff
                            , int msgOffset
                            , int msgLen) {

    MsgIoChannel_t* mioChanel = (MsgIoChannel_t*) mioChanelId;
}