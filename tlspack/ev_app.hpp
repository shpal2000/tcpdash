#ifndef __EV_APP__H
#define __EV_APP__H

#include "ev_socket.hpp"

#define MAX_APP_TYPE_NAME 1024

typedef std::map<std::string, ev_sockstats*> ev_stats_map;

class ev_app_cs_grp
{
public:

    std::vector<ev_sockaddrx*> m_clnt_addr_pool;
    int m_clnt_addr_index;
    int m_clnt_addr_count;

    ev_sockaddr m_srvr_addr;
    std::vector<ev_sockstats*> *m_stats_arr;

    ev_app_cs_grp (const char* srv_ip
                    , u_short srv_port
                    , const char* ip_begin
                    , const char* ip_end
                    , u_short port_begin
                    , u_short port_end
                    , std::vector<ev_sockstats*> *stats_arr)
    {
        m_clnt_addr_index = 0;
        m_clnt_addr_count = 0;

        char next_ip[128];
        strcpy (next_ip, ip_begin);
        ev_sockaddrx* next_addr = new ev_sockaddrx (port_begin, port_end);
        ev_socket::set_sockaddr (&next_addr->m_addr, next_ip, 0);
        m_clnt_addr_pool.push_back(next_addr);
        m_clnt_addr_count++;

        while (strcmp(next_ip, ip_end))
        {
            next_addr = new ev_sockaddrx (port_begin, port_end);
            ev_socket::get_nextip_str (next_ip, 1, next_ip);
            ev_socket::set_sockaddr (&next_addr->m_addr, next_ip, 0);
            m_clnt_addr_pool.push_back(next_addr);
            m_clnt_addr_count++;
        }

        ev_socket::set_sockaddr (&m_srvr_addr, srv_ip, htons(srv_port));

        m_stats_arr = stats_arr;
    }

    ev_sockaddr* get_server_addr () {return &m_srvr_addr;};
    ev_sockaddrx* get_next_clnt_addr () 
    {
        ev_sockaddrx* ret = nullptr;
        if (m_clnt_addr_count > 0)
        {
            ret = m_clnt_addr_pool[m_clnt_addr_index];
            m_clnt_addr_index++;
            if (m_clnt_addr_index == m_clnt_addr_count)
            {
                m_clnt_addr_index = 0;
            }
            u_short port = ret->m_portq->get_port();
            if (port)
            {
                ev_socket::set_sockaddr_port(&ret->m_addr, port);
            }
            else
            {
                ret = nullptr;
            }
        }
        return ret;
    };
};

class ev_app_srv_grp
{
public:

    ev_sockaddr m_srvr_addr;
    std::vector<ev_sockstats*> *m_stats_arr;

    ev_app_srv_grp (const char* srv_ip
                    , u_short srv_port
                    , std::vector<ev_sockstats*> *stats_arr)
    {
        ev_socket::set_sockaddr (&m_srvr_addr, srv_ip, htons(srv_port));
        m_stats_arr = stats_arr;   
    }

};

class ev_app
{
public:
    ev_app();
    virtual ~ev_app();

    virtual void run_iter(bool tick_sec);

    virtual ev_socket* alloc_socket() = 0;
    virtual void free_socket(ev_socket* ev_sock) = 0;

    ev_socket* new_tcp_connect (ev_sockaddr* laddr
                                , ev_sockaddr* raddr
                                , std::vector<ev_sockstats*>* statsArr
                                , ev_portq* portq)
    {
        return ev_socket::new_tcp_connect (m_epoll_ctx
                                            , laddr
                                            , raddr
                                            , statsArr
                                            , portq);
    }

    ev_socket* new_tcp_listen (ev_sockaddr* laddr
                                , int lqlen
                                , std::vector<ev_sockstats*>* statsArr)
    {
        return ev_socket::new_tcp_listen (m_epoll_ctx, laddr, lqlen, statsArr);
    }

    ev_stats_map* get_app_stats_map ()
    {
        return &m_stats_map;
    } 
    
    ev_sockstats* get_app_stats (const char* stats_label="")
    {
        if (strcmp(stats_label, "") == 0)
            return m_app_stats;

        return m_stats_map[stats_label];
    };

    void set_app_stats (ev_sockstats* stats, const char* stats_label)
    {
        m_stats_map.insert(ev_stats_map::value_type(stats_label, stats));
    }

    const char* get_app_type () {return m_app_type;};
    void set_app_type (const char* app_type) {strcpy (m_app_type, app_type);};

    const char* get_app_label () {return m_app_label;};
    void set_app_label(const char* app_label) {strcpy (m_app_label,app_label);};

    int get_new_conn_count ()
    {
        int n = 0;

        if (m_client_curr_conn_count == 0){
            // m_last_new_conn_time = std::chrono::steady_clock::now ();
            m_conn_init_time = std::chrono::steady_clock::now ();
            n = 1;
        } else if ((m_client_total_conn_count == 0) 
                    || (m_client_curr_conn_count < m_client_total_conn_count)) {
            // auto t = std::chrono::steady_clock::now();
            // auto span = std::chrono::duration_cast<std::chrono::microseconds>
            //                                 (t - m_last_new_conn_time).count();
            // n = (m_client_cps * span) / 1000000;
            // if (n) {
            //     m_last_new_conn_time = std::chrono::steady_clock::now ();
            // }

            auto t = std::chrono::steady_clock::now();
            auto span = std::chrono::duration_cast<std::chrono::microseconds>
                                            (t - m_conn_init_time).count();
            uint64_t c = (m_client_cps * span) / 1000000;
            if (c > m_client_curr_conn_count) {
                n = c - m_client_curr_conn_count;
            }
        }

        return n;
    }

private:
    epoll_ctx* m_epoll_ctx;
    ev_stats_map m_stats_map;
    char m_app_type[MAX_APP_TYPE_NAME];
    char m_app_label[MAX_APP_TYPE_NAME];
    std::chrono::steady_clock::time_point m_last_new_conn_time;
    std::chrono::steady_clock::time_point m_conn_init_time;
    int m_last_new_conn_count;

protected:
    uint32_t m_client_cps;
    uint64_t m_client_curr_conn_count;
    uint64_t m_client_total_conn_count;
    ev_sockstats* m_app_stats;
};

#endif