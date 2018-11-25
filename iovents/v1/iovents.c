#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "iovents.h"

static void InitConnection (IoVentConn_t* newConn) {

    CSInit(newConn);

    newConn->socketFd = 0;
    newConn->cSSL = NULL;
    newConn->savedLocalPort = 0;
    newConn->savedRemotePort = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->localPortPool = NULL  

    newConn->statusId = ;
    newConn->statusData = NULL;

    newConn->statusResponseId = ;
    newConn->statusResponseData = NULL;

    newConn->appData = NULL;   
}

static IoVentConn_t* GetFreeConnection (IoVentCtx_t* iovCtx) {

    IoVentConn_t* newConn 
        = GetFromPool (iovCtx->freeConnectionPool);

    if (newConn != NULL) {
        AddToPool (iovCtx->activeConnectionPool, newConn);
        InitConnection (newConn);
    }

    return newConn;
}

static void SetFreeSession (IoVentConn_t* newConn) {
    
    RemoveFromPool (iovCtx->activeConnectionPool, newConn);
    AddToPool (iovCtx->freeConnectionPool, newConn);
}

static void StoreErrConnection (IoVentConn_t* newConn) {

    if (iovCtx->errorConnectionCount 
                < iovCtx->options.maxErrorConnections) {

        IoVentConn_t* errConn 
                = &iovCtx->errorConnectionArr[iovCtx->errorConnectionCount];

        *errConn =  *newConn;

        errSession->tcConn.savedLocalPort 
            = ntohs(GetSockPort(errSession->tcConn.localAddress));

        errSession->tcConn.savedRemotePort 
            = ntohs(GetSockPort(errSession->tcConn.remoteAddress));

        iovCtx->errorConnectionCount++;
    }
}

void DumpErrConnections (IoVentCtx_t* iovCtx) {

    if (iovCtx->errorConnectionCount) {

        char srcAddr[INET6_ADDRSTRLEN];
        char dstAddr[INET6_ADDRSTRLEN];

        for (int i = 0; i < iovCtx->errorConnectionCount; i++) {

            IoVentConn_t* errConn 
                    = &iovCtx->errorConnectionArr[i];

            AddressToString (errConn->localAddress, srcAddr);

            AddressToString (errConn->remoteAddress, dstAddr);

            printf ("SS1 = %#018" PRIx64 
                    ", ES = %#018" PRIx64 
                    ", SysErr = %d"
                    ", SockErr = %d"
                    ", src = %s : %hu"
                    ", dst = %s : %hu\n"
                    , GetCS1(errConn)
                    , GetCES(errConn)
                    , GetSysErrno(errConn) 
                    , GetSockErrno(errConn) 
                    , srcAddr, errConn->savedLocalPort
                    , dstAddr, errConn->savedRemotePort); 
        }
    }
}

static void ReleasePort(IoVentConn_t* newConn) {

    if (IsIpv6(newConn->localAddress)) {
        AddToPool (newConn->localPortPool
            , ((struct sockaddr_in6*)newConn->localAddress)->sin6_port);
    } else {
        AddToPool (newConn->localPortPool
            , ((struct sockaddr_in*)newConn->localAddress)->sin_port);
    }
}

