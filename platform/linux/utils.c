#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "platform.h"

void AssignSocketLocalPort(SockAddr_t* localAddres
                            , LocalPortPool_t* portPool
                            , void* aStats
                            , void* bStats
                            , void* cState) {
   int nextSrcPort = GetPortFromPool(portPool);

   if (nextSrcPort) {
       SET_SOCK_PORT(localAddres, nextSrcPort);
       SetCS1(cState, STATE_TCP_PORT_ASSIGNED);
       
    }else{
        SetCES(cState, STATE_TCP_SOCK_PORT_ASSIGN_FAIL);
        IncConnStats2(aStats, bStats, tcpLocalPortAssignFail);    
    }
}

void AddressToString(SockAddr_t* addr, char* str) {

    //check ipv6
    int is_ipv6;
    CHECK_IPV6(addr, &is_ipv6);

   if (is_ipv6) {
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

void SetSockAddress(SockAddr_t* addr, char* str, int port) {

    //check str for ipv6 ???
    int is_ipv6 = 0;

    SetSockAddress0 (addr, is_ipv6);

    if (is_ipv6) {
        inet_pton(AF_INET6
                , str
                , &addr->in6Addr.sin6_addr);
        addr->in6Addr.sin6_port = htons(port);
    }else{
        inet_pton(AF_INET
                , str
                , &addr->inAddr.sin_addr);
        addr->inAddr.sin_port = htons(port);
    }
}

#if 0
int IsIpv6 (void* addr) {
    int isIpv6 = 0;
    struct sockaddr* uaddr = (struct sockaddr*) addr;
    if (uaddr->sa_family == AF_INET6) {
        isIpv6 = 1;
    }
    return isIpv6;
}

uint16_t GetSockPort(void* addr) {
   struct sockaddr* uaddr = addr;
   if (uaddr->sa_family == AF_INET6) {
       struct sockaddr_in6* addr_in6 = addr;
       return addr_in6->sin6_port; 
   }

    struct sockaddr_in* addr_in = addr;
    return addr_in->sin_port;
}

void SetSockPort (void* addr, uint16_t port) {
   struct sockaddr* uaddr = addr;
   if (uaddr->sa_family == AF_INET6) {
       struct sockaddr_in6* addr_in6 = addr;
       addr_in6->sin6_port = port;
   } else {
        struct sockaddr_in* addr_in = addr;
        addr_in->sin_port = port;
   }
}
#endif
