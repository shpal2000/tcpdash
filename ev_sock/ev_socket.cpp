#include "ev_socket.hpp"
#include "ev_app.hpp"

void ev_socket::init ()
{
    m_portq = nullptr;
    m_epoll_ctx = nullptr;
    m_fd = -1;
    m_saved_lport = 0;
    m_saved_rport = 0;

    m_state = 0;
    m_error_state = 0;

    m_sys_errno = 0;
    m_socket_errno = 0;

    m_ssl = nullptr;
    m_ssl_client = true;
    m_ssl_errno = 0;

    m_status = CONNAPP_STATE_INIT;
    
    m_sockstats_arr = nullptr;

    m_ipv6 = false;
    m_parent = nullptr;
    m_next = nullptr;
    m_prev = nullptr;
}

ev_socket::ev_socket()
{
    init ();
}

ev_socket::~ev_socket()
{
    if (m_portq) {
        m_portq->return_port (get_sockaddr_port (&m_local_addr));
    }
}

//////////////////////////////// platform functions///////////////////////////////
epoll_ctx* ev_socket::epoll_alloc(ev_app* app_ptr
                                    , int max_events
                                    , int epoll_timeout)
{
    return new epoll_ctx(app_ptr, max_events, epoll_timeout);
}

void ev_socket::epoll_free(epoll_ctx* epoll_ctxp)
{
    delete epoll_ctxp;
}

ev_socket* ev_socket::new_tcp_connect (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , ev_sockaddr* remoteAddress
                                        , std::vector<ev_sockstats*>* statsArr
                                        , ev_portq* portq
                                        , ev_socket_opt* ev_sock_opt)
{
    ev_socket* new_sock = epoll_ctxp->m_app->alloc_socket ();

    if (new_sock) {
        new_sock->init ();
        new_sock->set_sockstats_arr (statsArr);
        new_sock->set_socket_opt (ev_sock_opt);
        epoll_ctxp->m_app->add_to_active_list (new_sock);
        new_sock->tcp_connect (epoll_ctxp
                                , localAddress
                                , remoteAddress);
        if (portq) {
            new_sock->set_portq (portq);
        }
    } else {

    }

    return new_sock;
}

ev_socket* ev_socket::new_tcp_listen (epoll_ctx* epoll_ctxp
                                        , ev_sockaddr* localAddress
                                        , int lqlen
                                        , std::vector<ev_sockstats*>* statsArr
                                        , ev_socket_opt* ev_sock_opt)
{
    ev_socket* new_sock = epoll_ctxp->m_app->alloc_socket ();

    if (new_sock) {
        new_sock->set_sockstats_arr (statsArr);
        new_sock->set_socket_opt (ev_sock_opt);
        epoll_ctxp->m_app->add_to_active_list (new_sock);
        new_sock->tcp_listen (epoll_ctxp, localAddress, lqlen);
    } else {

    }
    
    return new_sock;
}

void ev_socket::enable_rd_only_notification () 
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 
            || is_set_state (STATE_TCP_POLL_WRITE_CURRENT) ) {

        struct epoll_event setEvent;
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

        struct epoll_event setEvent;
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

        struct epoll_event setEvent;
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

void ev_socket::disable_wr_notification ()
{
    if ( is_set_state (STATE_TCP_POLL_WRITE_CURRENT) ) {

        if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 ) {

            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_DEL, m_fd, NULL);

        } else {

            struct epoll_event setEvent;
            setEvent.events = 0;
            setEvent.data.ptr = this;
            setEvent.events = EPOLLIN;

            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        clear_state (STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void ev_socket::disable_rd_notification ()
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) ) {

        if ( is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_DEL, m_fd, NULL);

        } else {

            struct epoll_event setEvent;
            setEvent.events = 0;
            setEvent.data.ptr = this;
            setEvent.events = EPOLLOUT;

            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        clear_state (STATE_TCP_POLL_READ_CURRENT);
    }
}

