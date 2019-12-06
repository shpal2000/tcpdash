#include "ev_socket.hpp"

ev_socket::ev_socket(/* args */)
{
    m_saved_lport = 0;
    m_saved_rport = 0;

    m_state = 0;
    m_error_state = 0;

    m_local_addr = nullptr;
    m_remote_addr = nullptr;

    m_port_pool = nullptr;
    m_sockstats_arr = nullptr;

}

ev_socket::~ev_socket()
{
}

ev_socket* ev_socket::tcp_accept(epoll_ctx* epoll_ctxp)
{
    // ev_socket* new_ev_socket = new ev_socket ();
    return nullptr;
}

epoll_ctx* ev_socket::epoll_alloc(ev_app* app_ptr, int max_events, int epoll_timeout)
{
    epoll_ctx* epoll_ctxp = new epoll_ctx(app_ptr, max_events, epoll_timeout);
    return epoll_ctxp;
}

void ev_socket::epoll_free(epoll_ctx* epoll_ctxp)
{
    delete epoll_ctxp;
}

void ev_socket::epoll_process(epoll_ctx* epoll_ctxp)
{
    int event_count = epoll_wait (epoll_ctxp->m_epoll_id
                        , epoll_ctxp->m_epoll_event_arr
                        , epoll_ctxp->m_max_epoll_events
                        , epoll_ctxp->m_epoll_timeout);
    
    if (event_count > 0)
    {
        for (int event_index = 0; event_index < event_count; event_index++)
        {
            ev_socket* event_socket_ptr 
                = (ev_socket*) epoll_ctxp->m_epoll_event_arr[event_index].data.ptr;

            if ( event_socket_ptr->is_set_state (STATE_TCP_LISTENING) )
            {
                event_socket_ptr->tcp_accept (epoll_ctxp);
            } 
            else
            {

            }
        }
    }
}

int ev_socket::tcp_connect_platform ()
{
    //stats
    inc_stats (tcpConnInit);
    inc_stats (tcpConnInitInSec);

    //check ipv6
    int is_ipv6;
    CHECK_IPV6(m_local_addr, &is_ipv6);

    //create socket
    int socket_fd = -1;
    if (is_ipv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        inc_stats (socketCreateFail);
        set_error_state (STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        inc_stats (socketCreate);
        set_state (STATE_TCP_SOCK_CREATE);

        int setsockopt_status = -1;
        setsockopt_status = setsockopt(socket_fd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));

        //???
        setsockopt(socket_fd, SOL_IP, IP_TRANSPARENT, &(int){ 1 }, sizeof(int));
        // printf ("IP_TRANSPARENT = %d\n", ret_val);

        if (setsockopt_status == -1){
            inc_stats (socketReuseSetFail);
            set_error_state (STATE_TCP_SOCK_REUSE_FAIL);
        } else {
            inc_stats (socketReuseSet);
            set_state (STATE_TCP_SOCK_REUSE);
            //bind local socket
            int bind_status = -1;
            if (is_ipv6){
                bind_status = bind(socket_fd
                                    , (struct sockaddr*)m_local_addr
                                    , sizeof(struct sockaddr_in6));
            }else{
                bind_status = bind(socket_fd
                                    , (struct sockaddr*)m_local_addr
                                    , sizeof(struct sockaddr_in));
            }

            if (bind_status == -1){
                if (is_ipv6){
                    inc_stats (socketBindIpv6Fail);
                }else{
                    inc_stats (socketBindIpv4Fail);
                }
                set_error_state (STATE_TCP_SOCK_BIND_FAIL);
            }else{
                if (is_ipv6){
                    inc_stats (socketBindIpv6);
                }else{
                    inc_stats (socketBindIpv4);
                }
                set_state (STATE_TCP_SOCK_BIND);

                //connect socket
                int connect_status = -1;
                if (is_ipv6){
                    connect_status = connect(socket_fd
                                    , (struct sockaddr*)m_remote_addr
                                    , sizeof(struct sockaddr_in6));
                }else{
                    connect_status = connect(socket_fd
                                    , (struct sockaddr*)m_remote_addr
                                    , sizeof(struct sockaddr_in));
                }
                set_state (STATE_TCP_CONN_INIT);

                //check connect status
                if (connect_status < 0){
                    if (errno == EINPROGRESS){
                        set_state (STATE_TCP_CONN_IN_PROGRESS);
                    }else{
                        set_error_state (STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE);
                        if ( (get_sys_errno () == EADDRNOTAVAIL) ) {
                            inc_stats (tcpConnInitFailImmediateEaddrNotAvail);
                        }else{
                            inc_stats (tcpConnInitFailImmediateOther);
                        }
                    }
                }else{
                    set_state (STATE_TCP_CONN_IN_PROGRESS2);
                }
            }
        }
    }


    if ( get_error_state () ){

        inc_stats (tcpConnInitFail);

        if (socket_fd != -1){
            tcp_close_platform();
        }
        return -1;
    }

    return socket_fd;
}

