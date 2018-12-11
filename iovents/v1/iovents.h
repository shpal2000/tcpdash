#ifndef __IOV1_H__
#define __IOV1_H__

#include "platform/common.h"

#define DEFAULT_MAX_EVENTS                                  1000

#define STATUS_GET_SSL_CTX                                  1
#define STATUS_GET_SSL_CTX                                  1

#define CONNAPP_STATE_INIT                               0
#define CONNAPP_STATE_CONNECTION_IN_PROGRESS             1
#define CONNAPP_STATE_CONNECTION_ESTABLISHED             2
#define CONNAPP_STATE_CONNECTION_ESTABLISH_FAILED        3
#define CONNAPP_STATE_CONNECTION_CLOSED                  4
#define CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS         5
#define CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED         6
#define CONNAPP_STATE_SSL_CONNECTION_ESTABLISH_FAILED    7

typedef struct IoVentConnInfo {

    SSL* cSSL;

    void* connData;
    void* sessionData;
    void* groupCtx;
    void* appCtx;
    struct IoVentCtx* iovCtx;

    char* writeBuffer;
    int writeBuffOffset;
    int writeDataLen;

    int writeBuffOffsetCur;
    int writeDataLenCur;
    int writtenBytesLenCur;

    char* readBuffer;
    int readBuffOffset;
    int readDataLen;

    SockStats_t* groupStats;
    SockStats_t* summaryStats;

    SockAddr_t* localAddress;
    SockAddr_t* remoteAddress;

    LocalPortPool_t* localPortPool;

} IoVentConnInfo_t;

typedef struct IoVentConn {

    SockState_t ccState; 
    int socketFd;

    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    SockAddr_t remoteAddressAccept;
    
    uint32_t statusId;
    void* statusData;

    uint32_t statusResponseId;
    void* statusResponseData;

    int bytesSent; //remove this to connData

    IoVentConnInfo_t cInfo;

} IoVentConn_t;

typedef struct IoVentMethods {
    void (*OnEstablish) (IoVentConn_t* iovConn); 

    void (*OnWriteNext) (IoVentConn_t* iovConn); 

    void (*OnReadNext) (IoVentConn_t* iovConn); 

    void (*OnClose) (IoVentConn_t* iovConn);

    void (*OnCleanup) (IoVentConn_t* iovConn);

    void (*OnStatus) (IoVentConn_t* iovConn);

    void (*OnWriteStatus) (IoVentConn_t* iovConn
                            , int bytesSent);

    void (*OnReadStatus) (IoVentConn_t* iovConn
                            , int bytesReceived);

} IoVentMethods_t;

typedef struct IoVentOptions {
    uint32_t maxActiveConnections; 
    uint32_t maxErrorConnections;
    uint32_t maxEvents; 
} IoVentOptions_t;

typedef struct IoVentCtx {
    IoVentOptions_t options;
    IoVentMethods_t methods;
    ConnectionPool_t* freeConnectionPool; 
    ConnectionPool_t* activeConnectionPool;

    uint32_t errorConnectionCount;
    IoVentConn_t* errorConnectionArr;

    PollEvent_t* EventArr;
    int eventQ;
    int eventPTO;
} IoVentCtx_t;

void DumpErrConnections (IoVentCtx_t* iovCtx);

IoVentCtx_t* CreateIoVentCtx (IoVentMethods_t* methods
                        , IoVentOptions_t* options);

void DeleteIoVentCtx (IoVentCtx_t* iovCtx);

int ProcessIoVent (IoVentCtx_t* iovCtx);

void NewConnection (IoVentCtx_t* iovCtx
                        , void* groupCtx
                        , void* appCtx
                        , void* sessionData
                        , SockAddr_t* localAddress
                        , LocalPortPool_t* localPortPool 
                        , SockAddr_t* remoteAddress
                        , void* aStats
                        , void* bStats);

void InitServer (IoVentCtx_t* iovCtx
                        , void* groupCtx
                        , void* appCtx
                        , SockAddr_t* localAddress
                        , void* aStats
                        , void* bStats);
                    
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
                        , int readDataLen);

#endif