void ev_socket::enable_rd_notification () 
{
    if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0 ) {

        struct epoll_event setEvent;
        setEvent.events = 0;
        setEvent.data.ptr = this;

        if ( is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0) 
        {
            setEvent.events = EPOLLIN;
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_ADD, m_fd, &setEvent);
        } 
        else 
        {
            setEvent.events = EPOLLIN | EPOLLOUT;
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        set_state (STATE_TCP_POLL_READ_CURRENT);
    }
}

void ev_socket::enable_wr_notification () 
{
    if ( is_set_state (STATE_TCP_POLL_WRITE_CURRENT) == 0 ) {

        struct epoll_event setEvent;
        setEvent.events = 0;
        setEvent.data.ptr = this;

        if ( is_set_state (STATE_TCP_POLL_READ_CURRENT) == 0) 
        {
            setEvent.events = EPOLLOUT;
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_ADD, m_fd, &setEvent);
        } 
        else 
        {
            setEvent.events = EPOLLIN | EPOLLOUT;
            epoll_ctl(m_epoll_ctx->m_epoll_id, EPOLL_CTL_MOD, m_fd, &setEvent);
        }

        set_state (STATE_TCP_POLL_WRITE_CURRENT);
    }
}

void ev_socket::read_next_data (char* readBuffer
                                , int readBuffOffset
                                , int readDataLen
                                , bool partialRead) 
{
    set_state (STATE_CONN_READ_PENDING);

    m_read_buffer = readBuffer;
    m_read_buff_offset = readBuffOffset;
    m_read_data_len = readDataLen;

    m_read_buff_offset_cur = readBuffOffset;
    m_read_data_len_cur = readDataLen;
    m_read_bytes_len_cur = 0;

    m_read_status = READ_STATUS_NORMAL;

    if (partialRead) {
        set_state (STATE_CONN_PARTIAL_READ);
    } else {
        clear_state (STATE_CONN_PARTIAL_READ);
    }    
}

void ev_socket::write_next_data (char* writeBuffer
                                , int writeBuffOffset
                                , int writeDataLen
                                , bool partialWrite)
{
    set_state (STATE_CONN_WRITE_PENDING);

    m_write_buffer = writeBuffer;
    m_write_buff_offset = writeBuffOffset;
    m_write_data_len = writeDataLen;

    m_write_buff_offset_cur = writeBuffOffset;
    m_write_data_len_cur = writeDataLen;
    m_write_bytes_len_cur = 0;

    m_write_status = WRITE_STATUS_NORMAL;

    if (partialWrite) {
        set_state (STATE_CONN_PARTIAL_WRITE);
    } else {
        clear_state (STATE_CONN_PARTIAL_WRITE);
    }
}

void ev_socket::write_close (int send_close_notify) {
    
    enable_wr_notification ();

    if ( get_error_state() ) 
    {
        abort ();
    } 
    else 
    {
        set_state (STATE_NO_MORE_WRITE_DATA | STATE_TCP_TO_SEND_FIN);

        if (send_close_notify == SSL_SEND_CLOSE_NOTIFY) {
            set_state (STATE_SSL_TO_SEND_SHUTDOWN);
        } else if (send_close_notify == SSL_SEND_RECEIVE_CLOSE_NOTIFY) {
            set_state (STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);
        }
    }    
}

void ev_socket::abort ()
{
    if ( is_set_state (STATE_CONN_MARK_DELETE) == 0 ) {
        set_state (STATE_CONN_MARK_DELETE | STATE_TCP_TO_SEND_RST);
        m_epoll_ctx->m_abort_list.push (this);
    }
}

