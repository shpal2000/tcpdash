#ifndef __EV_SOCKET__H
#define __EV_SOCKET__H

#include "platform.hpp"

class ev_app;
class ev_socket;

union ev_sockaddr 
{
    struct sockaddr_in in_addr;
    struct sockaddr_in6 in_addr6;
};

struct ev_portq
{
    std::queue<u_short> m_queue;
    ev_portq(u_short start_port, u_short end_port)
    {
        while (start_port <= end_port)
        {
            m_queue.push(htons(start_port++));
        }
    };

    u_short get_port ()
    {
        u_short ret = 0;
        if ( not m_queue.empty () )
        {
            ret = m_queue.front ();
            m_queue.pop ();
        }
        return ret;
    };

    void return_port (u_short port) 
    {
        m_queue.push (port);
    }
};

struct ev_socket_opt {
    int snd_buff_len;
    int rcv_buff_len;
};

#define _STATE_TCP_PORT_ASSIGNED                        0x0000000000000001
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
#define _STATE_TCP_POLL_READ_STICKY                     0x0000000000002000
#define _STATE_TCP_POLL_WRITE_STICKY                    0x0000000000004000
#define STATE_TCP_CONN_ACCEPT                           0x0000000000008000
#define _STATE_TCP_CONN_ACCEPT_O_NONBLOCK               0x0000000000010000
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
#define _STATE_TCP_RECEIVED_FIN                         0x0000000080000000
#define _STATE_TCP_RECEIVED_RESET                       0x0000000100000000
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
#define _STATE_TCP_SOCK_PORT_ASSIGN_FAIL                0x0000000000000100
#define STATE_TCP_SOCK_FD_CLOSE_FAIL                    0x0000000000000200
#define _STATE_TCP_SOCK_POLL_UPDATE_FAIL                0x0000000000000400
#define STATE_TCP_CONN_ACCEPT_FAIL                      0x0000000000000800
#define _STATE_TCP_SOCK_F_GETFL_FAIL                    0x0000000000001000
#define _STATE_TCP_SOCK_F_SETFL_FAIL                    0x0000000000002000
#define _STATE_TCP_SOCK_O_NONBLOCK_FAIL                 0x0000000000004000
#define STATE_SSL_SOCK_CONNECT_FAIL                     0x0000000000008000
#define STATE_SSL_SOCK_FD_SET_ERROR                     0x0000000000010000
#define STATE_SSL_SOCK_GENERAL_ERROR                    0x0000000000020000
#define STATE_SSL_SOCK_HANDSHAKE_ERROR                  0x0000000000040000
#define STATE_TCP_FIN_SEND_FAIL                         0x0000000000080000
#define _STATE_TCP_RESET_SEND_FAIL                      0x0000000000100000
#define STATE_TCP_REMOTE_CLOSED_ERROR                   0x0000000000200000
#define _STATE_TCP_TIMEOUT_CLOSED_ERROR                 0x0000000000400000
#define STATE_TCP_SOCK_LINGER_FAIL                      0x0000000000800000
#define _STATE_TCP_GETSOCKNAME_FAIL                     0x0000000001000000
#define _STATE_TCP_CONNECTION_EXPIRE                    0x0000000002000000
#define STATE_TCP_TRANSPARENT_IP_FAIL                   0x0000000004000000
#define STATE_TCP_RCVBUFFORCE_FAIL                      0x0000000008000000
#define STATE_TCP_SNDBUFFORCE_FAIL                      0x0000000010000000


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

#define SSL_NOSEND_CLOSE_NOTIFY                         0
#define SSL_SEND_CLOSE_NOTIFY                           1
#define SSL_SEND_RECEIVE_CLOSE_NOTIFY                   2

#define inc_stats(__stat_name) \
{ \
    for (uint i=0; i < this->m_sockstats_arr->size(); i++) { \
        (*(this->m_sockstats_arr))[i]->__stat_name++; \
    } \
}

#define dec_stats(__stat_name) \
{ \
    for (uint i=0; i < this->m_sockstats_arr->size(); i++) { \
        (*(this->m_sockstats_arr))[i]->__stat_name--; \
    } \
}

