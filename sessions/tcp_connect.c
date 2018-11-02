#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "sessions.h"
#include "tcp_connect.h"
#include "logging/logs.h"


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
                        , void* bStats
                        , void* cState){
    //create socket
    int socket_fd = -1;
    if (IsIpv6(lAddr)){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        IncConnStats2(aStats, bStats, socketCreateFail);
        SetCES(cState, STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        IncConnStats2(aStats, bStats, socketCreate);
        SetCS1(cState, STATE_TCP_SOCK_CREATE);

        int setsockopt_status = -1;
        setsockopt_status = setsockopt(socket_fd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));

        if (setsockopt_status == -1){
            IncConnStats2(aStats, bStats, socketReuseSetFail);
            SetCES(cState, STATE_TCP_SOCK_REUSE_FAIL);
        } else {
            IncConnStats2(aStats, bStats, socketReuseSet);
            SetCS1(cState, STATE_TCP_SOCK_REUSE);
            //bind local socket
            int bind_status = -1;
            if (IsIpv6(lAddr)){
                bind_status = bind(socket_fd, (struct sockaddr*)lAddr, sizeof(struct sockaddr_in6));
            }else{
                bind_status = bind(socket_fd, (struct sockaddr*)lAddr, sizeof(struct sockaddr_in));
            }

            if (bind_status == -1){
                if (IsIpv6(lAddr)){
                    IncConnStats2(aStats, bStats, socketBindIpv6Fail);
                }else{
                    IncConnStats2(aStats, bStats, socketBindIpv4Fail);
                }
                SetCES(cState, STATE_TCP_SOCK_BIND_FAIL);
            }else{
                if (IsIpv6(lAddr)){
                    IncConnStats2(aStats, bStats, socketBindIpv6);
                }else{
                    IncConnStats2(aStats, bStats, socketBindIpv4);
                }
                SetCS1(cState, STATE_TCP_SOCK_BIND);

                //connect socket
                int connect_status = -1;
                if (IsIpv6(lAddr)){
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
                    }
                }else{
                    SetCS1(cState, STATE_TCP_CONN_IN_PROGRESS2);
                }
    }
        }
    }


    if ( GetCES(cState) ){
        if (socket_fd != -1){
            TcpClose(socket_fd, cState);
        }
        return -1;
    }

    return socket_fd;
}

void VerifyTcpConnectionEstablished(int fd, void* cState){
    int socketErr;
    socklen_t socketErrBufLen = sizeof(int);

    int retGetsockopt = getsockopt(fd
                                    , SOL_SOCKET
                                    , SO_ERROR
                                    , &socketErr
                                    , &socketErrBufLen);
    
    if ((retGetsockopt|socketErr) == 0){
        SetCS1 (cState, STATE_TCP_CONN_ESTABLISHED);
    }else {
        SetCES(cState, STATE_TCP_SOCK_CONNECT_FAIL);
        SaveSockErrno(cState, socketErr);
    }
}

int TcpListenStart(SockAddr_t* lAddr
                    , int listenQLen
                    , void* aStats
                    , void* bStats
                    , void* cState) {
    //create socket
    int socket_fd = -1;
    if (IsIpv6(lAddr)){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        IncConnStats2(aStats, bStats, socketCreateFail);
        SetCES(cState, STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        IncConnStats2(aStats, bStats, socketCreate);
        SetCS1(cState, STATE_TCP_SOCK_CREATE);

        //bind local socket
        int bind_status = -1;
        if (IsIpv6(lAddr)){
            bind_status = bind(socket_fd, (struct sockaddr*)lAddr, sizeof(struct sockaddr_in6));
        }else{
            bind_status = bind(socket_fd, (struct sockaddr*)lAddr, sizeof(struct sockaddr_in));
        }

        if (bind_status == -1){
            if (IsIpv6(lAddr)){
                IncConnStats2(aStats, bStats, socketBindIpv6Fail);
            }else{
                IncConnStats2(aStats, bStats, socketBindIpv4Fail);
            }
            SetCES(cState, STATE_TCP_SOCK_BIND_FAIL);
        }else{
            if (IsIpv6(lAddr)){
                IncConnStats2(aStats, bStats, socketBindIpv6);
            }else{
                IncConnStats2(aStats, bStats, socketBindIpv4);
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
        if (socket_fd != -1){
            TcpClose(socket_fd, cState);
        }
        return -1;
    }

    return socket_fd;
}

void TcpClose(int fd, void* cState){

    if ( close(fd) ) {
        SetCES(cState, STATE_TCP_SOCK_FD_CLOSE_FAIL);
    } else {
        SetCS1(cState, STATE_TCP_SOCK_FD_CLOSE);
    }
}

int TcpWrite(int fd
                , const char* dataBuffer
                , int dataLen
                , void* aStats
                , void* bStats
                , void* cState) {
    int bytesSent = send(fd, dataBuffer, dataLen, 0);

    if (bytesSent < 0){
        SetCES(cState, STATE_TCP_SOCK_WRITE_FAIL);
    }

    return bytesSent;
}

int TcpRead(int fd
                , char* dataBuffer
                , int dataLen
                , void* aStats
                , void* bStats
                , void* cState) {
    int bytesRead = recv(fd, dataBuffer, dataLen, 0);

    if (bytesRead < 0){
        SetCES(cState, STATE_TCP_SOCK_READ_FAIL);
    }

    return bytesRead;
}