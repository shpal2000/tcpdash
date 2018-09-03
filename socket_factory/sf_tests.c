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

#include "sf_utils.h"

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


    int socket_fd = TcpNewConnection(0, 
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