#ifndef __SF_UTILS_H

#define __SF_UTILS_H
int TcpNewConnection(int isIpv6 
                        , struct sockaddr* localAddress
                        , struct sockaddr* remoteAddress
                        , void* aSession
                        , void* aStats);

int IsNewTcpConnectionComplete(int fd);

int TcpListenStart(int isIpv6 
                    , struct sockaddr* localAddress
                    , int listenQLen
                    , void* aSession
                    , void* aStats);
#endif