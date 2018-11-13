#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "api/common.h"

void AssignSocketLocalPort(SockAddr_t* localAddres
                            , LocalPortPool_t* portPool
                            , void* aStats
                            , void* bStats
                            , void* cState) {
   int nextSrcPort = GetPortFromPool(portPool);
   if (nextSrcPort) {
       SetSockPort(localAddres, nextSrcPort);
       SetCS1(cState, STATE_TCP_PORT_ASSIGNED);
    }else{
        SetCES(cState, STATE_TCP_SOCK_PORT_ASSIGN_FAIL);
        IncConnStats2(aStats, bStats, tcpLocalPortAssignFail);    
    }
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