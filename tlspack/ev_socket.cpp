#include "ev_socket.hpp"
#include "ev_app.hpp"

ev_socket::ev_socket()
{
    m_epoll_ctx = nullptr;
    m_fd = -1;
    m_saved_lport = 0;
    m_saved_rport = 0;

    m_state = 0;
    m_error_state = 0;

    m_sys_errno = 0;
    m_socket_errno = 0;

    m_ssl = nullptr;
    m_ssl_client = 0;
    m_ssl_errno = 0;

    m_status = CONNAPP_STATE_INIT;

    m_local_addr = nullptr;
    m_remote_addr = nullptr;

    m_ipv6 = false;

    m_sockstats_arr = nullptr;
}

ev_socket::~ev_socket()
{
}

//////////////////////////////// platform functions///////////////////////////////
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

void ev_socket::tcp_verify_established ()
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

void ev_socket::tcp_close (int isLinger=0, int lingerTime=0) {

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

void ev_socket::tcp_write_shutdown() {    
    if (shutdown(m_fd, SHUT_WR)) {
        set_error_state (STATE_TCP_FIN_SEND_FAIL);
    } else {
        set_state (STATE_TCP_SENT_FIN);
    }
}

int ev_socket::tcp_write (const char* dataBuffer, int dataLen)
{
    // int bytesSent = send(fd, dataBuffer, dataLen, MSG_NOSIGNAL);
    int bytesSent = write(m_fd, dataBuffer, dataLen);

    if (bytesSent <= 0){
        set_error_state (STATE_TCP_SOCK_WRITE_FAIL);
        inc_stats (tcpWriteFail);

        if (bytesSent == 0) {
            inc_stats (tcpWriteReturnsZero);
        }
    }

    return bytesSent;
}

int ev_socket::tcp_read (char* dataBuffer, int dataLen)
{
    int bytesRead = read(m_fd, dataBuffer, dataLen);

    if (bytesRead == 0) {
        set_state (STATE_TCP_REMOTE_CLOSED);
        int socketErr;
        socklen_t socketErrBufLen = sizeof(int);

        int retGetsockopt = getsockopt(m_fd
                                        , SOL_SOCKET
                                        , SO_ERROR
                                        , &socketErr
                                        , &socketErrBufLen);
        if ((retGetsockopt|socketErr)) {
            set_error_state (STATE_TCP_SOCK_READ_FAIL);
        }
    } else if (bytesRead < 0) {
        if (errno == EAGAIN) {
            //nothing to read; retry
        } else {
            set_error_state (STATE_TCP_SOCK_READ_FAIL);
            inc_stats (tcpReadFail);
        }
    }

    return bytesRead;
}

int ev_socket::tcp_accept ()
{
    set_state (STATE_TCP_CONN_ACCEPT);

    int socket_fd = -1;
    socklen_t addrLen = sizeof (ev_sockaddr);

    socket_fd = accept(m_fd
                        , (struct sockaddr *) m_remote_addr
                        , &addrLen);
    
    if (socket_fd < 0) {
        inc_stats (tcpAcceptFail);
        set_error_state (STATE_TCP_CONN_ACCEPT_FAIL);        
    } else {
        set_state (STATE_TCP_CONN_ESTABLISHED);

        inc_stats (tcpAcceptSuccess);
        inc_stats (tcpAcceptSuccessInSec);

        int flags = fcntl(socket_fd, F_GETFL, 0);
        if (flags < 0) {
            set_error_state (STATE_TCP_SOCK_F_GETFL_FAIL 
                            | STATE_TCP_SOCK_O_NONBLOCK_FAIL);
        } else {
            int ret = fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
            if (ret < 0) {
                set_error_state (STATE_TCP_SOCK_F_SETFL_FAIL 
                                | STATE_TCP_SOCK_O_NONBLOCK_FAIL);
            } else {
                set_state (STATE_TCP_CONN_ACCEPT_O_NONBLOCK);

                ret = setsockopt(socket_fd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));
                if (ret < 0) {
                    inc_stats (socketReuseSetFail);
                    set_error_state (STATE_TCP_SOCK_REUSE_FAIL);
                } else {
                    inc_stats (socketReuseSet);
                    set_state (STATE_TCP_SOCK_REUSE);

                    addrLen = sizeof (ev_sockaddr);
                    ret = getsockname(socket_fd, (struct sockaddr*) m_remote_addr, &addrLen);
                    if (ret < 0) {
                        inc_stats (tcpGetSockNameFail);
                        set_error_state (STATE_TCP_GETSOCKNAME_FAIL);
                    }
                }
            }
        }
    }

    if ( get_error_state () ){
        if (socket_fd != -1){
            tcp_close_platform ();
        }
        return -1;
    }

    return socket_fd;    
}

