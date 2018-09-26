#ifndef __TCP_CONNECT_H

#define __TCP_CONNECT_H
int TcpNewConnection(int isIpv6 
                        , struct sockaddr* localAddress
                        , struct sockaddr* remoteAddress
                        , void* aStats
                        , void* bStats
                        , void* cState);

int IsNewTcpConnectionComplete(int fd);

int TcpListenStart(int isIpv6 
                    , struct sockaddr* localAddress
                    , int listenQLen
                    , void* aStats
                    , void* bStats
                    , void* cState);

int TcpWrite(int fd
                , const char* dataBuff
                , int dataLen
                , void* aStats
                , void* bStats
                , void* cState);

int TcpRead(int fd
                , char* dataBuffer
                , int dataLen
                , void* aStats
                , void* bStats
                , void* cState);
#endif