#include "platform.hpp"
class ev_app;

#define STATE_TCP_PORT_ASSIGNED                         0x0000000000000001
#define STATE_TCP_SOCK_CREATE                           0x0000000000000002
#define STATE_TCP_SOCK_REUSE                            0x0000000000000004
#define STATE_TCP_SOCK_BIND                             0x0000000000000008
#define STATE_TCP_CONN_INIT                             0x0000000000000010
#define STATE_TCP_CONN_IN_PROGRESS                      0x0000000000000020
#define STATE_TCP_CONN_IN_PROGRESS2                     0x0000000000000040
#define STATE_TCP_CONN_ESTABLISHED                      0x0000000000000080
#define STATE_TCP_SOCK_FD_CLOSE                         0x0000000000000100
#define STATE_TCP_LISTENING                             0x0000000000000200
#define STATE_TCP_LISTEN_STOP                           0x0000000000000400
#define STATE_TCP_POLL_READ_CURRENT                     0x0000000000000800
#define STATE_TCP_POLL_WRITE_CURRENT                    0x0000000000001000
#define STATE_TCP_POLL_READ_STICKY                      0x0000000000002000
#define STATE_TCP_POLL_WRITE_STICKY                     0x0000000000004000
#define STATE_TCP_CONN_ACCEPT                           0x0000000000008000
#define STATE_TCP_CONN_ACCEPT_O_NONBLOCK                0x0000000000010000
#define STATE_SSL_CONN_INIT                             0x0000000000020000
#define STATE_SSL_CONN_IN_PROGRESS                      0x0000000000040000
#define STATE_SSL_CONN_ESTABLISHED                      0x0000000000080000
#define STATE_SSL_SENT_SHUTDOWN                         0x0000000000100000
#define STATE_SSL_RECEIVED_SHUTDOWN                     0x0000000000200000
#define STATE_SSL_HANDSHAKE_WANT_READ                   0x0000000000400000
#define STATE_SSL_HANDSHAKE_WANT_WRITE                  0x0000000000800000
#define STATE_SSL_TO_SEND_SHUTDOWN                      0x0000000001000000
#define STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN              0x0000000002000000
#define STATE_NO_MORE_WRITE_DATA                        0x0000000004000000
#define STATE_TCP_TO_SEND_FIN                           0x0000000008000000
#define STATE_TCP_TO_SEND_RST                           0x0000000010000000
#define STATE_TCP_SENT_FIN                              0x0000000020000000
#define STATE_TCP_SENT_RESET                            0x0000000040000000
#define STATE_TCP_RECEIVED_FIN                          0x0000000080000000
#define STATE_TCP_RECEIVED_RESET                        0x0000000100000000
#define STATE_TCP_REMOTE_CLOSED                         0x0000000200000000
#define STATE_SSL_CONN_CLIENT                           0x0000000400000000
#define STATE_SSL_ENABLED_CONN                          0x0000000800000000
#define STATE_CONN_WRITE_PENDING                        0x0000001000000000
#define STATE_CONN_READ_PENDING                         0x0000002000000000
#define STATE_CONN_PARTIAL_WRITE                        0x0000004000000000
#define STATE_CONN_MARK_DELETE                          0x0000008000000000
#define STATE_TCP_SOCK_LINGER                           0x0000010000000000
#define STATE_CONN_PARTIAL_READ                         0x0000020000000000
#define STATE_TCP_SOCK_IP_TRANSPARENT                   0x0000040000000000
#define STATE_CONN_MARK_FINISH                          0x0000080000000000

#define STATE_TCP_SOCK_CREATE_FAIL                      0x0000000000000001
#define STATE_TCP_SOCK_BIND_FAIL                        0x0000000000000002
#define STATE_TCP_SOCK_CONNECT_FAIL                     0x0000000000000004
#define STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE           0x0000000000000008
#define STATE_TCP_SOCK_LISTEN_FAIL                      0x0000000000000010
#define STATE_TCP_SOCK_REUSE_FAIL                       0x0000000000000020
#define STATE_TCP_SOCK_READ_FAIL                        0x0000000000000040
#define STATE_TCP_SOCK_WRITE_FAIL                       0x0000000000000080
#define STATE_TCP_SOCK_PORT_ASSIGN_FAIL                 0x0000000000000100
#define STATE_TCP_SOCK_FD_CLOSE_FAIL                    0x0000000000000200
#define STATE_TCP_SOCK_POLL_UPDATE_FAIL                 0x0000000000000400
#define STATE_TCP_CONN_ACCEPT_FAIL                      0x0000000000000800
#define STATE_TCP_SOCK_F_GETFL_FAIL                     0x0000000000001000
#define STATE_TCP_SOCK_F_SETFL_FAIL                     0x0000000000002000
#define STATE_TCP_SOCK_O_NONBLOCK_FAIL                  0x0000000000004000
#define STATE_SSL_SOCK_CONNECT_FAIL                     0x0000000000008000
#define STATE_SSL_SOCK_FD_SET_ERROR                     0x0000000000010000
#define STATE_SSL_SOCK_GENERAL_ERROR                    0x0000000000020000
#define STATE_SSL_SOCK_HANDSHAKE_ERROR                  0x0000000000040000
#define STATE_TCP_FIN_SEND_FAIL                         0x0000000000080000
#define STATE_TCP_RESET_SEND_FAIL                       0x0000000000100000
#define STATE_TCP_REMOTE_CLOSED_ERROR                   0x0000000000200000
#define STATE_TCP_TIMEOUT_CLOSED_ERROR                  0x0000000000400000
#define STATE_TCP_SOCK_LINGER_FAIL                      0x0000000000800000
#define STATE_TCP_GETSOCKNAME_FAIL                      0x0000000001000000
#define STATE_TCP_CONNECTION_EXPIRE                     0x0000000002000000
#define STATE_TCP_TRANSPARENT_IP_FAIL                   0x0000000004000000


