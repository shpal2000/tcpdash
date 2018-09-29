#ifndef __TCP_APPS_H
#define __TCP_APPS_H

typedef struct TcpClientAppOptions {
    uint32_t maxEvents;
    uint32_t maxActiveSessions;
    uint32_t maxErrorSessions;
    uint64_t maxSessions;
    uint32_t connectionPerSec;
    uint32_t csDataLen;
} TcpClientAppOptions_t;

void TcpClientRun(TcpClientAppOptions_t* appOptions);

#endif