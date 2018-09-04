#ifndef __SF_UTILS_H
#define __SF_UTILS_H
int TcpNewConnection(int isIpv6 
                        , struct sockaddr* localAddress
                        , struct sockaddr* remoteAddress
                        , struct TDSessionState* tdSessionState);
#endif