int ev_socket::tcp_connect (epoll_ctx* epoll_ctxp
                            , ev_sockaddr* localAddress
                            , ev_sockaddr* remoteAddress)
{
    //socket stats
    inc_stats (tcpConnInit);
    inc_stats (tcpConnInitInUse);
    inc_stats (tcpConnInitInSec);

    //socket contexts
    m_epoll_ctx = epoll_ctxp;
    std::memcpy (&m_local_addr, localAddress, sizeof (ev_sockaddr));
    std::memcpy (&m_remote_addr, remoteAddress, sizeof (ev_sockaddr));

    //socket if ipv6
    struct sockaddr* uaddr = (struct sockaddr*) (localAddress);
    m_ipv6 = (uaddr->sa_family == AF_INET6);

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

        int tmp_op;
        
        tmp_op = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &tmp_op, sizeof(int));
        
        tmp_op = 3;
        setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, &tmp_op, sizeof(int));

        tmp_op = 5;
        setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, &tmp_op, sizeof(int));

        tmp_op = 1;
        setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, &tmp_op, sizeof(int));

        tmp_op = 5000;
        setsockopt(m_fd, SOL_TCP, TCP_USER_TIMEOUT, &tmp_op, sizeof(int));

        int so_status = 0;
        if (m_sock_opt->rcv_buff_len) {
            so_status = setsockopt(m_fd
                                    , SOL_SOCKET
                                    , SO_RCVBUFFORCE
                                    , &m_sock_opt->rcv_buff_len
                                    , sizeof(int));
            if (so_status == -1) {
                inc_stats (socketRcvBufForceFail);
                set_error_state (STATE_TCP_RCVBUFFORCE_FAIL);
            }
        }

        if (m_sock_opt->snd_buff_len) {
            so_status = setsockopt(m_fd
                            , SOL_SOCKET
                            , SO_SNDBUFFORCE
                            , &m_sock_opt->snd_buff_len
                            , sizeof(int));

            if (so_status == -1) {
                inc_stats (socketSndBufForceFail);
                set_error_state (STATE_TCP_SNDBUFFORCE_FAIL);
            }
        }

        int so_op = 1;
        int so_reuse_status = setsockopt(m_fd
                                        , SOL_SOCKET
                                        , SO_REUSEADDR
                                        , &so_op
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
        so_op = 1;
        int so_iptrans_status = setsockopt(m_fd
                                        , SOL_IP
                                        , IP_TRANSPARENT
                                        , &so_op
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
                                    , (struct sockaddr*)&m_local_addr
                                    , sizeof(struct sockaddr_in6));
            }
            else
            {
                bind_status = bind(m_fd
                                    , (struct sockaddr*)&m_local_addr
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
                                            , (struct sockaddr*)&m_remote_addr
                                            , sizeof(struct sockaddr_in6));
                }
                else
                {
                    connect_status = connect(m_fd
                                            , (struct sockaddr*)&m_remote_addr
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
        dec_stats (tcpConnInitInUse);

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
    std::memcpy (&m_local_addr, localAddress, sizeof (ev_sockaddr));

    //socket if ipv6 
    struct sockaddr* uaddr = (struct sockaddr*) (localAddress);
    m_ipv6 = (uaddr->sa_family == AF_INET6);

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

        int so_status = 0;
        if (m_sock_opt->rcv_buff_len) {
            so_status = setsockopt(m_fd
                                    , SOL_SOCKET
                                    , SO_RCVBUFFORCE
                                    , &m_sock_opt->rcv_buff_len
                                    , sizeof(int));
            if (so_status == -1) {
                inc_stats (socketRcvBufForceFail);
                set_error_state (STATE_TCP_RCVBUFFORCE_FAIL);
            }
        }

        if (m_sock_opt->snd_buff_len) {
            so_status = setsockopt(m_fd
                            , SOL_SOCKET
                            , SO_SNDBUFFORCE
                            , &m_sock_opt->snd_buff_len
                            , sizeof(int));

            if (so_status == -1) {
                inc_stats (socketSndBufForceFail);
                set_error_state (STATE_TCP_SNDBUFFORCE_FAIL);
            }
        }

        int so_op = 1;
        int so_reuse_status = setsockopt(m_fd
                                        , SOL_SOCKET
                                        , SO_REUSEADDR
                                        , &so_op
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
        so_op = 1;
        int so_iptrans_status = setsockopt(m_fd
                                        , SOL_IP
                                        , IP_TRANSPARENT
                                        , &so_op
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
                                    , (struct sockaddr*)&m_local_addr
                                    , sizeof(struct sockaddr_in6));
            }
            else
            {
                bind_status = bind(m_fd
                                    , (struct sockaddr*)&m_local_addr
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
        inc_stats (tcpActiveConns);
    }else {
        set_error_state (STATE_TCP_SOCK_CONNECT_FAIL);
        set_socket_errno (socketErr);
        inc_stats (tcpConnInitFail);
        dec_stats (tcpConnInitInUse);
    }
}

void ev_socket::tcp_close (int isLinger, int lingerTime) {

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

    if (is_set_state (STATE_TCP_CONN_ESTABLISHED)) {
        dec_stats (tcpActiveConns);
        if (is_set_state (STATE_TCP_CONN_INIT)) {
            dec_stats (tcpConnInitInUse);
        }
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

void ev_socket::tcp_accept (ev_socket* ev_sock_parent)
{
    set_state (STATE_TCP_CONN_ACCEPT);
    m_epoll_ctx = ev_sock_parent->m_epoll_ctx;
    m_parent = ev_sock_parent;

    socklen_t addrLen = sizeof (ev_sockaddr);
    m_fd = accept4(ev_sock_parent->m_fd
                    , (struct sockaddr *) &m_remote_addr
                    , &addrLen
                    , SOCK_NONBLOCK);
    
    if (m_fd < 0) {
        inc_stats (tcpAcceptFail);
        set_error_state (STATE_TCP_CONN_ACCEPT_FAIL);        
    } else {
        set_state (STATE_TCP_CONN_ESTABLISHED);
        inc_stats (tcpAcceptSuccess);
        inc_stats (tcpAcceptSuccessInSec);
        inc_stats (tcpActiveConns);

        //??? is it necessary
        // int recv_size = 1024 * 128;
        // setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(int));

        int tmp_op;

        tmp_op = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &tmp_op, sizeof(int));
        
        tmp_op = 3;
        setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, &tmp_op, sizeof(int));

        tmp_op = 5;
        setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, &tmp_op, sizeof(int));

        tmp_op = 1;
        setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, &tmp_op, sizeof(int));

        tmp_op = 5000;
        setsockopt(m_fd, SOL_TCP, TCP_USER_TIMEOUT, &tmp_op, sizeof(int));

        int so_op = 1;
        int ret = setsockopt(m_fd, SOL_SOCKET
                        , SO_REUSEADDR, &so_op, sizeof(int));
        if (ret < 0) {
            inc_stats (socketReuseSetFail);
            set_error_state (STATE_TCP_SOCK_REUSE_FAIL);
        } else {
            inc_stats (socketReuseSet);
            set_state (STATE_TCP_SOCK_REUSE);
        }

        socklen_t addr_len = sizeof (ev_sockaddr);
        ret = getsockname(m_fd, (struct sockaddr*) &m_local_addr, &addr_len);
        if (ret < 0){
            inc_stats (tcpGetSockNameFail);
            set_error_state (STATE_TCP_GETSOCKNAME_FAIL);
        }
    }

    if ( get_error_state () ){
        if (m_fd != -1){
            tcp_close ();
        }
    }  
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
            case SSL_ERROR_ZERO_RETURN:
                set_error_state (STATE_TCP_REMOTE_CLOSED_ERROR);
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
                // case SSL_ERROR_SYSCALL:
                // case SSL_ERROR_ZERO_RETURN:
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


void ev_socket::close_socket ()
{
    if ( is_fd_closed () == false ) {

        int isLinger = 0;
        int lingerTime = 0;

        if ( get_error_state() || is_set_state (STATE_TCP_TO_SEND_RST) ) {
            isLinger = 1;
        }

        disable_rd_wr_notification ();     

        tcp_close (isLinger, lingerTime);

        if ( is_set_state (STATE_CONN_MARK_FINISH) == 0) {
            m_epoll_ctx->m_finish_list.push (this);
            set_state (STATE_CONN_MARK_FINISH);
        }
    }
}

void ev_socket::tcp_connection_success ()
{
    set_status (CONNAPP_STATE_CONNECTION_ESTABLISHED);
    on_establish ();
}

void ev_socket::tcp_connection_fail ()
{
    set_status(CONNAPP_STATE_CONNECTION_ESTABLISH_FAILED);
    do_close_connection ();
}

void ev_socket::handle_tcp_accept ()
{
    ev_socket* ev_sock_ptr = m_epoll_ctx->m_app->alloc_socket ();

    if (ev_sock_ptr == nullptr) {
        inc_stats (tcpConnStructNotAvail);
    } else {
        ev_sock_ptr->init ();
        ev_sock_ptr->set_sockstats_arr ( get_sockstats_arr() );
        ev_sock_ptr->set_socket_opt ( get_socket_opt() );
        m_epoll_ctx->m_app->add_to_active_list (ev_sock_ptr);
        ev_sock_ptr->tcp_accept (this);

        if ( ev_sock_ptr->get_error_state() ) 
        {
            ev_sock_ptr->tcp_connection_fail ();
        } 
        else 
        {
            ev_sock_ptr->enable_rd_wr_notification ();
            ev_sock_ptr->tcp_connection_success ();
        }
    }
}

void ev_socket::handle_tcp_connect_complete ()
{
    tcp_verify_established ();

    if ( get_error_state() )
    {
        tcp_connection_fail ();
    }
    else
    {
        tcp_connection_success ();
    }
}

void ev_socket::do_ssl_handshake() 
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
        if (m_ssl_client) {
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
            set_status (CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED);
            if (m_ssl_client) {
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
                    set_status (CONNAPP_STATE_SSL_CONNECTION_ESTABLISH_FAILED);
                    inc_stats (sslConnInitFail);
                    do_close_connection ();
                    break;  
            }  
        } else {
            set_error_state_ssl (STATE_SSL_SOCK_CONNECT_FAIL, sslErrno);
            set_status (CONNAPP_STATE_SSL_CONNECTION_ESTABLISH_FAILED);
            inc_stats (sslConnInitFail);
            do_close_connection ();
        }               
    }
}


