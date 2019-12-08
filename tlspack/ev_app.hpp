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


    ev_socket* tcp_new_client (ev_sockaddr* laddr, ev_sockaddr* raddr)
    {
        return ev_socket::tcp_new_client (m_epoll_ctx, laddr, raddr);
    }

    ev_socket* tcp_new_server (ev_sockaddr* laddr, int lqlen)
    {
        return ev_socket::tcp_new_server (m_epoll_ctx, laddr, lqlen);
    }

private:
    epoll_ctx* m_epoll_ctx;
};

