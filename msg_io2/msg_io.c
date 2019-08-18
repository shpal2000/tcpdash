#include "msg_io.h"

static void MsgIoRecvNextInit (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel
        = (MsgIoChannel_t*) mioChannelId;

    mioChannel->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_LENGTH;
    mioChannel->recvMsg.len = 0;
    mioChannel->recvMsg.data = 
        mioChannel->recvMsg.msgBuff + MSG_IO_MESSAGEL_LENGTH_BYTES;
}

void MsgIoSend (MsgIoChannelId_t mioChannelId, const char* msg, int msgLen) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    if (msgLen > MSG_IO_READ_WRITE_DATA_MAXLEN) {
        // ??? log
    } else {
        MsgIoDataBuff_t* mioDataBuff = GetFromPool (&mioChannel->freeSendPool);
        if (mioDataBuff == NULL) {
            // ??? log
        } else {
            mioDataBuff->len = msgLen;
            sprintf (mioDataBuff->msgBuff
                        , MSG_IO_MESSAGEL_LENGTH_FORMAT
                        , mioDataBuff->len);
            mioDataBuff->msgBuff[MSG_IO_MESSAGEL_LENGTH_BYTES-1] = '\n';
            memcpy (mioDataBuff->data, msg, mioDataBuff->len);
            AddToPool (&mioChannel->actSendPool, mioDataBuff);
            EnableWriteNotification ( mioChannel->iovConn);
        }
    }
}

void MsgIoRecv (MsgIoChannelId_t mioChannelId
                        , char** pMsg
                        , int* pLen) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    *pMsg = mioChannel->recvMsg.data;
    *pLen = mioChannel->recvMsg.len;
}

void* MsgIoGetCtx (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    return mioChannel->mioCtx;
}

double MsgIoTimeElapsed (MsgIoChannelId_t mioChannelId) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) mioChannelId;

    return TimeElapsedIoVentCtx (mioChannel->iovCtx);
}

static void OnEstablish (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.connCtx;
 
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
        = (MsgIoChannel_t*) iovConn->cInfo.connCtx;

    char* readBuff;
    int readLen;

    if (mioChannel->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

        readBuff = mioChannel->recvMsg.msgBuff;
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
        = (MsgIoChannel_t*) iovConn->cInfo.connCtx;
    
    if (bytesRead > 0) {

        if (mioChannel->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

            char *endptr;
            mioChannel->recvMsg.msgBuff[MSG_IO_MESSAGEL_LENGTH_BYTES-1] = '\0';
            mioChannel->recvMsg.len = (int) strtol (mioChannel->recvMsg.msgBuff
                                                            ,  &endptr
                                                            , 10);
            
            if ( (errno != 0 && mioChannel->recvMsg.len == 0)
                    || (endptr == mioChannel->recvMsg.msgBuff)
                    || (errno == ERANGE && (mioChannel->recvMsg.len == LONG_MIN))
                    || (errno == ERANGE && (mioChannel->recvMsg.len == LONG_MAX))
                    || (mioChannel->recvMsg.len <= 0)
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
        = (MsgIoChannel_t*) iovConn->cInfo.connCtx;

    MsgIoDataBuff_t* mioDataBuff = GetFromPool (&mioChannel->actSendPool);
    if (mioDataBuff) {
        WriteNextData (iovConn
                        , mioDataBuff->msgBuff
                        , 0
                        , mioDataBuff->len 
                            + MSG_IO_MESSAGEL_LENGTH_BYTES
                        , 0);
        mioChannel->sendMsg = mioDataBuff;
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    MsgIoChannel_t* mioChannel 
        = (MsgIoChannel_t*) iovConn->cInfo.connCtx;

    (*mioChannel->mioMethods.OnMsgSent)( (MsgIoChannelId_t) mioChannel);

    AddToPool (&mioChannel->freeSendPool, mioChannel->sendMsg);
    mioChannel->sendMsg = NULL;

    if ( IsPoolEmpty (&mioChannel->actSendPool) ) {
        DisableWriteNotification (iovConn);
    }
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

    // ??? release sendPool, freePool

    DeleteStruct (MsgIoChannel_t, mioChannel);
}

MsgIoChannelId_t MsgIoNew (SockAddr_t* localAddress
                            , SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mioMethods
                            , void* mioCtx
                            , int sendQLen) {

    int status = -1;

    MsgIoChannel_t* mioChannel = CreateStruct0 (MsgIoChannel_t);

    if (mioChannel == NULL) {
        // ??? log
    } else {
        mioChannel->mioCtx = mioCtx;
        mioChannel->mioMethods = *mioMethods;

        InitPool (&mioChannel->freeSendPool);
        InitPool (&mioChannel->actSendPool);
        int sendPoolErr = 0;
        for (int i=0; i<sendQLen; i++) {
            MsgIoDataBuff_t* mioDataBuff = CreateStruct (MsgIoDataBuff_t);
            if (mioDataBuff == NULL) {
                sendPoolErr = -1;
                break;
            }
            mioDataBuff->data 
                = mioDataBuff->msgBuff + MSG_IO_MESSAGEL_LENGTH_BYTES;
            AddToPool (&mioChannel->freeSendPool, mioDataBuff);
        }

        if (sendPoolErr) {
            // ??? log
        } else {
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
            iovOptions.maxEvents = 0;
            iovOptions.eventPTO = 0;

            mioChannel->iovCtx 
                = CreateIoVentCtx (&iovMethods, &iovOptions, NULL);

            if (mioChannel->iovCtx == NULL) {
                // ??? log
            } else {
                mioChannel->statsCount = 1;
                mioChannel->statsArr[0] = (SockStats_t*) &mioChannel->cStats; 
                int newConnInitErr = 
                    NewConnection (mioChannel->iovCtx
                                    , mioChannel
                                    , localAddress
                                    , NULL
                                    , remoteAddress
                                    , 0
                                    , mioChannel->statsArr
                                    , mioChannel->statsCount);
                if (newConnInitErr) {
                    // ??? log
                } else {
                    status = 0;
                }
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
        DisableReadWriteNotification (mioChannel->iovConn);
        mioChannel->iovConn = NULL;
    }

    MsgIoCleanup (mioChannel);
}
