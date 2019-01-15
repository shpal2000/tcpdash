#include <sys/mman.h>
#include "iovents.h"

#include "msg_io.h"

void MsgIoResetRecvBuff (MsgIoChannelId_t mioChanelId) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) mioChanelId;

    mioChannel->recvMsg.len = 0;
    onMsgState->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_LENGTH;
}

void MsgIoEmitSendBuff (MsgIoChannelId_t mioChanelId) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) mioChanelId;

}

static void OnEstablish (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {

        (*mioChanel->ioChannelMethods.OnError)( (MsgIoChannelId_t) mioChanel);

    } else {

        MsgIoResetRecvBuff ((MsgIoChannelId_t) mioChanel);

        (*mioChanel->ioChannelMethods.OnOpen)( (MsgIoChannelId_t) mioChanel);
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    char* readBuff;
    int readLen;

    if (onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

        readBuff = mioChannel->recvBuff;
        readLen = MSG_IO_MESSAGEL_LENGTH_BYTES;

    } else {

        readBuff = mioChanel->recvMsg.data;
        readLen =  mioChanel->recvMsg.len;
    }

    ReadNextData (iovConn
                , readBuff
                , 0
                , readLen
                , 0);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;
    
    if (bytesRead > 0) {

        if (onMsgState->onMsgState == MSG_IO_ON_MESSAGE_STATE_READ_LENGTH) {

            char *endptr;
            mioChanel->recvBuff[MSG_IO_MESSAGEL_LENGTH_BYTES-1] = '\0'
            mioChannel->recvMsg.len = (int) strtol (mioChanel->recvBuff
                                                            ,  &endptr
                                                            , 10);
            
            if ( (errno != 0 && mioChanel->rcvMsgLen == 0)
                    || (endptr == mioChanel->msgLenBuff.dataBuff)
                    || (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
                    || (mioChanel->recvMsg.len < 0)
                    || (mioChanel->recvMsg.len > MSG_IO_READ_WRITE_DATA_MAXLEN) ) {

                AbortConnection (iovConn);

                (*mioChanel->ioChannelMethods.OnError)((MsgIoChannelId_t) mioChanel);

            } else {

                onMsgState->onMsgState = MSG_IO_ON_MESSAGE_STATE_READ_DATA; 
            }

        } else {

            (*mioChanel->ioChannelMethods.OnMsgRecv)( (MsgIoChannelId_t) mioChanel);
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

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

    WriteNextData (iovConn
                    , mioChannel->sendMsg.data
                    , 0
                    , mioChannel->sendMsg.len
                    , 0);

}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    MsgIoChannel_t* mioChanel 
        = (MsgIoChannel_t*) iovConn->cInfo.sessionData;

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