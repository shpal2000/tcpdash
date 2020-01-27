#include "ev_app.hpp"

#define MAX_READ_BUFFER_LEN 100000
#define MAX_WRITE_BUFFER_LEN 100000

class tls_client_cs_grp : public ev_app_cs_grp 
{
public:
    tls_client_cs_grp (const char* srv_ip
                    , u_short srv_port
                    , const char* ip_begin
                    , const char* ip_end
                    , u_short port_begin
                    , u_short port_end
                    , std::vector<ev_sockstats*> *stats_arr
                    , int cs_data_len
                    , int sc_data_len
                    , int cs_start_tls_len
                    , int sc_start_tls_len) 
                    
                    : ev_app_cs_grp (srv_ip
                        , srv_port
                        , ip_begin
                        , ip_end
                        , port_begin
                        , port_end
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

struct tls_client_stats_data : ev_sockstats
{
    uint64_t tls_client_stats_1;
    uint64_t tls_client_stats_100;

    virtual void dump_json (json &j)
    {
        ev_sockstats::dump_json (j);
        j["tls_client_stats_1"] = tls_client_stats_1;
        j["tls_client_stats_100"] = tls_client_stats_100;
    }

    virtual ~tls_client_stats_data() {};
};

struct tls_client_stats : tls_client_stats_data
{
    tls_client_stats () : tls_client_stats_data () {}
};

class tls_client_socket : public ev_socket
{
public:
    tls_client_socket()
    {
        m_bytes_written = 0;
        m_bytes_read = 0;
        m_ssl = nullptr;
    };

    ~tls_client_socket()
    {
        if (m_ssl) {
            SSL_free (m_ssl);
            m_ssl = nullptr;
        }
    };

    void on_establish ();
    void on_write ();
    void on_wstatus (int bytes_written, int write_status);
    void on_read ();
    void on_rstatus (int bytes_read, int read_status);
    void on_free ();

public:
    tls_client_cs_grp* m_cs_grp;
    SSL* m_ssl;
    int m_bytes_written;
    int m_bytes_read;

};

class tls_client_app : public ev_app
{
public:
    tls_client_app(json app_json
                    , tls_client_stats* all_app_stats
                    , ev_sockstats* all_ev_app_stats);

    ~tls_client_app();

    void run_iter(bool tick_sec);
    
    ev_socket* alloc_socket();
    void free_socket(ev_socket* ev_sock);

private:
    char m_read_buffer[MAX_READ_BUFFER_LEN];
    char m_write_buffer[MAX_WRITE_BUFFER_LEN];
    std::vector<tls_client_cs_grp*> m_cs_groups;
    int m_cs_group_index;
    int m_cs_group_count;
};
