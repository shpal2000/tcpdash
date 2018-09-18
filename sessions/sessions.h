#ifndef __TD_SESSIONS_H
#define __TD_SESSIONS_H

#include <gmodule.h>
#include <sys/epoll.h>

enum AppTypeId { TcpClientAppId = 1
                , TcpServerAppId = 2};

typedef struct SessionState{
    uint64_t state1;
    uint64_t state2;
    uint16_t lastErr;
    uint16_t lastErrCount;
    enum AppTypeId appId;
} SessionState_t;

typedef GQueue SessionPool_t;

static inline void SSInit(void* aSession, enum AppTypeId appId) {

    ((SessionState_t*)aSession)->state1 = 0;
    ((SessionState_t*)aSession)->state2 = 0;
    ((SessionState_t*)aSession)->lastErr = 0;
    ((SessionState_t*)aSession)->lastErrCount = 0;

    ((SessionState_t*)aSession)->appId = appId;
}

#define SetSS1(__aSession, __state) ((SessionState_t*)__aSession)->state1 |= __state

#define SetSS2(__aSession, __state) ((SessionState_t*)__aSession)->state2 |= __state

#define InitSSLastErr(__aSession, __err) ((SessionState_t*)__aSession)->lastErr = __err

static inline void SetSSLastErr(void* aSession, uint16_t err) {
    ((SessionState_t*)aSession)->lastErr = err;
    ((SessionState_t*)aSession)->lastErrCount += 1;
}

#define GetSSLastErr(__aSession) ((SessionState_t*)__aSession)->lastErr

#define GetSSLastErrCount(__aSession) ((SessionState_t*)__aSession)->lastErrCount

#define CopySS(__srcSS, __dstSS) 

void RegisterForWriteEvent(int pollId, int fd, void* data);

#define TD_NO_ERROR                                         0
#define TD_PROGRAM_ERROR                                    1

#define TD_SOCKET_CREATE_FAILED                             2
#define TD_SOCKET_BIND_FAILED                               3
#define TD_SOCKET_CONNECT_ESTABLISH_FAILED                  4
#define TD_SOCKET_CONNECT_ESTABLISH_FAILED2                 5
#define TD_PROGRAM_ERROR_TcpNewConnection                   6

#define STATE_TCP_SOCK_CREATE                               0x0000000000000001
#define STATE_TCP_SOCK_BIND                                 0x0000000000000002
#define STATE_TCP_CONN_INIT                                 0x0000000000000004
#define STATE_TCP_CONN_IN_PROGRESS                          0x0000000000000008
#define STATE_TCP_CONN_IN_PROGRESS2                         0x0000000000000010
#define STATE_TCP_CONN_ESTABLISHED                          0x0000000000000020
#define STATE_TCP_SOCK_FD_CLOSE                             0x0000000000000040


#define AllocSession(__type) g_slice_new(__type)
#define AddToSessionPool(__pool,__session) g_queue_push_tail (__pool,__session)
#define RemoveFromSessionPool(__pool,__session) g_queue_remove (__pool,__session)
#define AllocEmptySessionPool() g_queue_new()
#define CreateEventArray(__count) g_new(struct epoll_event, __count)
#define GetAnySesionFromPool(__pool) g_queue_pop_head(__pool)
#define IsSessionPoolEmpty(__pool) g_queue_is_empty(__pool)

#define CreateEventQ() epoll_create(1)
#define DeleteEventQ(__eventQId) close(__eventQId)
#define GetIOEvents(__eventQId, __eventArray, __maxEvents) epoll_wait(__eventQId, __eventArray, __maxEvents, 0)

#define GetIOEventData(__event) __event.data.ptr
#define IsWriteEventSet(__event) __event.events && EPOLLOUT 
#endif 

