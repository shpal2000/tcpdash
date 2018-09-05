#ifndef __SESSIONS_H
#define __SESSIONS_H

#include "sstates.h"

struct TDSession{
    uint64_t sessionState1;
    uint64_t sessionState2;
    uint16_t sessionLastErr;
    uint16_t sessionLastErrCount;
};

void TDSessionInit(struct TDSession* tdSessionState);

void TDSetSessionState1(struct TDSession* tdSessionState, uint64_t state);

void TDSetSessionState2(struct TDSession* tdSessionState, uint64_t state);

void TDSetSessionLastErr(struct TDSession* tdSessionState, uint16_t err);

uint16_t TDGetSessionLastErr(struct TDSession* tdSessionState);

uint16_t TDGetSessionLastErrCount(struct TDSession* tdSessionState);
#endif