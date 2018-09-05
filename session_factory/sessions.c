#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <gmodule.h>

#include "sessions.h"

inline void TDSessionInit(struct TDSession* tdSessionState) {
    tdSessionState->sessionState1 = 0;
    tdSessionState->sessionState2 = 0;
    tdSessionState->sessionLastErr = 0;
    tdSessionState->sessionLastErrCount = 0;
}

inline void TDSetSessionState1(struct TDSession* tdSessionState, uint64_t state) {
    tdSessionState->sessionState1 |= state;
}

inline void TDSetSessionState2(struct TDSession* tdSessionState, uint64_t state) {
    tdSessionState->sessionState2 |= state;
}

inline void TDSetSessionLastErr(struct TDSession* tdSessionState, uint16_t err) {
    tdSessionState->sessionLastErr = err;
    tdSessionState->sessionLastErrCount += 1;
}

inline uint16_t TDGetSessionLastErr(struct TDSession* tdSessionState){
    return tdSessionState->sessionLastErr;
}

inline uint16_t TDGetSessionLastErrCount(struct TDSession* tdSessionState){
    return tdSessionState->sessionLastErrCount;
}