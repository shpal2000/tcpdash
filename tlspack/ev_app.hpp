#ifndef __EV_APP__H
#define __EV_APP__H

#include "ev_socket.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

typedef std::map<const char*, ev_sockstats*> ev_stats_map;
class ev_app
{
public:
    ev_app();
    virtual ~ev_app();


    virtual void run_iter();
    
    virtual ev_socket* alloc_socket() = 0;
    virtual void on_establish (ev_socket* ev_sock) = 0;
    virtual void on_write (ev_socket* ev_sock) = 0;
    virtual void on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status) = 0;
    virtual void on_read (ev_socket* ev_sock) = 0;
    virtual void on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status) = 0;
    virtual void on_free (ev_socket* ev_sock) = 0;


    ev_socket* new_tcp_connect (ev_sockaddr* laddr
                                , ev_sockaddr* raddr
                                , std::vector<ev_sockstats*>* statsArr)
    {
        return ev_socket::new_tcp_connect (m_epoll_ctx, laddr, raddr, statsArr);
    }

    ev_socket* new_tcp_listen (ev_sockaddr* laddr
                                , int lqlen
                                , std::vector<ev_sockstats*>* statsArr)
    {
        return ev_socket::new_tcp_listen (m_epoll_ctx, laddr, lqlen, statsArr);
    }

private:
    epoll_ctx* m_epoll_ctx;
};

#endif