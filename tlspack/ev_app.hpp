#include "ev_socket.hpp"

class ev_app
{
public:
    ev_app(/* args */);
    ~ev_app();


    virtual void run_iter();
    
    virtual ev_socket* alloc_socket() = 0;
    virtual void on_establish (ev_socket* ev_sock) = 0;
    virtual void on_write (ev_socket* ev_sock) = 0;
    virtual void on_wstatus (ev_socket* ev_sock, int bytes_written) = 0;
    virtual void on_read (ev_socket* ev_sock) = 0;
    virtual void on_rstatus (ev_socket* ev_sock
                                , int bytes_read
                                , bool read_close
                                , int read_close_status) = 0;
    virtual void on_free (ev_socket* ev_sock) = 0;


    ev_socket* new_tcp_connect (ev_sockaddr* laddr, ev_sockaddr* raddr)
    {
        return ev_socket::new_tcp_connect (m_epoll_ctx, laddr, raddr);
    }

    ev_socket* new_tcp_listen (ev_sockaddr* laddr, int lqlen)
    {
        return ev_socket::new_tcp_listen (m_epoll_ctx, laddr, lqlen);
    }

private:
    epoll_ctx* m_epoll_ctx;
};

