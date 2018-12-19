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
    newConn->savedLocalPort = 0;
    newConn->savedRemotePort = 0;

    newConn->statusId = 0;
    newConn->statusData = NULL;

    newConn->statusResponseId = 0;
    newConn->statusResponseData = NULL;

    newConn->cInfo.cSSL = NULL;
    newConn->cInfo.localAddress = NULL;
    newConn->cInfo.remoteAddress = NULL;
    newConn->cInfo.localPortPool = NULL;  
    newConn->cInfo.connData = NULL;   
    newConn->cInfo.sessionData = NULL;   
    newConn->cInfo.appCtx = NULL;
    newConn->cInfo.groupCtx = NULL;
    newConn->cInfo.iovCtx = NULL;

    newConn->cInfo.writeBuffer = NULL;
    newConn->cInfo.writeBuffOffsetCur = 0;
    newConn->cInfo.writeDataLenCur = 0;

    newConn->cInfo.readBuffer = NULL;
    newConn->cInfo.readBuffOffset = 0;
    newConn->cInfo.readDataLen = 0;

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
    RemoveFromPool (newConn->cInfo.iovCtx->activeConnectionPool, newConn);
    AddToPool (newConn->cInfo.iovCtx->freeConnectionPool, newConn);
}

static void StoreErrConnection (IoVentConn_t* newConn) {
    int currentErrCount = newConn->cInfo.iovCtx->errorConnectionCount;
    if (currentErrCount 
                < newConn->cInfo.iovCtx->options.maxErrorConnections) {

        IoVentConn_t* errConn 
            = &newConn->cInfo.iovCtx->errorConnectionArr[currentErrCount];

        *errConn =  *newConn;

        uint16_t tmpPort;

        if (errConn->cInfo.localAddress) {
            GET_SOCK_PORT (errConn->cInfo.localAddress, &tmpPort);        
            errConn->savedLocalPort = ntohs(tmpPort);
        } else {
            errConn->savedLocalPort = 0;
        }

        if (errConn->cInfo.remoteAddress) {
            GET_SOCK_PORT (errConn->cInfo.remoteAddress, &tmpPort);        
            errConn->savedRemotePort = ntohs(tmpPort);
        } else {
            errConn->savedRemotePort = 0;
        }

        newConn->cInfo.iovCtx->errorConnectionCount++;
    }
}

void DumpConnection (IoVentConn_t* newConn) {

    char srcAddr[INET6_ADDRSTRLEN];
    char dstAddr[INET6_ADDRSTRLEN];

    AddressToString (newConn->cInfo.localAddress, srcAddr);

    AddressToString (newConn->cInfo.remoteAddress, dstAddr);

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
    uint16_t localPort;
    GET_SOCK_PORT(newConn->cInfo.localAddress, &localPort);
    SetPortToPool (newConn->cInfo.localPortPool, localPort);
}

