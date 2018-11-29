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
    newConn->localPortPool = NULL;  

    newConn->statusId = 0;
    newConn->statusData = NULL;

    newConn->statusResponseId = 0;
    newConn->statusResponseData = NULL;

    newConn->appData = NULL;   
}

IoVentConn_t* GetFreeConnection (IoVentCtx_t* iovCtx) {

    IoVentConn_t* newConn 
        = GetFromPool (iovCtx->freeConnectionPool);

    if (newConn != NULL) {
        AddToPool (iovCtx->activeConnectionPool, newConn);
        InitConnection (newConn);
    }

    return newConn;
}

void SetFreeConnection (IoVentConn_t* newConn) {
    RemoveFromPool (newConn->iovCtx->activeConnectionPool, newConn);
    AddToPool (newConn->iovCtx->freeConnectionPool, newConn);
}

static void StoreErrConnection (IoVentConn_t* newConn) {
    int currentErrCount = newConn->iovCtx->errorConnectionCount;
    if (currentErrCount 
                < newConn->iovCtx->options.maxErrorConnections) {

        IoVentConn_t* errConn 
            = &newConn->iovCtx->errorConnectionArr[currentErrCount];

        *errConn =  *newConn;

        errConn->savedLocalPort 
            = ntohs(GetSockPort(errConn->localAddress));

        errConn->savedRemotePort 
            = ntohs(GetSockPort(errConn->remoteAddress));

        newConn->iovCtx->errorConnectionCount++;
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
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in6*)newConn->localAddress)->sin6_port);
    } else {
        SetPortToPool (newConn->localPortPool
            , ((struct sockaddr_in*)newConn->localAddress)->sin_port);
    }
}