static void RemoveConnection(IoVentConn_t* newConn) {

    if ( IsSetCES(newConn, STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {
        StopPollReadWriteEvent(iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->summaryStats
                                , newConn->grouStats
                                , newConn);
    }       

    TcpClose(newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess);
    }

    SetFreeSession (newConn->tcSess);

    if ( IsSetCES(newConn, STATE_TCP_SOCK_FD_CLOSE_FAIL
                            | STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {
        ReleasePort(newConn);
    }
}

static void CloseConnection(IoVentConn_t* newConn) {

    if ( GetCES(newConn) ) {

        RemoveConnection (newConn);

    } else {

        int pendingSendReceiveCloseNotify 
            = IsSetCS1(newConn, STATE_SSL_TO_SEND_SHUTDOWN) 
            || IsSetCS1(newConn, STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);

        if ( IsSetCS1(newConn, STATE_SSL_CONN_ESTABLISHED)
                && pendingSendReceiveCloseNotify ) {

            SSLShutdown (newConn->cSSL, newConn);
        }

        int sentCloseNotifyOrNotRequired 
            = IsSetCS1(newConn, STATE_SSL_SENT_SHUTDOWN) 
                || ( (IsSetCS1(newConn, STATE_SSL_TO_SEND_SHUTDOWN) == 0)
                    && (IsSetCS1(newConn, STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN) == 0) );


        int wrShutdownDone 
            = IsSetCS1 (newConn, STATE_TCP_SENT_FIN)
                || IsSetCES (newConn, STATE_TCP_FIN_SEND_FAIL); 

        if ( IsSetCS1(newConn, STATE_TCP_CONN_ESTABLISHED)
                && (wrShutdownDone == 0)
                && sentCloseNotifyOrNotRequired ) {

            TcpWrShutdown (newConn->socketFd, newConn);
        }

        if ( GetCES(newConn) 
                || (IsSetCS1(newConn, STATE_TCP_REMOTE_CLOSED)  
                     &&  wrShutdownDone) ) {

            RemoveConnection (newConn);
        }
    }
}

static void HandleSslConnect (IoVentConn_t* newConn) {

   DoSSLConnect (newConn->cSSL
                        , newConn->socketFd
                        , 1
                        , newConn->summaryStats
                        , newConn->grouStats
                        , newConn);

    if (IsSetCS1(newConn, STATE_SSL_CONN_ESTABLISHED)) {
        SetAppState (newConn
                    , APP_STATE_SSL_CONNECTION_ESTABLISHED);
    } else if ( GetCES(newConn) ) {
        SetAppState (newConn
                    , APP_STATE_SSL_CONNECTION_ESTABLISH_FAILED);
        CloseConnection(newConn);
    }  
}

static void OnTcpConnectionCompletion (IoVentConn_t* newConn) {

    VerifyTcpConnectionEstablished (newConn->socketFd
                                    , newConn->summaryStats
                                    , newConn->grouStats
                                    , newConn);
    
    if ( GetCES(newConn) ) {
        SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISH_FAILED);
        CloseConnection(newConn);
    } else {
        SetAppState (newConn, APP_STATE_CONNECTION_ESTABLISHED);

        PollReadWriteEvent(AppO->eventQ
                            , newConn->socketFd
                            , newConn->summaryStats
                            , newConn->grouStats
                            , newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        }
    }
}

static void OnSslConnectionCompletion (IoVentConn_t* newConn) {
    HandleSslConnect (newConn);
}

static void OnWriteNextData (IoVentConn_t* newConn) {

    if (newConn->bytesSent < AppI->csDataLen ) {

        int bytesToSend 
            = AppI->csDataLen - newConn->bytesSent;

        const char* sendBuffer 
            = &AppO->sendBuffer[newConn->bytesSent];

        int bytesSent 
            = SSLWrite (newConn->cSSL
                        , sendBuffer
                        , bytesToSend
                        , newConn->summaryStats
                        , newConn->grouStats
                        , newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        } else {
            if (bytesSent <= 0) {
                // ssl want read write; skip
            } else {
                //process written data
                newConn->bytesSent += bytesSent;

                if (newConn->bytesSent == AppI->csDataLen) {
                    SetCS1(newConn, STATE_NO_MORE_WRITE_DATA 
                                    | STATE_SSL_TO_SEND_SHUTDOWN
                                    | STATE_TCP_TO_SEND_FIN);
                }
            }
        }
    }
}

static void OnReadNextData (IoVentConn_t* newConn) {

    if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) == 0) {
        int bytesReceived 
            = SSLRead ( newConn->cSSL
                        , AppO->readBuffer
                        , AppI->csDataLen
                        , newConn->summaryStats
                        , newConn->grouStats
                        , newConn);
        
        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        } else {
            if (bytesReceived <= 0) {
                if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) ) {
                    CloseConnection(newConn);
                }
            } else {
                //process read data
            }
        }
    }
}

static void InitIoVentCtx (IoVentCtx_t* iovCtx
                    , IoVentMethods_t* methods
                    , IoVentOptions_t* options) {

    iovCtx->methods = *methods;

    iovCtx->options = *options;

    if (iovCtx->options->maxEvents == 0) {
        iovCtx->options->maxEvents = DEFAULT_MAX_EVENTS;
    }
    
    iovCtx->freeConnectionPool = AllocEmptyPool ();
    
    iovCtx->activeConnectionPool = AllocEmptyPool ();

    for (int i = 0; i < iovCtx->options.maxActiveConnections; i++) {

        IoVentConn_t* newConn = CreateStruct (IoVentConn_t);

        InitConnection (newConn);

        AddToPool (iovCtx->freeConnectionPool, newConn);
    }

    iovCtx->errorConnectionCount = 0;
    
    iovCtx->errorConnectionArr 
        = CreateArray (IoVentConn_t, iovCtx->options.maxErrorConnections); 

    iovCtx->EventArr = CreateArray(PollEvent_t
                                , iovCtx->options->maxEvents);

    iovCtx->eventQ = CreateEventQ();

    iovCtx->eventPTO = 0;
    
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();
} 

IoVentCtx_t* CreateIoVentCtx (IoVentMethods_t* methods
                            , IoVentOptions_t* options) {

    iovCtx = CreateStruct0 (IoVentCtx_t);

    InitIoVentCtx (iovCtx, methods, options);

    return iovCtx;
}

void DeleteIoVentCtx (IoVentCtx_t* iovCtx) {

}

int ProcessIoVent (IoVentCtx_t* app) {

}

