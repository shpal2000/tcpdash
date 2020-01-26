#include "tls_client.hpp"

tls_client_app::tls_client_app(json app_json
                    , tls_client_stats* zone_app_stats
                    , ev_sockstats* zone_sock_stats)
{
    m_app_stats = new tls_client_stats();
    m_client_cps = app_json["conn_per_sec"].get<uint32_t>();
    m_client_total_conn_count = app_json["total_conn_count"].get<uint64_t>();
    m_client_curr_conn_count = 0;

    auto cs_grp_list = app_json["cs_grp_list"];
    m_cs_group_count = 0;
    m_cs_group_index = 0;
    for (auto it = cs_grp_list.begin(); it != cs_grp_list.end(); ++it)
    {
        auto cs_grp_cfg = it.value ();

        const char* cs_grp_label 
            = cs_grp_cfg["cs_grp_label"].get<std::string>().c_str();

        tls_client_stats* cs_grp_stats = new tls_client_stats();
        set_app_stats (cs_grp_stats, cs_grp_label);

        std::vector<ev_sockstats*> *cs_grp_stats_arr 
            = new std::vector<ev_sockstats*> ();

        cs_grp_stats_arr->push_back (cs_grp_stats);
        cs_grp_stats_arr->push_back (m_app_stats);
        cs_grp_stats_arr->push_back (zone_app_stats);
        cs_grp_stats_arr->push_back (zone_sock_stats);

        tls_client_cs_grp* next_cs_grp 
            = new tls_client_cs_grp (cs_grp_cfg["srv_ip"].get<std::string>().c_str()
                        , cs_grp_cfg["srv_port"].get<u_short>()
                        , cs_grp_cfg["clnt_ip_begin"].get<std::string>().c_str()
                        , cs_grp_cfg["clnt_ip_end"].get<std::string>().c_str()
                        , cs_grp_cfg["clnt_port_begin"].get<u_short>()
                        , cs_grp_cfg["clnt_port_end"].get<u_short>()
                        , cs_grp_stats_arr
                        , cs_grp_cfg["cs_data_len"].get<int>()
                        , cs_grp_cfg["sc_data_len"].get<int>()
                        , cs_grp_cfg["cs_start_tls_len"].get<int>()
                        , cs_grp_cfg["sc_start_tls_len"].get<int>());

        next_cs_grp->m_ssl_ctx = SSL_CTX_new(SSLv23_client_method());

        SSL_CTX_set_verify(next_cs_grp->m_ssl_ctx, SSL_VERIFY_NONE, 0);

        SSL_CTX_set_options(next_cs_grp->m_ssl_ctx
                                                , SSL_OP_NO_SSLv2 
                                                | SSL_OP_NO_SSLv3 
                                                | SSL_OP_NO_COMPRESSION);

        SSL_CTX_set_mode(next_cs_grp->m_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

        SSL_CTX_set_session_cache_mode(next_cs_grp->m_ssl_ctx
                                                , SSL_SESS_CACHE_OFF); 

        m_cs_groups.push_back (next_cs_grp);

        m_cs_group_count++;
    }
}

tls_client_app::~tls_client_app()
{

}

void tls_client_app::run_iter(bool tick_sec)
{
    if (m_cs_group_count == 0){
        return;
    }

    ev_app::run_iter (tick_sec);

    if (tick_sec)
    {
        m_app_stats->tick_sec();
    }

    for (int i=0; i < get_new_conn_count(); i++)
    {
        tls_client_cs_grp* cs_grp = m_cs_groups [m_cs_group_index];

        m_cs_group_index++;
        if (m_cs_group_index == m_cs_group_count)
        {
            m_cs_group_index = 0;
        }

        ev_sockaddrx* client_addr = cs_grp->get_next_clnt_addr ();
        if (client_addr)
        {
            tls_client_socket* client_socket = (tls_client_socket*) 
                                    new_tcp_connect (&client_addr->m_addr
                                                , cs_grp->get_server_addr()
                                                , cs_grp->m_stats_arr
                                                , client_addr->m_portq);
            if (client_socket) 
            {
                m_client_curr_conn_count++;
                client_socket->m_cs_grp = cs_grp;
            }
            else
            {
                printf ("new_tcp_connect fail!\n");
            }
        }
        else
        {
            printf ("get_next_clnt_addr fail!\n");
        }
    }
}

ev_socket* tls_client_app::alloc_socket()
{
    return new tls_client_socket();
}

void tls_client_app::on_establish (ev_socket* ev_sock)
{
    // printf ("on_establish\n");
    tls_client_socket* app_sock = (tls_client_socket*) ev_sock;

    app_sock->m_ssl = SSL_new (app_sock->m_cs_grp->m_ssl_ctx);

    if (app_sock->m_ssl){
        app_sock->set_as_ssl_client (app_sock->m_ssl);
    } else {
        //stats
        app_sock->abort ();
    }
}

void tls_client_app::on_write (ev_socket* ev_sock)
{
    // printf ("on_write\n");
    tls_client_socket* app_sock = (tls_client_socket*) ev_sock;

    if (app_sock->m_bytes_written < app_sock->m_cs_grp->m_cs_data_len) {
        int next_chunk 
            = app_sock->m_cs_grp->m_cs_data_len - app_sock->m_bytes_written;

        if ( next_chunk > 1200){
            next_chunk = 1200;
        }

        app_sock->write_next_data (m_write_buffer, 0, next_chunk, true);
    } else {
        app_sock->disable_wr_notification ();
    }
}

void tls_client_app::on_wstatus (ev_socket* /*ev_sock*/
                            , int /*bytes_written*/
                            , int /*write_status*/)
{
    // printf ("on_wstatus\n");
    tls_client_socket* app_sock = (tls_client_socket*) ev_sock;

    if (write_status == WRITE_STATUS_NORMAL) {
        app_sock->m_bytes_written += bytes_written;
        if (app_sock->m_bytes_written == app_sock->m_cs_grp->m_cs_data_len) {
            // app_sock->write_close ();
            app_sock->abort ();
        }
    } else {
        app_sock->abort ();
    }
}

void tls_client_app::on_read (ev_socket* ev_sock)
{
    // printf ("on_read\n");
    tls_client_socket* app_sock = (tls_client_socket*) ev_sock;

    if (app_sock->m_bytes_read < app_sock->m_cs_grp->m_sc_data_len) {
        app_sock->read_next_data (m_read_buffer, 0, MAX_READ_BUFFER_LEN, true);
    }
}

void tls_client_app::on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status)
{
    // printf ("on_rstatus\n");
    tls_client_socket* app_sock = (tls_client_socket*) ev_sock;

    if (bytes_read == 0) 
    {
        if (read_status != READ_STATUS_TCP_CLOSE) {
            app_sock->abort ();
        }
    } else {
        app_sock->m_bytes_read += bytes_read;
    }
}

void tls_client_app::on_free (ev_socket* ev_sock)
{
    // printf ("on_free\n");
    delete ev_sock;
}
