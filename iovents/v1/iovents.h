#ifndef __IOV1_H__
#define __IOV1_H__

#include "platform/common.h"

#define DEFAULT_MAX_EVENTS                                  1000

#define STATUS_GET_SSL_CTX                                  1
#define STATUS_GET_SSL_CTX                                  1


typedef struct IoVentMethods {
    void (*OnEstablish) (IoVentConn_t*); 
    void (*OnWriteNext) (IoVentConn_t*); 
    void (*OnReadNext) (IoVentConn_t*); 
    void (*OnCleanup) (IoVentConn_t*);
    void (*OnStatus) (IoVentConn_t*);
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

typedef struct IoVentConn {

    SockState_t ccState; 
    int socketFd;

    SSL* cSSL;

    uint16_t savedLocalPort;
    uint16_t savedRemotePort;
    SockAddr_t* localAddress;
    SockAddr_t* remoteAddress;
    LocalPortPool_t* localPortPool;   
    
    uint32_t statusId;
    void* statusData;

    uint32_t statusResponseId;
    void* statusResponseData;
   
    void* appData;

    IoVentCtx_t* iovCtx;

    SockStats_t* groupStats;
    SockStats_t* summaryStats;

} IoVentConn_t;



IoVentCtx_t* CreateIoVentCtx (IoVentMethods_t* methods
                , IoVentOptions_t* options);

void DeleteIoVentCtx (IoVentCtx_t* iovCtx);

int ProcessIoVent (IoVentCtx_t* iovCtx);

#endif