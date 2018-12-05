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

    newConn->connData = NULL;   
    newConn->sessionData = NULL;   
    newConn->appCtx = NULL;

    newConn->writeBuffer = NULL;
    newConn->writeBuffOffset = 0;
    newConn->writeDataLen = 0;

    newConn->readBuffer = NULL;
    newConn->readBuffOffset = 0;
    newConn->readDataLen = 0;

    newConn->bytesSent = 0;
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
            = ntohs(GetSockPort(&errConn->localAddress));

        errConn->savedRemotePort 
            = ntohs(GetSockPort(&errConn->remoteAddress));

        newConn->iovCtx->errorConnectionCount++;
    }
}

static void DumpConnection (IoVentConn_t* newConn) {

    char srcAddr[INET6_ADDRSTRLEN];
    char dstAddr[INET6_ADDRSTRLEN];

    AddressToString (newConn->localAddress, srcAddr);

    AddressToString (newConn->remoteAddress, dstAddr);

    printf ("SS1 = %#018" PRIx64 
            ", ES = %#018" PRIx64 
            ", SysErr = %d"
            ", SockErr = %d"
            ", src = %s : %hu"
            ", dst = %s : %hu\n"
            , GetCS1(newConn)
            , GetCES(newConn)
            , GetSysErrno(newConn) 
            , GetSockErrno(newConn) 
            , srcAddr, newConn->savedLocalPort
            , dstAddr, newConn->savedRemotePort);
}

