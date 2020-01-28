#include "ev_app.hpp"

#define MAX_READ_BUFFER_LEN 100000
#define MAX_WRITE_BUFFER_LEN 100000

class tls_server_srv_grp : public ev_app_srv_grp 
{
public:
    tls_server_srv_grp (const char* srv_ip
                    , u_short srv_port
                    , std::vector<ev_sockstats*> *stats_arr
                    , int cs_data_len
                    , int sc_data_len
                    , int cs_start_tls_len
                    , int sc_start_tls_len) 
                    
                    : ev_app_srv_grp (srv_ip
                        , srv_port
                        , stats_arr) 
    {
        m_cs_data_len = cs_data_len;
        m_sc_data_len = sc_data_len;
        m_cs_start_tls_len = cs_start_tls_len;
        m_sc_start_tls_len = sc_start_tls_len;
    }

    int m_cs_data_len;
    int m_sc_data_len;
    int m_cs_start_tls_len;
    int m_sc_start_tls_len;

    SSL_CTX* m_ssl_ctx;
};

struct tls_server_stats_data : ev_sockstats
{
    uint64_t tls_server_stats_1;
    uint64_t tls_server_stats_100;

    virtual void dump_json (json &j)
    {
        ev_sockstats::dump_json (j);
        j["tls_server_stats_1"] = tls_server_stats_1;
        j["tls_server_stats_100"] = tls_server_stats_100;
    }

    virtual ~tls_server_stats_data() {};
};

struct tls_server_stats : tls_server_stats_data
{
    tls_server_stats () : tls_server_stats_data () {}
};
class tls_server_app : public ev_app
{
public:
    tls_server_app(json app_json
                    , tls_server_stats* all_app_stats
                    , ev_sockstats* all_ev_app_stats);

    ~tls_server_app();

    void run_iter(bool tick_sec);

    ev_socket* alloc_socket();
    void free_socket(ev_socket* ev_sock);

public:
    char m_read_buffer[MAX_READ_BUFFER_LEN];
    char m_write_buffer[MAX_WRITE_BUFFER_LEN];
    std::vector<tls_server_srv_grp*> m_srv_groups;
};

class tls_server_socket : public ev_socket
{
public:
    tls_server_socket()
    {
        m_bytes_written = 0;
        m_bytes_read = 0;
        m_ssl = nullptr;    
    };

    virtual ~tls_server_socket()
    {

    };

    void on_establish ();
    void on_write ();
    void on_wstatus (int bytes_written, int write_status);
    void on_read ();
    void on_rstatus (int bytes_read, int read_status);
    void on_finish ();

public:
    tls_server_srv_grp* m_srv_grp;
    tls_server_app* m_app;
    tls_server_socket* m_lsock;
    SSL* m_ssl;
    int m_bytes_written;
    int m_bytes_read;
};

#define inc_tls_server_stats(__stat_name) \
{ \
    inc_app_stats(ev_sock,tls_server_stats,__stat_name); \
}
