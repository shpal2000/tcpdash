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
 *  @param isIpv6 0 : ipv4; else ipv6
 *  @param localAddress local socket address pointer
 *  @param remoteAddress remote socket address pointer
 *  @return file descriptor of newly created tcp socket; -1 for error
 */
int TcpNewConnection(int isIpv6 
                        , struct sockaddr* localAddress
                        , struct sockaddr* remoteAddress
                        , void* aStats
                        , void* bStats
                        , void* cState){

    ResetErrno();
    InitSSLastErr(cState, TD_PROGRAM_ERROR_TcpNewConnection);

    //create socket
    int socket_fd = -1;
    if (isIpv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        IncSStats2(aStats, bStats, socketCreateFail);
        SetSSLastErr(cState, TD_SOCKET_CREATE_FAILED);
    }else{
        IncSStats2(aStats, bStats, socketCreate);
        SetSS1(cState, STATE_TCP_SOCK_CREATE);

        int setsockopt_status = -1;
        setsockopt_status = setsockopt(socket_fd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));

        if (setsockopt_status == -1){
            IncSStats2(aStats, bStats, socketReuseSetFail);
            SetSSLastErr(cState, TD_SOCKET_REUSE_FAILED);
        } else {
            IncSStats2(aStats, bStats, socketReuseSet);
            SetSS1(cState, STATE_TCP_SOCK_REUSE);
            //bind local socket
            int bind_status = -1;
            if (isIpv6){
                bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in6));
            }else{
                bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in));
            }

            if (bind_status == -1){
                if (isIpv6){
                    IncSStats2(aStats, bStats, socketBindIpv6Fail);
                }else{
                    IncSStats2(aStats, bStats, socketBindIpv4Fail);
                }
                SetSSLastErr(cState, TD_SOCKET_BIND_FAILED);
            }else{
                if (isIpv6){
                    IncSStats2(aStats, bStats, socketBindIpv6);
                }else{
                    IncSStats2(aStats, bStats, socketBindIpv4);
                }
                SetSS1(cState, STATE_TCP_SOCK_BIND);

                //connect socket
                int connect_status = -1;
                if (isIpv6){
                    connect_status = connect(socket_fd
                                    , remoteAddress
                                    , sizeof(struct sockaddr_in6));
                }else{
                    connect_status = connect(socket_fd
                                        , remoteAddress
                                        , sizeof(struct sockaddr_in));
                }
                SetSS1(cState, STATE_TCP_CONN_INIT);

                //check connect status
                if (connect_status < 0){
                    if (errno == EINPROGRESS){
                        SetSS1(cState, STATE_TCP_CONN_IN_PROGRESS);
                        SetSSLastErr(cState, TD_NO_ERROR);
                    }else{
                        SetSSLastErr(cState
                                , TD_SOCKET_CONNECT_FAILED_IMMEDIATE);
                    }
                }else{
                    SetSS1(cState, STATE_TCP_CONN_IN_PROGRESS2);
                    SetSSLastErr(cState, TD_NO_ERROR);
                }
    }
        }
    }

    if (GetSSLastErr(cState) != TD_NO_ERROR){
        SaveErrno(cState);
        if (socket_fd != -1){
            close(socket_fd);
            SetSS1(cState, STATE_TCP_SOCK_FD_CLOSE);
        }
        return -1;
    }

    return socket_fd;
}

int IsNewTcpConnectionComplete(int fd){
    int socketErr;
    socklen_t socketErrBufLen = sizeof(int);

    int retGetsockopt = getsockopt(fd
                                    , SOL_SOCKET
                                    , SO_ERROR
                                    , &socketErr
                                    , &socketErrBufLen);
    
    if ((retGetsockopt|socketErr) == 0){
        return 1;
    }

    return 0;
}

int TcpListenStart(int isIpv6 
                    , struct sockaddr* localAddress
                    , int listenQLen
                    , void* aStats
                    , void* bStats
                    , void* cState) {

    ResetErrno();
    InitSSLastErr(cState, TD_PROGRAM_ERROR_TcpListenStart);

    //create socket
    int socket_fd = -1;
    if (isIpv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        IncSStats2(aStats, bStats, socketCreateFail);
        SetSSLastErr(cState, TD_SOCKET_CREATE_FAILED);
    }else{
        IncSStats2(aStats, bStats, socketCreate);
        SetSS1(cState, STATE_TCP_SOCK_CREATE);

        //bind local socket
        int bind_status = -1;
        if (isIpv6){
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in6));
        }else{
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in));
        }

        if (bind_status == -1){
            if (isIpv6){
                IncSStats2(aStats, bStats, socketBindIpv6Fail);
            }else{
                IncSStats2(aStats, bStats, socketBindIpv4Fail);
            }
            SetSSLastErr(cState, TD_SOCKET_BIND_FAILED);
        }else{
            if (isIpv6){
                IncSStats2(aStats, bStats, socketBindIpv6);
            }else{
                IncSStats2(aStats, bStats, socketBindIpv4);
            }
            SetSS1(cState, STATE_TCP_SOCK_BIND);

            //listen socket
            int listen_status = -1;
            listen_status = listen(socket_fd, listenQLen);

            //check listen status
            if (listen_status < 0) {
               SetSSLastErr(cState, TD_SOCKET_LISTEN_FAILED); 
            } else {
                SetSS1(cState, STATE_TCP_LISTENING);
                SetSSLastErr(cState, TD_NO_ERROR); 
            }
        }
    }

    if (GetSSLastErr(cState) != TD_NO_ERROR){
        SaveErrno(cState);
        if (socket_fd != -1){
            close(socket_fd);
            SetSS1(cState, STATE_TCP_SOCK_FD_CLOSE);
        }
        return -1;
    }

    return socket_fd;
}