#ifndef __APP_COMMON_H
#define __APP_COMMON_H

enum ConnCloseMethod {EmTcpFIN
                    , EmTcpRST
                    , EmCloseNotify};

enum ConnCloseType {EmClientClose
                    , EmServerClose
                    , EmDataFinish};



#endif