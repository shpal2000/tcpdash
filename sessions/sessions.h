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

static inline void SSInit(void* aSession, enum AppTypeId appId) {
    SessionState_t* sS = (SessionState_t*) aSession;

    sS->state1 = 0;
    sS->state2 = 0;
    sS->lastErr = 0;
    sS->lastErrCount = 0;

    sS->appId = appId;
}

static inline void SetSS1(void* aSession, uint64_t state) {
    SessionState_t* sS = (SessionState_t*) aSession;
    sS->state1 |= state;
}

static inline void SetSS2(void* aSession, uint64_t state) {
    SessionState_t* sS = (SessionState_t*) aSession;
    sS->state2 |= state;
}

static inline void SetSSLastErr(void* aSession, uint16_t err) {
    SessionState_t* sS = (SessionState_t*) aSession;
    sS->lastErr = err;
    sS->lastErrCount += 1;
}

static inline uint16_t GetSSLastErr(void* aSession) {
    SessionState_t* sS = (SessionState_t*) aSession;
    return sS->lastErr;
}

static inline uint16_t GetSSLastErrCount(void* aSession) {
    SessionState_t* sS = (SessionState_t*) aSession;
    return sS->lastErrCount;
}

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

#define CreateEventQ() epoll_create(1)
#define DeleteEventQ(__eventQId) close(__eventQId)
#define GetIOEvents(__eventQId, __eventArray, __maxEvents) epoll_wait(__eventQId, __eventArray, __maxEvents, 0)

#define GetIOEventData(__event) __event.data.ptr
#define IsWriteEventBitSet(__event) __event.events && EPOLLOUT 
#endif 

