#ifndef __TLS_SERVER_H__
#define __TLS_SERVER_H__

#include "app.hpp"

#define MAX_READ_BUFFER_LEN 100000
#define MAX_WRITE_BUFFER_LEN 100000

class tls_server_srv_grp : public ev_app_srv_grp 
{
public:
    tls_server_srv_grp (json jcfg
                        , std::vector<ev_sockstats*> *stats_arr) 
                    
                    : ev_app_srv_grp (jcfg, stats_arr) 
    {
        m_cs_data_len = jcfg["cs_data_len"].get<int>();
        m_sc_data_len = jcfg["sc_data_len"].get<int>();
        m_cs_start_tls_len = jcfg["cs_start_tls_len"].get<int>();
        m_sc_start_tls_len = jcfg["sc_start_tls_len"].get<int>();
        m_srv_cert = jcfg["srv_cert"].get<std::string>().c_str();
        m_srv_key = jcfg["srv_key"].get<std::string>().c_str();
        m_cipher = jcfg["cipher"].get<std::string>().c_str();

        const char* tls_version 
            = jcfg["tls_version"].get<std::string>().c_str();
        const char* close_type 
            = jcfg["close_type"].get<std::string>().c_str();
        const char* close_notify 
            = jcfg["close_notify"].get<std::string>().c_str();

        m_write_chunk = jcfg["write_chunk"].get<int>();

       
        if (strcmp(close_type, "fin") == 0) {
            m_close = close_fin;
        } else if (strcmp(close_type, "reset") == 0) {
            m_close = close_reset;
        }else {
            m_close = close_fin;
        }

        if (strcmp(close_notify, "send") == 0) {
            m_close_notify = close_notify_send;
        } else if (strcmp(close_notify, "send_recv") == 0) {
            m_close_notify = close_notify_send_recv;
        } else if (strcmp(close_notify, "no_send") == 0)  {
            m_close_notify = close_notify_no_send;
        } else {
            m_close_notify = close_notify_send_recv;
        }

        if (strcmp(tls_version, "sslv3") == 0){
            m_version = sslv3;
        } else if (strcmp(tls_version, "tls1") == 0){
            m_version = tls1;
        } else if (strcmp(tls_version, "tls1_1") == 0){
            m_version = tls1_1;
        } else if (strcmp(tls_version, "tls1_2") == 0){
            m_version = tls1_2;
        } else if (strcmp(tls_version, "tls1_3") == 0){
            m_version = tls1_3;
        } else if (strcmp(tls_version, "tls_all") == 0){
            m_version = tls_all;
        } else {
            m_version = tls_all;
        }
    }

    int m_cs_data_len;
    int m_sc_data_len;
    int m_cs_start_tls_len;
    int m_sc_start_tls_len;
    int m_write_chunk;

    std::string m_srv_cert;
    std::string m_srv_key;
    std::string m_cipher;
    std::string m_cipher2;
    enum_close_type m_close;
    enum_close_notify m_close_notify;
    enum_tls_version m_version;

    SSL_CTX* m_ssl_ctx;
};

struct tls_server_stats_data : app_stats
{
    uint64_t tls_server_stats_1;
    uint64_t tls_server_stats_100;

    virtual void dump_json (json &j)
    {
        app_stats::dump_json (j);
        
        j["tls_server_stats_1"] = tls_server_stats_1;
        j["tls_server_stats_100"] = tls_server_stats_100;
    }

    virtual ~tls_server_stats_data() {};
};

struct tls_server_stats : tls_server_stats_data
{
    tls_server_stats () : tls_server_stats_data () {}
};
class tls_server_app : public app
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
    int m_emulation_id;
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

#endif