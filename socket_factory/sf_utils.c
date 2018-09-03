#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <gmodule.h>

#include "td_states.h"
#include "lf_utils.h"
#include "sf_utils.h"

/** @brief Initiate the TCP connection establishment process 
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
                        , struct TDLogEntry* tdLogEntry)
{
    int ret_state = TD_PROGRAM_ERROR; 

    //create socket
    int socket_fd = -1;
    if (isIpv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        TDLog(tdLogEntry,"failed to create socket");
        ret_state = TD_SOCKET_CREATE_FAILED;
    }else{
        //bind local socket
        int bind_status = -1;
        if (isIpv6){
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in6));
        }else{
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in));
        }
        if (bind_status == -1){
            TDLog(tdLogEntry,"failed to bind socket");
            ret_state = TD_SOCKET_BIND_FAILED;
        }else{
            //connect socket
            int connect_status = -1;
            if (isIpv6){
                connect_status = connect(socket_fd, remoteAddress, sizeof(struct sockaddr_in6));
            }else{
                connect_status = connect(socket_fd, remoteAddress, sizeof(struct sockaddr_in));
            }
            //check connect status
            if (connect_status < 0){
                if (errno == EINPROGRESS){
                    TDLog(tdLogEntry,"connection in progress");
                    ret_state = TD_NO_ERROR;
                }else{
                    TDLog(tdLogEntry,"failed to connect");
                    ret_state = TD_SOCKET_CONNECT_FAILED;
                }
            }else{
                TDLog(tdLogEntry,"connection in progress2");
                ret_state = TD_NO_ERROR;
            }
        }
    }

    if (ret_state != TD_NO_ERROR){
        if (socket_fd != -1){
            close(socket_fd);
            TDLog(tdLogEntry,"socket close");
        }
        return -1;
    }

    return socket_fd;
}