#define inc_app_stats(__sock_ptr,__stat_class,__stat_name) \
{ \
    for (uint i=0; i < __sock_ptr->get_sockstats_arr()->size(); i++) { \
        ev_sockstats* __stats_ptr = (*(__sock_ptr->get_sockstats_arr()))[i]; \
        if (isclass<__stat_class>(__stats_ptr)) \
        { \
            ((__stat_class*)(__stats_ptr))->__stat_name++; \
        } \
    } \
}

#define dec_app_stats(__sock_ptr,__stat_class,__stat_name) \
{ \
    for (uint i=0; i < __sock_ptr->get_sockstats_arr()->size(); i++) { \
        ev_sockstats* __stats_ptr = (*(__sock_ptr->get_sockstats_arr()))[i]; \
        if (isclass<__stat_class>(__stats_ptr)) \
        { \
            ((__stat_class*)(__stats_ptr))->__stat_name--; \
        } \
    } \
}

struct ev_sockstats_data
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

    uint64_t socketRcvBufForceFail;
    uint64_t socketSndBufForceFail;

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

    uint64_t tcpActiveConns;
};

struct ev_sockstats : ev_sockstats_data
{
    ev_sockstats () : ev_sockstats_data () {}
    virtual ~ev_sockstats () {};

    virtual void tick_sec ()
    {
        tcpConnInitRate = tcpConnInitInSec;
        tcpConnInitInSec = 0;

        tcpConnInitSuccessRate = tcpConnInitSuccessInSec;
        tcpConnInitSuccessInSec = 0;

        tcpAcceptSuccessRate = tcpAcceptSuccessInSec;
        tcpAcceptSuccessInSec = 0;

        sslConnInitRate = sslConnInitInSec;
        sslConnInitInSec = 0;

        sslConnInitSuccessRate = sslConnInitSuccessInSec;
        sslConnInitSuccessInSec = 0;

        sslAcceptSuccessRate = sslAcceptSuccessInSec;
        sslAcceptSuccessInSec = 0;
    }
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
    ev_portq* m_portq;
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

    ev_socket* m_parent;

    ev_socket_opt* m_sock_opt;

public:
    ev_socket* m_next;
    ev_socket* m_prev;


public:
    ev_socket();
    virtual ~ev_socket();

    virtual void on_establish () = 0;
    virtual void on_write () = 0;
    virtual void on_wstatus (int bytes_written
                            , int write_status) = 0;
    virtual void on_read () = 0;
    virtual void on_rstatus (int bytes_read
                            , int read_status) = 0;
    virtual void on_finish () = 0;

    ev_socket* get_parent () {return m_parent;};

    void set_portq (ev_portq* portq)
    {
        m_portq = portq;
    }

    ev_portq* get_portq ()
    {
        return m_portq;
    }

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

