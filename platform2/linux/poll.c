#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "platform.h"

void NewPollReadWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    struct epoll_event setEvent = {0};
    setEvent.events = 0;
    setEvent.data.ptr = cState;
    setEvent.events = EPOLLIN | EPOLLOUT;

    epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
}

void NewPollReadEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    struct epoll_event setEvent = {0};
    setEvent.events = 0;
    setEvent.data.ptr = cState;
    setEvent.events = EPOLLIN;

    epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
}

void NewPollWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    struct epoll_event setEvent = {0};
    setEvent.events = 0;
    setEvent.data.ptr = cState;
    setEvent.events = EPOLLOUT;

    epoll_ctl(pollId, EPOLL_CTL_ADD, fd, &setEvent);
}

void UpdatePollReadWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    struct epoll_event setEvent = {0};
    setEvent.events = 0;
    setEvent.data.ptr = cState;
    setEvent.events = EPOLLIN | EPOLLOUT;

    epoll_ctl(pollId, EPOLL_CTL_MOD, fd, &setEvent);
}

void UpdatePollReadEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    struct epoll_event setEvent = {0};
    setEvent.events = 0;
    setEvent.data.ptr = cState;
    setEvent.events = EPOLLIN;

    epoll_ctl(pollId, EPOLL_CTL_MOD, fd, &setEvent);
}

void UpdatePollWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    struct epoll_event setEvent = {0};
    setEvent.events = 0;
    setEvent.data.ptr = cState;
    setEvent.events = EPOLLOUT;

    epoll_ctl(pollId, EPOLL_CTL_MOD, fd, &setEvent);
}

void StopPollReadWriteEvent(int pollId
                        , int fd
                        , void* aStats
                        , void* cState) {

    epoll_ctl(pollId, EPOLL_CTL_DEL, fd, NULL);
}