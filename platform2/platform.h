#ifndef __TD_SESSIONS_H
#define __TD_SESSIONS_H

#include <glib.h>
#include <gmodule.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

//#####################Socket#####################
typedef union SockAddr {
    struct sockaddr_in inAddr;
    struct sockaddr_in6 in6Addr;
} SockAddr_t;

typedef struct SockStats{
    uint64_t socketCreate;    
    uint64_t socketCreateFail;
    uint64_t socketListenFail;
    uint64_t socketReuseSet;
    uint64_t socketReuseSetFail;
    uint64_t socketLingerSet;
    uint64_t socketLingerSetFail;
    uint64_t socketBindIpv4;    
    uint64_t socketBindIpv4Fail;
    uint64_t socketBindIpv6;    
    uint64_t socketBindIpv6Fail;

    uint64_t socketConnectEstablishFail;    
    uint64_t socketConnectEstablishFail2;    

    uint64_t tcpConnInit;
    uint64_t tcpConnInitInSec;
    uint64_t tcpConnInitRate;
    uint64_t tcpConnInitSuccess;
    uint64_t tcpConnInitSuccessInSec;
    uint64_t tcpConnInitSuccessRate;
    uint64_t tcpConnInitFail;
    uint64_t tcpConnInitFailImmediateEaddrNotAvail;
    uint64_t tcpConnInitFailImmediateOther;
    uint64_t tcpConnInitProgress;
    uint64_t tcpWriteFail;
    uint64_t tcpWriteReturnsZero;
    uint64_t tcpReadFail;

    uint64_t tcpListenStart;
    uint64_t tcpListenStop;
    uint64_t tcpListenStartFail;
    uint64_t tcpAcceptFail;
    uint64_t tcpAcceptSuccess;
    uint64_t tcpAcceptSuccessInSec;
    uint64_t tcpAcceptSuccessRate;

    uint64_t tcpLocalPortAssignFail;
    uint64_t tcpPollRegUnregFail;

    uint64_t sslConnInit;
    uint64_t sslConnInitInSec;
    uint64_t sslConnInitRate;
    uint64_t sslConnInitSuccess;
    uint64_t sslConnInitSuccessInSec;
    uint64_t sslConnInitSuccessRate;
    uint64_t sslConnInitFail;
    uint64_t sslConnInitProgress;
    uint64_t sslAcceptSuccess; 
    uint64_t sslAcceptSuccessInSec;
    uint64_t sslAcceptSuccessRate; 

    uint64_t tcpConnStructNotAvail;
    uint64_t tcpListenStructNotAvail;

    uint64_t appSessStructNotAvail;

    uint64_t tcpInitServerFail;

    uint64_t tcpGetSockNameFail;

} SockStats_t;

typedef struct SockState {
    uint64_t state1;
    uint64_t state2;
    uint64_t errState;
    int sysErrno;
    int appState;
    int sockErrno;
    int sslErrno;
} SockState_t;

typedef struct epoll_event PollEvent_t;
typedef GQueue LocalPortPool_t;
typedef GQueue SessionPool_t;
typedef GQueue ConnectionPool_t;
typedef GQueue Pool_t;
typedef GQueue List_t;

typedef GTimer TimerWheel_t;

static inline void CSInit(void* cState) {

    ((SockState_t*)cState)->state1 = 0;
    ((SockState_t*)cState)->state2 = 0;
    ((SockState_t*)cState)->errState = 0;
    ((SockState_t*)cState)->sysErrno = 0;
    ((SockState_t*)cState)->appState = 0;
    ((SockState_t*)cState)->sockErrno = 0;
    ((SockState_t*)cState)->sslErrno = 0;
}

#define SetAppState(__aConn, __state) ((SockState_t*)__aConn)->appState = __state

#define GetAppState(__aConn) ((SockState_t*)__aConn)->appState

#define SetCS1(__aConn, __state) ((SockState_t*)__aConn)->state1 |= __state

#define SetCS2(__aConn, __state) ((SockState_t*)__aConn)->state2 |= __state

#define ClearCS1(__aConn, __state) ((SockState_t*)__aConn)->state1 &= ~__state

#define ClearCS2(__aConn, __state) ((SockState_t*)__aConn)->state2 &= ~__state

