#include "platform.h"


int SetSockStatsJ (SockStats_t* cStats
                    , JObject* jObj) {

    JSET_MEMBER_INT (jObj, "socketCreate", cStats->socketCreate);
    JSET_MEMBER_INT (jObj, "socketCreateFail", cStats->socketCreateFail);
    JSET_MEMBER_INT (jObj, "socketListenFail", cStats->socketListenFail);

    JSET_MEMBER_INT (jObj, "tcpAcceptFail", cStats->tcpAcceptFail);
    JSET_MEMBER_INT (jObj, "tcpAcceptSuccess", cStats->tcpAcceptSuccess);

    JSET_MEMBER_INT (jObj, "tcpConnInit", cStats->tcpConnInit);
    JSET_MEMBER_INT (jObj, "tcpConnInitSuccess", cStats->tcpConnInitSuccess);
    JSET_MEMBER_INT (jObj, "tcpConnInitFail", cStats->tcpConnInitFail);

    return 0;
}