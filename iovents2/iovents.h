#ifndef __IOV1_H__
#define __IOV1_H__

#include "platform.h"

#define STATUS_GET_SSL_CTX                                  1

#define CONNAPP_STATE_INIT                               0
#define CONNAPP_STATE_CONNECTION_IN_PROGRESS             1
#define CONNAPP_STATE_CONNECTION_ESTABLISHED             2
#define CONNAPP_STATE_CONNECTION_ESTABLISH_FAILED        3
#define CONNAPP_STATE_CONNECTION_CLOSED                  4
#define CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS         5
#define CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED         6
#define CONNAPP_STATE_SSL_CONNECTION_ESTABLISH_FAILED    7

#define ON_CLOSE_ERROR_NONE                                  0
#define ON_CLOSE_ERROR_UNKNOWN                              -1
#define ON_CLOSE_ERROR_GENERAL                              -2
#define ON_CLOSE_ERROR_TCP_TIMEOUT                          -3
#define ON_CLOSE_ERROR_TCP_RESET                            -4

#define MARK_EOF_WITH_TCP_RST                               0x00000001
#define MARK_EOF_WITH_TCP_FIN                               0x00000002
#define MARK_EOF_SEND_SSL_CLOSE_NOTIFY                      0x00000004
#define MARK_EOF_WAIT_SSL_CLOSE_NOTIFY                      0x00000008

typedef struct IoVentConnInfo {

    SSL* cSSL;
    void* connCtx;
    struct IoVentCtx* iovCtx;
    SockAddr_t* localAddress;
    LocalPortPool_t* localPortPool;
    SockAddr_t* remoteAddress;
    SockStats_t** statsArr;
    int statsCount;

    char* writeBuffer;
    int writeBuffOffset;
    int writeDataLen;

    int writeBuffOffsetCur;
    int writeDataLenCur;
    int writtenBytesLenCur;

    char* readBuffer;
    int readBuffOffset;
    int readDataLen;

    int readBuffOffsetCur;
    int readDataLenCur;
    int readBytesLenCur;

} IoVentConnInfo_t;

typedef struct IoVentConn {

    SockState_t ccState; 
    int socketFd;

    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    SockAddr_t localAddressAccept;
    SockAddr_t remoteAddressAccept;
    
    IoVentConnInfo_t cInfo;

} IoVentConn_t;

typedef struct IoVentMethods {
    void (*OnEstablish) (IoVentConn_t* iovConn); 

    void (*OnWriteNext) (IoVentConn_t* iovConn); 

    void (*OnReadNext) (IoVentConn_t* iovConn); 

    void (*OnCleanup) (IoVentConn_t* iovConn);

    void (*OnStatus) (IoVentConn_t* iovConn);

    void (*OnWriteStatus) (IoVentConn_t* iovConn
                            , int bytesSent);

    void (*OnReadStatus) (IoVentConn_t* iovConn
                            , int bytesReceived);

    int (*OnContinue) (void* appData);

} IoVentMethods_t;

typedef struct IoVentOptions {
    uint32_t maxActiveConnections; 
    uint32_t maxErrorConnections;
    uint32_t maxEvents;
    uint32_t eventPTO;
} IoVentOptions_t;

typedef struct IoVentCtx {

    IoVentOptions_t options;
    IoVentMethods_t methods;
    ConnectionPool_t* freeConnectionPool; 
    ConnectionPool_t* activeConnectionPool;
    ConnectionPool_t* cleanupConnectionPool;
    ConnectionPool_t* abortConnectionPool;
    ConnectionPool_t* eventConnectionPool;

    uint32_t errorConnectionCount;
    IoVentConn_t* errorConnectionArr;

    PollEvent_t* EventArr;
    int eventQ;

    TimerWheel_t* timerWheel;

    void* appCtx;

} IoVentCtx_t;

enum ToContinueAppState { EmAppExit
                            , EmAppContinue
                            , EmAppContinueZeroActive};

void DumpConnection (IoVentConn_t* newConn);

void DumpErrConnections (IoVentCtx_t* iovCtx);

IoVentCtx_t* CreateIoVentCtx (IoVentMethods_t* methods
                        , IoVentOptions_t* options
                        , void* appCtx);

void InitIoVentCtx (IoVentCtx_t* iovCtx
                    , IoVentMethods_t* methods
                    , IoVentOptions_t* options
                    , void* appCtx);

void DeleteIoVentCtx (IoVentCtx_t* iovCtx);

double TimeElapsedIoVentCtx(IoVentCtx_t* iovCtx);

int ProcessIoVent (IoVentCtx_t* iovCtx);

int NewConnection (IoVentCtx_t* iovCtx
                        , void* connCtx
                        , SockAddr_t* localAddress
                        , LocalPortPool_t* localPortPool 
                        , SockAddr_t* remoteAddress
                        , int connLifetime
                        , SockStats_t** statsArr
                        , int statsCount);

void InitServer (IoVentCtx_t* iovCtx
                        , void* groupCtx
                        , SockAddr_t* localAddress
                        , SockStats_t** statsArr
                        , int statsCount);

void InitSslConnection(IoVentConn_t* newConn
                        , SSL* newSSL
                        , int isClient);

#define IOV_GET_SSL(__iovconn) (__iovconn)->cInfo.cSSL
#define IOV_GET_CONN_CTX(__iovconn) (__iovconn)->cInfo.connCtx
#define IOV_SET_CONN_CTX(__iovconn,__ctx) (__iovconn)->cInfo.connCtx = __ctx 

void SslClientInit (IoVentConn_t* newConn
                        , SSL* newSSL);

void SslServerInit (IoVentConn_t* newConn
                        , SSL* newSSL);

void WriteNextData (IoVentConn_t* newConn
                        , char* writeBuffer
                        , int writeBuffOffset
                        , int writeDataLen
                        , int partialWrite);

void ReadNextData (IoVentConn_t* newConn
                        , char* readBuffer
                        , int readBuffOffset
                        , int readDataLen
                        , int partialRead);

void EnableReadNotification (IoVentConn_t* newConn);

void DisableReadNotification (IoVentConn_t* newConn);

void EnableWriteNotification (IoVentConn_t* newConn);

void DisableWriteNotification (IoVentConn_t* newConn);

void EnableReadWriteNotification (IoVentConn_t* newConn);

void DisableReadWriteNotification (IoVentConn_t* newConn);

void EnableReadOnlyNotification (IoVentConn_t* newConn);

void EnableWriteOnlyNotification (IoVentConn_t* newConn);

void WriteClose (IoVentConn_t* newConn);

void AbortConnection (IoVentConn_t* newConn);

#define IOVENT_SSL(__iovconn) ((__iovconn)->cInfo.cSSL)

#endif