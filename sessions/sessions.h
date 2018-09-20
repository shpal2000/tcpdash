#ifndef __TD_SESSIONS_H
#define __TD_SESSIONS_H

#include <gmodule.h>
#include <sys/epoll.h>

typedef struct SessionStats{
    uint64_t socketCreate;    
    uint64_t socketCreateFail;
    uint64_t socketListenFail;
    uint64_t socketBindIpv4;    
    uint64_t socketBindIpv4Fail;
    uint64_t socketBindIpv6;    
    uint64_t socketBindIpv6Fail;
    uint64_t socketConnectEstablishFail;    
    uint64_t socketConnectEstablishFail2;    
    uint64_t programErrorTcpNewConnection;    
} SessionStats_t;

typedef struct SessionState{
    uint64_t state1;
    uint64_t state2;
    uint16_t lastErr;
    uint16_t lastErrCount;
    int sysErrno;
    int appState;    
} SessionState_t;

typedef GQueue SessionPool_t;

static inline void SSInit(void* aSession) {

    ((SessionState_t*)aSession)->state1 = 0;
    ((SessionState_t*)aSession)->state2 = 0;
    ((SessionState_t*)aSession)->lastErr = 0;
    ((SessionState_t*)aSession)->lastErrCount = 0;
}

#define SetAppState(__aSession, __state) ((SessionState_t*)__aSession)->appState = __state

#define GetAppState(__aSession) ((SessionState_t*)__aSession)->appState

#define SetSS1(__aSession, __state) ((SessionState_t*)__aSession)->state1 |= __state

#define SetSS2(__aSession, __state) ((SessionState_t*)__aSession)->state2 |= __state

#define GetSS1(__aSession) ((SessionState_t*)__aSession)->state1

#define GetSS2(__aSession) ((SessionState_t*)__aSession)->state2

#define InitSSLastErr(__aSession, __err) ((SessionState_t*)__aSession)->lastErr = __err

#define IncSStats(__aStats, __stat) ((SessionStats_t*)__aStats)->__stat++

#define GetSStats(__aStats, __stat) ((SessionStats_t*)__aStats)->__stat

static inline void SetSSLastErr(void* aSession, uint16_t err) {
    ((SessionState_t*)aSession)->lastErr = err;
    ((SessionState_t*)aSession)->lastErrCount += 1;
}

#define GetSSLastErr(__aSession) ((SessionState_t*)__aSession)->lastErr

#define GetSSLastErrCount(__aSession) ((SessionState_t*)__aSession)->lastErrCount

#define CopySS(__dstSS, __srcSS) *((SessionState_t*)(__dstSS)) = *((SessionState_t*)(__srcSS))

#define SaveErrno(__aSession) ((SessionState_t*)__aSession)->sysErrno = errno

#define GetErrno(__aSession) ((SessionState_t*)__aSession)->sysErrno

void RegisterForWriteEvent(int pollId, int fd, void* data);

void DumpSessionStats(void* aStats);
#define TD_NO_ERROR                                         0

#define TD_SOCKET_CREATE_FAILED                             1
#define TD_SOCKET_BIND_FAILED                               2
#define TD_SOCKET_CONNECT_FAILED                            3
#define TD_SOCKET_CONNECT_FAILED_IMMEDIATE                  4
#define TD_PROGRAM_ERROR_TcpNewConnection                   5
#define TD_PROGRAM_ERROR_TcpListenStart                     6
#define TD_SOCKET_LISTEN_FAILED                             7

#define STATE_TCP_SOCK_CREATE                               0x0000000000000001
#define STATE_TCP_SOCK_BIND                                 0x0000000000000002
#define STATE_TCP_CONN_INIT                                 0x0000000000000004
#define STATE_TCP_CONN_IN_PROGRESS                          0x0000000000000008
#define STATE_TCP_CONN_IN_PROGRESS2                         0x0000000000000010
#define STATE_TCP_CONN_ESTABLISHED                          0x0000000000000020
#define STATE_TCP_SOCK_FD_CLOSE                             0x0000000000000040
#define STATE_TCP_LISTENING                                 0x0000000000000080
#define STATE_TCP_LISTEN_STOP                               0x0000000000000100


#define AllocSession(__type) g_slice_new(__type)
#define AddToSessionPool(__pool,__session) g_queue_push_tail (__pool,__session)
#define RemoveFromSessionPool(__pool,__session) g_queue_remove (__pool,__session)
#define CreateSessionArray(__type,__count) g_new(__type, __count)
#define AllocEmptySessionPool() g_queue_new()
#define CreateEventArray(__count) g_new(struct epoll_event, __count)
#define GetAnySesionFromPool(__pool) g_queue_pop_head(__pool)
#define IsSessionPoolEmpty(__pool) g_queue_is_empty(__pool)

#define CreateEventQ() epoll_create(1)
#define DeleteEventQ(__eventQId) close(__eventQId)
#define GetIOEvents(__eventQId, __eventArray, __maxEvents) epoll_wait(__eventQId, __eventArray, __maxEvents, 0)

#define GetIOEventData(__event) __event.data.ptr
#define IsWriteEventSet(__event) __event.events && EPOLLOUT 

#define ResetErrno() errno = 0
#endif 

