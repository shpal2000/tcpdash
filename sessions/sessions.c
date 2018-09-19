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

void RegisterForWriteEvent(int pollId, int fd, void* data) {
    struct epoll_event setEvent;
    setEvent.events = EPOLLOUT;
    setEvent.data.ptr = data;
    epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
}

void DumpSessionStats(void* aStats) {
    printf ("\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64 "\n%" PRIu64"\n"
                , GetSStats(aStats, socketCreate)
                , GetSStats(aStats, socketCreateFail)
                , GetSStats(aStats, socketBindIpv4)
                , GetSStats(aStats, socketBindIpv4Fail)
                , GetSStats(aStats, socketBindIpv6)
                , GetSStats(aStats, socketBindIpv6Fail)
                , GetSStats(aStats, socketConnectEstablishFail)
                , GetSStats(aStats, socketConnectEstablishFail2)
                , GetSStats(aStats, programErrorTcpNewConnection)
                );
}
