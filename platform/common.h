#ifndef __TD_SESSIONS_H
#define __TD_SESSIONS_H

#include <glib.h>
#include <gmodule.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>



//#####################Socket#####################
typedef union SockAddr {
    struct sockaddr_in inAddr;
    struct sockaddr_in6 in6Addr;
} SockAddr_t;

typedef struct ConnStats{

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
    uint64_t tcpConnInitFailImmediateEaddrNotAvail;
    uint64_t tcpConnInitFailImmediateOther;
    uint64_t tcpConnInitProgress;
    uint64_t tcpWriteFail;
    uint64_t tcpReadFail;

    uint64_t tcpListenStart;
    uint64_t tcpListenStop;
    uint64_t tcpListenStartFail;
    uint64_t tcpAcceptFail;
    uint64_t tcpNonReadEventOnListener;

    uint64_t tcpLocalPortAssignFail;
    uint64_t tcpPollRegUnregFail;
} ConnStats_t;

typedef struct ConnState {
    uint64_t state1;
    uint64_t state2;
    uint64_t errState;
    int sysErrno;
    int appState;
    int sockErrno;                                                              
} ConnState_t;

typedef struct epoll_event PollEvent_t;
typedef GQueue LocalPortPool_t;
typedef GQueue SessionPool_t;

typedef GTimer TimerWheel_t;

static inline void CSInit(void* cState) {

    ((ConnState_t*)cState)->state1 = 0;
    ((ConnState_t*)cState)->state2 = 0;
    ((ConnState_t*)cState)->errState = 0;
    ((ConnState_t*)cState)->sysErrno = 0;
    ((ConnState_t*)cState)->appState = 0;
}

#define SetAppState(__aConn, __state) ((ConnState_t*)__aConn)->appState = __state

#define GetAppState(__aConn) ((ConnState_t*)__aConn)->appState

#define SetCS1(__aConn, __state) ((ConnState_t*)__aConn)->state1 |= __state

#define ClearCS1(__aConn, __state) ((ConnState_t*)__aConn)->state1 &= ~__state

#define SetCS2(__aConn, __state) ((ConnState_t*)__aConn)->state2 |= __state

#define GetCS1(__aConn) ((ConnState_t*)__aConn)->state1

#define GetCS2(__aConn) ((ConnState_t*)__aConn)->state2

#define GetCES(__aConn) ((ConnState_t*)__aConn)->errState

#define IsSetCS1(__aConn,__state) (((ConnState_t*)__aConn)->state1&(__state))

#define IsSetCS2(__aConn,__state) (((ConnState_t*)__aConn)->state2&(__state))

#define IsSetCES(__aConn,__state) (((ConnState_t*)__aConn)->errState&(__state))

#define IsFdClosed(__aConn) ((((ConnState_t*)__aConn)->state1&STATE_TCP_SOCK_FD_CLOSE) || (((ConnState_t*)__aConn)->errState&STATE_TCP_SOCK_FD_CLOSE_FAIL))

#define InitConLastErr(__aConn, __err) ((ConnState_t*)__aConn)->lastErr = __err

#define IncConnStats(__aStats, __stat) ((ConnStats_t*)__aStats)->__stat++

#define IncConnStats2(__aStats, __bStats, __stat) ((ConnStats_t*)__aStats)->__stat++;((ConnStats_t*)__bStats)->__stat++

#define GetConnStats(__aStats, __stat) ((ConnStats_t*)__aStats)->__stat

static inline void SetCES(void* aSession, uint64_t errState) {
    ((ConnState_t*)aSession)->sysErrno = errno;
    ((ConnState_t*)aSession)->errState |= errState;
}

#define CopyCS(__dstCS, __srcCS) *((ConnState_t*)(__dstCS)) = *((ConnState_t*)(__srcCS))

#define SaveSockErrno(__aConn, __sockErrno) ((ConnState_t*)__aConn)->sockErrno = __sockErrno

#define GetSysErrno(__aConn) ((ConnState_t*)__aConn)->sysErrno

#define GetSockErrno(__aConn) ((ConnState_t*)__aConn)->sockErrno

void PollReadWriteEvent(int pollId
                        , int fd
                        , void* cState);

void PollReadEventOnly(int pollId
                        , int fd
                        , void* cState);

void PollWriteEventOnly(int pollId
                        , int fd
                        , void* cState);

