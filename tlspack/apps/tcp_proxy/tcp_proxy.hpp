#ifndef __TCP_PROXY__H
#define __TCP_PROXY__H

#include "app.hpp"

struct tp_stats_data : app_stats
{
    uint64_t tp_stats_1;
    uint64_t tp_stats_100;

    virtual void dump_json (json &j)
    {
        app_stats::dump_json (j);
        
        j["tp_stats_1"] = tp_stats_1;
        j["tp_stats_100"] = tp_stats_100;
    }

    virtual ~tp_stats_data() {};
};

struct tp_stats : tp_stats_data
{
    tp_stats () : tp_stats_data () {}
};

class tp_app : public app
{
public:
    tp_app(json app_json, tp_stats* app_stats);
    ~tp_app();

    void run_iter(bool tick_sec);
    
    ev_socket* alloc_socket();
    void free_socket(ev_socket* ev_sock);

private:
    ev_sockaddr m_listen_addr;
    tp_stats* m_app_stats;
public:
    std::vector<ev_sockstats*> *m_app_stats_arr;
    ev_socket_opt m_sock_opt;
    int m_proxy_type;
};

class tp_session;
class tp_socket : public ev_socket
{
public:
    tp_socket()
    {
        m_app = nullptr;
        m_session = nullptr;
    };

    virtual ~tp_socket()
    {

    };
    
    void on_establish ();
    void on_write ();
    void on_wstatus (int bytes_written, int write_status);
    void on_read ();
    void on_rstatus (int bytes_read, int read_status);
    void on_finish ();

public:
    tp_app* m_app;
    tp_session* m_session;
};

class tp_session
{
public:

    tp_session()
    {
        m_server_sock = nullptr;
        m_client_sock = nullptr;
        m_session_established = false;

        m_client_current_wbuff = nullptr;
        m_server_current_wbuff = nullptr;

        m_client_current_rbuff = nullptr;
        m_server_current_rbuff = nullptr;
    }

    ~tp_session()
    {

    }
    
public:
    tp_socket* m_server_sock;
    tp_socket* m_client_sock;
    bool m_session_established;

    ev_buff* m_client_current_wbuff;
    ev_buff* m_server_current_wbuff;

    ev_buff* m_client_current_rbuff;
    ev_buff* m_server_current_rbuff;
    
    std::queue<ev_buff*> m_client_rbuffs;
    std::queue<ev_buff*> m_server_rbuffs;
};

#endif