void ev_socket::do_close_connection ()
{
    if ( get_error_state() || is_set_state(STATE_TCP_TO_SEND_RST) )
    {
        close_socket ();
    }
    else
    {
        bool pendingSendReceiveCloseNotify 
            = is_set_state (STATE_SSL_TO_SEND_SHUTDOWN) 
            || is_set_state (STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN);

        if ( is_set_state (STATE_SSL_CONN_ESTABLISHED)
                && pendingSendReceiveCloseNotify ) {

            ssl_shutdown ();
        }

        bool sentCloseNotifyOrNotRequired 
            =   (is_set_state (STATE_SSL_ENABLED_CONN) == 0) 
                || is_set_state (STATE_SSL_SENT_SHUTDOWN) 
                || ( (is_set_state (STATE_SSL_TO_SEND_SHUTDOWN) == 0)
                    && (is_set_state (STATE_SSL_TO_SEND_RECEIVE_SHUTDOWN)==0) );


        bool wrShutdownDone 
            = is_set_state (STATE_TCP_SENT_FIN)
                || is_set_error_state (STATE_TCP_FIN_SEND_FAIL); 

        if ( is_set_state (STATE_TCP_CONN_ESTABLISHED)
                && (wrShutdownDone == false)
                && sentCloseNotifyOrNotRequired ) {

            tcp_write_shutdown ();

            disable_wr_notification ();
        }

        if ( get_error_state () 
            || ( is_set_state (STATE_TCP_REMOTE_CLOSED)  &&  wrShutdownDone) ) {

            close_socket ();
        }        
    }
}

