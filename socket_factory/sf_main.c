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

int tcp_start_new_connection(int is_ipv6 
                        , struct sockaddr* localAddress
                        , struct sockaddr* remoteAddress)
{
    enum RetState {
        RetState_NoErr=0,
        RetState_ProgErr,
        RetState_SocketCreateFailed,
        RetState_BindFailed,
        RetState_ConnectFailed
    };

    enum RetState ret_state = RetState_ProgErr; 

    //create socket
    int socket_fd = -1;
    if (is_ipv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        puts("failed to create socket");
        ret_state = RetState_SocketCreateFailed;
    }else{
        //bind local socket
        int bind_status = -1;
        if (is_ipv6){
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in6));
        }else{
            bind_status = bind(socket_fd, localAddress, sizeof(struct sockaddr_in));
        }
        if (bind_status == -1){
            puts("failed to bind socket");
            ret_state = RetState_BindFailed;
        }else{
            //connect socket
            int connect_status = -1;
            if (is_ipv6){
                connect_status = connect(socket_fd, remoteAddress, sizeof(struct sockaddr_in6));
            }else{
                connect_status = connect(socket_fd, remoteAddress, sizeof(struct sockaddr_in));
            }
            //check connect status
            if (connect_status < 0){
                if (errno == EINPROGRESS){
                    puts("connection in progress");
                    ret_state = RetState_NoErr;
                }else{
                    puts("failed to connect");
                    ret_state = RetState_ConnectFailed;
                }
            }else{
                puts("connection in progress2");
                ret_state = RetState_NoErr;
            }
        }
    }

    if (ret_state != RetState_NoErr){
        if (socket_fd != -1){
            close(socket_fd);
            puts("socket close");
        }
        return -1;
    }

    return socket_fd;
}

int main(int argc, char** argv)
{
    struct sockaddr_in localAddr;
    struct sockaddr_in remoteAddr;

    GSList* connCtxList = g_slist_alloc ();

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "10.116.0.61", &(localAddr.sin_addr));

    memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(80);
    inet_pton(AF_INET, "172.217.6.78", &(remoteAddr.sin_addr));

    int epfd = epoll_create(1);


    int socket_fd = tcp_start_new_connection(0, 
                        (struct sockaddr*) &localAddr, 
                        (struct sockaddr*) &remoteAddr);

    if (socket_fd == -1){
        puts("failed to connect");
    }else{
        struct epoll_event event;
        struct epoll_event events[1];
        event.events = EPOLLOUT;
        event.data.fd = socket_fd;
        event.data.ptr = &socket_fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);

        connCtxList = g_slist_append (connCtxList, GINT_TO_POINTER(socket_fd));

        int continue_app = 1;
        while(continue_app == 1){
            int num_ready = epoll_wait(epfd, events, 1, 0);
            if (num_ready > 0){
                for(int event_index = 0; event_index < num_ready; event_index++){
                    socket_fd = *((int*)events[event_index].data.ptr);
                    //check if connection succeed
                    int code;
                    socklen_t len = sizeof(int);
                    int ret = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &code, &len);
                    if ((ret|code) == 0){
                        puts("connected");
                        continue_app = 0;
                    }else{
                        perror("connect or write fail");
                        continue_app = 0;
                    }
                }
            }
        }


        puts("exit");
    }

    close(epfd);
}