#define GetCS1(__aConn) ((SockState_t*)__aConn)->state1

#define GetCS2(__aConn) ((SockState_t*)__aConn)->state2

#define GetCES(__aConn) ((SockState_t*)__aConn)->errState

#define IsSetCS1(__aConn,__state) (((SockState_t*)__aConn)->state1&(__state))

#define IsSetCS2(__aConn,__state) (((SockState_t*)__aConn)->state2&(__state))

#define IsSetCES(__aConn,__state) (((SockState_t*)__aConn)->errState&(__state))

#define IsFdClosed(__aConn) ((((SockState_t*)__aConn)->state1&STATE_TCP_SOCK_FD_CLOSE) || (((SockState_t*)__aConn)->errState&STATE_TCP_SOCK_FD_CLOSE_FAIL))

#define IsConnErr(__aConn) ((SockState_t*)__aConn)->errState

#define InitConLastErr(__aConn, __err) ((SockState_t*)__aConn)->lastErr = __err

// #define IncConnStats(__aStats, __stat) ((SockStats_t*)__aStats)->__stat++

#define GetConnStats(__aStats, __stat) ((SockStats_t*)__aStats)->__stat

#define IncConnStats(__statsArr,__statsCount,__stat) \
{ \
    for (int __stats_index = 0; __stats_index < __statsCount; __stats_index++) { \
        ((SockStats_t*) (__statsArr[__stats_index]))->__stat++; \
    } \
}

#define SET_CONN_RATE(__aStats,__stat) \
{ \
    __aStats->__stat##Rate = __aStats->__stat##InSec; \
    __aStats->__stat##InSec = 0; \
}

static inline void SetCES(void* aSession, uint64_t errState) {
    ((SockState_t*)aSession)->sysErrno = errno;
    ((SockState_t*)aSession)->errState |= errState;
}

static inline void SetCESSL(void* aSession, uint64_t errState, int sslErrno) {
    SetCES (aSession, errState);
    ((SockState_t*)aSession)->sslErrno = sslErrno;
}

#define CopyCS(__dstCS, __srcCS) *((SockState_t*)(__dstCS)) = *((SockState_t*)(__srcCS))

#define SaveSockErrno(__aConn, __sockErrno) ((SockState_t*)__aConn)->sockErrno = __sockErrno

#define GetSysErrno(__aConn) ((SockState_t*)__aConn)->sysErrno

#define GetSockErrno(__aConn) ((SockState_t*)__aConn)->sockErrno

void NewPollReadWriteEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void NewPollReadEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void NewPollWriteEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void UpdatePollReadWriteEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void UpdatePollReadEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);


void UpdatePollWriteEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void StopPollReadWriteEvent(int pollId
                        , int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void AssignSocketLocalPort(SockAddr_t* localAddres
                        , LocalPortPool_t* portPool
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void AddressToString(SockAddr_t* addr, char* str);

void DumpCStats(void* aStats);

#define DEFAULT_MAX_POLL_TIMEOUT 100

#define STATE_TCP_PORT_ASSIGNED                             0x0000000000000001
#define STATE_TCP_SOCK_CREATE                               0x0000000000000002
#define STATE_TCP_SOCK_REUSE                                0x0000000000000004
#define STATE_TCP_SOCK_BIND                                 0x0000000000000008
#define STATE_TCP_CONN_INIT                                 0x0000000000000010
#define STATE_TCP_CONN_IN_PROGRESS                          0x0000000000000020
#define STATE_TCP_CONN_IN_PROGRESS2                         0x0000000000000040
#define STATE_TCP_CONN_ESTABLISHED                          0x0000000000000080
#define STATE_TCP_SOCK_FD_CLOSE                             0x0000000000000100
#define STATE_TCP_LISTENING                                 0x0000000000000200
#define STATE_TCP_LISTEN_STOP                               0x0000000000000400
#define STATE_TCP_POLL_READ_CURRENT                         0x0000000000000800
#define STATE_TCP_POLL_WRITE_CURRENT                        0x0000000000001000
#define STATE_TCP_POLL_READ_STICKY                          0x0000000000002000
#define STATE_TCP_POLL_WRITE_STICKY                         0x0000000000004000
#define STATE_TCP_CONN_ACCEPT                               0x0000000000008000
#define STATE_TCP_CONN_ACCEPT_O_NONBLOCK                    0x0000000000010000
#define STATE_SSL_CONN_INIT                                 0x0000000000020000
#define STATE_SSL_CONN_IN_PROGRESS                          0x0000000000040000
#define STATE_SSL_CONN_ESTABLISHED                          0x0000000000080000
#define STATE_SSL_SENT_SHUTDOWN                             0x0000000000100000
#define STATE_SSL_RECEIVED_SHUTDOWN                         0x0000000000200000
#define STATE_SSL_HANDSHAKE_WANT_READ                       0x0000000000400000
#define STATE_SSL_HANDSHAKE_WANT_WRITE                      0x0000000000800000
#define STATE_SSL_TO_SEND_SHUTDOWN                          0x0000000001000000
#define STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN                  0x0000000002000000
#define STATE_NO_MORE_WRITE_DATA                            0x0000000004000000
#define STATE_TCP_TO_SEND_FIN                               0x0000000008000000
#define STATE_TCP_TO_SEND_RST                               0x0000000010000000
#define STATE_TCP_SENT_FIN                                  0x0000000020000000
#define STATE_TCP_SENT_RESET                                0x0000000040000000
#define STATE_TCP_RECEIVED_FIN                              0x0000000080000000
#define STATE_TCP_RECEIVED_RESET                            0x0000000100000000
#define STATE_TCP_REMOTE_CLOSED                             0x0000000200000000
#define STATE_SSL_CONN_CLIENT                               0x0000000400000000
#define STATE_SSL_ENABLED_CONN                              0x0000000800000000
#define STATE_CONN_WRITE_PENDING                            0x0000001000000000
#define STATE_CONN_READ_PENDING                             0x0000002000000000
#define STATE_CONN_PARTIAL_WRITE                            0x0000004000000000
#define STATE_CONN_MARK_DELETE                              0x0000008000000000
#define STATE_TCP_SOCK_LINGER                               0x0000010000000000
#define STATE_CONN_PARTIAL_READ                             0x0000020000000000

#define STATE_TCP_SOCK_CREATE_FAIL                          0x0000000000000001
#define STATE_TCP_SOCK_BIND_FAIL                            0x0000000000000002
#define STATE_TCP_SOCK_CONNECT_FAIL                         0x0000000000000004
#define STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE               0x0000000000000008
#define STATE_TCP_SOCK_LISTEN_FAIL                          0x0000000000000010
#define STATE_TCP_SOCK_REUSE_FAIL                           0x0000000000000020
#define STATE_TCP_SOCK_READ_FAIL                            0x0000000000000040
#define STATE_TCP_SOCK_WRITE_FAIL                           0x0000000000000080
#define STATE_TCP_SOCK_PORT_ASSIGN_FAIL                     0x0000000000000100
#define STATE_TCP_SOCK_FD_CLOSE_FAIL                        0x0000000000000200
#define STATE_TCP_SOCK_POLL_UPDATE_FAIL                     0x0000000000000400
#define STATE_TCP_CONN_ACCEPT_FAIL                          0x0000000000000800
#define STATE_TCP_SOCK_F_GETFL_FAIL                         0x0000000000001000
#define STATE_TCP_SOCK_F_SETFL_FAIL                         0x0000000000002000
#define STATE_TCP_SOCK_O_NONBLOCK_FAIL                      0x0000000000004000
#define STATE_SSL_SOCK_CONNECT_FAIL                         0x0000000000008000
#define STATE_SSL_SOCK_FD_SET_ERROR                         0x0000000000010000
#define STATE_SSL_SOCK_GENERAL_ERROR                        0x0000000000020000
#define STATE_SSL_SOCK_HANDSHAKE_ERROR                      0x0000000000040000
#define STATE_TCP_FIN_SEND_FAIL                             0x0000000000080000
#define STATE_TCP_RESET_SEND_FAIL                           0x0000000000100000
#define STATE_TCP_REMOTE_CLOSED_ERROR                       0x0000000000200000
#define STATE_TCP_TIMEOUT_CLOSED_ERROR                      0x0000000000400000
#define STATE_TCP_SOCK_LINGER_FAIL                          0x0000000000800000
#define STATE_TCP_GETSOCKNAME_FAIL                          0x0000000001000000
#define STATE_TCP_CONNECTION_EXPIRE                         0x0000000002000000


#define AllocSession(__type) g_slice_new(__type)
#define SetSessionToPool(__pool,__session) g_queue_push_tail (__pool,__session)
#define RemoveFromSessionPool(__pool,__session) g_queue_remove (__pool,__session)

#define AllocEmptySessionPool() g_queue_new()
#define GetSesionFromPool(__pool) g_queue_pop_head(__pool)
#define IsSessionPoolEmpty(__pool) g_queue_is_empty(__pool)
#define GetSessionCount(__pool) g_queue_get_length(__pool)

#define AllocEmptyPool() g_queue_new()
#define GetFromPool(__pool) g_queue_pop_head((__pool))
#define IsPoolEmpty(__pool) g_queue_is_empty((__pool))
#define GetPoolCount(__pool) g_queue_get_length((__pool))
#define InitPool(__pool) g_queue_init ((__pool))
#define AddToPool(__pool,__session) g_queue_push_tail ((__pool),__session)
#define RemoveFromPool(__pool,__session) g_queue_remove ((__pool),__session)

#define CreateEmptyPool(__pool) *((__pool)) = g_queue_new ()

#define CreatePool(__pool,__count,__type,__init) \
{ \
    *(__pool) = g_queue_new (); \
    if (*(__pool)) { \
        for (int i = 0; i < __count; i++) \
        { \
            __type *__new_item = g_slice_new0 (__type); \
            if (__new_item == NULL) { \
                *(__pool) = NULL; \
                break; \
            } \
            g_queue_push_tail (*(__pool), __new_item); \
        } \
    } \
} \


#define CreateEventQ() epoll_create(1)
#define DeleteEventQ(__eventQId) close(__eventQId)
#define GetIOEvents(__eventQId, __eventArray, __maxEvents, __to) epoll_wait(__eventQId, __eventArray, __maxEvents, __to)

#define GetIOEventData(__event) __event.data.ptr
#define IsWriteEventSet(__event) (__event.events & EPOLLOUT)
#define IsReadEventSet(__event) (__event.events & EPOLLIN) 
#define IsReadHupEventSet(__event) (__event.events & EPOLLRDHUP)
#define IsOtherEventSet(__event) (__event.events & ~EPOLLOUT & ~EPOLLIN)

#define CreateTimerWheel() g_timer_new()
#define DeleteTimerWheel(__timer) g_timer_destroy(__timer)
#define TimeElapsedTimerWheel(__timer) g_timer_elapsed(__timer, NULL)

#define AllocEmptyPortBindQ() g_queue_new()
#define InitPortBindQ(__bindq) g_queue_init(__bindq)
#define IsPortBindQEmpty(__bindq) g_queue_is_empty(__bindq)
#define GetPortFromPool(__bindq) GPOINTER_TO_INT(g_queue_pop_head(__bindq))
#define SetPortToPool(__bindq,__port) g_queue_push_tail (__bindq,GINT_TO_POINTER(__port))

#define TdMalloc(__size) malloc(__size)

#define AllocMem(__size) g_malloc(__size)
#define FreeMem(__mem) g_free(__mem)

// #define IsIpv6(__saddr) (((struct sockaddr*)__saddr)->sa_family == AF_INET6)

// #define GetSockPort(__laddr)(IsIpv6(__laddr)) ? ((struct sockaddr_in6*)__laddr)->sin6_port : ((struct sockaddr_in*)__laddr)->sin_port

// #define SetSockPort(__laddr,__lport)if (IsIpv6(__laddr)) ((struct sockaddr_in6*)__laddr)->sin6_port=__lport;else ((struct sockaddr_in*)__laddr)->sin_port=__lport

// int IsIpv6 (void* addr);

// uint16_t GetSockPort(void* addr);

// void SetSockPort (void* addr, uint16_t port);

#define GET_SOCK_PORT(__addr,__sockport) \
{ \
    struct sockaddr* __uaddr = (struct sockaddr*) (__addr); \
    if (__uaddr->sa_family == AF_INET6) { \
        struct sockaddr_in6* __addr_in6 = (struct sockaddr_in6*) __addr; \
        *(__sockport) = __addr_in6->sin6_port; \
    } else { \
        struct sockaddr_in* __addr_in = (struct sockaddr_in*) (__addr); \
        *(__sockport) = __addr_in->sin_port; \
    } \
} \

#define SET_SOCK_PORT(__addr,__sockport) \
{ \
    struct sockaddr* __uaddr = (struct sockaddr*) (__addr); \
    if (__uaddr->sa_family == AF_INET6) { \
        struct sockaddr_in6* __addr_in6 = (struct sockaddr_in6*) __addr; \
        __addr_in6->sin6_port = __sockport; \
    } else { \
        struct sockaddr_in* __addr_in = (struct sockaddr_in*) (__addr); \
        __addr_in->sin_port = __sockport; \
    } \
} \

#define CHECK_IPV6(__addr,__is_ipv6) \
{ \
    struct sockaddr* __uaddr = (struct sockaddr*) __addr; \
    if (__uaddr->sa_family == AF_INET6) { \
        *(__is_ipv6) = 1; \
    } else { \
        *(__is_ipv6) = 0; \
    } \
} \

#define SetSockAddress0(__addr,__is_ipv6) \
{ \
    memset((__addr), 0, sizeof(SockAddr_t)); \
    if (__is_ipv6) { \
        (__addr)->in6Addr.sin6_family = AF_INET6; \
    } else { \
        (__addr)->inAddr.sin_family = AF_INET; \
    } \
}\

void GetNextIpStr (char* str, int skip, char* n_str);
void SetSockAddress(SockAddr_t* addr, char* str, int port);

//#################TCP Start###############
int TcpNewConnection(SockAddr_t* localAddress
                        , SockAddr_t* remoteAddress
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

void VerifyTcpConnectionEstablished(int fd
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);

int TcpListenStart(SockAddr_t* localAddress
                    , int listenQLen
                    , SockStats_t** statsArr
                    , int statsCount
                    , void* cState);

int TcpAcceptConnection(int listenerFd
                        , SockAddr_t* lAddr
                        , SockAddr_t* rAddr
                        , SockStats_t** statsArr
                        , int statsCount
                        , void* cState);
                        
void TcpClose(int fd
                , int isLinger
                , int lingerTime
                , SockStats_t** statsArr
                , int statsCount
                , void* cState);
                
void TcpWrShutdown(int fd, void* cState);

int TcpWrite(int fd
                , const char* dataBuff
                , int dataLen
                , SockStats_t** statsArr
                , int statsCount
                , void* cState);

int TcpRead(int fd
                , char* dataBuffer
                , int dataLen
                , SockStats_t** statsArr
                , int statsCount
                , void* cState);


//##################resources######################
#define CreateArray0(__type,__count) g_new0(__type, __count)
#define CreateArray(__type,__count) g_new(__type, __count)
#define DeleteArray(__memptr) g_free(__memptr)
#define CreateStruct0(__type) g_slice_new0(__type) 
#define CreateStruct(__type) g_slice_new(__type) 
#define DeleteStruct(__type, __memptr) g_slice_free(__type,__memptr)

enum ConnCloseMethod {EmTcpFIN = 1
                    , EmTcpRST = 2
                    , EmCloseNotify = 3};

enum ConnCloseType {EmClientClose = 1
                    , EmServerClose = 2
                    , EmDataFinish = 3};
                    
//##################TLS######################
void DoSSLConnect(SSL* newSSL
                    , int fd
                    , int isClient
                    , SockStats_t** statsArr
                    , int statsCount
                    , void* cState);

int SSLRead (SSL* newSSL
                    , char* dataBuffer
                    , int dataLen
                    , SockStats_t** statsArr
                    , int statsCount
                    , void* cState);

int SSLWrite (SSL* newSSL
                    , const char* dataBuffer
                    , int dataLen
                    , SockStats_t** statsArr
                    , int statsCount
                    , void* cState);

void SSLShutdown (SSL* newSSL
                , void* cState);


// json helpers

typedef JsonNode JNode;
typedef JsonObject JObject;
typedef JsonArray JArray;

#define JSET_MEMBER_INT(__obj,__name,__intv) json_object_set_int_member(__obj,__name,__intv)

#define JGET_MEMBER_INT(__obj,__name,__intp) \
{ \
    JsonNode* __node = json_object_get_member(__obj,__name); \
    *(__intp) = json_node_get_int(__node); \
} \

#define JSET_MEMBER_STR(__obj,__name,__strp) json_object_set_string_member(__obj,__name,__strp)

#define JGET_MEMBER_STR(__obj,__name,__strp) \
{ \
    JsonNode* __node = json_object_get_member(__obj,__name); \
    *(__strp) = (char*) json_node_get_string(__node); \
} \

#define JSET_MEMBER_OBJ(__obj,__name,__objp) \
{ \
    *(__objp) = json_object_new (); \
    if ( *(__objp) ) { \
        json_object_set_object_member (__obj,__name,*(__objp)); \
    } \
}

#define JGET_MEMBER_OBJ(__obj,__name,__objp) \
{ \
    JsonNode* __node = json_object_get_member(__obj,__name); \
    *(__objp) = (JObject*) json_node_get_object(__node); \
} \

#define JSET_MEMBER_ARR(__obj,__name,__arrp) \
{ \
    *(__arrp) = json_array_new (); \
    if ( *(__arrp) ) { \
        json_object_set_array_member (__obj,__name,*(__arrp)); \
    } \
}

#define JGET_MEMBER_ARR(__obj,__name,__arrp) \
{ \
    JsonNode* __node = json_object_get_member(__obj,__name); \
    *(__arrp) = json_node_get_array(__node); \
}

#define JGET_ARR_LEN(__arr) json_array_get_length(__arr)

#define JADD_ARR_ELEMENT_INT(__arr,__intv) json_array_add_int_element(__arr,__intv)

#define JGET_ARR_ELEMENT_INT(__arr,__index) json_array_get_int_element(__arr,__index)

#define JADD_ARR_ELEMENT_STR(__arr,__strp) json_array_add_string_element(__arr,__strp)

#define JGET_ARR_ELEMENT_STR(__arr,__index) json_array_get_string_element(__arr,__index)

#define JADD_ARR_ELEMENT_OBJ(__arr,__objp) \
{ \
    *(__objp) = json_object_new (); \
    if ( *(__objp) ) { \
        json_array_add_object_element (__arr,*(__objp)); \
    } \
}

#define JGET_ARR_ELEMENT_OBJ(__arr,__index) json_array_get_object_element(__arr,__index)

#define JADD_ARR_ELEMENT_ARR(__arr,__arrp) \
{ \
    *(__arrp) = json_array_new (); \
    if ( *(__arrp) ) { \
        json_array_add_array_element(__arr,*(__arrp)); \
    } \
}

#define JGET_ARR_ELEMENT_ARR(__arr,__index) json_array_get_array_element(__arr,__index)

#define JGET_ROOT_NODE(__jstr,__rootNodep,__rootNodeObjp) \
{ \
    GError* __rootNodeErr; \
    *(__rootNodep) = json_from_string (__jstr, &__rootNodeErr); \
    if (*(__rootNodep)) { \
        *(__rootNodeObjp) = json_node_get_object (*(__rootNodep)); \
    } \
} \

#define JCREATE_ROOT_NODE(__rootNodep,__rootNodeObjp) \
{ \
    *(__rootNodeObjp) = NULL; \
    *(__rootNodep) = json_node_alloc (); \
    if ( *(__rootNodep) ) { \
        *(__rootNodeObjp) = json_object_new (); \
        if ( *(__rootNodeObjp) ) { \
            json_node_init_object ( *(__rootNodep), *(__rootNodeObjp) ); \
        } else { \
            json_node_free ( *(__rootNodep) ); \
            *(__rootNodep) = NULL; \
        } \
    } \
}

#define JFREE_ROOT_NODE(__rootNode,__rootNodeObj) json_node_free (__rootNode)

#define JNODE_TO_STRING(__node,__pretty) json_to_string(__node, __pretty)

void SetSockStats (SockStats_t* cStats
                        , JObject* jObj);

void SetSockStatsRate (SockStats_t* cStats);

#endif

