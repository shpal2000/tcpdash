#ifndef __TD_SESSIONS_H
#define __TD_SESSIONS_H

#include <glib.h>
#include <gmodule.h>
#include <sys/epoll.h>
#include "utils/resource.h"

typedef union SockAddr {
    struct sockaddr_in inAddr;
    struct sockaddr_in6 in6Addr;
} SockAddr_t;

typedef struct CommonConnStats{

    uint64_t socketCreate;    
    uint64_t socketCreateFail;
    uint64_t socketListenFail;
    uint64_t socketReuseSet;
    uint64_t socketReuseSetFail;
    uint64_t socketBindIpv4;    
    uint64_t socketBindIpv4Fail;
    uint64_t socketBindIpv6;    
    uint64_t socketBindIpv6Fail;

    uint64_t socketConnectEstablishFail;    
    uint64_t socketConnectEstablishFail2;    

    uint64_t tcpConnInit;
    uint64_t tcpConnInitSuccess;
    uint64_t tcpConnInitFail;
    uint64_t tcpConnInitFailEaddrNotAvail;
    uint64_t tcpConnInitProgress;
    uint64_t tcpWriteFail;

    uint64_t tcpListenStart;
    uint64_t tcpListenStop;
    uint64_t tcpListenStartFail;
    uint64_t appDataWriteComplete;

    uint64_t tcpConnRegisterForWriteEventFail;
    uint64_t tcpConnRegisterForReadEventFail;
    uint64_t tcpConnRegisterForListenerReadEventFail;
    uint64_t tcpConnUnRegisterForWriteEventFail;
    uint64_t tcpConnUnRegisterForReadEventFail;
    uint64_t tcpLocalPortAssignFail;

} CommonConnStats_t;

typedef struct CommonConnState{
    uint64_t state1;
    uint64_t state2;
    uint16_t lastErr;
    uint16_t lastErrCount;
    int sysErrno;
    int appState;                                                                  
} CommonConnState_t;

typedef struct epoll_event PollEvent_t;
typedef GQueue LocalPortPool_t;
typedef GQueue SessionPool_t;

typedef GTimer TimerWheel_t;

static inline void CSInit(void* cState) {

    ((CommonConnState_t*)cState)->state1 = 0;
    ((CommonConnState_t*)cState)->state2 = 0;
    ((CommonConnState_t*)cState)->lastErr = 0;
    ((CommonConnState_t*)cState)->lastErrCount = 0;
    ((CommonConnState_t*)cState)->sysErrno = 0;
    ((CommonConnState_t*)cState)->appState = 0;
}

#define SetAppState(__aConn, __state) ((CommonConnState_t*)__aConn)->appState = __state

#define GetAppState(__aConn) ((CommonConnState_t*)__aConn)->appState

#define SetCS1(__aConn, __state) ((CommonConnState_t*)__aConn)->state1 |= __state

#define SetCS2(__aConn, __state) ((CommonConnState_t*)__aConn)->state2 |= __state

#define GetCS1(__aConn) ((CommonConnState_t*)__aConn)->state1

#define GetCS2(__aConn) ((CommonConnState_t*)__aConn)->state2

#define InitConLastErr(__aConn, __err) ((CommonConnState_t*)__aConn)->lastErr = __err

#define IncConnStats(__aStats, __stat) ((CommonConnStats_t*)__aStats)->__stat++

#define IncConnStats2(__aStats, __bStats, __stat) ((CommonConnStats_t*)__aStats)->__stat++;((CommonConnStats_t*)__bStats)->__stat++

#define GetConnStats(__aStats, __stat) ((CommonConnStats_t*)__aStats)->__stat

static inline void SetConnLastErr(void* aSession, uint16_t err) {
    ((CommonConnState_t*)aSession)->lastErr = err;
    ((CommonConnState_t*)aSession)->lastErrCount += 1;
}

#define GetConnLastErr(__aConn) ((CommonConnState_t*)__aConn)->lastErr

#define GetConnLastErrCount(__aConn) ((CommonConnState_t*)__aConn)->lastErrCount

#define CopyCS(__dstCS, __srcCS) *((CommonConnState_t*)(__dstCS)) = *((CommonConnState_t*)(__srcCS))

#define SaveErrno(__aConn) ((CommonConnState_t*)__aConn)->sysErrno = errno

#define GetErrno(__aConn) ((CommonConnState_t*)__aConn)->sysErrno

