#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "platform.h"


/** @brief Initiate the TCP connection etablishment process 
 *         to remote host.
 *         
 *  Supports both IPv4 and IPv6 internet address
 *
 *  @param IsIpv6(ddr) 0 : ipv4; else ilApv6
 *  @param ddr local socket address pointer
 *  @param rAddr remote socket address pointer
 *  @return file descriptor of newly created tcp socket; -1 for error
 */
int TcpNewConnection(SockAddr_t* lAddr
                        , SockAddr_t* rAddr
                        , void* aStats
                        , void* cState){

    //stats
    IncConnStats(aStats, tcpConnInit);

    //check ipv6
    int is_ipv6;
    CHECK_IPV6(lAddr, &is_ipv6);

    //create socket
    int socket_fd = -1;
    if (is_ipv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        IncConnStats(aStats, socketCreateFail);
        SetCES(cState, STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        IncConnStats(aStats, socketCreate);
        SetCS1(cState, STATE_TCP_SOCK_CREATE);

        int setsockopt_status = -1;
        setsockopt_status = setsockopt(socket_fd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));

        //???
        setsockopt(socket_fd, SOL_IP, IP_TRANSPARENT, &(int){ 1 }, sizeof(int));
        // printf ("IP_TRANSPARENT = %d\n", ret_val);

        if (setsockopt_status == -1){
            IncConnStats(aStats, socketReuseSetFail);
            SetCES(cState, STATE_TCP_SOCK_REUSE_FAIL);
        } else {
            IncConnStats(aStats, socketReuseSet);
            SetCS1(cState, STATE_TCP_SOCK_REUSE);
            //bind local socket
            int bind_status = -1;
            if (is_ipv6){
                bind_status = bind(socket_fd
                                    , (struct sockaddr*)lAddr
                                    , sizeof(struct sockaddr_in6));
            }else{
                bind_status = bind(socket_fd
                                    , (struct sockaddr*)lAddr
                                    , sizeof(struct sockaddr_in));
            }

            if (bind_status == -1){
                if (is_ipv6){
                    IncConnStats(aStats, socketBindIpv6Fail);
                }else{
                    IncConnStats(aStats, socketBindIpv4Fail);
                }
                SetCES(cState, STATE_TCP_SOCK_BIND_FAIL);
            }else{
                if (is_ipv6){
                    IncConnStats(aStats, socketBindIpv6);
                }else{
                    IncConnStats(aStats, socketBindIpv4);
                }
                SetCS1(cState, STATE_TCP_SOCK_BIND);

                //connect socket
                int connect_status = -1;
                if (is_ipv6){
                    connect_status = connect(socket_fd
                                    , (struct sockaddr*)rAddr
                                    , sizeof(struct sockaddr_in6));
                }else{
                    connect_status = connect(socket_fd
                                    , (struct sockaddr*)rAddr
                                    , sizeof(struct sockaddr_in));
                }
                SetCS1(cState, STATE_TCP_CONN_INIT);

                //check connect status
                if (connect_status < 0){
                    if (errno == EINPROGRESS){
                        SetCS1(cState, STATE_TCP_CONN_IN_PROGRESS);
                    }else{
                        SetCES(cState, STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE);
                        if ( (GetSysErrno(cState) == EADDRNOTAVAIL) ) {
                            IncConnStats(aStats, tcpConnInitFailImmediateEaddrNotAvail);
                        }else{
                            IncConnStats(aStats, tcpConnInitFailImmediateOther);
                        }
                    }
                }else{
                    SetCS1(cState, STATE_TCP_CONN_IN_PROGRESS2);
                }
            }
        }
    }


    if ( GetCES(cState) ){

        IncConnStats(aStats, tcpConnInitFail);

        if (socket_fd != -1){
            TcpClose(socket_fd, 0, 0, aStats, cState);
        }
        return -1;
    }

    return socket_fd;
}