int ev_socket::ssl_read (char* dataBuffer, int dataLen) 
{
    int bytesSent = SSL_read(m_ssl, dataBuffer, dataLen);

    if (bytesSent <= 0) {
        int sslError = SSL_get_error(m_ssl, bytesSent);
        switch (sslError) {
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
                set_state (STATE_TCP_REMOTE_CLOSED);
                break;
  
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            
            default:
                set_error_state (STATE_SSL_SOCK_GENERAL_ERROR);
                break;
        }
    }

    return bytesSent;
}

int ev_socket::ssl_write (const char* dataBuffer, int dataLen) 
{

    int bytesSent = SSL_write(m_ssl, dataBuffer, dataLen);

    if (bytesSent <= 0) {
        int sslError = SSL_get_error(m_ssl, bytesSent);
        switch (sslError) {
            case SSL_ERROR_SYSCALL:
                set_error_state (STATE_TCP_REMOTE_CLOSED_ERROR);
                break;

            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            
            default:
                set_error_state (STATE_SSL_SOCK_GENERAL_ERROR);
                break;
        }
    }

    return bytesSent;
}

void ev_socket::ssl_shutdown () 
{

    int status = SSL_shutdown(m_ssl);
    int sslError = SSL_get_error(m_ssl, status);
    
    switch (status) {
        case 1:
            set_state (STATE_SSL_RECEIVED_SHUTDOWN);
            clear_state (STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);
            set_state (STATE_SSL_SENT_SHUTDOWN);
            clear_state (STATE_SSL_TO_SEND_SHUTDOWN);
            break;

        case 0:
            set_state (STATE_SSL_SENT_SHUTDOWN);
            clear_state (STATE_SSL_TO_SEND_SHUTDOWN);
            break;

        default:
            switch (sslError) {
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    break;
                
                default:
                    clear_state (STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);
                    clear_state (STATE_SSL_TO_SEND_SHUTDOWN);
                    set_error_state (STATE_SSL_SOCK_GENERAL_ERROR);
                    break;
            }
            break;
    }
}

void ev_socket::enable_rd_only_notification () 
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 
            || is_set_state (STATE_TCP_POLL_WRITE_CURRENT) ) {

        struct epoll_event setEvent = {0};
        setEvent.events = 0;
        setEvent.data.ptr = this;
        setEvent.events = EPOLLIN;

        if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 
                && is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0 ) 
        {
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_ADD, m_fd, &setEvent);
        } 
        else 
        {
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        set_state (STATE_TCP_POLL_READ_CURRENT);
        clear_state (STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void ev_socket::enable_wr_only_notification () 
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) 
            || is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

        struct epoll_event setEvent = {0};
        setEvent.events = 0;
        setEvent.data.ptr = this;
        setEvent.events = EPOLLOUT;

        if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 
                && is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0 ) 
        {
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_ADD, m_fd, &setEvent);
        } 
        else 
        {
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        clear_state (STATE_TCP_POLL_READ_CURRENT);
        set_state (STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void ev_socket::enable_rd_wr_notification () 
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 
            || is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

        struct epoll_event setEvent = {0};
        setEvent.events = 0;
        setEvent.data.ptr = this;
        setEvent.events = EPOLLIN | EPOLLOUT;

        if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0
                &&  is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0) 
        {
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_ADD, m_fd, &setEvent);
        } 
        else 
        {
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        set_state (STATE_TCP_POLL_READ_CURRENT);
        set_state (STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void ev_socket::disable_rd_wr_notification ()
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) 
            || is_set_state (STATE_TCP_POLL_WRITE_CURRENT) ) {

        epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_DEL, m_fd, NULL);

        clear_state (STATE_TCP_POLL_READ_CURRENT);
        clear_state (STATE_TCP_POLL_WRITE_CURRENT);
    }
}