static void RemoveConnection(IoVentConn_t* newConn) {

    if ( IsSetCES(newConn, STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {
        StopPollReadWriteEvent(newConn->iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->summaryStats
                                , newConn->groupStats
                                , newConn);
    }       

    TcpClose(newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {
        StoreErrConnection (newConn);
    }

    SetFreeConnection (newConn);

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
                        , newConn->groupStats
                        , newConn);

    if (IsSetCS1(newConn, STATE_SSL_CONN_ESTABLISHED)) {
        SetAppState (newConn
                    , CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED);
    } else if ( GetCES(newConn) ) {
        SetAppState (newConn
                    , CONNAPP_STATE_SSL_CONNECTION_ESTABLISH_FAILED);
        CloseConnection(newConn);
    }  
}

void NewConnection (IoVentCtx_t* iovCtx
                        , SockAddr_t* localAddress
                        , LocalPortPool_t* localPortPool 
                        , SockAddr_t* remoteAddress
                        , void* aStats
                        , void* bStats
                        , int isSSL) {

    IoVentConn_t* newConn = GetFreeConnection (iovCtx); 
    
    if (newConn) {

        newConn->localAddress = localAddress;
        newConn->localPortPool = localPortPool;
        SetAppState(newConn, CONNAPP_STATE_CONNECTION_IN_PROGRESS);

        AssignSocketLocalPort(newConn->localAddress
                            , newConn->localPortPool
                            , aStats
                            , bStats
                            , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
            SetFreeConnection (newConn);
        } else {

            newConn->socketFd 
                = TcpNewConnection(newConn->localAddress
                        , newConn->remoteAddress
                        , aStats
                        , bStats
                        , newConn);

            if ( GetCES(newConn) ) {
                StoreErrConnection (newConn);
                SetFreeConnection (newConn);
                ReleasePort (newConn);
            } else {
                PollWriteEventOnly(iovCtx->eventQ
                                    , newConn->socketFd
                                    , aStats
                                    , bStats
                                    , newConn);

                if ( GetCES(newConn) ) {
                    CloseConnection(newConn);
                }
            }
        }
    } else {
        //todo
    }
}

static void InitSslConnection(IoVentConn_t* newConn) {

    SetAppState(newConn, CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS);

    HandleSslConnect (newConn);
}

static void OnTcpConnectionCompletion (IoVentConn_t* newConn) {

    VerifyTcpConnectionEstablished (newConn->socketFd
                                    , newConn->summaryStats
                                    , newConn->groupStats
                                    , newConn);
    
    if ( GetCES(newConn) ) {
        SetAppState (newConn, CONNAPP_STATE_CONNECTION_ESTABLISH_FAILED);
        CloseConnection(newConn);
    } else {
        SetAppState (newConn, CONNAPP_STATE_CONNECTION_ESTABLISHED);

        PollReadWriteEvent(newConn->iovCtx->eventQ
                            , newConn->socketFd
                            , newConn->summaryStats
                            , newConn->groupStats
                            , newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        }
    }
}

static void OnSslConnectionCompletion (IoVentConn_t* newConn) {
    HandleSslConnect (newConn);
}

static void OnWriteNext (IoVentConn_t* newConn) {
    (*newConn->iovCtx->methods.OnWriteNext)(newConn);
}

static void OnReadNext (IoVentConn_t* newConn) {
    (*newConn->iovCtx->methods.OnReadNext)(newConn);
}

static void InitIoVentCtx (IoVentCtx_t* iovCtx
                    , IoVentMethods_t* methods
                    , IoVentOptions_t* options) {

    iovCtx->methods = *methods;

    iovCtx->options = *options;

    if (iovCtx->options.maxEvents == 0) {
        iovCtx->options.maxEvents = DEFAULT_MAX_EVENTS;
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
                                , iovCtx->options.maxEvents);

    iovCtx->eventQ = CreateEventQ();

    iovCtx->eventPTO = 0;
    
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();
} 

IoVentCtx_t* CreateIoVentCtx (IoVentMethods_t* methods
                            , IoVentOptions_t* options) {

    IoVentCtx_t* iovCtx = CreateStruct0 (IoVentCtx_t);

    InitIoVentCtx (iovCtx, methods, options);

    return iovCtx;
}

void DeleteIoVentCtx (IoVentCtx_t* iovCtx) {
}

int ProcessIoVent (IoVentCtx_t* iovCtx) {

    int eCount = GetIOEvents(iovCtx->eventQ
                                , iovCtx->EventArr
                                , iovCtx->options.maxEvents
                                , iovCtx->eventPTO);
    if (eCount > 0) {

        iovCtx->eventPTO = 0;

        for(int eIndex = 0; eIndex < eCount; eIndex++) {

            IoVentConn_t* newConn 
                = (IoVentConn_t*) 
                    GetIOEventData(iovCtx->EventArr[eIndex]);

            //Handle Tcp Connect
            if ( (GetAppState(newConn) 
                        == CONNAPP_STATE_CONNECTION_IN_PROGRESS)
                    &&  IsWriteEventSet(iovCtx->EventArr[eIndex]) ) {
                
                OnTcpConnectionCompletion (newConn);

            //Handle SSL Connect Init
            } else if ( (GetAppState(newConn) 
                        == CONNAPP_STATE_CONNECTION_ESTABLISHED)
                    && IsWriteEventSet(iovCtx->EventArr[eIndex]) ) {

                InitSslConnection (newConn);

            //Handle SSL Connect Handshake
            } else if (GetAppState(newConn) 
                        == CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS) {

                int finishSslHandshake = 0;

                if ( IsSetCS1(newConn, STATE_SSL_HANDSHAKE_WANT_WRITE) 
                        && IsWriteEventSet(iovCtx->EventArr[eIndex]) ) {
                    finishSslHandshake = 1;
                    ClearCS1(newConn, STATE_SSL_HANDSHAKE_WANT_WRITE);   
                } 
                
                if ( IsSetCS1(newConn, STATE_SSL_HANDSHAKE_WANT_READ) 
                        && IsReadEventSet(iovCtx->EventArr[eIndex]) ) {
                    finishSslHandshake = 1;
                    ClearCS1(newConn, STATE_SSL_HANDSHAKE_WANT_READ);  
                }

                if ( finishSslHandshake ) {
                    OnSslConnectionCompletion (newConn);
                }

            // Handle Read, Write Data
            } else if ( GetAppState(newConn) 
                        == CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED ) {

                // Handle Write
                if ( IsWriteEventSet(iovCtx->EventArr[eIndex]) 
                                    && !IsFdClosed(newConn) ) {
                    
                    if ( IsSetCS1(newConn,  STATE_NO_MORE_WRITE_DATA) ) {
                        CloseConnection (newConn);
                    } else {
                        OnWriteNext (newConn);
                    }
                }

                // Handle Read
                if (IsReadEventSet(iovCtx->EventArr[eIndex])
                                    && !IsFdClosed(newConn) ) {

                    OnReadNext (newConn);

                }
            }
        }
    }else{
        if (iovCtx->eventPTO < MAX_POLL_TIMEOUT) {
            iovCtx->eventPTO++;
        }
    }
    
    if ( IsPoolEmpty (iovCtx->activeConnectionPool) ) {
        return 0;
    }

    return 1;
}
