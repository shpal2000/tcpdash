#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"

#define MSG_IO_MESSAGEL_LENGTH_BYTES            7
#define MSG_IO_READ_WRITE_DATA_MAXLEN           1048576
#define MSG_IO_READ_WRITE_BUFF_MAXLEN           1048583

#define MSG_IO_ON_MESSAGE_STATE_READ_LENGTH      1
#define MSG_IO_ON_MESSAGE_STATE_READ_DATA        2

typedef void* MsgIoChannelId_t;

typedef struct MsgIoDataBuff {
    char* data;
    int len;
} MsgIoDataBuff_t;

typedef struct MsgIoChannelStats {
    SockStats_t connStats;
} MsgIoChannelStats_t;

typedef struct MsgIoMethods {

    void (*OnOpen) (MsgIoChannelId_t mioChanelId); 

    void (*OnError) (MsgIoChannelId_t mioChanelId);

    void (*OnMsgRecv) (MsgIoChannelId_t mioChanelId);

    void (*OnMsgSent) (MsgIoChannelId_t mioChanelId);

} MsgIoMethods_t;

typedef struct MsgIoChannel {
    IoVentConn_t* iovConn;
    IoVentCtx_t* iovCtx;
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

MsgIoChannelId_t MsgIoNew (SockAddr_t* localAddress
                            , SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mioMethods);

void MsgIoDelete (MsgIoChannelId_t mioChanelId);

MsgIoDataBuff_t* MsgIoGetRecvBuff (MsgIoChannelId_t mioChanelId);

MsgIoDataBuff_t* MsgIoGetSendBuff (MsgIoChannelId_t mioChanelId);
void MsgIoSendNextInit (MsgIoChannelId_t mioChanelId);

#endif

