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
            srv_socket->m_app = this;
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

void tls_server_app::free_socket(ev_socket* ev_sock)
{
    delete ev_sock;
}

void tls_server_socket::on_establish ()
{
    // printf ("on_establish\n");
    m_lsock = (tls_server_socket*) get_parent();
    m_app = m_lsock->m_app;
    m_srv_grp = m_lsock->m_srv_grp;

    m_ssl = SSL_new (m_srv_grp->m_ssl_ctx);

    if (m_ssl){
        set_as_ssl_server (m_ssl);
    } else {
        //stats
        abort ();
    }
}

void tls_server_socket::on_write ()
{
    // printf ("on_write\n");
    if (m_bytes_written < m_srv_grp->m_sc_data_len) {
        int next_chunk 
            = m_srv_grp->m_sc_data_len - m_bytes_written;

        if ( next_chunk > 1200){
            next_chunk = 1200;
        }

        write_next_data (m_app->m_write_buffer, 0, next_chunk, true);
    } else {
        disable_wr_notification ();
    }
}

void tls_server_socket::on_wstatus (int bytes_written, int write_status)
{
    // printf ("on_wstatus\n");
    if (write_status == WRITE_STATUS_NORMAL) {
        m_bytes_written += bytes_written;
        if (m_bytes_written == m_srv_grp->m_sc_data_len) {
            // abort ();
            write_close ();
        }
    } else {
        abort ();
    }
}

void tls_server_socket::on_read ()
{
    // printf ("on_read\n");
    read_next_data (m_app->m_read_buffer, 0, MAX_READ_BUFFER_LEN, true);
}

void tls_server_socket::on_rstatus (int bytes_read, int read_status)
{
    // printf ("on_rstatus\n");
    if (bytes_read == 0) 
    {
        if (read_status != READ_STATUS_TCP_CLOSE) {
            abort ();
        }
    } else {
        m_bytes_read += bytes_read;
    }
}

void tls_server_socket::on_finish ()
{
    // printf ("on_free\n");
    if (m_ssl) {
        SSL_free (m_ssl);
        m_ssl = nullptr;
    }
}
