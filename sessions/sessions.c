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

int RegisterForWriteEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLOUT;
    setEvent.data.ptr = cState;
    int status = epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
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
    }
    return status;
}

int UnRegisterForEvent(int pollId, int fd, void* cState) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLIN | EPOLLOUT;
    setEvent.data.ptr = NULL;
    int status = epoll_ctl(pollId, EPOLL_CTL_DEL, fd, &setEvent);
    if (status) {
       SaveErrno(cState); 
    }
    return status;
}

int AssignSocketLocalPort(struct sockaddr* localAddres
                            , LocalPortPool_t* portPool
                            , void* cState)
{
   InitSSLastErr(cState, TD_PROGRAM_ERROR_AssignSocketLocalPort);

   int nextSrcPort = GetPortFromPool(portPool);
   if (nextSrcPort) {
       SetSockPort(localAddres, nextSrcPort);
       SetSS1(cState, STATE_TCP_PORT_ASSIGNED);
       SetSSLastErr(cState, TD_NO_ERROR);       
       return 0;
   }

   SetSSLastErr(cState, TD_ASSIGN_PORT_FAILED);
   return -1; 
}

void DumpSStats(void* aStats) {
    CommonConnStats_t* cStats = (CommonConnStats_t*) aStats;
    printf ("\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64"\n"
                , cStats->socketCreate
                , cStats->socketCreateFail
                , cStats->socketBindIpv4
                , cStats->socketBindIpv4Fail
                , cStats->socketBindIpv6
                , cStats->socketBindIpv6Fail
                , cStats->socketConnectEstablishFail
                , cStats->socketConnectEstablishFail2
                , cStats->programErrorTcpNewConnection);
}