#define CONNAPP_STATE_INIT                               0
#define CONNAPP_STATE_CONNECTION_IN_PROGRESS             1
#define CONNAPP_STATE_CONNECTION_ESTABLISHED             2
#define CONNAPP_STATE_CONNECTION_ESTABLISH_FAILED        3
#define CONNAPP_STATE_CONNECTION_CLOSED                  4
#define CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS         5
#define CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED         6
#define CONNAPP_STATE_SSL_CONNECTION_ESTABLISH_FAILED    7
#define CONNAPP_STATE_LISTEN                             1000

#define READ_STATUS_NORMAL                               0
#define READ_STATUS_TCP_RESET                            1
#define READ_STATUS_TCP_TIMEOUT                          2
#define READ_STATUS_ERROR                                3
#define READ_STATUS_TCP_CLOSE                            4

#define WRITE_STATUS_NORMAL                              0
#define WRITE_STATUS_TCP_RESET                           1
#define WRITE_STATUS_TCP_TIMEOUT                         2
#define WRITE_STATUS_ERROR                               3

#define inc_stats(__stat_name) \
{ \
    for (ev_sockstats* __stats_ptr : *this->m_sockstats_arr) { \
        __stats_ptr->__stat_name++; \
    } \
}

#define inc_stats2(__ev_sock,__stat_name) \
{ \
    for (ev_sockstats* __stats_ptr : *__ev_sock->get_sockstats_arr()) { \
        __stats_ptr->__stat_name++; \
    } \
}

#define CHECK_IPV6(__addr,__is_ipv6) \
{ \
    struct sockaddr* __uaddr = (struct sockaddr*) __addr; \
    if (__uaddr->sa_family == AF_INET6) { \
        *(__is_ipv6) = true; \
    } else { \
        *(__is_ipv6) = false; \
    } \
}

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
}


union ev_sockaddr 
{
    struct sockaddr_in in_addr;
    struct sockaddr_in6 in_addr6;
};

struct ev_sockstats
{
    uint64_t socketCreate;    
    uint64_t socketCreateFail;
    uint64_t socketListenFail;
    uint64_t socketReuseSet;
    uint64_t socketReuseSetFail;
    uint64_t socketIpTransparentSet;
    uint64_t socketIpTransparentSetFail;
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
};

class epoll_ctx 
{
public:
    epoll_ctx(ev_app* app_ptr, int max_events, int epoll_timeout)
    {
        m_epoll_id = epoll_create (1);
        m_max_epoll_events = max_events;
        m_epoll_timeout = epoll_timeout;
        m_epoll_event_arr = new struct epoll_event [max_events];
        m_app = app_ptr;
    };

    ~epoll_ctx()
    {
        //todo
    }

    ev_app* m_app;
    int m_epoll_id;
    int m_epoll_timeout;
    int m_max_epoll_events;
    struct epoll_event* m_epoll_event_arr;
    std::queue<ev_socket*> m_abort_list;
    std::queue<ev_socket*> m_finish_list;
};

class ev_socket
{
private:
    epoll_ctx* m_epoll_ctx;
    int m_fd;
    uint16_t m_saved_lport;
    uint16_t m_saved_rport;

    uint64_t m_state;
    uint64_t m_error_state;

    int m_sys_errno;
    int m_socket_errno;

    SSL* m_ssl;
    bool m_ssl_client;    
    int m_ssl_errno;

    int m_status;

    std::vector<ev_sockstats*> *m_sockstats_arr;

    bool m_ipv6;
    ev_sockaddr m_local_addr;
    ev_sockaddr m_remote_addr;

    int m_read_status;

    char* m_read_buffer;
    int m_read_buff_offset;
    int m_read_data_len;

    int m_read_buff_offset_cur;
    int m_read_data_len_cur;
    int m_read_bytes_len_cur;

    int m_write_status;

