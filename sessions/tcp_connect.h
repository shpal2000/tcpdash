#ifndef __TCP_CONNECT_H

#define __TCP_CONNECT_H
int TcpNewConnection(SockAddr_t* localAddress
                        , SockAddr_t* remoteAddress
                        , void* aStats
                        , void* bStats
                        , void* cState);

int VerifyTcpConnectionEstablished(int fd, void* cState);

int TcpListenStart(SockAddr_t* localAddress
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