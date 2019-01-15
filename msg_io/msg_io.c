#include <sys/mman.h>
#include "iovents.h"

#include "msg_io.h"

static void InitMsgIoLenBuff (MsgIoLenBuff_t* newBuff) {

    newBuff->buffLen = MSG_IO_MESSAGEL_LENGTH_BYTES;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void InitMsgIoDataBuff (MsgIoDataBuff_t* newBuff) {

    newBuff->buffLen = MSG_IO_READ_WRITE_BUFF_MAXLEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void SetNextReadMsg (MsgIoChannel_t* mioChanel) {

    mioChanel->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_LENGTH;

    InitMsgIoLenBuff (&mioChanel->msgLenBuff);

    InitMsgIoDataBuff (&mioChanel->msgDataBuff);
}

static void OnEstablish (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {

        (*mioChanel->ioChannelMethods.OnError)( (MsgIoChannelId_t) mioChanel );

    } else {

        SetNextReadMsg (mioChanel);

        (*mioChanel->ioChannelMethods.OnOpen)( (MsgIoChannelId_t) mioChanel );
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    int nextReadBuff;
    int nextReadOff;
    int nextReadLen;

    if (onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

        nextReadBuff = mioChanel->msgLenBuff.dataBuff;
        nextReadOff = mioChanel->msgLenBuff.buffOffset;
        nextReadLen = MSG_IO_MESSAGEL_LENGTH_BYTES - mioChanel->msgLenBuff.dataLen;

    } else {

        nextReadBuff = mioChanel->msgDataBuff.dataBuff;
        nextReadOff = mioChanel->msgDataBuff.buffOffset;
        nextReadLen =  mioChanel->rcvMsgLen - mioChanel->msgDataBuff.dataLen;
    }

    ReadNextData (iovConn
                , nextReadBuff
                , nextReadOff
                , nextReadLen);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
    
    if (bytesRead > 0) {

        if (onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

            mioChanel->msgLenBuff.dataLen += bytesRead;

            if (mioChanel->msgLenBuff.dataLen == MSG_IO_MESSAGEL_LENGTH_BYTES) {

                onMsgState->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_DATA;

                mioChanel->msgLenBuff.dataBuff[mioChanel->msgLenBuff.dataLen -1] = '\0';

                char *endptr;
                mioChanel->rcvMsgLen = (int) strtol (mioChanel->msgLenBuff.dataBuff
                                                ,  &endptr
                                                , 10);
                
                if ( (errno != 0 && mioChanel->rcvMsgLen == 0)
                        || (endptr == mioChanel->msgLenBuff.dataBuff)
                        || (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
                        || (mioChanel->rcvMsgLen <= 0)
                        || (mioChanel->rcvMsgLen >= MSG_IO_READ_WRITE_BUFF_MAXLEN) ) {

                    AbortConnection (iovConn);

                    (*mioChanel->ioChannelMethods.OnError)((MsgIoChannelId_t) mioChanel);

                }
            }

        } else {

            mioChanel->msgDataBuff.dataLen += bytesRead;

            if (mioChanel->msgDataBuff.dataLen == mioChanel->rcvMsgLen) {

                (*mioChanel->ioChannelMethods.OnMsg)( (MsgIoChannelId_t) mioChanel
                                                    , mioChanel->msgDataBuff.dataBuff
                                                    , mioChanel->msgDataBuff.dataLen);

                SetNextReadMsg (mioChanel);

            }
        }


    } else {

        int closeErr = bytesRead;   

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
            (*mioChanel->ioChannelMethods.OnError)((MsgIoChannelId_t) mioChanel);

        } else {
            WriteClose (iovConn);
            (*mioChanel->ioChannelMethods.OnClose)((MsgIoChannelId_t) mioChanel);
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