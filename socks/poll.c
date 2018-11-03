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

#include "libsock.h"

void SetPollEvent(int pollId
                , int fd
                , int pollRead
                , int pollWrite
                , void* cState) {
    enum Actions {PollAdd, PollMod, PollDel, PollNop} pollAction;

    pollAction = PollNop;

   if ( IsSetCS1(cState, STATE_TCP_POLL_READ_CURRENT 
                            | STATE_TCP_POLL_WRITE_CURRENT) ) {
        if (pollRead || pollWrite) {
            pollAction = PollMod;
        }else {
            pollAction = PollDel;
        }
    }else{
        if (pollRead || pollWrite) {
            pollAction = PollAdd;
        }
    }

    if (pollAction != PollNop) {

        struct epoll_event setEvent;

        setEvent.data.ptr = cState;

        if (pollRead && pollWrite) {
            setEvent.events = EPOLLIN | EPOLLOUT;
        } else if (pollRead) {
            setEvent.events = EPOLLIN;
        } else if (pollWrite) {
            setEvent.events = EPOLLOUT;
        }

        int status = -1;
        switch (pollAction) {
            case PollAdd:
                status = epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
                break;
            case PollMod:
                status = epoll_ctl(pollId, EPOLL_CTL_MOD, fd, &setEvent);
                break;
            case PollDel:
                status = epoll_ctl(pollId, EPOLL_CTL_DEL, fd, &setEvent);
                break;
            case PollNop:
                break;
        }

        if (status) {
            SetCES(cState, STATE_TCP_SOCK_POLL_UPDATE_FAIL);
        }else{
            if (pollRead) {
                SetCS1(cState, STATE_TCP_POLL_READ_CURRENT
                                | STATE_TCP_POLL_READ_STICKY);
            }else{
                ClearCS1(cState, STATE_TCP_POLL_READ_CURRENT);
            }
            if (pollWrite) {
                SetCS1(cState, STATE_TCP_POLL_WRITE_CURRENT
                                | STATE_TCP_POLL_WRITE_STICKY);
            }else{
                ClearCS1(cState, STATE_TCP_POLL_WRITE_CURRENT);
            }
        }
    }
}

void PollReadWriteEvent(int pollId
                        , int fd
                        , void* cState) {
    SetPollEvent(pollId, fd, 1, 1, cState);
}

void PollOnlyReadEvent(int pollId
                        , int fd
                        , void* cState) {
    SetPollEvent(pollId, fd, 1, 0, cState);
}

void PollOnlyWriteEvent(int pollId
                        , int fd
                        , void* cState) {
    SetPollEvent(pollId, fd, 0, 1, cState);
}

void StopPollReadWriteEvent(int pollId
                        , int fd
                        , void* cState) {
    SetPollEvent(pollId, fd, 0, 0, cState);
}


void AssignSocketLocalPort(SockAddr_t* localAddres
                            , LocalPortPool_t* portPool
                            , void* cState) {
   int nextSrcPort = GetPortFromPool(portPool);
   if (nextSrcPort) {
       SetSockPort(localAddres, nextSrcPort);
       SetCS1(cState, STATE_TCP_PORT_ASSIGNED);
    }else{
        SetCES(cState, STATE_TCP_SOCK_PORT_ASSIGN_FAIL);
    }
}

void DumpCStats(void* aStats) {
    ConnStats_t* cStats = (ConnStats_t*) aStats;
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

void AddressToString(SockAddr_t* addr, char* str){

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