void ev_socket::do_read_next_data ()
{
    int bytesReceived;
    if ( is_set_state (STATE_SSL_ENABLED_CONN) ) {
        bytesReceived  
            = ssl_read ( m_read_buffer + m_read_buff_offset_cur
                        , m_read_data_len_cur);
    } else {
        bytesReceived  
            = tcp_read ( m_read_buffer + m_read_buff_offset_cur
                        , m_read_data_len_cur);
    }

    bool notifyReadStatus = false;

    if ( get_error_state() ) 
    {
        notifyReadStatus = true;
        switch ( get_sys_errno() ) 
        {
            case ETIMEDOUT:
                m_read_status = READ_STATUS_TCP_TIMEOUT; 
                break;
            case ECONNRESET:
                m_read_status = READ_STATUS_TCP_RESET;
                break;
            default:
                m_read_status  = READ_STATUS_ERROR;
                break;
        }
        do_close_connection ();
    } 
    else
    {
        if (bytesReceived <= 0) {
            if ( is_set_state (STATE_TCP_REMOTE_CLOSED) ) 
            {
                notifyReadStatus = true;
                m_read_status = READ_STATUS_TCP_CLOSE;
                disable_rd_notification ();
            } 
            else
            {
                // ssl want read write; skip;
            }
        }
        else 
        {
            //process read data
            m_read_bytes_len_cur += bytesReceived;
            m_read_buff_offset_cur += bytesReceived;
            m_read_data_len_cur -= bytesReceived;

            if ( is_set_state (STATE_CONN_PARTIAL_READ) 
                || (m_read_bytes_len_cur == m_read_data_len) )
            {
                notifyReadStatus = true;
            }
        }
    }

    if (notifyReadStatus) 
    {
        clear_state (STATE_CONN_READ_PENDING);
        on_rstatus (m_read_bytes_len_cur, m_read_status);
    }

}

