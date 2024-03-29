#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "iovents.h"

static void InitConnection (IoVentConn_t* newConn) {
    CSInit(newConn);
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

    if ( IsFdClosed (newConn) == 0 ) {


        int isLinger = 0;
        int lingerTime = 0;

        if ( GetCES(newConn) || IsSetCS1 (newConn, STATE_TCP_TO_SEND_RST) ) {
            isLinger = 1;
        }

        DisableReadWriteNotification (newConn);     

        TcpClose(newConn->socketFd
                    , isLinger
                    , lingerTime
                    , newConn->cInfo.statsArr
                    , newConn->cInfo.statsCount
                    , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
        }

        //only for client connection
        if ( newConn->cInfo.localPortPool 
                && ( IsSetCES(newConn, STATE_TCP_SOCK_FD_CLOSE_FAIL) == 0 ) ) {
            ReleasePort(newConn);
        }

        AddToPool (newConn->cInfo.iovCtx->cleanupConnectionPool, newConn);
    }
}

void CloseConnection(IoVentConn_t* newConn) {

    if ( GetCES(newConn) || IsSetCS1 (newConn, STATE_TCP_TO_SEND_RST) ) {
        
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

            DisableWriteNotification (newConn);
        }

        if ( GetCES(newConn) 
                || (IsSetCS1(newConn, STATE_TCP_REMOTE_CLOSED)  
                     &&  wrShutdownDone) ) {

            RemoveConnection (newConn);
        }
    }
}

void AbortConnection (IoVentConn_t* newConn) {

    if ( IsSetCS1 (newConn, STATE_CONN_MARK_DELETE) == 0) {
        
        SetCS1 (newConn, STATE_CONN_MARK_DELETE 
                        | STATE_TCP_TO_SEND_RST);
        
        AddToPool (newConn->cInfo.iovCtx->abortConnectionPool, newConn);
    }
}

static void ProcessAbortConnections (IoVentCtx_t* iovCtx) {

    IoVentConn_t* newConn 
            = GetFromPool (iovCtx->abortConnectionPool);

    while (newConn) {
        CloseConnection (newConn);
        newConn = GetFromPool (iovCtx->abortConnectionPool);
    }
}

