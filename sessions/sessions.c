#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <gmodule.h>

#include "sessions.h"

void RegisterForWriteEvent(int pollId, int fd, void* data){
    struct epoll_event setEvent;
    setEvent.events = EPOLLOUT;
    setEvent.data.ptr = data;
    epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
}

