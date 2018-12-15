#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "platform/common.h"

void SetPollEvent(int pollId
                , int fd
                , int pollRead
                , int pollWrite
                , void* aStats
                , void* bStats
                , void* cState) {
    enum Actions {PollAdd, PollMod, PollDel, PollNop} pollAction;

    pollAction = PollNop;

   if ( IsSetCS1(cState, STATE_TCP_POLL_READ_CURRENT 
                            | STATE_TCP_POLL_WRITE_CURRENT) ) {
        if (pollRead || pollWrite) {
            pollAction = PollMod;
        }else {
            pollAction = PollDel;
        }
    }else{
        if (pollRead || pollWrite) {
            pollAction = PollAdd;
        }
    }

    if (pollAction != PollNop) {

        struct epoll_event setEvent;

        setEvent.data.ptr = cState;

        if (pollRead && pollWrite) {
            setEvent.events = EPOLLIN | EPOLLOUT;
        } else if (pollRead) {
            setEvent.events = EPOLLIN;
        } else if (pollWrite) {
            setEvent.events = EPOLLOUT;
        }

        int status = -1;
        switch (pollAction) {
            case PollAdd:
                setEvent.events |= EPOLLRDHUP;
                status = epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
                break;
            case PollMod:
                setEvent.events |= EPOLLRDHUP;
                status = epoll_ctl(pollId, EPOLL_CTL_MOD, fd, &setEvent);
                break;
            case PollDel:
                status = epoll_ctl(pollId, EPOLL_CTL_DEL, fd, &setEvent);
                break;
            case PollNop:
                break;
        }

        if (status) {
            SetCES(cState, STATE_TCP_SOCK_POLL_UPDATE_FAIL);
        }else{
            if (pollRead) {
                SetCS1(cState, STATE_TCP_POLL_READ_CURRENT
                                | STATE_TCP_POLL_READ_STICKY);
            }else{
                ClearCS1(cState, STATE_TCP_POLL_READ_CURRENT);
            }
            if (pollWrite) {
                SetCS1(cState, STATE_TCP_POLL_WRITE_CURRENT
                                | STATE_TCP_POLL_WRITE_STICKY);
            }else{
                ClearCS1(cState, STATE_TCP_POLL_WRITE_CURRENT);
            }
        }

        if ( IsSetCES(cState, STATE_TCP_SOCK_POLL_UPDATE_FAIL) ) {
            IncConnStats2(aStats, bStats, tcpPollRegUnregFail);
        }
    }
}

void PollReadWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* bStats
                        , void* cState) {
    SetPollEvent(pollId, fd, 1, 1, aStats, bStats, cState);
}

void PollReadEventOnly(int pollId
                        , int fd
                        , void* aStats
                        , void* bStats
                        , void* cState) {
    SetPollEvent(pollId, fd, 1, 0, aStats, bStats, cState);
}

void PollWriteEventOnly(int pollId
                        , int fd
                        , void* aStats
                        , void* bStats
                        , void* cState) {
    SetPollEvent(pollId, fd, 0, 1, aStats, bStats, cState);
}

void StopPollReadWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* bStats
                        , void* cState) {
    SetPollEvent(pollId, fd, 0, 0, aStats, bStats, cState);
}