void DumpErrConnections (IoVentCtx_t* iovCtx) {

    if (iovCtx->errorConnectionCount) {

        for (int i = 0; i < iovCtx->errorConnectionCount; i++) {

            IoVentConn_t* errConn 
                    = &iovCtx->errorConnectionArr[i];

            DumpConnection (errConn);
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

    // DumpConnection (newConn);

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

    //only for client connection
    if ( newConn->localPortPool 
            && ( IsSetCES(newConn, STATE_TCP_SOCK_FD_CLOSE_FAIL
                        | STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) ) {
        ReleasePort(newConn);
    }

    SetFreeConnection (newConn);

    (*newConn->iovCtx->methods.OnCleanup)(newConn->appCtx
                                                , newConn); 
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

static void DoSslHandshake (IoVentConn_t* newConn) {
    DoSSLConnect (newConn->cSSL
                        , newConn->socketFd
                        , IsSetCS1 (newConn, STATE_SSL_CONN_CLIENT) ? 1 : 0
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

static void OnConnectionEstablshedHelper (IoVentConn_t* newConn) {
    SetAppState (newConn, CONNAPP_STATE_CONNECTION_ESTABLISHED);

    PollReadWriteEvent(newConn->iovCtx->eventQ
                        , newConn->socketFd
                        , newConn->summaryStats
                        , newConn->groupStats
                        , newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    } else {
        (*newConn->iovCtx->methods.OnEstablish)(newConn->appCtx
                                                    , newConn);
    }
}

void NewConnection (IoVentCtx_t* iovCtx
                        , void* appCtx
                        , SockAddr_t* localAddress
                        , LocalPortPool_t* localPortPool 
                        , SockAddr_t* remoteAddress
                        , void* aStats
                        , void* bStats) {

    IoVentConn_t* newConn = GetFreeConnection (iovCtx); 

    if (newConn == NULL) {
        IncConnStats2(aStats, bStats, tcpConnStructNotAvail);
    } else {
        SetAppState(newConn, CONNAPP_STATE_CONNECTION_IN_PROGRESS);
        newConn->iovCtx = iovCtx;
        newConn->appCtx = appCtx;
        newConn->localAddress = localAddress;
        newConn->localPortPool = localPortPool;
        newConn->remoteAddress = remoteAddress;
        newConn->summaryStats = aStats;
        newConn->groupStats = bStats;
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
    }
}

void InitServer (IoVentCtx_t* iovCtx
                    , void* appCtx
                    , SockAddr_t* localAddress
                    , void* aStats
                    , void* bStats) {
    int status = -1;
    IoVentConn_t* newConn = GetFreeConnection (iovCtx);
    if (newConn == NULL) {
        IncConnStats2(aStats, bStats, tcpListenStructNotAvail);
    } else {
        newConn->iovCtx = iovCtx;
        newConn->appCtx = appCtx;
        newConn->localAddress = localAddress;
        newConn->summaryStats = aStats;
        newConn->groupStats = bStats;
        
        newConn->socketFd 
            = TcpListenStart(newConn->localAddress
                                , 1000 //remove hardcoded 
                                , newConn->summaryStats
                                , newConn->groupStats
                                , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
            SetFreeConnection (newConn);
        } else {
            PollReadEventOnly(newConn->iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->summaryStats
                                , newConn->groupStats
                                , newConn);

            if ( GetCES(newConn) ) {
                CloseConnection(newConn);
            }

            status = 0;
        }
    }

    if (status != 0 ) {
        IncConnStats2(aStats, bStats, tcpInitServerFail);
    }
}

static void OnTcpAcceptConnection(IoVentConn_t* lSockConn) {
                
    IoVentConn_t* newConn = GetFreeConnection (lSockConn->iovCtx);
    
    if (newConn == NULL) {
        IncConnStats2(lSockConn->summaryStats
                    , lSockConn->groupStats
                    , tcpConnStructNotAvail);
    } else {
        newConn->iovCtx = lSockConn->iovCtx;
        newConn->appCtx = lSockConn->appCtx;
        newConn->localAddress = lSockConn->localAddress;
        newConn->remoteAddress = &newConn->remoteAddressAccept; 
        newConn->summaryStats = lSockConn->summaryStats;
        newConn->groupStats = lSockConn->groupStats;

        newConn->socketFd = TcpAcceptConnection(lSockConn->socketFd
                                            , newConn->remoteAddress
                                            , newConn->summaryStats
                                            , newConn->groupStats
                                            , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
            SetFreeConnection (newConn);
        } else {
            OnConnectionEstablshedHelper (newConn);
        }
    }
}

static void InitSslConnection(IoVentConn_t* newConn
                                        , SSL* newSSL
                                        , int isClient) {

    SetAppState(newConn, CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS);

    if (isClient) {
        SetCS1 (newConn, STATE_SSL_CONN_CLIENT);
    }

    newConn->cSSL = newSSL;

    DoSslHandshake (newConn);
}

void SslClientInit (IoVentConn_t* newConn
                        , SSL* newSSL) {

    InitSslConnection(newConn, newSSL, 1);
}

void SslServerInit (IoVentConn_t* newConn
                        , SSL* newSSL) {

    InitSslConnection(newConn, newSSL, 0);
}

//handle plain tcp write also
static void HandleWriteNextData (IoVentConn_t* newConn) {
    int bytesSent 
        = SSLWrite (newConn->cSSL
                    , newConn->writeBuffer + newConn->writeBuffOffset
                    , newConn->writeDataLen
                    , newConn->summaryStats
                    , newConn->groupStats
                    , newConn);

    if ( GetCES(newConn) ) {
        newConn->writeBuffer = NULL;
        CloseConnection(newConn);
    } else {
        if (bytesSent <= 0) {
            // ssl want read write; skip
        } else {
            //process written data
            newConn->writeBuffer = NULL;
            (*newConn->iovCtx->methods.OnWriteNextStatus)(newConn->appCtx
                                                            , newConn
                                                            , bytesSent);
        }
    }
}

void WriteNextData (IoVentConn_t* newConn
                        , char* writeBuffer
                        , int writeBuffOffset
                        , int writeDataLen) {

    newConn->writeBuffer = writeBuffer;
    newConn->writeBuffOffset = writeBuffOffset;
    newConn->writeDataLen = writeDataLen;

    HandleWriteNextData (newConn);
}

//handle plain tcp read also
static void HandleReadNextData (IoVentConn_t* newConn) {

    if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) == 0) {
        int bytesReceived 
            = SSLRead ( newConn->cSSL
                        , newConn->readBuffer + newConn->readBuffOffset
                        , newConn->readDataLen
                        , newConn->summaryStats
                        , newConn->groupStats
                        , newConn);
        
        if ( GetCES(newConn) ) {
            newConn->readBuffer = NULL;
            CloseConnection(newConn);
        } else {
            if (bytesReceived <= 0) {
                if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) ) {
                    newConn->readBuffer = NULL;
                } else {
                    // ssl want read write; skip
                }
            } else {
                //process read data
                newConn->readBuffer = NULL;
                (*newConn->iovCtx->methods.OnReadNextStatus)(newConn->appCtx
                                                            , newConn
                                                            , bytesReceived);
            }
        }
    }
}

void ReadNextData (IoVentConn_t* newConn
                        , char* readBuffer
                        , int readBuffOffset
                        , int readDataLen) {

    newConn->readBuffer = readBuffer;
    newConn->readBuffOffset = readBuffOffset;
    newConn->readDataLen = readDataLen;

    HandleReadNextData (newConn);
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
        OnConnectionEstablshedHelper (newConn);
    }
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

            if IsSetCS1 (newConn, STATE_TCP_LISTENING) {
                OnTcpAcceptConnection(newConn);
            } else {

                //Handle Tcp Connect
                if ( (GetAppState(newConn) 
                            == CONNAPP_STATE_CONNECTION_IN_PROGRESS)
                        &&  IsWriteEventSet(iovCtx->EventArr[eIndex]) ) {
                    
                    OnTcpConnectionCompletion (newConn);

                } else if (GetAppState(newConn) 
                            == CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS) {

                    int doSslHandshake = 0;

                    if ( IsSetCS1(newConn, STATE_SSL_HANDSHAKE_WANT_WRITE) 
                            && IsWriteEventSet(iovCtx->EventArr[eIndex]) ) {
                        doSslHandshake = 1;
                        ClearCS1(newConn, STATE_SSL_HANDSHAKE_WANT_WRITE);   
                    } 
                    
                    if ( IsSetCS1(newConn, STATE_SSL_HANDSHAKE_WANT_READ) 
                            && IsReadEventSet(iovCtx->EventArr[eIndex]) ) {
                        doSslHandshake = 1;
                        ClearCS1(newConn, STATE_SSL_HANDSHAKE_WANT_READ);  
                    }

                    if ( doSslHandshake ) {
                        DoSslHandshake (newConn);
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
                            if (newConn->writeBuffer) {
                                HandleWriteNextData (newConn);
                            } else {
                                (*newConn->iovCtx->methods.OnWriteNext)(newConn->appCtx
                                                                        , newConn);
                            }
                        }
                    }

                    // Handle Read
                    if (IsReadEventSet(iovCtx->EventArr[eIndex])
                                        && !IsFdClosed(newConn) ) {
                        if (newConn->readBuffer) {
                            HandleReadNextData (newConn);
                        } else {
                            (*newConn->iovCtx->methods.OnReadNext)(newConn->appCtx
                                                                        , newConn);
                        }
                    }
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

