#ifndef __TCP_CLIENT_SERVER_APP_H
#define __TCP_CLIENT_SERVER_APP_H

#include "apps/common.h"

#define MSG_IO_DELEMETER "--MsgIoDelemeter--"

typedef struct MsgIoChannel {
    IoVentConn_t* iovConn;
    IoVentCtx_t* iovCtx;
} MsgIoChannel_t;

typedef struct MsgIoMethods {

    void (*OnOpen) (MsgIoChannel_t* iovConn); 

    void (*OnClose) (MsgIoChannel_t* iovConn);

    void (*OnError) (MsgIoChannel_t* iovConn); 

    void (*OnMsg) (MsgIoChannel_t* iovConn); 

} MsgIoMethods_t;

MsgIoChannel_t* MsgIoNew (SockAddr_t* remoteAddress
                            , MsgIoMethods_t* mIoMethods);

void MsgIoDelete (MsgIoChannel_t* mIoConn);

void MsgIoSend (MsgIoChannel_t* mIoConn
                            , char* msgBuff
                            , int msgOffset
                            , int msgLen);

#endif