static void RemoveConnection(IoVentConn_t* newConn) {

    // DumpConnection (newConn);

    if ( IsSetCES(newConn, STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {
        StopPollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.summaryStats
                                , newConn->cInfo.groupStats
                                , newConn);
    }       

    TcpClose(newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {
        StoreErrConnection (newConn);
    }

    //only for client connection
    if ( newConn->cInfo.localPortPool 
            && ( IsSetCES(newConn, STATE_TCP_SOCK_FD_CLOSE_FAIL
                        | STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) ) {
        ReleasePort(newConn);
    }

    AddToPool (newConn->cInfo.iovCtx->cleanupConnectionPool, newConn);
}

void CloseConnection(IoVentConn_t* newConn) {

    if ( GetCES(newConn) ) {

        RemoveConnection (newConn);

    } else {

        int pendingSendReceiveCloseNotify 
            = IsSetCS1(newConn, STATE_SSL_TO_SEND_SHUTDOWN) 
            || IsSetCS1(newConn, STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);

        if ( IsSetCS1(newConn, STATE_SSL_CONN_ESTABLISHED)
                && pendingSendReceiveCloseNotify ) {

            SSLShutdown (newConn->cInfo.cSSL, newConn);
        }

        int sentCloseNotifyOrNotRequired 
            =   (IsSetCS1(newConn, STATE_SSL_ENABLED_CONN) == 0) 
                || IsSetCS1(newConn, STATE_SSL_SENT_SHUTDOWN) 
                || ( (IsSetCS1(newConn, STATE_SSL_TO_SEND_SHUTDOWN) == 0)
                    && (IsSetCS1(newConn, STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN) == 0) );


        int wrShutdownDone 
            = IsSetCS1 (newConn, STATE_TCP_SENT_FIN)
                || IsSetCES (newConn, STATE_TCP_FIN_SEND_FAIL); 

        if ( IsSetCS1(newConn, STATE_TCP_CONN_ESTABLISHED)
                && (wrShutdownDone == 0)
                && sentCloseNotifyOrNotRequired ) {

            TcpWrShutdown (newConn->socketFd, newConn);

            StopPollWriteEvent (newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.summaryStats
                                , newConn->cInfo.groupStats
                                , newConn);
        }

        if ( GetCES(newConn) 
                || (IsSetCS1(newConn, STATE_TCP_REMOTE_CLOSED)  
                     &&  wrShutdownDone) ) {

            RemoveConnection (newConn);
        }
    }
}

static void DoSslHandshake (IoVentConn_t* newConn) {
    DoSSLConnect (newConn->cInfo.cSSL
                        , newConn->socketFd
                        , IsSetCS1 (newConn, STATE_SSL_CONN_CLIENT) ? 1 : 0
                        , newConn->cInfo.summaryStats
                        , newConn->cInfo.groupStats
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

    PollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                        , newConn->socketFd
                        , newConn->cInfo.summaryStats
                        , newConn->cInfo.groupStats
                        , newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    } else {
        (*newConn->cInfo.iovCtx->methods.OnEstablish)(newConn);
    }
}

void NewConnection (IoVentCtx_t* iovCtx
                        , void* groupCtx
                        , void* appCtx
                        , void* sessionData
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
        newConn->cInfo.iovCtx = iovCtx;
        newConn->cInfo.groupCtx = groupCtx;
        newConn->cInfo.appCtx = appCtx;
        newConn->cInfo.sessionData = sessionData;
        newConn->cInfo.localAddress = localAddress;
        newConn->cInfo.localPortPool = localPortPool;
        newConn->cInfo.remoteAddress = remoteAddress;
        newConn->cInfo.summaryStats = aStats;
        newConn->cInfo.groupStats = bStats;
        
        int localPortAssignError = 0;
        if (newConn->cInfo.localPortPool) {
            AssignSocketLocalPort(newConn->cInfo.localAddress
                                , newConn->cInfo.localPortPool
                                , aStats
                                , bStats
                                , newConn);
            
            if ( GetCES(newConn) ) {
                        localPortAssignError = 1;
                        StoreErrConnection (newConn);
                        SetFreeConnection (newConn);
            }
        }

        if (localPortAssignError == 0) {
            newConn->socketFd 
                = TcpNewConnection(newConn->cInfo.localAddress
                        , newConn->cInfo.remoteAddress
                        , aStats
                        , bStats
                        , newConn);

            if ( GetCES(newConn) ) {
                StoreErrConnection (newConn);
                SetFreeConnection (newConn);
                if (newConn->cInfo.localPortPool) {
                    ReleasePort (newConn);
                }
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
                    , void* groupCtx
                    , void* appCtx
                    , SockAddr_t* localAddress
                    , void* aStats
                    , void* bStats) {
    int status = -1;
    IoVentConn_t* newConn = GetFreeConnection (iovCtx);
    if (newConn == NULL) {
        IncConnStats2(aStats, bStats, tcpListenStructNotAvail);
    } else {
        newConn->cInfo.iovCtx = iovCtx;
        newConn->cInfo.groupCtx = groupCtx;
        newConn->cInfo.appCtx = appCtx;
        newConn->cInfo.localAddress = localAddress;
        newConn->cInfo.summaryStats = aStats;
        newConn->cInfo.groupStats = bStats;
        
        newConn->socketFd 
            = TcpListenStart(newConn->cInfo.localAddress
                                , 1000 //remove hardcoded 
                                , newConn->cInfo.summaryStats
                                , newConn->cInfo.groupStats
                                , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
            SetFreeConnection (newConn);
        } else {
            PollReadEventOnly(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.summaryStats
                                , newConn->cInfo.groupStats
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
                
    IoVentConn_t* newConn = GetFreeConnection (lSockConn->cInfo.iovCtx);
    
    if (newConn == NULL) {
        IncConnStats2(lSockConn->cInfo.summaryStats
                    , lSockConn->cInfo.groupStats
                    , tcpConnStructNotAvail);
    } else {
        newConn->cInfo.iovCtx = lSockConn->cInfo.iovCtx;
        newConn->cInfo.groupCtx = lSockConn->cInfo.groupCtx;
        newConn->cInfo.appCtx = lSockConn->cInfo.appCtx;
        newConn->cInfo.localAddress = lSockConn->cInfo.localAddress;
        newConn->cInfo.remoteAddress = &newConn->remoteAddressAccept; 
        newConn->cInfo.summaryStats = lSockConn->cInfo.summaryStats;
        newConn->cInfo.groupStats = lSockConn->cInfo.groupStats;

        newConn->socketFd = TcpAcceptConnection(lSockConn->socketFd
                                            , newConn->cInfo.remoteAddress
                                            , newConn->cInfo.summaryStats
                                            , newConn->cInfo.groupStats
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
        SetCS1 (newConn, STATE_SSL_ENABLED_CONN | STATE_SSL_CONN_CLIENT);
    } else {
        SetCS1 (newConn,  STATE_SSL_ENABLED_CONN);
    }

    newConn->cInfo.cSSL = newSSL;

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

void WriteNextDataRemaing (IoVentConn_t* newConn);

int MapConnectionError (IoVentConn_t* newConn) {

    int iovConnErr  = ON_CLOSE_ERROR_NONE;

    if ( GetCES (newConn) ) {

        switch ( GetSysErrno (newConn) ) {

            case ETIMEDOUT:
                iovConnErr = ON_CLOSE_ERROR_TCP_TIMEOUT; 
                break;

            case ECONNRESET:
                iovConnErr = ON_CLOSE_ERROR_TCP_RESET;
                break;

            default:
                iovConnErr = ON_CLOSE_ERROR_GENERAL; 
                break;
        }
    }

    return iovConnErr;
}

//handle plain tcp write also
static void HandleWriteNextData (IoVentConn_t* newConn) {

    int bytesSent;
    if ( IsSetCS1 (newConn, STATE_SSL_ENABLED_CONN) ) { 
        bytesSent = SSLWrite (newConn->cInfo.cSSL
            , newConn->cInfo.writeBuffer + newConn->cInfo.writeBuffOffsetCur
            , newConn->cInfo.writeDataLenCur
            , newConn->cInfo.summaryStats
            , newConn->cInfo.groupStats
            , newConn);
    } else {
        bytesSent = TcpWrite (newConn->socketFd
            , newConn->cInfo.writeBuffer + newConn->cInfo.writeBuffOffsetCur
            , newConn->cInfo.writeDataLenCur
            , newConn->cInfo.summaryStats
            , newConn->cInfo.groupStats
            , newConn);
    }

    if ( GetCES(newConn) ) {
        ClearCS1 (newConn, STATE_CONN_WRITE_PENDING);
        CloseConnection(newConn);
    } else {
        if (bytesSent <= 0) {
            // ssl want read write; skip
        } else {
            //process written data
            newConn->cInfo.writtenBytesLenCur += bytesSent;

            int notifyWriteStatus;
            if ( IsSetCS1 (newConn, STATE_CONN_PARTIAL_WRITE) ){
                notifyWriteStatus = 1;
            } else {
                if (newConn->cInfo.writtenBytesLenCur 
                        == newConn->cInfo.writeDataLen) {
                    notifyWriteStatus = 1;
                } else {
                    notifyWriteStatus = 0;
                }
            }

            if (notifyWriteStatus) {
                ClearCS1 (newConn, STATE_CONN_WRITE_PENDING);
                (*newConn->cInfo.iovCtx->methods.OnWriteStatus)(newConn
                                            , newConn->cInfo.writtenBytesLenCur);
            } else {
                WriteNextDataRemaing (newConn);
            }
        }
    }
}

void WriteNextData (IoVentConn_t* newConn
                        , char* writeBuffer
                        , int writeBuffOffset
                        , int writeDataLen
                        , int partialWrite) {

    SetCS1 (newConn, STATE_CONN_WRITE_PENDING); 
    newConn->cInfo.writeBuffer = writeBuffer;
    newConn->cInfo.writeBuffOffset = writeBuffOffset;
    newConn->cInfo.writeDataLen = writeDataLen;

    newConn->cInfo.writeBuffOffsetCur = writeBuffOffset;
    newConn->cInfo.writeDataLenCur = writeDataLen;
    newConn->cInfo.writtenBytesLenCur = 0;

    if (partialWrite) {
        SetCS1 (newConn, STATE_CONN_PARTIAL_WRITE);
    } else {
        ClearCS1 (newConn, STATE_CONN_PARTIAL_WRITE);
    }

    HandleWriteNextData (newConn);
}

void WriteNextDataRemaing (IoVentConn_t* newConn) {

    newConn->cInfo.writeBuffOffsetCur += newConn->cInfo.writtenBytesLenCur;
    newConn->cInfo.writeDataLenCur -= newConn->cInfo.writtenBytesLenCur;

    HandleWriteNextData (newConn);
}

//handle plain tcp read also
static void HandleReadNextData (IoVentConn_t* newConn) {

    int bytesReceived;
    if ( IsSetCS1 (newConn, STATE_SSL_ENABLED_CONN) ) {
        bytesReceived  
            = SSLRead ( newConn->cInfo.cSSL
            , newConn->cInfo.readBuffer + newConn->cInfo.readBuffOffset
            , newConn->cInfo.readDataLen
            , newConn->cInfo.summaryStats
            , newConn->cInfo.groupStats
            , newConn);
    } else {
        bytesReceived  
            = TcpRead ( newConn->socketFd
            , newConn->cInfo.readBuffer + newConn->cInfo.readBuffOffset
            , newConn->cInfo.readDataLen
            , newConn->cInfo.summaryStats
            , newConn->cInfo.groupStats
            , newConn);
    }

    if ( GetCES(newConn) ) {
        int iovConnErr = MapConnectionError (newConn);
        ClearCS1 (newConn, STATE_CONN_READ_PENDING);
        CloseConnection(newConn);
        (*newConn->cInfo.iovCtx->methods.OnClose)(newConn, iovConnErr);
    } else {
        if (bytesReceived <= 0) {
            if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) ) {
                ClearCS1 (newConn, STATE_CONN_READ_PENDING);
                StopPollReadEvent (newConn->cInfo.iovCtx->eventQ
                                    , newConn->socketFd
                                    , newConn->cInfo.summaryStats
                                    , newConn->cInfo.groupStats
                                    , newConn);
                if ( GetCES(newConn) ) {
                    int iovConnErr = MapConnectionError (newConn);
                    (*newConn->cInfo.iovCtx->methods.OnClose)(newConn, iovConnErr);
                    CloseConnection(newConn);
                } else {
                    (*newConn->cInfo.iovCtx->methods.OnClose)(newConn
                                                        , ON_CLOSE_ERROR_NONE);
                }
            } else {
                // ssl want read write; skip;
            }
        } else {
            //process read data
            ClearCS1 (newConn, STATE_CONN_READ_PENDING);
            (*newConn->cInfo.iovCtx->methods.OnReadStatus)(newConn
                                                , bytesReceived);
        }
    }
}

void ReadNextData (IoVentConn_t* newConn
                        , char* readBuffer
                        , int readBuffOffset
                        , int readDataLen) {
    
    if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) == 0) {

        SetCS1 (newConn, STATE_CONN_READ_PENDING);

        newConn->cInfo.readBuffer = readBuffer;
        newConn->cInfo.readBuffOffset = readBuffOffset;
        newConn->cInfo.readDataLen = readDataLen;

        HandleReadNextData (newConn);
    }
}

static void OnTcpConnectionCompletion (IoVentConn_t* newConn) {

    VerifyTcpConnectionEstablished (newConn->socketFd
                                    , newConn->cInfo.summaryStats
                                    , newConn->cInfo.groupStats
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

    iovCtx->cleanupConnectionPool = AllocEmptyPool ();

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
                } else if ( ( GetAppState(newConn) 
                                == CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED )
                            || ( (GetAppState(newConn)
                                == CONNAPP_STATE_CONNECTION_ESTABLISHED) 
                                    && (IsSetCS1 (newConn, STATE_SSL_ENABLED_CONN) == 0) )
                                ) {


                //    printf ("<<<\nfd = %d"
                //             ", SS1 = %#018" PRIx64 
                //             ", ES = %#018" PRIx64 
                //             ", SysErr = %d"
                //             ", SockErr = %d"
                //             ", Events = %x\n"
                //             , newConn->socketFd
                //             , GetCS1(newConn)
                //             , GetCES(newConn)
                //             , GetSysErrno(newConn) 
                //             , GetSockErrno(newConn)
                //             , iovCtx->EventArr[eIndex].events);


                    // Handle Write
                    if ( IsWriteEventSet(iovCtx->EventArr[eIndex]) 
                                        && !IsFdClosed(newConn) ) {

                        if (IsSetCS1 (newConn, STATE_CONN_WRITE_PENDING) ) {
                            HandleWriteNextData (newConn);
                        } else {
                            if ( IsSetCS1(newConn,  STATE_NO_MORE_WRITE_DATA) ) {
                                CloseConnection(newConn);
                            } else {
                                (*newConn->cInfo.iovCtx->methods.OnWriteNext)(newConn);
                            }
                        }
                    }

                    // Handle Read
                    if (IsReadEventSet(iovCtx->EventArr[eIndex])
                                        && !IsFdClosed(newConn) ) {
                        if ( IsSetCS1 (newConn, STATE_CONN_READ_PENDING) ) {
                            HandleReadNextData (newConn);
                        } else if (IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) == 0 ) {
                            (*newConn->cInfo.iovCtx->methods.OnReadNext)(newConn);
                        }
                    }

                    if ( !IsFdClosed(newConn) 
                            && IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) 
                            && (IsSetCS1 (newConn, STATE_TCP_SENT_FIN)
                                || IsSetCES (newConn, STATE_TCP_FIN_SEND_FAIL)) ) {
                        CloseConnection(newConn);
                    }

                    
                    IoVentConn_t* newConn 
                        = GetFromPool (iovCtx->cleanupConnectionPool);

                    while (newConn) {

                        (*newConn->cInfo.iovCtx->methods.OnCleanup)(newConn);

                        SetFreeConnection (newConn);

                        newConn = GetFromPool (iovCtx->cleanupConnectionPool);
                    }

                //    printf ("\nfd = %d"
                //             ", SS1 = %#018" PRIx64 
                //             ", ES = %#018" PRIx64 
                //             ", SysErr = %d"
                //             ", SockErr = %d"
                //             ", Events = %x\n>>\n"
                //             , newConn->socketFd
                //             , GetCS1(newConn)
                //             , GetCES(newConn)
                //             , GetSysErrno(newConn) 
                //             , GetSockErrno(newConn)
                //             , iovCtx->EventArr[eIndex].events);
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

void EnableReadNotification (IoVentConn_t* newConn) {

    PollReadEvent(newConn->cInfo.iovCtx->eventQ
                        , newConn->socketFd
                        , newConn->cInfo.summaryStats
                        , newConn->cInfo.groupStats
                        , newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    }
}

void DisableReadNotification (IoVentConn_t* newConn) {

    StopPollReadEvent(newConn->cInfo.iovCtx->eventQ
                        , newConn->socketFd
                        , newConn->cInfo.summaryStats
                        , newConn->cInfo.groupStats
                        , newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    }
}

void EnableWriteNotification (IoVentConn_t* newConn) {

    PollWriteEvent(newConn->cInfo.iovCtx->eventQ
                        , newConn->socketFd
                        , newConn->cInfo.summaryStats
                        , newConn->cInfo.groupStats
                        , newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    }
}

void DisableWriteNotification (IoVentConn_t* newConn) {

    StopPollWriteEvent(newConn->cInfo.iovCtx->eventQ
                        , newConn->socketFd
                        , newConn->cInfo.summaryStats
                        , newConn->cInfo.groupStats
                        , newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    }
}

