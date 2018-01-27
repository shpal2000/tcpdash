#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>


int open_new_connection(int is_ipv6 
                        , struct sockaddr* local_address
                        , struct sockaddr* remote_address)
{
    enum RetState {
        RetState_NoErr=0,
        RetState_ProgErr,
        RetState_SocketCreateFailed,
        RetState_BindFailed,
        RetState_FcntlGetFailed,
        RetState_FcntlSetFailed,
        RetState_ConnectFailed        
    };

    enum RetState ret_state = RetState_ProgErr; 

    int socket_fd = -1;
    int bind_status = -1;
    int connect_status = -1;
    int socket_flags = -1;

    //create socket
    if (is_ipv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM , 0);

    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM , 0);
    }

    if (socket_fd == -1) {
        puts("failed to create socket");
        ret_state = RetState_SocketCreateFailed;
    }else{
        //bind local socket
        if (is_ipv6){
            bind_status = bind(socket_fd, local_address, sizeof(struct sockaddr_in6));
        }else{
            bind_status = bind(socket_fd, local_address, sizeof(struct sockaddr_in));
        }

        if (bind_status == -1){
            puts("failed to bind socket");
            ret_state = RetState_BindFailed;
        }else{
            //set socket in non-blocking mode
            socket_flags = fcntl(socket_fd, F_GETFL, 0);
            if (socket_flags < 0){
                puts("failed to get fcntl for socket");
                ret_state = RetState_FcntlGetFailed;
            }else{
                socket_flags |= O_NONBLOCK;
                if (fcntl(socket_fd, F_SETFL, socket_flags) < 0){
                    puts("failed to set fcntl for socket");
                    ret_state = RetState_FcntlSetFailed;
                }else{
                    //connect socket
                    if (is_ipv6){
                        connect_status = connect(socket_fd, remote_address, sizeof(struct sockaddr_in6));
                    }else{
                        connect_status = connect(socket_fd, remote_address, sizeof(struct sockaddr_in));
                    }
                    //check connect status
                    if (connect_status < 0){
                        puts("failed to connect");
                        ret_state = RetState_ConnectFailed;
                    }else{
                        puts("connect succeed");
                        ret_state = RetState_NoErr;
                    }                    
                }
            }
        }
    }

    if (ret_state != RetState_NoErr){
        //todo error handling
        return 0;
    }

    return -1;
}

int main(int argc, char** argv)
{
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_len = sizeof(struct sockaddr_in);    
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(9000);
    local_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_len = sizeof(struct sockaddr_in);    
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(9000);
    remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    open_new_connection(0, 
                        (struct sockaddr*) &local_addr, 
                        (struct sockaddr*) &remote_addr);

}