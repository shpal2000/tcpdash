#ifndef __SF_UTILS_H

#include "lf_utils.h"
#include "sstates.h"
#include "sessions.h"

#define __SF_UTILS_H
int TcpNewConnection(int isIpv6 
                        , struct sockaddr* localAddress
                        , struct sockaddr* remoteAddress
                        , struct TDSession* tdSessionState);
#endif