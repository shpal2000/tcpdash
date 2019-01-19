#ifndef __MSG_IO_H
#define __MSG_IO_H

#include "iovents.h"
#include "apps/common.h"

#define MSG_IO_MESSAGEL_LENGTH_BYTES            8
#define MSG_IO_MESSAGEL_LENGTH_FORMAT           "%07d" 
#define MSG_IO_READ_WRITE_DATA_MAXLEN           1048576
#define MSG_IO_READ_WRITE_BUFF_MAXLEN           1048585

#define MSG_IO_ON_MESSAGE_STATE_READ_LENGTH      1
#define MSG_IO_ON_MESSAGE_STATE_READ_DATA        2

typedef struct MsgIoDataBuff {
    char* data;
    int len;
} MsgIoDataBuff_t;

typedef struct MsgIoChannel {
    IoVentConn_t* iovConn;
    IoVentCtx_t* iovCtx;
    void* mioCtx;
    MsgIoMethods_t mioMethods;
    MsgIoChannelStats_t cStats;
    MsgIoChannelStats_t gStats;
    
    int onMsgState;
    int expectedRecvMsgLen;

    char recvBuff[MSG_IO_READ_WRITE_BUFF_MAXLEN];
    char sendBuff[MSG_IO_READ_WRITE_BUFF_MAXLEN];

    MsgIoDataBuff_t sendMsg;
    MsgIoDataBuff_t recvMsg;

} MsgIoChannel_t;

#endif

