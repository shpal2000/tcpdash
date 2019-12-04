#include "ev_socket.hpp"

class ev_app
{
public:
    ev_app(/* args */);
    ~ev_app();


    virtual int run_iter() = 0;
    
    virtual ev_socket* alloc_socket() = 0;
    virtual void free_socket(ev_socket* ev_sock) = 0;

    virtual void on_establish (ev_socket* ev_sock) = 0;
    virtual void on_write (ev_socket* ev_sock) = 0;
    virtual void on_wstatus (ev_socket* ev_sock) = 0;
    virtual void on_read (ev_socket* ev_sock) = 0;
    virtual void on_rstatus (ev_socket* ev_sock) = 0;
    virtual void on_finish (ev_socket* ev_sock) = 0;


    ev_socket* tcp_connect ()
    {
        return ev_socket::tcp_connect (m_epoll_ctx);
    }

    ev_socket* tcp_accept ()
    {
        return ev_socket::tcp_accept (m_epoll_ctx);
    }

    ev_socket* tcp_listen ()
    {
        return ev_socket::tcp_listen (m_epoll_ctx);
    }

private:
    epoll_ctx* m_epoll_ctx;
};