    char* m_write_buffer;
    int m_write_buff_offset;
    int m_write_data_len;

    int m_write_buff_offset_cur;
    int m_write_data_len_cur;
    int m_write_bytes_len_cur;


public:
    ev_socket();
    virtual ~ev_socket();

    void set_status (int status)
    {
        m_status = status;
    };

    int get_status ()
    {
        return m_status;
    }

    uint64_t is_set_state (uint64_t state_bits)
    {
        return m_state & state_bits;
    };

    void set_state (uint64_t state_bits)
    {
        m_state |= state_bits;
    };

    void clear_state (uint64_t state_bits)
    {
        m_state &= ~state_bits;
    };

    uint64_t is_set_error_state (uint64_t state_bits)
    {
        return m_error_state & state_bits;
    };

    void set_error_state (uint64_t state_bits)
    {
        m_error_state |= state_bits;
        m_sys_errno = errno;
    };

    uint64_t get_error_state ()
    {
        return m_error_state;
    }

    int get_sys_errno ()
    {
        return m_sys_errno;
    }

    void set_socket_errno (int sock_errno)
    {
        m_socket_errno = sock_errno;
    }

    bool is_fd_closed ()
    {
        return ( is_set_state (STATE_TCP_SOCK_FD_CLOSE) 
                    || is_set_error_state (STATE_TCP_SOCK_FD_CLOSE_FAIL) );
    }

    void set_error_state_ssl (uint64_t state_bits, int ssl_errno)
    {
        set_error_state (state_bits);
        m_ssl_errno = ssl_errno;
    };

private:
    void set_ssl (SSL* ssl)
    {
        m_ssl = ssl;
        set_status (CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS);
        if (m_ssl_client) 
        {
            set_state (STATE_SSL_ENABLED_CONN | STATE_SSL_CONN_CLIENT);
        } 
        else 
        {
            set_state (STATE_SSL_ENABLED_CONN);
        }
        do_ssl_handshake ();
    }

public:

    SSL* get_ssl () 
    {
        return m_ssl;
    }

    void set_as_ssl_client (SSL* ssl)
    {
        m_ssl_client = true;
        set_ssl (ssl);
    }

    void set_as_ssl_server (SSL* ssl)
    {
        m_ssl_client = false;
        set_ssl (ssl);
    }

    epoll_ctx* get_epoll_ctx () {
        return m_epoll_ctx;
    }

    void set_sockstats_arr (std::vector<ev_sockstats*>* sockstats_arr)
    {
        m_sockstats_arr = sockstats_arr;
    }

    std::vector<ev_sockstats*>* get_sockstats_arr ()
    {
        return m_sockstats_arr;
    }

    static epoll_ctx* epoll_alloc (ev_app* app_ptr
                                        , int max_events
                                        , int epoll_timeout);
    static void epoll_free (epoll_ctx* epoll_ctxp);
    static void epoll_process (epoll_ctx* epoll_ctxp);

    static ev_socket* new_tcp_connect (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , ev_sockaddr* remoteAddress);

    static ev_socket* new_tcp_listen (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , int listenQLen);

    void enable_rd_only_notification ();
    void enable_wr_only_notification ();

    void enable_rd_wr_notification ();
    void disable_rd_wr_notification ();

    void disable_wr_notification ();
    void disable_rd_notification ();

    void enable_wr_notification ();
    void enable_rd_notification ();

    void read_next_data (char* readBuffer
                            , int readBuffOffset
                            , int readDataLen
                            , bool partialRead);

    void write_next_data (char* writeBuffer
                            , int writeBuffOffset
                            , int writeDataLen
                            , bool partialWrite);
    
    void abort ();



private:
    ////////////////////////////tcp ssl platform functions////////////////////
    int tcp_connect (epoll_ctx* epoll_ctxp
                    , ev_sockaddr* localAddress
                    , ev_sockaddr* remoteAddress);
    
    int tcp_listen (epoll_ctx* epoll_ctxp
                    , ev_sockaddr* localAddress
                    , int listenQLen);

    void tcp_verify_established ();
    void tcp_close (int isLinger, int lingerTime);
    void tcp_write_shutdown ();
    int tcp_write (const char* dataBuffer, int dataLen);
    int tcp_read (char* dataBuffer, int dataLen);
    void tcp_accept (ev_socket* ev_sock_parent);

    int ssl_read (char* dataBuffer, int dataLen);
    int ssl_write (const char* dataBuffer, int dataLen);
    void ssl_shutdown ();
    
    ///////////////////////////////helper functions/////////////////////////
    void invoke_app_cb (int cbid);
    void close_socket ();
    void tcp_connection_success ();
    void tcp_connection_fail ();
    void handle_tcp_accept ();
    void handle_tcp_connect_complete ();
    void do_ssl_handshake ();
    void do_close_connection ();
    void do_read_next_data ();
    void do_write_next_data ();
};