void ev_socket::do_write_next_data ()
{
    int bytesSent;
    if ( is_set_state (STATE_SSL_ENABLED_CONN) ) { 
        bytesSent = ssl_write ( m_write_buffer + m_write_buff_offset_cur
                                , m_write_data_len_cur );
    } else {
        bytesSent = tcp_write ( m_write_buffer + m_write_buff_offset_cur
                                , m_write_data_len_cur );
    }

    bool notifyWriteStatus = false;
    if ( get_error_state() )
    {
        notifyWriteStatus = true;
        switch ( get_sys_errno() ) 
        {
            case ETIMEDOUT:
                m_write_status = WRITE_STATUS_TCP_TIMEOUT; 
                break;
            case ECONNRESET:
                m_write_status = WRITE_STATUS_TCP_RESET;
                break;
            default:
                m_write_status  = WRITE_STATUS_ERROR;
                break;
        }
        do_close_connection ();
    }
    else
    {
        if (bytesSent <= 0)
        {
            // ssl want read write; skip
        }
        else
        {
            //process written data
            m_write_bytes_len_cur += bytesSent;
            m_write_buff_offset_cur += bytesSent;
            m_write_data_len_cur -= bytesSent;

            if ( is_set_state (STATE_CONN_PARTIAL_WRITE) 
                || (m_write_bytes_len_cur == m_write_data_len) )
            {
                notifyWriteStatus = true;
            }
        }
    }

    if (notifyWriteStatus) 
    {
        clear_state (STATE_CONN_WRITE_PENDING);
        on_wstatus (m_write_bytes_len_cur, m_write_status);
    }

}

