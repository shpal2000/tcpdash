#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"

#define MSG_IO_DELEMETER "--MsgIoDelemeter--"

#define MSG_IO_MESSAGEL_LENGTH_BYTES            7                    
#define MSG_IO_READ_WRITE_BUFF_MAXLEN           1000000

#define MSG_IO_ON_MESSAGE_STATE_READ_LENGTH      1
#define MSG_IO_ON_MESSAGE_STATE_READ_DATA        2

typedef void* MsgIoChannelId_t;

typedef struct MsgIoLenBuff {
    int buffLen;
    int buffOffset;
    int dataLen;
    char dataBuff[MSG_IO_MESSAGEL_LENGTH_BYTES];
} MsgIoLenBuff_t;

typedef struct MsgIoDataBuff {
    int buffLen;
    int buffOffset;
    int dataLen;
    char dataBuff[MSG_IO_READ_WRITE_BUFF_MAXLEN];
} MsgIoDataBuff_t;

typedef struct MsgIoChannelStats {
    SockStats_t connStats;
} MsgIoChannelStats_t;

typedef struct MsgIoMethods {

    void (*OnOpen) (MsgIoChannelId_t mioChanelId); 

    void (*OnClose) (MsgIoChannelId_t mioChanelId);

    void (*OnError) (MsgIoChannelId_t mioChanelId); 

    void (*OnMsg) (MsgIoChannelId_t mioChanelId); 

} MsgIoMethods_t;

typedef struct MsgIoChannel {
    IoVentConn_t* iovConn;
    IoVentCtx_t* iovCtx;
    MsgIoMethods_t ioChannelMethods;
    MsgIoChannelStats_t cStats;
    MsgIoChannelStats_t gStats;
    
    int onMsgState;
    int rcvMsgLen; 
    MsgIoLenBuff_t msgLenBuff;
    MsgIoDataBuff_t msgDataBuff;

} MsgIoChannel_t;

MsgIoChannelId_t MsgIoNew (SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mIoMethods);

void MsgIoDelete (MsgIoChannelId_t mioChanelId);

void MsgIoSend (MsgIoChannelId_t mioChanelId
                            , char* msgBuff
                            , int msgOffset
                            , int msgLen);

#endif