int ev_socket::tcp_connect (epoll_ctx* epoll_ctxp
                            , ev_sockaddr* localAddress
                            , ev_sockaddr* remoteAddress)
{
    //socket stats
    inc_stats (tcpConnInit);
    inc_stats (tcpConnInitInSec);

    //socket contexts
    m_epoll_ctx = epoll_ctxp;
    m_local_addr = localAddress;
    m_remote_addr = remoteAddress;

    //socket if ipv6 
    CHECK_IPV6(m_local_addr, &m_ipv6);

    //create socket
    if (m_ipv6){
        m_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        m_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (m_fd == -1) 
    {
        inc_stats (socketCreateFail);
        set_error_state (STATE_TCP_SOCK_CREATE_FAIL);
    }
    else
    {
        inc_stats (socketCreate);
        set_state (STATE_TCP_SOCK_CREATE);

        int so_reuse_status = setsockopt(m_fd
                                        , SOL_SOCKET
                                        , SO_REUSEADDR
                                        , &(int){ 1 }
                                        , sizeof(int));
        if (so_reuse_status == -1)
        {
            inc_stats (socketReuseSetFail);
            set_error_state (STATE_TCP_SOCK_REUSE_FAIL);
        }
        else
        {
            inc_stats (socketReuseSet);
            set_state (STATE_TCP_SOCK_REUSE);
        }

        //???
        int so_iptrans_status = setsockopt(m_fd
                                        , SOL_IP
                                        , IP_TRANSPARENT
                                        , &(int){ 1 }
                                        , sizeof(int));
        if (so_iptrans_status == -1)
        {
            inc_stats (socketIpTransparentSetFail);
            set_error_state (STATE_TCP_TRANSPARENT_IP_FAIL);
        }
        else
        {
            inc_stats (socketIpTransparentSet);
            set_state (STATE_TCP_SOCK_IP_TRANSPARENT);
        }

        if (so_reuse_status == 0 && so_iptrans_status == 0)
        {
            //socket bind
            int bind_status = -1;
            if (m_ipv6)
            {
                bind_status = bind(m_fd
                                    , (struct sockaddr*)m_local_addr
                                    , sizeof(struct sockaddr_in6));
            }
            else
            {
                bind_status = bind(m_fd
                                    , (struct sockaddr*)m_local_addr
                                    , sizeof(struct sockaddr_in));
            }

            if (bind_status == -1)
            {
                if (m_ipv6)
                {
                    inc_stats (socketBindIpv6Fail);
                }
                else
                {
                    inc_stats (socketBindIpv4Fail);
                }
                set_error_state (STATE_TCP_SOCK_BIND_FAIL);
            }
            else
            {
                if (m_ipv6)
                {
                    inc_stats (socketBindIpv6);
                }
                else
                {
                    inc_stats (socketBindIpv4);
                }
                set_state (STATE_TCP_SOCK_BIND);

                //socket connect
                int connect_status = -1;
                if (m_ipv6)
                {
                    connect_status = connect(m_fd
                                            , (struct sockaddr*)m_remote_addr
                                            , sizeof(struct sockaddr_in6));
                }
                else
                {
                    connect_status = connect(m_fd
                                            , (struct sockaddr*)m_remote_addr
                                            , sizeof(struct sockaddr_in));
                }
                set_state (STATE_TCP_CONN_INIT);

                //check connect status
                if (connect_status < 0)
                {
                    if (errno == EINPROGRESS)
                    {
                        set_state (STATE_TCP_CONN_IN_PROGRESS);
                    }
                    else
                    {
                        set_error_state (STATE_TCP_SOCK_CONNECT_FAIL_IMMEDIATE);
                        if ( (get_sys_errno () == EADDRNOTAVAIL) ) 
                        {
                            inc_stats (tcpConnInitFailImmediateEaddrNotAvail);
                        }
                        else
                        {
                            inc_stats (tcpConnInitFailImmediateOther);
                        }
                    }
                }
                else
                {
                    set_state (STATE_TCP_CONN_IN_PROGRESS2);
                } 
            }
        }
    }

    int ret_status = 0;
    if ( get_error_state () )
    {
        ret_status = -1;
        inc_stats (tcpConnInitFail);

        if (m_fd != -1){
            tcp_close();
        }
    }
    else
    {
        //socket status
        set_status (CONNAPP_STATE_CONNECTION_IN_PROGRESS);
        enable_rd_wr_notification ();
    }

    return ret_status;
}

int ev_socket::tcp_listen(epoll_ctx* epoll_ctxp
                            , ev_sockaddr* localAddress
                            , int listenQLen)
{
    //stats
    inc_stats (tcpListenStart);

    //socket contexts
    m_epoll_ctx = epoll_ctxp;
    m_local_addr = localAddress;

    //socket if ipv6 
    CHECK_IPV6(m_local_addr, &m_ipv6);

    //create socket
    if (m_ipv6){
        m_fd = socket(AF_INET6 , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }else{
        m_fd = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    }

    if (m_fd == -1) {
        inc_stats (socketCreateFail);
        set_error_state (STATE_TCP_SOCK_CREATE_FAIL);
    }else{
        inc_stats (socketCreate);
        set_state (STATE_TCP_SOCK_CREATE);

        int so_reuse_status = setsockopt(m_fd
                                        , SOL_SOCKET
                                        , SO_REUSEADDR
                                        , &(int){ 1 }
                                        , sizeof(int));
        if (so_reuse_status == -1)
        {
            inc_stats (socketReuseSetFail);
            set_error_state (STATE_TCP_SOCK_REUSE_FAIL);
        }
        else
        {
            inc_stats (socketReuseSet);
            set_state (STATE_TCP_SOCK_REUSE);
        }

        //???
        int so_iptrans_status = setsockopt(m_fd
                                        , SOL_IP
                                        , IP_TRANSPARENT
                                        , &(int){ 1 }
                                        , sizeof(int));
        if (so_iptrans_status == -1)
        {
            inc_stats (socketIpTransparentSetFail);
            set_error_state (STATE_TCP_TRANSPARENT_IP_FAIL);
        }
        else
        {
            inc_stats (socketIpTransparentSet);
            set_state (STATE_TCP_SOCK_IP_TRANSPARENT);
        }
        
        if (so_reuse_status == 0 && so_iptrans_status == 0)
        {
            //socket bind
            int bind_status = -1;
            if (m_ipv6)
            {
                bind_status = bind(m_fd
                                    , (struct sockaddr*)m_local_addr
                                    , sizeof(struct sockaddr_in6));
            }
            else
            {
                bind_status = bind(m_fd
                                    , (struct sockaddr*)m_local_addr
                                    , sizeof(struct sockaddr_in));
            }

            if (bind_status == -1)
            {
                if (m_ipv6)
                {
                    inc_stats (socketBindIpv6Fail);
                }
                else
                {
                    inc_stats (socketBindIpv4Fail);
                }
                set_error_state (STATE_TCP_SOCK_BIND_FAIL);
            }
            else
            {
                if (m_ipv6)
                {
                    inc_stats (socketBindIpv6);
                }
                else
                {
                    inc_stats (socketBindIpv4);
                }
                set_state (STATE_TCP_SOCK_BIND);

                //listen socket
                int listen_status = -1;
                listen_status = listen(m_fd, listenQLen);

                //check listen status
                if (listen_status < 0) {
                    set_error_state (STATE_TCP_SOCK_LISTEN_FAIL); 
                } else {
                    set_state (STATE_TCP_LISTENING);
                }
            }
        }
    }

    int ret_status = 0;
    if ( get_error_state () )
    {
        ret_status = -1;
        inc_stats (tcpListenStartFail);

        if (m_fd != -1){
            tcp_close ();
        }
    }
    else
    {
        set_status (CONNAPP_STATE_LISTEN);
        enable_rd_only_notification ();
    }

    return ret_status;
}

ev_socket* ev_socket::new_tcp_connect (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , ev_sockaddr* remoteAddress)
{
    ev_socket* new_sock = epoll_ctxp->m_app->alloc_socket ();

    if (new_sock) {
        new_sock->tcp_connect (epoll_ctxp, localAddress, remoteAddress);
    } else {

    }

    return new_sock;
}

ev_socket* ev_socket::new_tcp_listen (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , int lqlen)
{
    ev_socket* new_sock = epoll_ctxp->m_app->alloc_socket ();

    if (new_sock) {
        new_sock->tcp_listen (epoll_ctxp, localAddress, lqlen);
    } else {

    }
    
    return new_sock;
}


ev_socket* ev_socket::do_tcp_accept ()
{
    // ev_socket* new_ev_socket = new ev_socket ();
    return nullptr;
}

void ev_socket::do_ssl_connect(int isClient) 
{
    if (is_set_state (STATE_SSL_CONN_INIT) == 0) {
        set_state (STATE_SSL_CONN_INIT);
        inc_stats (sslConnInit);
        inc_stats (sslConnInitInSec);
        int status = SSL_set_fd(m_ssl, m_fd);

        if (status != 1) {
            set_error_state (STATE_SSL_SOCK_CONNECT_FAIL
                            | STATE_SSL_SOCK_FD_SET_ERROR);
        }
        if (isClient) {
            SSL_set_connect_state (m_ssl);
        } else {
            SSL_set_accept_state (m_ssl);
        }
    }

    if ( (is_set_state (STATE_SSL_CONN_ESTABLISHED)
            && is_set_error_state (STATE_SSL_SOCK_CONNECT_FAIL)) == 0 ) {

        int status = SSL_do_handshake(m_ssl);
        int sslErrno = SSL_get_error (m_ssl, status);
        if (status == 1) {
            set_state (STATE_SSL_CONN_ESTABLISHED);
            if (isClient) {
                inc_stats (sslConnInitSuccess);
                inc_stats (sslConnInitSuccessInSec);
            } else {
                inc_stats (sslAcceptSuccess);
                inc_stats (sslAcceptSuccessInSec);
            }
        } else if (status == -1) {
            if (is_set_state (STATE_SSL_CONN_IN_PROGRESS) == 0) {
                set_state (STATE_SSL_CONN_IN_PROGRESS);
                inc_stats (sslConnInitProgress);
            }
            switch (sslErrno) {
                case SSL_ERROR_WANT_READ:
                    set_state (STATE_SSL_HANDSHAKE_WANT_READ);
                    break;
                case SSL_ERROR_WANT_WRITE:
                    set_state (STATE_SSL_HANDSHAKE_WANT_WRITE);
                    break;
                default:
                    set_error_state_ssl (STATE_SSL_SOCK_CONNECT_FAIL, sslErrno);
                    inc_stats (sslConnInitFail);
                    break;  
            }  
        } else {
            set_error_state_ssl (STATE_SSL_SOCK_CONNECT_FAIL, sslErrno);
            inc_stats (sslConnInitFail);
        }               
    }
}
