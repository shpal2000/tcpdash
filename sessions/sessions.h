#ifndef __TD_SESSIONS_H
#define __TD_SESSIONS_H

#include <glib.h>
#include <gmodule.h>
#include <sys/epoll.h>
#include "utils/resource.h"

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

    uint64_t programErrorTcpNewConnection;    
    uint64_t tcpConnRegisterForWriteEventFail;
    uint64_t tcpConnRegisterForReadEventFail;
} CommonConnStats_t;

typedef struct CommonConnState{
    uint64_t state1;
    uint64_t state2;
    uint16_t lastErr;
    uint16_t lastErrCount;
    int sysErrno;
    int appState;                                                                  
} CommonConnState_t;

typedef GQueue LocalPortPool_t;
typedef GQueue SessionPool_t;

typedef GTimer TimerWheel_t;

static inline void SSInit(void* cState) {

    ((CommonConnState_t*)cState)->state1 = 0;
    ((CommonConnState_t*)cState)->state2 = 0;
    ((CommonConnState_t*)cState)->lastErr = 0;
    ((CommonConnState_t*)cState)->lastErrCount = 0;
}

#define SetAppState(__aSession, __state) ((CommonConnState_t*)__aSession)->appState = __state

#define GetAppState(__aSession) ((CommonConnState_t*)__aSession)->appState

#define SetSS1(__aSession, __state) ((CommonConnState_t*)__aSession)->state1 |= __state

#define SetSS2(__aSession, __state) ((CommonConnState_t*)__aSession)->state2 |= __state

#define GetSS1(__aSession) ((CommonConnState_t*)__aSession)->state1

#define GetSS2(__aSession) ((CommonConnState_t*)__aSession)->state2

#define InitSSLastErr(__aSession, __err) ((CommonConnState_t*)__aSession)->lastErr = __err

#define IncSStats(__aStats, __stat) ((CommonConnStats_t*)__aStats)->__stat++

#define IncSStats2(__aStats, __bStats, __stat) ((CommonConnStats_t*)__aStats)->__stat++;((CommonConnStats_t*)__bStats)->__stat++

#define GetSStats(__aStats, __stat) ((CommonConnStats_t*)__aStats)->__stat

static inline void SetSSLastErr(void* aSession, uint16_t err) {
    ((CommonConnState_t*)aSession)->lastErr = err;
    ((CommonConnState_t*)aSession)->lastErrCount += 1;
}

#define GetSSLastErr(__aSession) ((CommonConnState_t*)__aSession)->lastErr

#define GetSSLastErrCount(__aSession) ((CommonConnState_t*)__aSession)->lastErrCount

#define CopySS(__dstSS, __srcSS) *((CommonConnState_t*)(__dstSS)) = *((CommonConnState_t*)(__srcSS))

#define SaveErrno(__aSession) ((CommonConnState_t*)__aSession)->sysErrno = errno

#define GetErrno(__aSession) ((CommonConnState_t*)__aSession)->sysErrno

int RegisterForWriteEvent(int pollId, int fd, void* cState);
int RegisterForReadEvent(int pollId, int fd, void* cState);
int UnRegisterForEvent(int pollId, int fd, void* cState);

void DumpSStats(void* aStats);
#define TD_NO_ERROR                                         0

#define TD_SOCKET_CREATE_FAILED                             1
#define TD_SOCKET_BIND_FAILED                               2
#define TD_SOCKET_CONNECT_FAILED                            3
#define TD_SOCKET_CONNECT_FAILED_IMMEDIATE                  4
#define TD_PROGRAM_ERROR_TcpNewConnection                   5
#define TD_PROGRAM_ERROR_TcpListenStart                     6
#define TD_SOCKET_LISTEN_FAILED                             7
#define TD_SOCKET_REUSE_FAILED                              8
#define TD_PROGRAM_ERROR_TcpWrite                           9
#define TD_SOCKET_WRITE_ERROR                               10
#define TD_PROGRAM_ERROR_TcpRead                            11
#define TD_SOCKET_READ_ERROR                                12


#define STATE_TCP_SOCK_CREATE                               0x0000000000000001
#define STATE_TCP_SOCK_REUSE                                0x0000000000000002
#define STATE_TCP_SOCK_BIND                                 0x0000000000000004
#define STATE_TCP_CONN_INIT                                 0x0000000000000008
#define STATE_TCP_CONN_IN_PROGRESS                          0x0000000000000010
#define STATE_TCP_CONN_IN_PROGRESS2                         0x0000000000000020
#define STATE_TCP_CONN_ESTABLISHED                          0x0000000000000040
#define STATE_TCP_SOCK_FD_CLOSE                             0x0000000000000080
#define STATE_TCP_LISTENING                                 0x0000000000000100
#define STATE_TCP_LISTEN_STOP                               0x0000000000000200


#define AllocSession(__type) g_slice_new(__type)
#define SetSessionToPool(__pool,__session) g_queue_push_tail (__pool,__session)
#define RemoveFromSessionPool(__pool,__session) g_queue_remove (__pool,__session)

#define AllocEmptySessionPool() g_queue_new()
#define CreateEventArray(__count) g_new(struct epoll_event, __count)
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
#endif