void StopPollReadWriteEvent(int pollId
                        , int fd
                        , void* cState);

void AssignSocketLocalPort(SockAddr_t* localAddres
                            , LocalPortPool_t* portPool
                            , void* cState);

void AddressToString(SockAddr_t* addr, char* str);

void DumpCStats(void* aStats);

#define MAX_POLL_TIMEOUT 100

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
#define STATE_TCP_POLL_READ_CURRENT                         0x0000000000000800
#define STATE_TCP_POLL_WRITE_CURRENT                        0x0000000000001000
#define STATE_TCP_POLL_READ_STICKY                          0x0000000000002000
#define STATE_TCP_POLL_WRITE_STICKY                         0x0000000000004000
#define STATE_TCP_CONN_ACCEPT                               0x0000000000008000
#define STATE_TCP_CONN_ACCEPT_O_NONBLOCK                    0x0000000000008000

#define STATE_TCP_SOCK_CREATE_FAIL                          0x0000000000000001
#define STATE_TCP_SOCK_BIND_FAIL                            0x0000000000000002
#define STATE_TCP_SOCK_CONNECT_FAIL                         0x0000000000000004
#define STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE               0x0000000000000008
#define STATE_TCP_SOCK_LISTEN_FAIL                          0x0000000000000010
#define STATE_TCP_SOCK_REUSE_FAIL                           0x0000000000000020
#define STATE_TCP_SOCK_READ_FAIL                            0x0000000000000040
#define STATE_TCP_SOCK_WRITE_FAIL                           0x0000000000000080
#define STATE_TCP_SOCK_PORT_ASSIGN_FAIL                     0x0000000000000100
#define STATE_TCP_SOCK_FD_CLOSE_FAIL                        0x0000000000000200
#define STATE_TCP_SOCK_POLL_UPDATE_FAIL                     0x0000000000000400
#define STATE_TCP_CONN_ACCEPT_FAIL                          0x0000000000000800
#define STATE_TCP_SOCK_F_GETFL_FAIL                         0x0000000000001000
#define STATE_TCP_SOCK_F_SETFL_FAIL                         0x0000000000002000
#define STATE_TCP_SOCK_O_NONBLOCK_FAIL                      0x0000000000004000

#define AllocSession(__type) g_slice_new(__type)
#define SetSessionToPool(__pool,__session) g_queue_push_tail (__pool,__session)
#define RemoveFromSessionPool(__pool,__session) g_queue_remove (__pool,__session)

#define AllocEmptySessionPool() g_queue_new()
#define GetSesionFromPool(__pool) g_queue_pop_head(__pool)
#define IsSessionPoolEmpty(__pool) g_queue_is_empty(__pool)
#define GetSessionCount(__pool) g_queue_get_length(__pool)

#define CreateEventQ() epoll_create(1)
#define DeleteEventQ(__eventQId) close(__eventQId)
#define GetIOEvents(__eventQId, __eventArray, __maxEvents, __to) epoll_wait(__eventQId, __eventArray, __maxEvents, __to)

#define GetIOEventData(__event) __event.data.ptr
#define IsWriteEventSet(__event) __event.events && EPOLLOUT 
#define IsReadEventSet(__event) __event.events && EPOLLIN 

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


//#################TCP Start###############
int TcpNewConnection(SockAddr_t* localAddress
                        , SockAddr_t* remoteAddress
                        , void* aStats
                        , void* bStats
                        , void* cState);

void VerifyTcpConnectionEstablished(int fd
                        , void* aStats
                        , void* bStats
                        , void* cState);

int TcpListenStart(SockAddr_t* localAddress
                    , int listenQLen
                    , void* aStats
                    , void* bStats
                    , void* cState);

int TcpAcceptConnection(int listenerFd
                        , SockAddr_t* rAddr
                        , void* aStats
                        , void* bStats
                        , void* cState);
                        
void TcpClose(int fd, void* cState);

int TcpWrite(int fd
                , const char* dataBuff
                , int dataLen
                , void* aStats
                , void* bStats
                , void* cState);

int TcpRead(int fd
                , char* dataBuffer
                , int dataLen
                , void* aStats
                , void* bStats
                , void* cState);


//##################resources######################
#define CreateArray(__type,__count) g_new(__type, __count)
#define CreateStruct0(__type) g_slice_new0(__type) 

#endif
