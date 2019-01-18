#include "msg_io.h"

static void MsgIoRecvNextInit (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel
        = (MsgIoChannel_t*) mioChannelId;

    mioChannel->recvMsg.len = 0;
    mioChannel->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_LENGTH;
}

void MsgIoSendInit (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    EnableWriteNotification ( mioChannel->iovConn );
}

MsgIoDataBuff_t* MsgIoGetRecvBuff (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    return &mioChannel->recvMsg;
}

MsgIoDataBuff_t* MsgIoGetSendBuff (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    return &mioChannel->sendMsg;
}

void* MsgIoGetCtx (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    return mioChannel->mioCtx;
}

static void OnEstablish (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {

        mioChannel->iovConn = NULL;

        (*mioChannel->mioMethods.OnError)( (MsgIoChannelId_t) mioChannel);

    } else {

        mioChannel->iovConn = iovConn;

        DisableWriteNotification (iovConn);

        MsgIoRecvNextInit ( (MsgIoChannelId_t) mioChannel );

        (*mioChannel->mioMethods.OnOpen)( (MsgIoChannelId_t) mioChannel);
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    char* readBuff;
    int readLen;

    if (mioChannel->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

        readBuff = mioChannel->recvBuff;
        readLen = MSG_IO_MESSAGEL_LENGTH_BYTES;

    } else {

        readBuff = mioChannel->recvMsg.data;
        readLen =  mioChannel->recvMsg.len;
    }

    ReadNextData (iovConn
                , readBuff
                , 0
                , readLen
                , 0);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
    
    if (bytesRead > 0) {

        if (mioChannel->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

            char *endptr;
            mioChannel->recvBuff[MSG_IO_MESSAGEL_LENGTH_BYTES-1] = '\0';
            mioChannel->recvMsg.len = (int) strtol (mioChannel->recvBuff
                                                            ,  &endptr
                                                            , 10);
            
            if ( (errno != 0 && mioChannel->recvMsg.len == 0)
                    || (endptr == mioChannel->recvBuff)
                    || (errno == ERANGE && (mioChannel->recvMsg.len == LONG_MIN))
                    || (errno == ERANGE && (mioChannel->recvMsg.len == LONG_MAX))
                    || (mioChannel->recvMsg.len < 0)
                    || (mioChannel->recvMsg.len > MSG_IO_READ_WRITE_DATA_MAXLEN) ) {

                AbortConnection (iovConn);

                (*mioChannel->mioMethods.OnError)((MsgIoChannelId_t) mioChannel);

            } else {

                mioChannel->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_DATA; 
            }

        } else {

            (*mioChannel->mioMethods.OnMsgRecv)( (MsgIoChannelId_t) mioChannel);

            MsgIoRecvNextInit ( (MsgIoChannelId_t) mioChannel );
        }

    } else {

        int closeErr = bytesRead;   

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
            (*mioChannel->mioMethods.OnError)((MsgIoChannelId_t) mioChannel);
        } else {
            WriteClose (iovConn);
        }
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    WriteNextData (iovConn
                    , mioChannel->sendMsg.data
                    , 0
                    , mioChannel->sendMsg.len
                    , 0);
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    DisableWriteNotification (iovConn);

    (*mioChannel->mioMethods.OnMsgSent)( (MsgIoChannelId_t) mioChannel);
}

static void OnCleanup (struct IoVentConn* iovConn) {
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {
    return EmAppContinue;
}

static void MsgIoCleanup (MsgIoChannel_t* mioChannel) {

    if (mioChannel->iovCtx) {

        DeleteIoVentCtx (mioChannel->iovCtx);
    }

    DeleteStruct (MsgIoChannel_t, mioChannel);
}

MsgIoChannelId_t MsgIoNew (SockAddr_t* localAddress
                            , SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mioMethods
                            , void* mioCtx) {

    int status = -1;

    MsgIoChannel_t* mioChannel
        = CreateStruct0 (MsgIoChannel_t);

    if (mioChannel) {

        mioChannel->mioCtx = mioCtx;

        mioChannel->mioMethods = *mioMethods;

        mioChannel->sendMsg.len = 0;
        mioChannel->recvMsg.len = 0;

        mioChannel->sendMsg.data 
            = mioChannel->sendBuff + MSG_IO_MESSAGEL_LENGTH_BYTES;

        mioChannel->recvMsg.data 
            = mioChannel->recvBuff + MSG_IO_MESSAGEL_LENGTH_BYTES;

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
            = CreateIoVentCtx (&iovMethods, &iovOptions, NULL);

        if (mioChannel->iovCtx) {

            int newConnInitErr = 
                NewConnection (mioChannel->iovCtx
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

void MsgIoProcess (MsgIoChannelId_t mioChannelId) {
    
    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    ProcessIoVent (mioChannel->iovCtx);
}

void MsgIoDelete (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    if (mioChannel->iovConn) {
        AbortConnection (mioChannel->iovConn);
        mioChannel->iovConn = NULL;
    }

    MsgIoCleanup (mioChannel);
}