//////////////////////////event processing///////////////////////////////////
void ev_socket::epoll_process (epoll_ctx* epoll_ctxp)
{
    int event_count = epoll_wait (epoll_ctxp->m_epoll_id
                        , epoll_ctxp->m_epoll_event_arr
                        , epoll_ctxp->m_max_epoll_events
                        , epoll_ctxp->m_epoll_timeout);
    
    if (event_count > 0)
    {
        for (int eindex = 0; eindex < event_count; eindex++)
        {
            epoll_event* eventp = &(epoll_ctxp->m_epoll_event_arr[eindex]);
            ev_socket* sockp = (ev_socket*) eventp->data.ptr;
            uint32_t events = eventp->events;
            //handle tcp accept
            if ( sockp->is_set_state (STATE_TCP_LISTENING) )
            {
                sockp->handle_tcp_accept ();
            } 
            else
            {
                //handle tcp connect
                if ( (sockp->get_status() 
                            == CONNAPP_STATE_CONNECTION_IN_PROGRESS)
                            && (events & EPOLLOUT) )
                {
                    sockp->handle_tcp_connect_complete ();
                }
                //handle ssl handshake
                else if ( sockp->get_status() 
                            == CONNAPP_STATE_SSL_CONNECTION_IN_PROGRESS )
                {
                    bool doSslHandshake = false;

                    if ( sockp->is_set_state(STATE_SSL_HANDSHAKE_WANT_WRITE)
                        && (events & EPOLLOUT) ) 
                    {
                        doSslHandshake = true;
                        sockp->clear_state (STATE_SSL_HANDSHAKE_WANT_WRITE);
                    }

                    if ( sockp->is_set_state(STATE_SSL_HANDSHAKE_WANT_READ)
                        && (events & EPOLLIN) ) 
                    {
                        doSslHandshake = true;
                        sockp->clear_state (STATE_SSL_HANDSHAKE_WANT_READ);
                    }

                    if (doSslHandshake) {
                        sockp->do_ssl_handshake ();
                    }
                }
                //handle read, write both ssl and non-ssl tcp
                else if ( (sockp->get_status()
                            == CONNAPP_STATE_SSL_CONNECTION_ESTABLISHED) ||
                            ( (sockp->get_status()
                            == CONNAPP_STATE_CONNECTION_ESTABLISHED) && 
                            (sockp->is_set_state(STATE_SSL_ENABLED_CONN)== 0)) )
                {
                    //handle read
                    if ( (events & EPOLLIN) 
                            && (sockp->is_fd_closed() == false) )
                    {
                        if (sockp->is_set_state(STATE_TCP_REMOTE_CLOSED) == 0)
                        {
                            if (sockp->is_set_state(STATE_CONN_READ_PENDING) 
                                == 0)
                            {
                                sockp->on_read();
                            }

                            if (sockp->is_set_state(STATE_CONN_READ_PENDING)
                                && (sockp->is_fd_closed() == false))
                            {
                                sockp->do_read_next_data ();
                            }
                        }
                    }
                    //handle write
                    if ( (events & EPOLLOUT) 
                            && (sockp->is_fd_closed() == false) )
                    {
                        if (sockp->is_set_state(STATE_CONN_WRITE_PENDING) 
                            == 0)
                        {
                            if (sockp->is_set_state(STATE_NO_MORE_WRITE_DATA))
                            {
                                sockp->do_close_connection ();
                            }
                            else
                            {
                                sockp->on_write();
                            }
                        }

                        if (sockp->is_set_state(STATE_CONN_WRITE_PENDING)
                            && (sockp->is_fd_closed() == false))
                        {
                            sockp->do_write_next_data ();
                        }
                    }

                    if ( (sockp->is_fd_closed() == false)
                            && sockp->is_set_state(STATE_TCP_REMOTE_CLOSED)
                            && (sockp->is_set_state(STATE_TCP_SENT_FIN)
                                ||(sockp->is_set_error_state(STATE_TCP_FIN_SEND_FAIL))) )
                    {
                        sockp->do_close_connection ();
                    }
                }
            }
        }

        //abort process
        if ( epoll_ctxp->m_abort_list.empty() == false )
        {
            ev_socket* ev_sock_ptr = epoll_ctxp->m_abort_list.front();

            ev_sock_ptr->do_close_connection ();

            epoll_ctxp->m_abort_list.pop();
        }

        //free up process
        if ( epoll_ctxp->m_finish_list.empty() == false )
        {
            ev_socket* ev_sock_ptr = epoll_ctxp->m_finish_list.front();
            ev_sock_ptr->on_finish ();
            epoll_ctxp->m_app->remove_from_active_list (ev_sock_ptr);
            epoll_ctxp->m_app->free_socket (ev_sock_ptr);
            epoll_ctxp->m_finish_list.pop();
        }

    }
}