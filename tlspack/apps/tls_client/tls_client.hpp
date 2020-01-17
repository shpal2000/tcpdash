#include "ev_app.hpp"

#define MAX_READ_BUFFER_LEN 100000

struct tls_client_stats_data : ev_sockstats
{
    uint64_t tls_client_stats_1;
    uint64_t tls_client_stats_100;
};

struct tls_client_stats : tls_client_stats_data
{
    tls_client_stats () : tls_client_stats_data () {}
};

class tls_client_socket : public ev_socket
{
private:
    /* data */
public:
    tls_client_socket(/* args */);
    ~tls_client_socket();
};

class tls_client_app : public ev_app
{
public:
    tls_client_app(json cfg_json
                    , int c_index
                    , int a_index
                    , tls_client_stats* all_app_stats
                    , ev_sockstats* all_ev_app_stats);

    ~tls_client_app();

    void run_iter();
    ev_socket* alloc_socket();
    void on_establish (ev_socket* ev_sock);
    void on_write (ev_socket* ev_sock);
    void on_wstatus (ev_socket* ev_sock, int bytes_written, int write_status);
    void on_read (ev_socket* ev_sock);
    void on_rstatus (ev_socket* ev_sock, int bytes_read, int read_status);
    void on_free (ev_socket* ev_sock);

private:
    char m_read_buffer[MAX_READ_BUFFER_LEN];
    tls_client_stats m_stats;
    ev_stats_map m_stats_map;
};
