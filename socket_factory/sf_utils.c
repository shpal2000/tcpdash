#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <gmodule.h>

#include "lf_utils.h"
#include "sstates.h"
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
                        , struct TDSessionState* tdSessionState)
{
    TDSetSessionLastErr(tdSessionState, TD_PROGRAM_ERROR_TcpNewConnection);

    //create socket
    int socket_fd = -1;
    if (isIpv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        TDSetSessionLastErr(tdSessionState, TD_SOCKET_CREATE_FAILED);
    }else{
        TDSetSessionState1(tdSessionState, STATE_TCP_SOCK_CREATE);

        //bind local socket
        int bind_status = -1;
        if (isIpv6){
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in6));
        }else{
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in));
        }
        
        if (bind_status == -1){
            TDSetSessionLastErr(tdSessionState, TD_SOCKET_BIND_FAILED);
        }else{
            TDSetSessionState1(tdSessionState, STATE_TCP_SOCK_BIND);

            //connect socket
            int connect_status = -1;
            if (isIpv6){
                connect_status = connect(socket_fd, remoteAddress, sizeof(struct sockaddr_in6));
            }else{
                connect_status = connect(socket_fd, remoteAddress, sizeof(struct sockaddr_in));
            }
            TDSetSessionState1(tdSessionState, STATE_TCP_CONN_INIT);

            //check connect status
            if (connect_status < 0){
                if (errno == EINPROGRESS){
                    TDSetSessionState1(tdSessionState, STATE_TCP_CONN_IN_PROGRESS);
                    TDSetSessionLastErr(tdSessionState, TD_NO_ERROR);
                }else{
                    TDSetSessionLastErr(tdSessionState, TD_SOCKET_CONNECT_ESTABLISH_FAILED);
                    TDSetSessionLastErr(tdSessionState, TD_NO_ERROR);
                }
            }else{
                TDSetSessionState1(tdSessionState, STATE_TCP_CONN_IN_PROGRESS2);
                TDSetSessionLastErr(tdSessionState, TD_NO_ERROR);
            }
        }
    }

    if (TDGetSessionLastErr(tdSessionState) != TD_NO_ERROR){
        if (socket_fd != -1){
            close(socket_fd);
            TDSetSessionState1(tdSessionState, STATE_TCP_SOCK_FD_CLOSE);
        }
        return -1;
    }

    return socket_fd;
}
