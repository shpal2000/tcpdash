#ifndef __EV_APP__H
#define __EV_APP__H

#include "ev_socket.hpp"

#define MAX_APP_TYPE_NAME 1024

typedef std::map<std::string, ev_sockstats*> ev_stats_map;
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

    ev_sockstats* get_stats (const char* stats_label="")
    {
        return m_stats_map[stats_label];
    };

    void set_stats (ev_sockstats* stats, const char* stats_label="")
    {
        m_stats_map.insert(ev_stats_map::value_type(stats_label, stats));
    }

    const char* get_app_type () {return m_app_type;};
    void set_app_type (const char* app_type) {strcpy (m_app_type, app_type);};

private:
    epoll_ctx* m_epoll_ctx;
    char m_app_type[MAX_APP_TYPE_NAME];
    ev_stats_map m_stats_map;
};

#endif