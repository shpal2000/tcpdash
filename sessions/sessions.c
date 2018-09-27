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

int RegisterForWriteEvent(int pollId, int fd, void* data) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLOUT;
    setEvent.data.ptr = data;
    return epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
}

void RegisterForReadEvent(int pollId, int fd, void* data) {
    ResetErrno();
    struct epoll_event setEvent;
    setEvent.events = EPOLLIN;
    setEvent.data.ptr = data;
    epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
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
