#include "tls_server.hpp"


tls_server_app::tls_server_app(json app_json
                                , tls_server_stats* zone_app_stats
                                , ev_sockstats* zone_sock_stats)
{
    m_app_stats = new tls_server_stats();


    m_sock_opt.rcv_buff_len = app_json["tcp_rcv_buff"].get<uint32_t>();
    m_sock_opt.snd_buff_len = app_json["tcp_snd_buff"].get<uint32_t>();

    auto srv_list = app_json["srv_list"];
    for (auto it = srv_list.begin(); it != srv_list.end(); ++it)
    {
        auto srv_cfg = it.value ();

        const char* srv_label 
            = srv_cfg["srv_label"].get<std::string>().c_str();
        
        int srv_enable = srv_cfg["enable"].get<int>();
        if (srv_enable == 0) {
            continue;
        }

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
                        , srv_cfg["sc_start_tls_len"].get<int>()
                        , srv_cfg["srv_cert"].get<std::string>().c_str()
                        , srv_cfg["srv_key"].get<std::string>().c_str()
                        , srv_cfg["cipher"].get<std::string>().c_str()
                        , srv_cfg["tls_version"].get<std::string>().c_str()
                        , srv_cfg["close_type"].get<std::string>().c_str()
                        , srv_cfg["close_notify"].get<std::string>().c_str());

        next_srv_grp->m_ssl_ctx = SSL_CTX_new(TLS_server_method());

        int status;
        if (next_srv_grp->m_version == sslv3) {
            status = SSL_CTX_set_min_proto_version (next_srv_grp->m_ssl_ctx, SSL3_VERSION);
            status = SSL_CTX_set_max_proto_version (next_srv_grp->m_ssl_ctx, SSL3_VERSION);
        } else if (next_srv_grp->m_version == tls1) {
            status = SSL_CTX_set_min_proto_version (next_srv_grp->m_ssl_ctx, TLS1_VERSION);
            status = SSL_CTX_set_max_proto_version (next_srv_grp->m_ssl_ctx, TLS1_VERSION);
        } else if (next_srv_grp->m_version == tls1_1) {
            status = SSL_CTX_set_min_proto_version (next_srv_grp->m_ssl_ctx, TLS1_1_VERSION);
            status = SSL_CTX_set_max_proto_version (next_srv_grp->m_ssl_ctx, TLS1_1_VERSION);
        } else if (next_srv_grp->m_version == tls1_2) {
            status = SSL_CTX_set_min_proto_version (next_srv_grp->m_ssl_ctx, TLS1_2_VERSION);
            status = SSL_CTX_set_max_proto_version (next_srv_grp->m_ssl_ctx, TLS1_2_VERSION);
        } else if (next_srv_grp->m_version == tls1_3) {
            status = SSL_CTX_set_min_proto_version (next_srv_grp->m_ssl_ctx, TLS1_3_VERSION);
            SSL_CTX_set_max_proto_version (next_srv_grp->m_ssl_ctx, TLS1_3_VERSION);
        } else {
            status = SSL_CTX_set_min_proto_version (next_srv_grp->m_ssl_ctx
                                                    , SSL3_VERSION);
            status = SSL_CTX_set_max_proto_version (next_srv_grp->m_ssl_ctx
                                                    , TLS1_3_VERSION);       
        }

        if (next_srv_grp->m_version == tls_all) {
            next_srv_grp->m_cipher2 = srv_cfg["cipher2"].get<std::string>().c_str();
            SSL_CTX_set_ciphersuites (next_srv_grp->m_ssl_ctx
                                        , next_srv_grp->m_cipher2.c_str());
            SSL_CTX_set_cipher_list (next_srv_grp->m_ssl_ctx
                                        , next_srv_grp->m_cipher.c_str());
        } else if (next_srv_grp->m_version == tls1_3) {
            SSL_CTX_set_ciphersuites (next_srv_grp->m_ssl_ctx
                                        , next_srv_grp->m_cipher.c_str());
        } else {
            SSL_CTX_set_cipher_list (next_srv_grp->m_ssl_ctx
                                        , next_srv_grp->m_cipher.c_str());
        }

        SSL_CTX_set_mode(next_srv_grp->m_ssl_ctx
                                , SSL_MODE_ENABLE_PARTIAL_WRITE);

        SSL_CTX_set_session_cache_mode(next_srv_grp->m_ssl_ctx
                                                , SSL_SESS_CACHE_OFF);

        status = SSL_CTX_set1_groups_list(next_srv_grp->m_ssl_ctx
                                            , "P-521:P-384:P-256");

        SSL_CTX_set_dh_auto(next_srv_grp->m_ssl_ctx, 1);

        std::ifstream f(next_srv_grp->m_srv_cert);
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

        std::ifstream f2(next_srv_grp->m_srv_key);
        std::ostringstream ss2;
        std::string str2;
        ss2 << f2.rdbuf();
        str2 = ss2.str();
        kbio = BIO_new_mem_buf(str2.c_str(), -1);
        EVP_PKEY *key = NULL;
        key = PEM_read_bio_PrivateKey(kbio, NULL, 0, NULL);
        SSL_CTX_use_PrivateKey(next_srv_grp->m_ssl_ctx, key);

        BIO_free(bio);
        BIO_free(kbio);
        EVP_PKEY_free(key);
        X509_free(cert);

        m_srv_groups.push_back (next_srv_grp);
    }

    for (auto srv_grp : m_srv_groups)
    {
        tls_server_socket* srv_socket 
            = (tls_server_socket*) new_tcp_listen (&srv_grp->m_srvr_addr
                                                    , 10000
                                                    , srv_grp->m_stats_arr
                                                    , &m_sock_opt);
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

        if ( next_chunk > 100000){
            next_chunk = 100000 ;
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
            if (m_srv_grp->m_close == close_reset){
                abort ();
            } else {
                switch (m_srv_grp->m_close_notify)
                {
                    case close_notify_no_send:
                        write_close ();
                        break;
                    case close_notify_send:
                        write_close (SSL_SEND_CLOSE_NOTIFY);
                        break;
                    case close_notify_send_recv:
                        write_close (SSL_SEND_RECEIVE_CLOSE_NOTIFY);
                        break;
                }
            }
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