void ev_socket::tcp_close_platform (int isLinger=0, int lingerTime=0) {

    if (isLinger) {
        struct linger sl;
        sl.l_onoff = 1;
        sl.l_linger = lingerTime;

        int lingerStatus 
            = setsockopt(m_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
        
        if (lingerStatus < 0) {
            inc_stats (socketLingerSetFail);
            set_error_state (STATE_TCP_SOCK_LINGER_FAIL);
        } else {
            inc_stats (socketLingerSet);
            set_state (STATE_TCP_SOCK_LINGER);
        }
    }

    if ( close (m_fd) ) {
        set_error_state (STATE_TCP_SOCK_FD_CLOSE_FAIL);
    } else {
        set_state (STATE_TCP_SOCK_FD_CLOSE);
    }
}

void ev_socket::tcp_verify_established_platform ()
{
    int socketErr;
    socklen_t socketErrBufLen = sizeof(int);

    int retGetsockopt = getsockopt(m_fd
                                    , SOL_SOCKET
                                    , SO_ERROR
                                    , &socketErr
                                    , &socketErrBufLen);
    
    if ((retGetsockopt|socketErr) == 0){
        set_state (STATE_TCP_CONN_ESTABLISHED);
        inc_stats (tcpConnInitSuccess);
        inc_stats (tcpConnInitSuccessInSec);
    }else {
        set_error_state (STATE_TCP_SOCK_CONNECT_FAIL);
        set_socket_errno (socketErr);
        inc_stats (tcpConnInitFail);
    }
}

int ev_socket::tcp_listen_platform(int listenQLen) {

    //check ipv6
    int is_ipv6;
    CHECK_IPV6(m_local_addr, &is_ipv6);

    //create socket
    int socket_fd = -1;
    if (is_ipv6){
        socket_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        socket_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (socket_fd == -1) {
        inc_stats (socketCreateFail);
        set_error_state (STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        inc_stats (socketCreate);
        set_state (STATE_TCP_SOCK_CREATE);

        //???
        setsockopt(socket_fd, SOL_IP, IP_TRANSPARENT, &(int){ 1 }, sizeof(int));
        // printf ("IP_TRANSPARENT = %d\n", ret_val);

        

        //bind local socket
        int bind_status = -1;
        if (is_ipv6){
            bind_status = bind(socket_fd
                                , (struct sockaddr*) m_local_addr
                                , sizeof(struct sockaddr_in6));
        }else{

            //???
            // struct addrinfo hints, *res;            
            // memset(&hints, 0, sizeof(hints));
            // if(getaddrinfo(NULL, "883", &hints, &res) != 0){
            //     perror("getaddrinfo: ");
            //     exit(EXIT_FAILURE);
            // }

            // bind_status = bind(socket_fd, res->ai_addr, res->ai_addrlen);

            bind_status = bind(socket_fd
                                , (struct sockaddr*) m_local_addr
                                , sizeof(struct sockaddr_in));
        }

        if (bind_status == -1){
            if (is_ipv6){
                inc_stats (socketBindIpv6Fail);
            }else{
                inc_stats (socketBindIpv4Fail);
            }
            set_error_state (STATE_TCP_SOCK_BIND_FAIL);
        }else{
            if (is_ipv6){
                inc_stats (socketBindIpv6);
            }else{
                inc_stats (socketBindIpv4);
            }
            set_state (STATE_TCP_SOCK_BIND);

            //listen socket
            int listen_status = -1;
            listen_status = listen(socket_fd, listenQLen);

            //check listen status
            if (listen_status < 0) {
                set_error_state (STATE_TCP_SOCK_LISTEN_FAIL); 
            } else {
                set_state (STATE_TCP_LISTENING);
            }
        }
    }

    if ( get_error_state () ){

        inc_stats (tcpListenStartFail);

        if (socket_fd != -1){
            tcp_close_platform ();
        }
        return -1;
    }

    return socket_fd;
}