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

    virtual void start() = 0;
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

    ev_sockstats* get_app_stats (const char* stats_label="")
    {
        return m_stats_map[stats_label];
    };

    void set_app_stats (ev_sockstats* stats, const char* stats_label="")
    {
        m_stats_map.insert(ev_stats_map::value_type(stats_label, stats));
    }

    const char* get_app_type () {return m_app_type;};
    void set_app_type (const char* app_type) {strcpy (m_app_type, app_type);};

    json get_app_json () {return m_app_json;}
    void set_app_json (json app_json)  {m_app_json = app_json;};

    ev_sockstats* get_zone_app_stats () {return m_zone_app_stats;}
    void set_zone_app_stats (ev_sockstats* app_stats) 
    {
        m_zone_app_stats = app_stats;
    };

    ev_sockstats* get_zone_sock_stats () {return m_zone_sock_stats;}
    void set_zone_sock_stats (ev_sockstats* sock_stats) 
    {
        m_zone_sock_stats = sock_stats;
    };

private:
    epoll_ctx* m_epoll_ctx;
    ev_stats_map m_stats_map;
    char m_app_type[MAX_APP_TYPE_NAME];
    json m_app_json;
    ev_sockstats* m_zone_app_stats;
    ev_sockstats* m_zone_sock_stats;
};

#endif