void VerifyTcpConnectionEstablished(int fd
                                    , void* aStats
                                    , void* cState){
    int socketErr;
    socklen_t socketErrBufLen = sizeof(int);

    int retGetsockopt = getsockopt(fd
                                    , SOL_SOCKET
                                    , SO_ERROR
                                    , &socketErr
                                    , &socketErrBufLen);
    
    if ((retGetsockopt|socketErr) == 0){
        SetCS1 (cState, STATE_TCP_CONN_ESTABLISHED);
        IncConnStats(aStats, tcpConnInitSuccess);
    }else {
        SetCES(cState, STATE_TCP_SOCK_CONNECT_FAIL);
        SaveSockErrno(cState, socketErr);
        IncConnStats(aStats, tcpConnInitFail);
    }
}

int TcpListenStart(SockAddr_t* lAddr
                    , int listenQLen
                    , void* aStats
                    , void* cState) {

    //check ipv6
    int is_ipv6;
    CHECK_IPV6(lAddr, &is_ipv6);

    //create socket
    int socket_fd = -1;
    if (is_ipv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        IncConnStats(aStats, socketCreateFail);
        SetCES(cState, STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        IncConnStats(aStats, socketCreate);
        SetCS1(cState, STATE_TCP_SOCK_CREATE);

        //???
        setsockopt(socket_fd, SOL_IP, IP_TRANSPARENT, &(int){ 1 }, sizeof(int));
        // printf ("IP_TRANSPARENT = %d\n", ret_val);

        

        //bind local socket
        int bind_status = -1;
        if (is_ipv6){
            bind_status = bind(socket_fd
                                , (struct sockaddr*)lAddr
                                , sizeof(struct sockaddr_in6));
        }else{

            //???
            // struct addrinfo hints, *res;            
            // memset(&hints, 0, sizeof(hints));
            // if(getaddrinfo(NULL, "883", &hints, &res) != 0){
            //     perror("getaddrinfo: ");
            //     exit(EXIT_FAILURE);
            // }

            // bind_status = bind(socket_fd, res->ai_addr, res->ai_addrlen);

            bind_status = bind(socket_fd
                                , (struct sockaddr*)lAddr
                                , sizeof(struct sockaddr_in));
        }

        if (bind_status == -1){
            if (is_ipv6){
                IncConnStats(aStats, socketBindIpv6Fail);
            }else{
                IncConnStats(aStats, socketBindIpv4Fail);
            }
            SetCES(cState, STATE_TCP_SOCK_BIND_FAIL);
        }else{
            if (is_ipv6){
                IncConnStats(aStats, socketBindIpv6);
            }else{
                IncConnStats(aStats, socketBindIpv4);
            }
            SetCS1(cState, STATE_TCP_SOCK_BIND);

            //listen socket
            int listen_status = -1;
            listen_status = listen(socket_fd, listenQLen);

            //check listen status
            if (listen_status < 0) {
               SetCES(cState, STATE_TCP_SOCK_LISTEN_FAIL); 
            } else {
                SetCS1(cState, STATE_TCP_LISTENING);
            }
        }
    }

    if ( GetCES(cState) ){

        IncConnStats(aStats, tcpListenStartFail);

        if (socket_fd != -1){
            TcpClose(socket_fd, 0, 0, aStats, cState);
        }
        return -1;
    }

    return socket_fd;
}

void TcpClose(int fd
                , int isLinger
                , int lingerTime
                , void* aStats
                , void* cState) {

    if (isLinger) {
        struct linger sl;
        sl.l_onoff = 1;
        sl.l_linger = lingerTime;

        int lingerStatus 
            = setsockopt(fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
        
        if (lingerStatus < 0) {
            IncConnStats(aStats, socketLingerSetFail);
            SetCES(cState, STATE_TCP_SOCK_LINGER_FAIL);
        } else {
            IncConnStats(aStats, socketLingerSet);
            SetCS1(cState, STATE_TCP_SOCK_LINGER);
        }
    }

    if ( close(fd) ) {
        SetCES(cState, STATE_TCP_SOCK_FD_CLOSE_FAIL);
    } else {
        SetCS1(cState, STATE_TCP_SOCK_FD_CLOSE);
    }
}

void TcpWrShutdown(int fd, void* cState) {    
    if (shutdown(fd, SHUT_WR)) {
        SetCES(cState, STATE_TCP_FIN_SEND_FAIL);
    } else {
        SetCS1(cState, STATE_TCP_SENT_FIN);
    }
}

int TcpWrite(int fd
                , const char* dataBuffer
                , int dataLen
                , void* aStats
                , void* cState) {
    // int bytesSent = send(fd, dataBuffer, dataLen, MSG_NOSIGNAL);
    int bytesSent = write(fd, dataBuffer, dataLen);

    if (bytesSent <= 0){
        SetCES(cState, STATE_TCP_SOCK_WRITE_FAIL);
        IncConnStats(aStats, tcpWriteFail);

        if (bytesSent == 0) {
            IncConnStats(aStats, tcpWriteReturnsZero);
        }
    }

    return bytesSent;
}

int TcpRead(int fd
                , char* dataBuffer
                , int dataLen
                , void* aStats
                , void* cState) {
    int bytesRead = read(fd, dataBuffer, dataLen);

    if (bytesRead == 0) {
        SetCS1 (cState, STATE_TCP_REMOTE_CLOSED);
        int socketErr;
        socklen_t socketErrBufLen = sizeof(int);

        int retGetsockopt = getsockopt(fd
                                        , SOL_SOCKET
                                        , SO_ERROR
                                        , &socketErr
                                        , &socketErrBufLen);
        if ((retGetsockopt|socketErr)) {
            SetCES(cState, STATE_TCP_SOCK_READ_FAIL);
        }
    } else if (bytesRead < 0) {
        if (errno == EAGAIN) {
            //nothing to read; retry
        } else {
            SetCES(cState, STATE_TCP_SOCK_READ_FAIL);
            IncConnStats(aStats, tcpReadFail);
        }
    }

    return bytesRead;
}

int TcpAcceptConnection(int listenerFd
                        , SockAddr_t* lAddr
                        , SockAddr_t* rAddr
                        , void* aStats
                        , void* cState) {
    
    SetCS1(cState, STATE_TCP_CONN_ACCEPT);

    int socket_fd = -1;
    socklen_t addrLen = sizeof (SockAddr_t);

    socket_fd = accept(listenerFd
                        , (struct sockaddr *) rAddr
                        , &addrLen);
    
    if (socket_fd < 0) {
        IncConnStats(aStats, tcpAcceptFail);
        SetCES(cState, STATE_TCP_CONN_ACCEPT_FAIL);        
    } else {
        SetCS1 (cState, STATE_TCP_CONN_ESTABLISHED);

        IncConnStats(aStats, tcpAcceptSuccess);

        int flags = fcntl(socket_fd, F_GETFL, 0);
        if (flags < 0) {
            SetCES(cState, STATE_TCP_SOCK_F_GETFL_FAIL 
                            | STATE_TCP_SOCK_O_NONBLOCK_FAIL);
        } else {
            int ret = fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
            if (ret < 0) {
                SetCES(cState, STATE_TCP_SOCK_F_SETFL_FAIL 
                                | STATE_TCP_SOCK_O_NONBLOCK_FAIL);
            } else {
                SetCS1(cState, STATE_TCP_CONN_ACCEPT_O_NONBLOCK);

                ret = setsockopt(socket_fd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));
                if (ret < 0) {
                    IncConnStats(aStats, socketReuseSetFail);
                    SetCES(cState, STATE_TCP_SOCK_REUSE_FAIL);
                } else {
                    IncConnStats(aStats, socketReuseSet);
                    SetCS1(cState, STATE_TCP_SOCK_REUSE);

                    addrLen = sizeof (SockAddr_t);
                    ret = getsockname(socket_fd, (struct sockaddr*) lAddr, &addrLen);
                    if (ret < 0) {
                        IncConnStats(aStats, tcpGetSockNameFail);
                        SetCES(cState, STATE_TCP_GETSOCKNAME_FAIL);
                    }
                }
            }
        }
    }

    if ( GetCES(cState) ){
        if (socket_fd != -1){
            TcpClose(socket_fd, 0, 0, aStats, cState);
        }
        return -1;
    }

    return socket_fd;    
}

void DoSSLConnect(SSL* newSSL
                    , int fd
                    , int isClient
                    , void* aStats
                    , void* cState) {

    if (IsSetCS1(cState, STATE_SSL_CONN_INIT) == 0) {
        SetCS1(cState, STATE_SSL_CONN_INIT);
        IncConnStats(aStats, sslConnInit);
        int status = SSL_set_fd(newSSL, fd);

        if (status != 1) {
            SetCES(cState, STATE_SSL_SOCK_CONNECT_FAIL
                            | STATE_SSL_SOCK_FD_SET_ERROR);
        }
        if (isClient) {
            SSL_set_connect_state (newSSL);
        } else {
            SSL_set_accept_state (newSSL);
        }
    }

    if ( (IsSetCS1(cState, STATE_SSL_CONN_ESTABLISHED)
            && IsSetCES (cState, STATE_SSL_SOCK_CONNECT_FAIL)) == 0 ) {

        int status = SSL_do_handshake(newSSL);
        int sslErrno = SSL_get_error (newSSL, status);
        if (status == 1) {
            SetCS1(cState, STATE_SSL_CONN_ESTABLISHED);
            IncConnStats(aStats, sslConnInitSuccess);
        } else if (status == -1) {
            if (IsSetCS1(cState, STATE_SSL_CONN_IN_PROGRESS) == 0) {
                SetCS1(cState, STATE_SSL_CONN_IN_PROGRESS);
                IncConnStats(aStats, sslConnInitProgress);
            }
            switch (sslErrno) {
                case SSL_ERROR_WANT_READ:
                    SetCS1(cState, STATE_SSL_HANDSHAKE_WANT_READ);
                    break;
                case SSL_ERROR_WANT_WRITE:
                    SetCS1(cState, STATE_SSL_HANDSHAKE_WANT_WRITE);
                    break;
                default:
                    SetCESSL(cState
                                , STATE_SSL_SOCK_CONNECT_FAIL
                                , sslErrno);
                    IncConnStats(aStats, sslConnInitFail);
                    break;  
            }  
        } else {
            SetCESSL(cState
                        , STATE_SSL_SOCK_CONNECT_FAIL
                        , sslErrno);
            IncConnStats(aStats, sslConnInitFail);
        }               
    }
}

int SSLRead (SSL* newSSL
                , char* dataBuffer
                , int dataLen
                , void* aStats
                , void* cState) {

    int bytesSent = SSL_read(newSSL, dataBuffer, dataLen);

    if (bytesSent <= 0) {
        int sslError = SSL_get_error(newSSL, bytesSent);
        switch (sslError) {
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
                SetCS1 (cState, STATE_TCP_REMOTE_CLOSED);
                break;
  
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            
            default:
                SetCES(cState, STATE_SSL_SOCK_GENERAL_ERROR);
                break;
        }
    }

    return bytesSent;
}

int SSLWrite (SSL* newSSL
                , const char* dataBuffer
                , int dataLen
                , void* aStats
                , void* cState) {

    int bytesSent = SSL_write(newSSL, dataBuffer, dataLen);

    if (bytesSent <= 0) {
        int sslError = SSL_get_error(newSSL, bytesSent);
        switch (sslError) {
            case SSL_ERROR_SYSCALL:
                SetCES(cState, STATE_TCP_REMOTE_CLOSED_ERROR);
                break;

            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            
            default:
                SetCES(cState, STATE_SSL_SOCK_GENERAL_ERROR);
                break;
        }
    }

    return bytesSent;
}

void SSLShutdown (SSL* newSSL
                , void* cState) {

    int status = SSL_shutdown(newSSL);
    int sslError = SSL_get_error(newSSL, status);
    
    switch (status) {
        case 1:
            SetCS1 (cState, STATE_SSL_RECEIVED_SHUTDOWN);
            ClearCS1 (cState, STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);
            SetCS1 (cState, STATE_SSL_SENT_SHUTDOWN);
            ClearCS1 (cState, STATE_SSL_TO_SEND_SHUTDOWN);
            break;

        case 0:
            SetCS1 (cState, STATE_SSL_SENT_SHUTDOWN);
            ClearCS1 (cState, STATE_SSL_TO_SEND_SHUTDOWN);
            break;

        default:
            switch (sslError) {
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    break;
                
                default:
                    ClearCS1 (cState, STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);
                    ClearCS1 (cState, STATE_SSL_TO_SEND_SHUTDOWN);
                    SetCES(cState, STATE_SSL_SOCK_GENERAL_ERROR);
                    break;
            }
            break;
    }
}
