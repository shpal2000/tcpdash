#ifndef __SESSION_STATES_H
#define __SESSION_STATES_H

struct TDSessionState{
    uint64_t sessionState1;
    uint64_t sessionState2;
    uint16_t sessionLastErr;
    uint16_t sessionLastErrCount;
};

static inline void TDSessionSateInit(struct TDSessionState* tdSessionState) {
    tdSessionState->sessionState1 = 0;
    tdSessionState->sessionState2 = 0;
    tdSessionState->sessionLastErr = 0;
    tdSessionState->sessionLastErrCount = 0;
}

static inline void TDSetSessionState1(struct TDSessionState* tdSessionState
                                        , uint64_t state) {
    tdSessionState->sessionState1 |= state;
}

static inline void TDSetSessionState2(struct TDSessionState* tdSessionState
                                        , uint64_t state) {
    tdSessionState->sessionState2 |= state;
}

static inline void TDSetSessionStateLastErr(struct TDSessionState* tdSessionState
                                            , uint16_t err) {
    tdSessionState->sessionLastErr = err;
    tdSessionState->sessionLastErrCount += 1;
}

static inline uint16_t TDGetSessionStateLastErr(struct TDSessionState* tdSessionState) {
    return tdSessionState->sessionLastErr;
}

static inline uint16_t TDGetSessionStateLastErrCount(struct TDSessionState* tdSessionState) {
    return tdSessionState->sessionLastErrCount;
}


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

#endif 

