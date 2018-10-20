#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <gmodule.h>

#include "sessions.h"

int RegisterForReadWriteEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLIN | EPOLLOUT;
    setEvent.data.ptr = cState;
    int status = epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }else{
        SetCS1(cState
            , STATE_TCP_REGISTER_READ | STATE_TCP_REGISTER_WRITE);
    }
    return status;
}

int RegisterForReadEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLIN;
    setEvent.data.ptr = cState;
    int status = epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }else{
        SetCS1(cState, STATE_TCP_REGISTER_READ);
    }
    return status;
}

int RegisterForWriteEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLOUT;
    setEvent.data.ptr = cState;
    int status = epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }else{
        SetCS1(cState, STATE_TCP_REGISTER_WRITE);
    }
    return status;
}

int UnRegisterForReadWriteEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLIN | EPOLLOUT;
    setEvent.data.ptr = NULL;
    int status = epoll_ctl(pollId, EPOLL_CTL_DEL, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }else{
        SetCS1(cState
            , STATE_TCP_UNREGISTER_READ | STATE_TCP_UNREGISTER_WRITE);
    }
    return status;
}

int UnRegisterForReadEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLIN;
    setEvent.data.ptr = NULL;
    int status = epoll_ctl(pollId, EPOLL_CTL_DEL, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }else{
        SetCS1(cState, STATE_TCP_UNREGISTER_READ);
    }
    return status;
}

int UnRegisterForWriteEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLOUT;
    setEvent.data.ptr = NULL;
    int status = epoll_ctl(pollId, EPOLL_CTL_DEL, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }else{
        SetCS1(cState, STATE_TCP_UNREGISTER_WRITE);
    }
    return status;
}

int AssignSocketLocalPort(struct sockaddr* localAddres
                            , LocalPortPool_t* portPool
                            , void* cState) {
   int nextSrcPort = GetPortFromPool(portPool);
   if (nextSrcPort) {
       SetSockPort(localAddres, nextSrcPort);
       SetCS1(cState, STATE_TCP_PORT_ASSIGNED);
       SetConnLastErr(cState, TD_NO_ERROR);       
       return 0;
   }

   SetConnLastErr(cState, TD_ASSIGN_PORT_FAILED);
   return -1; 
}

void DumpCStats(void* aStats) {
    CommonConnStats_t* cStats = (CommonConnStats_t*) aStats;
    printf ("\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n"
                , cStats->socketCreate
                , cStats->socketCreateFail
                , cStats->socketBindIpv4
                , cStats->socketBindIpv4Fail
                , cStats->socketBindIpv6
                , cStats->socketBindIpv6Fail
                , cStats->socketConnectEstablishFail
                , cStats->socketConnectEstablishFail2);
}

void AddressToString(struct sockaddr* addr, char* str){

   if IsIpv6(addr) {
        inet_ntop(AF_INET6
            , &( ((struct sockaddr_in6*)addr)->sin6_addr )
            , str
            , INET6_ADDRSTRLEN);
   }else{
        inet_ntop(AF_INET
            , &( ((struct sockaddr_in*)addr)->sin_addr )
            , str
            , INET_ADDRSTRLEN);
   }

}