    uint64_t get_state ()
    {
        return m_state;
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

    void set_socket_opt (ev_socket_opt* ev_sock_opt)
    {
        m_sock_opt = ev_sock_opt;
    }

    ev_socket_opt* get_socket_opt ()
    {
        return m_sock_opt;
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
    void init ();
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
                                        , ev_sockaddr* remoteAddress
                                        , std::vector<ev_sockstats*>* statsArr
                                        , ev_portq* portq
                                        , ev_socket_opt* ev_sock_opt);

    static ev_socket* new_tcp_listen (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , int listenQLen
                                        , std::vector<ev_sockstats*>* statsArr
                                        , ev_socket_opt* ev_sock_opt);

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
    
    void write_close (int send_close_notify=0);
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
    void tcp_close (int isLinger=0, int lingerTime=0);
    void tcp_write_shutdown ();
    int tcp_write (const char* dataBuffer, int dataLen);
    int tcp_read (char* dataBuffer, int dataLen);
    void tcp_accept (ev_socket* ev_sock_parent);

    int ssl_read (char* dataBuffer, int dataLen);
    int ssl_write (const char* dataBuffer, int dataLen);
    void ssl_shutdown ();
    
    ///////////////////////////////helper functions/////////////////////////
    void close_socket ();
    void tcp_connection_success ();
    void tcp_connection_fail ();
    void handle_tcp_accept ();
    void handle_tcp_connect_complete ();
    void do_ssl_handshake ();
    void do_close_connection ();
    void do_read_next_data ();
    void do_write_next_data ();

    ///////////////////////////////utility functions/////////////////////////
public:
    static void sockaddr_to_ip_str (ev_sockaddr* addr, char* str)
    {
        struct sockaddr* uaddr = (struct sockaddr*) addr;

        if (uaddr->sa_family == AF_INET6) {
                inet_ntop(AF_INET6
                    , &( ((struct sockaddr_in6*)addr)->sin6_addr )
                    , str
                    , INET6_ADDRSTRLEN);
        }else{
                inet_ntop(AF_INET
                    , &( ((struct sockaddr_in*)addr)->sin_addr )
                    , str
                    , INET_ADDRSTRLEN);
        }
    };

    static void get_nextip_str (char* str, int skip, char* n_str)
    {
        //check str for ipv6 ???
        bool is_ipv6 = false;

        if (is_ipv6) 
        {
            strcpy (n_str, str);
        } 
        else 
        {
            int d1, d2, d3, d4;
            sscanf (str, "%d.%d.%d.%d", &d1, &d2, &d3, &d4);
            while (skip) {
                skip--;
                d4++;
                if (d4 >= 255) {
                    d4 = 0;
                    d3++;
                    if (d3 >= 255) {
                        d3 = 0;
                        d2++;
                        if (d2 >= 255) {
                            d2 = 0;
                            d1++;
                        }
                    }
                } 
            }
            sprintf (n_str, "%d.%d.%d.%d", d1, d2, d3, d4);
        }
    };

    static void set_sockaddr (ev_sockaddr* addr
                                , const char* str
                                , u_short port)
    {
        //check str for ipv6 ???
        bool is_ipv6 = false;

        memset((addr), 0, sizeof(ev_sockaddr));

        if (is_ipv6)
        {
            addr->in_addr6.sin6_family = AF_INET6;
            inet_pton(AF_INET6
                , str
                , &addr->in_addr6.sin6_addr);
            addr->in_addr6.sin6_port = port;
        } 
        else 
        {
            addr->in_addr.sin_family = AF_INET;
            inet_pton(AF_INET
                    , str
                    , &addr->in_addr.sin_addr);
            addr->in_addr.sin_port = port;
        }
    }

    static void set_sockaddr_port (ev_sockaddr* addr, int port)
    {
        struct sockaddr* uaddr = (struct sockaddr*) (addr);
        if (uaddr->sa_family == AF_INET6)
        {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)(addr);
            addr_in6->sin6_port = port;
        } 
        else
        {
            struct sockaddr_in* addr_in = (struct sockaddr_in*) (addr);
            addr_in->sin_port = port;
        }
    }

    static u_short get_sockaddr_port (ev_sockaddr* addr)
    {
        u_short port;
        struct sockaddr* uaddr = (struct sockaddr*) (addr);
        if (uaddr->sa_family == AF_INET6)
        {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*) addr;
            port = addr_in6->sin6_port;
        } 
        else
        {
            struct sockaddr_in* addr_in = (struct sockaddr_in*) (addr);
            port = addr_in->sin_port;
        }
        return port;
    }
};

struct ev_sockaddrx
{
    ev_sockaddr m_addr;
    ev_portq* m_portq;

    ev_sockaddrx(ev_portq* portq)
    {
        m_portq = portq;
    }

    ev_sockaddrx(u_short start_port, u_short end_port)
        : ev_sockaddrx ( new ev_portq (start_port, end_port) )
    {
    }
};

struct ev_sock_ll { 
   ev_socket *m_head;
   int m_count;

    ev_sock_ll ()
    {
        m_count = 0;
        m_head = nullptr;
    }
    
    void add (ev_socket* ev_sock) 
    {
        ev_sock->m_prev = nullptr;

        if (m_head == nullptr) {
            m_head = ev_sock;
            ev_sock->m_next = nullptr;
        } else {
            ev_sock->m_next = m_head;
            m_head->m_prev = ev_sock;
            m_head = ev_sock;
        }

        m_count++;
    }

    void remove (ev_socket* ev_sock)
    {
        if (m_head == ev_sock) {
            m_head = ev_sock->m_next;
            if (m_head){
                m_head->m_prev = nullptr;
            }
        } else {
            ev_sock->m_prev->m_next = ev_sock->m_next;
            if (ev_sock->m_next) {
                ev_sock->m_next->m_prev = ev_sock->m_prev;
            }
        }

        m_count--;
    }
};

#endif