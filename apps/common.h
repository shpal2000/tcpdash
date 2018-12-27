#ifndef __APP_COMMON_H
#define __APP_COMMON_H

enum ConnCloseMethod {TcpFIN
                    , TcpRST
                    , CloseNotify};

enum ConnCloseType {ClientClose
                    , ServerClose
                    , DataClose};
#endif