static void DoSslHandshake (IoVentConn_t* newConn) {
    DoSSLConnect (newConn->cInfo.cSSL
                        , newConn->socketFd
                        , IsSetCS1 (newConn, STATE_SSL_CONN_CLIENT) ? 1 : 0
                        , newConn->cInfo.statsArr
                        , newConn->cInfo.statsCount
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

static void OnConnectionEstablshedHelper (IoVentConn_t* newConn, int isAccept) {

    SetAppState (newConn, CONNAPP_STATE_CONNECTION_ESTABLISHED);

    if (isAccept) {
        
        EnableReadWriteNotification(newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        }
    }
}

int IsConnExpire (IoVentConn_t* newConn) {
    
    int status = 0;

    // if (newConn->cInfo.connLifetime) {
    //     if ( TimeElapsedIoVentCtx (newConn->cInfo.iovCtx)  
    //                         >  newConn->cInfo.connLifetime) {
    //         status = 1;
    //     }
    // }

    return status;
}

int NewConnection (IoVentCtx_t* iovCtx
                        , void* connCtx
                        , SockAddr_t* localAddress
                        , LocalPortPool_t* localPortPool 
                        , SockAddr_t* remoteAddress
                        , int connLifetime
                        , SockStats_t** statsArr
                        , int statsCount) {
    
    int isErr = -1;

    IoVentConn_t* newConn = GetFreeConnection (iovCtx); 

    if (newConn == NULL) {
        IncConnStats(statsArr, statsCount, tcpConnStructNotAvail);
    } else {
        SetAppState(newConn, CONNAPP_STATE_CONNECTION_IN_PROGRESS);
        newConn->cInfo.cSSL = NULL;
        newConn->cInfo.iovCtx = iovCtx;
        newConn->cInfo.connCtx = connCtx;
        newConn->cInfo.localAddress = localAddress;
        newConn->cInfo.localPortPool = localPortPool;
        newConn->cInfo.remoteAddress = remoteAddress;
        newConn->cInfo.statsArr = statsArr;
        newConn->cInfo.statsCount = statsCount;

        int localPortAssignError = 0;
        if (newConn->cInfo.localPortPool) {
            AssignSocketLocalPort(newConn->cInfo.localAddress
                                , newConn->cInfo.localPortPool
                                , newConn->cInfo.statsArr 
                                , newConn->cInfo.statsCount
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
                        , newConn->cInfo.statsArr 
                        , newConn->cInfo.statsCount
                        , newConn);

            if ( GetCES(newConn) ) {
                StoreErrConnection (newConn);
                SetFreeConnection (newConn);
                if (newConn->cInfo.localPortPool) {
                    ReleasePort (newConn);
                }
            } else {
                EnableReadWriteNotification(newConn);

                if ( GetCES(newConn) ) {
                    CloseConnection(newConn);
                } else {
                    isErr = 0;
                }
            }
        }
    }

    return isErr;
}

void InitServer (IoVentCtx_t* iovCtx
                    , void* srvCtx
                    , SockAddr_t* localAddress
                    , SockStats_t** statsArr
                    , int statsCount) {
    int status = -1;
    IoVentConn_t* newConn = GetFreeConnection (iovCtx);
    if (newConn == NULL) {
        IncConnStats(statsArr, statsCount, tcpListenStructNotAvail);
    } else {
        newConn->cInfo.cSSL = NULL;
        newConn->cInfo.iovCtx = iovCtx;
        newConn->cInfo.connCtx = srvCtx;
        newConn->cInfo.localAddress = localAddress;
        newConn->cInfo.localPortPool = NULL;
        newConn->cInfo.remoteAddress = NULL;
        newConn->cInfo.statsArr = statsArr;
        newConn->cInfo.statsCount = statsCount;
        
        newConn->socketFd 
            = TcpListenStart(newConn->cInfo.localAddress
                                , 20000 //???remove hardcoded 
                                , statsArr
                                , statsCount
                                , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
            SetFreeConnection (newConn);
        } else {

            EnableReadWriteNotification(newConn);

            if ( GetCES(newConn) ) {
                CloseConnection(newConn);
            }

            status = 0;
        }
    }

    if (status != 0 ) {
        IncConnStats(statsArr, statsCount, tcpInitServerFail);
    }
}

static void OnTcpAcceptConnection(IoVentConn_t* lSockConn) {
                
    IoVentConn_t* newConn = GetFreeConnection (lSockConn->cInfo.iovCtx);
    
    if (newConn == NULL) {
        IncConnStats(lSockConn->cInfo.statsArr
                    , lSockConn->cInfo.statsCount 
                    , tcpConnStructNotAvail);
    } else {
        newConn->cInfo.iovCtx = lSockConn->cInfo.iovCtx;
        newConn->cInfo.connCtx = lSockConn->cInfo.connCtx;
        newConn->cInfo.localAddress = &newConn->localAddressAccept;
        newConn->cInfo.remoteAddress = &newConn->remoteAddressAccept; 
        newConn->cInfo.statsArr = lSockConn->cInfo.statsArr;
        newConn->cInfo.statsCount = lSockConn->cInfo.statsCount;

        newConn->socketFd = TcpAcceptConnection(lSockConn->socketFd
                                            , newConn->cInfo.localAddress
                                            , newConn->cInfo.remoteAddress
                                            , newConn->cInfo.statsArr
                                            , newConn->cInfo.statsCount 
                                            , newConn);

        if ( GetCES(newConn) ) {
            StoreErrConnection (newConn);
            SetFreeConnection (newConn);
        } else {
            OnConnectionEstablshedHelper (newConn, 1);

            if ( GetCES(newConn) == 0 ) {

                (*newConn->cInfo.iovCtx->methods.OnEstablish)(newConn);

                ProcessAbortConnections (newConn->cInfo.iovCtx);
            } 
        }
    }
}

void InitSslConnection(IoVentConn_t* newConn
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

int MapConnectionError (IoVentConn_t* newConn) {

    int iovConnErr  = ON_CLOSE_ERROR_UNKNOWN;

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
            , newConn->cInfo.statsArr
            , newConn->cInfo.statsCount 
            , newConn);
    } else {
        bytesSent = TcpWrite (newConn->socketFd
            , newConn->cInfo.writeBuffer + newConn->cInfo.writeBuffOffsetCur
            , newConn->cInfo.writeDataLenCur
            , newConn->cInfo.statsArr
            , newConn->cInfo.statsCount
            , newConn);
    }

    if ( GetCES(newConn) ) {
        int iovConnErr = MapConnectionError (newConn);
        ClearCS1 (newConn, STATE_CONN_WRITE_PENDING);
        CloseConnection(newConn);
        (*newConn->cInfo.iovCtx->methods.OnWriteStatus)(newConn, iovConnErr);
        ProcessAbortConnections (newConn->cInfo.iovCtx);
    } else {
        if (bytesSent <= 0) {
            // ssl want read write; skip
        } else {
            //process written data
            newConn->cInfo.writtenBytesLenCur += bytesSent;
            newConn->cInfo.writeBuffOffsetCur += bytesSent;
            newConn->cInfo.writeDataLenCur -= bytesSent;

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
                
                ProcessAbortConnections (newConn->cInfo.iovCtx);

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
}

//handle plain tcp read also
static void HandleReadNextData (IoVentConn_t* newConn) {

    int bytesReceived;
    if ( IsSetCS1 (newConn, STATE_SSL_ENABLED_CONN) ) {
        bytesReceived  
            = SSLRead ( newConn->cInfo.cSSL
            , newConn->cInfo.readBuffer + newConn->cInfo.readBuffOffsetCur
            , newConn->cInfo.readDataLenCur
            , newConn->cInfo.statsArr
            , newConn->cInfo.statsCount
            , newConn);
    } else {
        bytesReceived  
            = TcpRead ( newConn->socketFd
            , newConn->cInfo.readBuffer + newConn->cInfo.readBuffOffsetCur
            , newConn->cInfo.readDataLenCur
            , newConn->cInfo.statsArr
            , newConn->cInfo.statsCount
            , newConn);
    }

    if ( GetCES(newConn) ) {
        int iovConnErr = MapConnectionError (newConn);
        ClearCS1 (newConn, STATE_CONN_READ_PENDING);
        CloseConnection(newConn);
        (*newConn->cInfo.iovCtx->methods.OnReadStatus)(newConn, iovConnErr);
        ProcessAbortConnections (newConn->cInfo.iovCtx);        
    } else {
        if (bytesReceived <= 0) {
            if ( IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) ) {
                ClearCS1 (newConn, STATE_CONN_READ_PENDING);
                DisableReadNotification (newConn);
                if ( GetCES(newConn) ) {
                    int iovConnErr = MapConnectionError (newConn);
                    CloseConnection(newConn);
                    (*newConn->cInfo.iovCtx->methods.OnReadStatus)(newConn, iovConnErr);
                    ProcessAbortConnections (newConn->cInfo.iovCtx);
                } else {
                    (*newConn->cInfo.iovCtx->methods.OnReadStatus)(newConn
                                                        , ON_CLOSE_ERROR_NONE);

                    ProcessAbortConnections (newConn->cInfo.iovCtx);
                }
            } else {
                // ssl want read write; skip;
            }
        } else {
            //process read data
            newConn->cInfo.readBytesLenCur += bytesReceived;
            newConn->cInfo.readBuffOffsetCur += bytesReceived;
            newConn->cInfo.readDataLenCur -= bytesReceived;

            int notifyReadStatus;
            if ( IsSetCS1 (newConn, STATE_CONN_PARTIAL_READ) ){
                notifyReadStatus = 1;
            } else {
                if (newConn->cInfo.readBytesLenCur 
                        == newConn->cInfo.readDataLen) {
                    notifyReadStatus = 1;
                } else {
                    notifyReadStatus = 0;
                }                
            }

            if (notifyReadStatus) {
                ClearCS1 (newConn, STATE_CONN_READ_PENDING);

                (*newConn->cInfo.iovCtx->methods.OnReadStatus)(newConn
                                        , newConn->cInfo.readBytesLenCur);

                ProcessAbortConnections (newConn->cInfo.iovCtx);
            }
        }
    }
}

void ReadNextData (IoVentConn_t* newConn
                        , char* readBuffer
                        , int readBuffOffset
                        , int readDataLen
                        , int partialRead) {
    
    SetCS1 (newConn, STATE_CONN_READ_PENDING);

    newConn->cInfo.readBuffer = readBuffer;
    newConn->cInfo.readBuffOffset = readBuffOffset;
    newConn->cInfo.readDataLen = readDataLen;

    newConn->cInfo.readBuffOffsetCur = readBuffOffset;
    newConn->cInfo.readDataLenCur = readDataLen;
    newConn->cInfo.readBytesLenCur = 0;

    if (partialRead) {
        SetCS1 (newConn, STATE_CONN_PARTIAL_READ);
    } else {
        ClearCS1 (newConn, STATE_CONN_PARTIAL_READ);
    }    
}

static void OnTcpConnectionCompletion (IoVentConn_t* newConn) {

    VerifyTcpConnectionEstablished (newConn->socketFd
                                    , newConn->cInfo.statsArr
                                    , newConn->cInfo.statsCount
                                    , newConn);
    
    if ( GetCES(newConn) ) {
        SetAppState (newConn, CONNAPP_STATE_CONNECTION_ESTABLISH_FAILED);
        CloseConnection(newConn);
    } else {
        OnConnectionEstablshedHelper (newConn, 0);
    }

    (*newConn->cInfo.iovCtx->methods.OnEstablish)(newConn);

    ProcessAbortConnections (newConn->cInfo.iovCtx);    
}

void InitIoVentCtx (IoVentCtx_t* iovCtx
                    , IoVentMethods_t* methods
                    , IoVentOptions_t* options
                    , void* appCtx) {

    iovCtx->methods = *methods;

    iovCtx->options = *options;

    iovCtx->appCtx = appCtx;

    if (iovCtx->options.maxEvents == 0) {
        iovCtx->options.maxEvents = iovCtx->options.maxActiveConnections;
    }
    
    iovCtx->freeConnectionPool = AllocEmptyPool ();
    
    iovCtx->activeConnectionPool = AllocEmptyPool ();

    iovCtx->cleanupConnectionPool = AllocEmptyPool ();

    iovCtx->abortConnectionPool = AllocEmptyPool ();

    iovCtx->eventConnectionPool = AllocEmptyPool ();

    for (int i = 0; i < iovCtx->options.maxActiveConnections; i++) {

        IoVentConn_t* newConn = CreateStruct0 (IoVentConn_t);

        InitConnection (newConn);

        AddToPool (iovCtx->freeConnectionPool, newConn);
    }

    iovCtx->errorConnectionCount = 0;
    
    iovCtx->errorConnectionArr 
        = CreateArray (IoVentConn_t, iovCtx->options.maxErrorConnections); 

    iovCtx->EventArr = CreateArray (PollEvent_t
                                   , iovCtx->options.maxEvents);

    iovCtx->eventQ = CreateEventQ ();

    iovCtx->timerWheel = CreateTimerWheel ();
} 

IoVentCtx_t* CreateIoVentCtx (IoVentMethods_t* methods
                            , IoVentOptions_t* options
                            , void* appCtx) {

    IoVentCtx_t* iovCtx = CreateStruct0 (IoVentCtx_t);

    InitIoVentCtx (iovCtx, methods, options, appCtx);

    return iovCtx;
}

void DeleteIoVentCtx (IoVentCtx_t* iovCtx) {

    DeleteTimerWheel (iovCtx->timerWheel);

    //cleanup

    DeleteStruct (IoVentCtx_t, iovCtx);
}

double TimeElapsedIoVentCtx(IoVentCtx_t* iovCtx) {
    return TimeElapsedTimerWheel (iovCtx->timerWheel);
}

static void ShowConnIfo (   char* prefix
                            , IoVentConn_t* newConn
                            , uint16_t events
                            , char* postfix) {

    return;

       printf ("%sfd = %d"
                ", SS1 = %#018" PRIx64 
                ", ES = %#018" PRIx64 
                ", SysErr = %d"
                ", SockErr = %d"
                ", Events = %x%s"
                , prefix
                , newConn->socketFd
                , GetCS1(newConn)
                , GetCES(newConn)
                , GetSysErrno(newConn) 
                , GetSockErrno(newConn)
                , events
                , postfix);

}

void ConnExpirationCheck (gpointer ptr) {

    IoVentConn_t* newConn = (IoVentConn_t*) ptr;

    if (IsConnExpire (newConn) ) {
        SetCES (newConn, STATE_TCP_CONNECTION_EXPIRE);
        AbortConnection (newConn);
    }
}

int ProcessIoVent (IoVentCtx_t* iovCtx) {

    int eCount = GetIOEvents(iovCtx->eventQ
                                , iovCtx->EventArr
                                , iovCtx->options.maxEvents
                                , iovCtx->options.eventPTO);
    if (eCount > 0) {

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

                    ShowConnIfo ( "\n<<<\n"
                                , newConn
                                , iovCtx->EventArr[eIndex].events
                                , "\n");

                    // Handle Read
                    if (IsReadEventSet(iovCtx->EventArr[eIndex])
                                        && !IsFdClosed(newConn) ) {
                        
                        if (IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) == 0 ) {

                            if ( IsSetCS1 (newConn, STATE_CONN_READ_PENDING) == 0 ) {

                                (*newConn->cInfo.iovCtx->methods.OnReadNext)(newConn);

                                ProcessAbortConnections (newConn->cInfo.iovCtx);                                
                            }

                            if ( IsSetCS1 (newConn, STATE_CONN_READ_PENDING) 
                                                    && !IsFdClosed(newConn) ) {

                                HandleReadNextData (newConn);
                            }
                        }
                    }



                    // Handle Write
                    if ( IsWriteEventSet(iovCtx->EventArr[eIndex]) 
                                        && !IsFdClosed(newConn) ) {

                        if ( IsSetCS1 (newConn, STATE_CONN_WRITE_PENDING) == 0) {

                           if ( IsSetCS1(newConn,  STATE_NO_MORE_WRITE_DATA) ) {

                                CloseConnection(newConn);

                            } else {

                                (*newConn->cInfo.iovCtx->methods.OnWriteNext)(newConn);

                                ProcessAbortConnections (newConn->cInfo.iovCtx);
                            } 
                        }

                        if ( IsSetCS1 (newConn, STATE_CONN_WRITE_PENDING) 
                                                    && !IsFdClosed(newConn)) {

                            HandleWriteNextData (newConn);
                        }
                    }


                    //Handle Cleanup; If both end connection closed
                    if ( !IsFdClosed(newConn) 
                            && IsSetCS1 (newConn, STATE_TCP_REMOTE_CLOSED) 
                            && (IsSetCS1 (newConn, STATE_TCP_SENT_FIN)
                                || IsSetCES (newConn, STATE_TCP_FIN_SEND_FAIL)) ) {
                        CloseConnection(newConn);
                    }

                    ShowConnIfo ( "\n"
                                , newConn
                                , iovCtx->EventArr[eIndex].events
                                , "\n>>>\n");
                }
            }
        }
    }

    // Handle Expired Connections
    g_queue_foreach (iovCtx->activeConnectionPool,
                    (GFunc) ConnExpirationCheck,
                    NULL);

    ProcessAbortConnections (iovCtx);

    // Handle Application Cleanup
    IoVentConn_t* newConn 
        = GetFromPool (iovCtx->cleanupConnectionPool);

    while (newConn) {

        (*newConn->cInfo.iovCtx->methods.OnCleanup)(newConn);

        SetFreeConnection (newConn);

        newConn = GetFromPool (iovCtx->cleanupConnectionPool);
    }

    int toContinue = 1;

    if (iovCtx->methods.OnContinue) {

        enum ToContinueAppState  toContinueAppState 
            = (*iovCtx->methods.OnContinue) (iovCtx->appCtx);
        
        if (toContinueAppState == EmAppExit) {

            toContinue = 0;

        } else if (toContinueAppState == EmAppContinueZeroActive) {

            if ( IsPoolEmpty (iovCtx->activeConnectionPool) ) {
                toContinue = 0;
            }
        }
        
    } else {

        if ( IsPoolEmpty (iovCtx->activeConnectionPool) ) {
            toContinue = 0;
        }
    }

    return toContinue;
}

void EnableReadNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

            NewPollReadEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        }

        SetCS1(newConn, STATE_TCP_POLL_READ_CURRENT);
    }
}

void DisableReadNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

            StopPollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        }

        ClearCS1(newConn, STATE_TCP_POLL_READ_CURRENT);
    }
}

void EnableWriteNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 ) {

            NewPollWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        }
        SetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void DisableWriteNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 ) {

            StopPollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollReadEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        }
        ClearCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void EnableReadWriteNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 
            || IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0
                &&  IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0) {

            NewPollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        }
        SetCS1(newConn, STATE_TCP_POLL_READ_CURRENT);
        SetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void DisableReadWriteNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) 
            || IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) ) {

        StopPollReadWriteEvent(newConn->cInfo.iovCtx->eventQ
                            , newConn->socketFd
                            , newConn->cInfo.statsArr
                            , newConn->cInfo.statsCount
                            , newConn);

        ClearCS1(newConn, STATE_TCP_POLL_READ_CURRENT);
        ClearCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void EnableReadOnlyNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 
            || IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 
                && IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

            NewPollReadEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollReadEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount 
                                , newConn);
        }
        SetCS1(newConn, STATE_TCP_POLL_READ_CURRENT);
        ClearCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void EnableWriteOnlyNotification (IoVentConn_t* newConn) {

    if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) 
            || IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

        if ( IsSetCS1(newConn, STATE_TCP_POLL_READ_CURRENT) == 0 
                && IsSetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

            NewPollWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        } else {

            UpdatePollWriteEvent(newConn->cInfo.iovCtx->eventQ
                                , newConn->socketFd
                                , newConn->cInfo.statsArr
                                , newConn->cInfo.statsCount
                                , newConn);
        }
        ClearCS1(newConn, STATE_TCP_POLL_READ_CURRENT);
        SetCS1(newConn, STATE_TCP_POLL_WRITE_CURRENT);
    }
}
void WriteClose (IoVentConn_t* newConn) {
    
    EnableWriteNotification(newConn);

    if ( GetCES(newConn) ) {
        CloseConnection(newConn);
    } else {
        SetCS1(newConn, STATE_NO_MORE_WRITE_DATA 
                        | STATE_TCP_TO_SEND_FIN);
    }    
}

