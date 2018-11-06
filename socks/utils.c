#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "libsock.h"


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