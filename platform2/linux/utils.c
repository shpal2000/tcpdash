#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "platform.h"

void AssignSocketLocalPort(SockAddr_t* localAddres
                            , LocalPortPool_t* portPool
                            , SockStats_t** statsArr
                            , int statsCount
                            , void* cState) {
   int nextSrcPort = GetPortFromPool(portPool);

   if (nextSrcPort) {
       SET_SOCK_PORT(localAddres, nextSrcPort);
       SetCS1(cState, STATE_TCP_PORT_ASSIGNED);
       
    }else{
        SetCES(cState, STATE_TCP_SOCK_PORT_ASSIGN_FAIL);
        IncConnStats(statsArr, statsCount, tcpLocalPortAssignFail);    
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

void SetSockStats (SockStats_t* cStats
                    , JObject* jObj) {

    // JSET_MEMBER_INT (jObj, "socketCreate", cStats->socketCreate);
    // JSET_MEMBER_INT (jObj, "socketCreateFail", cStats->socketCreateFail);
    // JSET_MEMBER_INT (jObj, "socketListenFail", cStats->socketListenFail);

    // JSET_MEMBER_INT (jObj, "tcpAcceptFail", cStats->tcpAcceptFail);
    // JSET_MEMBER_INT (jObj, "tcpAcceptSuccess", cStats->tcpAcceptSuccess);

    // JSET_MEMBER_INT (jObj, "tcpConnInit", cStats->tcpConnInit);
    // JSET_MEMBER_INT (jObj, "tcpConnInitSuccess", cStats->tcpConnInitSuccess);
    // JSET_MEMBER_INT (jObj, "tcpConnInitFail", cStats->tcpConnInitFail);

    JSET_MEMBER_INT (jObj, "tcpConnInitRate", cStats->tcpConnInitRate);
    JSET_MEMBER_INT (jObj, "tcpConnInitSuccessRate", cStats->tcpConnInitSuccessRate);
    JSET_MEMBER_INT (jObj, "sslConnInitRate", cStats->sslConnInitRate);
    JSET_MEMBER_INT (jObj, "sslConnInitSuccessRate", cStats->sslConnInitSuccessRate);

    JSET_MEMBER_INT (jObj, "tcpConnInitFail", cStats->tcpConnInitFail);
    JSET_MEMBER_INT (jObj, "tcpConnInitFail-I", cStats->tcpConnInitFailImmediateEaddrNotAvail);
    JSET_MEMBER_INT (jObj, "tcpConnInitFail-IO", cStats->tcpConnInitFailImmediateOther);

}

void SetSockStatsRate (SockStats_t* cStats) {

    SET_CONN_RATE (cStats, tcpConnInit);
    SET_CONN_RATE (cStats, tcpConnInitSuccess);
    SET_CONN_RATE (cStats, tcpAcceptSuccess);

    SET_CONN_RATE (cStats, sslConnInit);
    SET_CONN_RATE (cStats, sslConnInitSuccess);
    SET_CONN_RATE (cStats, sslAcceptSuccess);
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
