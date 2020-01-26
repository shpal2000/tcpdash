#include "tls_server.hpp"


tls_server_app::tls_server_app(json app_json
                                , tls_server_stats* zone_app_stats
                                , ev_sockstats* zone_sock_stats)
{
    m_app_stats = new tls_server_stats();

    auto srv_list = app_json["srv_list"];
    for (auto it = srv_list.begin(); it != srv_list.end(); ++it)
    {
        auto srv_cfg = it.value ();

        const char* srv_label 
            = srv_cfg["srv_label"].get<std::string>().c_str();

        tls_server_stats* srv_stats = new tls_server_stats();
        set_app_stats (srv_stats, srv_label);

        std::vector<ev_sockstats*> *srv_stats_arr
            = new std::vector<ev_sockstats*> ();

        srv_stats_arr->push_back (srv_stats);
        srv_stats_arr->push_back (m_app_stats);
        srv_stats_arr->push_back (zone_app_stats);
        srv_stats_arr->push_back (zone_sock_stats);


        tls_server_srv_grp* next_srv_grp
            = new tls_server_srv_grp (srv_cfg["srv_ip"].get<std::string>().c_str()
                        , srv_cfg["srv_port"].get<u_short>()
                        , srv_stats_arr
                        , srv_cfg["cs_data_len"].get<int>()
                        , srv_cfg["sc_data_len"].get<int>()
                        , srv_cfg["cs_start_tls_len"].get<int>()
                        , srv_cfg["sc_start_tls_len"].get<int>());

        next_srv_grp->m_ssl_ctx = SSL_CTX_new(SSLv23_server_method());

        SSL_CTX_set_verify(next_srv_grp->m_ssl_ctx, SSL_VERIFY_NONE, 0);

        SSL_CTX_set_options(next_srv_grp->m_ssl_ctx
                                                , SSL_OP_NO_SSLv2 
                                                | SSL_OP_NO_SSLv3 
                                                | SSL_OP_NO_COMPRESSION);

        SSL_CTX_set_mode(next_srv_grp->m_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

        SSL_CTX_set_session_cache_mode(next_srv_grp->m_ssl_ctx
                                                , SSL_SESS_CACHE_OFF);

        std::ifstream f("/rundir/certs/server.cert");
        std::ostringstream ss;
        std::string str;
        ss << f.rdbuf();
        str = ss.str();

        BIO *bio = NULL;
        BIO *kbio = NULL;
        X509 *cert = NULL;
        bio = BIO_new_mem_buf((char *)str.c_str(), -1);
        cert = PEM_read_bio_X509(bio, NULL, 0, NULL);

        SSL_CTX_use_certificate (next_srv_grp->m_ssl_ctx, cert);

        std::ifstream f2("/rundir/certs/server.key");
        std::ostringstream ss2;
        std::string str2;
        ss2 << f2.rdbuf();
        str2 = ss2.str();

        kbio = BIO_new_mem_buf(str2.c_str(), -1);

        RSA *rsa = NULL;
        rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);

        SSL_CTX_use_RSAPrivateKey(next_srv_grp->m_ssl_ctx, rsa);

        BIO_free(bio);
        BIO_free(kbio);
        RSA_free(rsa);
        X509_free(cert);
        
        // SSL_CTX_use_certificate_file(next_srv_grp->m_ssl_ctx
        //                     , "/rundir/certs/server.cert"
        //                     , SSL_FILETYPE_PEM);

        // SSL_CTX_use_PrivateKey_file(next_srv_grp->m_ssl_ctx
        //                     , "/rundir/certs/server.key"
        //                     , SSL_FILETYPE_PEM);

        m_srv_groups.push_back (next_srv_grp);
    }

    for (auto srv_grp : m_srv_groups)
    {
        tls_server_socket* srv_socket 
            = (tls_server_socket*) new_tcp_listen (&srv_grp->m_srvr_addr
                                                    , 10000
                                                    , srv_grp->m_stats_arr);
        if (srv_socket) 
        {
            srv_socket->m_srv_grp = srv_grp;
        }
        else
        {
            //todo error handling
        }
    }
}

tls_server_app::~tls_server_app()
{
}

void tls_server_app::run_iter(bool tick_sec)
{
    ev_app::run_iter (tick_sec);

    if (tick_sec)
    {
        m_app_stats->tick_sec();
    }
}

ev_socket* tls_server_app::alloc_socket()
{
    return new tls_server_socket();
}

void tls_server_app::on_establish (ev_socket* ev_sock)
{
    // printf ("on_establish\n");
    tls_server_socket* app_sock = (tls_server_socket*) ev_sock;
    app_sock->m_srv_grp = ((tls_server_socket*) ev_sock->get_parent())->m_srv_grp;

    app_sock->m_ssl = SSL_new (app_sock->m_srv_grp->m_ssl_ctx);

    if (app_sock->m_ssl){
        app_sock->set_as_ssl_server (app_sock->m_ssl);
    } else {
        //stats
        app_sock->abort ();
    }
}

void tls_server_app::on_write (ev_socket* ev_sock)
{
    // printf ("on_write\n");
    tls_server_socket* app_sock = (tls_server_socket*) ev_sock;

    if (app_sock->m_bytes_written < app_sock->m_srv_grp->m_cs_data_len) {
        int next_chunk 
            = app_sock->m_srv_grp->m_cs_data_len - app_sock->m_bytes_written;

        if ( next_chunk > 1200){
            next_chunk = 1200;
        }

        app_sock->write_next_data (m_write_buffer, 0, next_chunk, true);
    } else {
        app_sock->disable_wr_notification ();
    }
}

void tls_server_app::on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status)
{
    // printf ("on_wstatus\n");
    tls_server_socket* app_sock = (tls_server_socket*) ev_sock;

    if (write_status == WRITE_STATUS_NORMAL) {
        app_sock->m_bytes_written += bytes_written;
        if (app_sock->m_bytes_written == app_sock->m_srv_grp->m_cs_data_len) {
            app_sock->abort ();
            // app_sock->write_close ();
        }
    } else {
        app_sock->abort ();
    }
}

void tls_server_app::on_read (ev_socket* ev_sock)
{
    // printf ("on_read\n");
    tls_server_socket* app_sock = (tls_server_socket*) ev_sock;

    if (app_sock->m_bytes_read < app_sock->m_srv_grp->m_sc_data_len) {
        app_sock->read_next_data (m_read_buffer, 0, MAX_READ_BUFFER_LEN, true);
    }
}

void tls_server_app::on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status)
{
    // printf ("on_rstatus\n");
    tls_server_socket* app_sock = (tls_server_socket*) ev_sock;

    if (bytes_read == 0) 
    {
        if (read_status != READ_STATUS_TCP_CLOSE) {
            app_sock->abort ();
        }
    } else {
        app_sock->m_bytes_read += bytes_read;
    }
}

void tls_server_app::on_free (ev_socket* ev_sock)
{
    // printf ("on_free\n");
    delete ev_sock;
}
