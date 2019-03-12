#ifndef __MSG_IO_H
#define __MSG_IO_H

#include "iovents.h"

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

typedef void* MsgIoChannelId_t;


typedef struct MsgIoMethods {

    void (*OnOpen) (MsgIoChannelId_t mioChanelId); 

    void (*OnError) (MsgIoChannelId_t mioChanelId);

    void (*OnMsgRecv) (MsgIoChannelId_t mioChanelId);

    void (*OnMsgSent) (MsgIoChannelId_t mioChanelId);

} MsgIoMethods_t;

typedef struct MsgIoChannelStats {
    SockStats_t connStats;
} MsgIoChannelStats_t;

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

MsgIoChannelId_t MsgIoNew (SockAddr_t* localAddress
                            , SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mioMethods
                            , void* mioCtx);
void MsgIoDelete (MsgIoChannelId_t mioChanelId);
void MsgIoRecv (MsgIoChannelId_t mioChanelId, char** pMsg, int* pLen);
void MsgIoSend (MsgIoChannelId_t mioChanelId, const char* msg, int msg_len);
void MsgIoProcess (MsgIoChannelId_t mioChannelId);
void* MsgIoGetCtx (MsgIoChannelId_t mioChannelId);
double MsgIoTimeElapsed (MsgIoChannelId_t mioChannelId);

#endif