int RegisterForReadWriteEvent(int pollId, int fd, void* cState);
int RegisterForReadEvent(int pollId, int fd, void* cState);
int RegisterForWriteEvent(int pollId, int fd, void* cState);
int UnRegisterForReadWriteEvent(int pollId, int fd, void* cState);
int UnRegisterForReadEvent(int pollId, int fd, void* cState);
int UnRegisterForWriteEvent(int pollId, int fd, void* cState);
int AssignSocketLocalPort(SockAddr_t* localAddres
                            , LocalPortPool_t* portPool
                            , void* cState);
void AddressToString(SockAddr_t* addr, char* str);

void DumpCStats(void* aStats);
#define TD_NO_ERROR                                         0

#define TD_SOCKET_CREATE_FAILED                             1
#define TD_SOCKET_BIND_FAILED                               2
#define TD_SOCKET_CONNECT_FAILED                            3
#define TD_SOCKET_CONNECT_FAILED_IMMEDIATE                  4
#define TD_SOCKET_LISTEN_FAILED                             5
#define TD_SOCKET_REUSE_FAILED                              6
#define TD_SOCKET_WRITE_ERROR                               7
#define TD_SOCKET_READ_ERROR                                8
#define TD_ASSIGN_PORT_FAILED                               9


#define STATE_TCP_PORT_ASSIGNED                             0x0000000000000001
#define STATE_TCP_SOCK_CREATE                               0x0000000000000002
#define STATE_TCP_SOCK_REUSE                                0x0000000000000004
#define STATE_TCP_SOCK_BIND                                 0x0000000000000008
#define STATE_TCP_CONN_INIT                                 0x0000000000000010
#define STATE_TCP_CONN_IN_PROGRESS                          0x0000000000000020
#define STATE_TCP_CONN_IN_PROGRESS2                         0x0000000000000040
#define STATE_TCP_CONN_ESTABLISHED                          0x0000000000000080
#define STATE_TCP_SOCK_FD_CLOSE                             0x0000000000000100
#define STATE_TCP_LISTENING                                 0x0000000000000200
#define STATE_TCP_LISTEN_STOP                               0x0000000000000400
#define STATE_TCP_REGISTER_WRITE                            0x0000000000000800
#define STATE_TCP_REGISTER_READ                             0x0000000000001000
#define STATE_TCP_UNREGISTER_WRITE                          0x0000000000002000
#define STATE_TCP_UNREGISTER_READ                           0x0000000000004000

#define AllocSession(__type) g_slice_new(__type)
#define SetSessionToPool(__pool,__session) g_queue_push_tail (__pool,__session)
#define RemoveFromSessionPool(__pool,__session) g_queue_remove (__pool,__session)

#define AllocEmptySessionPool() g_queue_new()
#define GetSesionFromPool(__pool) g_queue_pop_head(__pool)
#define IsSessionPoolEmpty(__pool) g_queue_is_empty(__pool)

#define CreateEventQ() epoll_create(1)
#define DeleteEventQ(__eventQId) close(__eventQId)
#define GetIOEvents(__eventQId, __eventArray, __maxEvents) epoll_wait(__eventQId, __eventArray, __maxEvents, 0)

#define GetIOEventData(__event) __event.data.ptr
#define IsWriteEventSet(__event) __event.events && EPOLLOUT 
#define IsReadEventSet(__event) __event.events && EPOLLIN 

#define ResetErrno() errno = 0

#define CreateTimerWheel() g_timer_new()
#define DeleteTimerWheel(__timer) g_timer_destroy(__timer)
#define TimeElapsedTimerWheel(__timer) g_timer_elapsed(__timer, NULL)

#define AllocEmptyPortBindQ() g_queue_new()
#define InitPortBindQ(__bindq) g_queue_init(__bindq)
#define IsPortBindQEmpty(__bindq) g_queue_is_empty(__bindq)
#define GetPortFromPool(__bindq) GPOINTER_TO_INT(g_queue_pop_head(__bindq))
#define SetPortToPool(__bindq,__port) g_queue_push_tail (__bindq,GINT_TO_POINTER(__port))

#define TdMalloc(__size) malloc(__size)

#define IsIpv6(__saddr) (((struct sockaddr*)__saddr)->sa_family == AF_INET6)

#define SetSockPort(__laddr,__lport)if (IsIpv6(__laddr)) ((struct sockaddr_in6*)__laddr)->sin6_port=__lport;else ((struct sockaddr_in*)__laddr)->sin_port=__lport

#define GetSockPort(__laddr)(IsIpv6(__laddr)) ? ((struct sockaddr_in6*)__laddr)->sin6_port : ((struct sockaddr_in*)__laddr)->sin_port

#define GetSockAddr(__laddr) ((struct sockaddr*)&(__laddr))
#endif

