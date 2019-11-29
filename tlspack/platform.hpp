#ifndef __TD_PLATFORM_H
#define __TD_PLATFORM_H

#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <queue>

union td_sockaddr {
    struct sockaddr_in in_addr;
    struct sockaddr_in6 in_addr6;
};

class td_sockstats{
public:
    uint64_t socketCreate;    
    uint64_t socketCreateFail;
    uint64_t socketListenFail;
    uint64_t socketReuseSet;
    uint64_t socketReuseSetFail;
    uint64_t socketLingerSet;
    uint64_t socketLingerSetFail;
    uint64_t socketBindIpv4;    
    uint64_t socketBindIpv4Fail;
    uint64_t socketBindIpv6;    
    uint64_t socketBindIpv6Fail;

    uint64_t socketConnectEstablishFail;    
    uint64_t socketConnectEstablishFail2;    

    uint64_t tcpConnInit;
    uint64_t tcpConnInitInSec;
    uint64_t tcpConnInitRate;
    uint64_t tcpConnInitSuccess;
    uint64_t tcpConnInitSuccessInSec;
    uint64_t tcpConnInitSuccessRate;
    uint64_t tcpConnInitFail;
    uint64_t tcpConnInitFailImmediateEaddrNotAvail;
    uint64_t tcpConnInitFailImmediateOther;
    uint64_t tcpConnInitProgress;
    uint64_t tcpWriteFail;
    uint64_t tcpWriteReturnsZero;
    uint64_t tcpReadFail;

    uint64_t tcpListenStart;
    uint64_t tcpListenStop;
    uint64_t tcpListenStartFail;
    uint64_t tcpAcceptFail;
    uint64_t tcpAcceptSuccess;
    uint64_t tcpAcceptSuccessInSec;
    uint64_t tcpAcceptSuccessRate;

    uint64_t tcpLocalPortAssignFail;
    uint64_t tcpPollRegUnregFail;

    uint64_t sslConnInit;
    uint64_t sslConnInitInSec;
    uint64_t sslConnInitRate;
    uint64_t sslConnInitSuccess;
    uint64_t sslConnInitSuccessInSec;
    uint64_t sslConnInitSuccessRate;
    uint64_t sslConnInitFail;
    uint64_t sslConnInitProgress;
    uint64_t sslAcceptSuccess; 
    uint64_t sslAcceptSuccessInSec;
    uint64_t sslAcceptSuccessRate; 

    uint64_t tcpConnStructNotAvail;
    uint64_t tcpListenStructNotAvail;
    uint64_t appSessStructNotAvail;
    uint64_t tcpInitServerFail;
    uint64_t tcpGetSockNameFail;
};

typedef std::queue<uint16_t> td_portpool;
typedef std::vector<td_sockstats*> td_sockstats_arr;

#endif