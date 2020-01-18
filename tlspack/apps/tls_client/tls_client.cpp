#include "tls_client.hpp"

tls_client_app::tls_client_app(json cfg_json
                    , int c_index
                    , int a_index
                    , tls_client_stats* all_app_stats
                    , ev_sockstats* all_ev_app_stats)
{
    auto cs_grp_list = cfg_json["client"]
                                ["containers"]
                                [c_index]
                                ["apps"]
                                [a_index]
                                ["cs_grp_list"];

    set_app_type ("tls_client");
    tls_client_stats* app_stats = new tls_client_stats();
    set_stats (app_stats);

    for (auto it = cs_grp_list.begin(); it != cs_grp_list.end(); ++it)
    {
        auto cs_grp_cfg = it.value ();

        const char* cs_grp_stats_label 
            = cs_grp_cfg["stats_label"].get<std::string>().c_str();

        tls_client_stats* cs_grp_stats = new tls_client_stats();
        set_stats (cs_grp_stats, cs_grp_stats_label);

        std::vector<ev_sockstats*> *cs_grp_stats_arr 
            = new std::vector<ev_sockstats*> ();

        cs_grp_stats_arr->push_back (cs_grp_stats);
        cs_grp_stats_arr->push_back (app_stats);
        cs_grp_stats_arr->push_back (all_app_stats);
        cs_grp_stats_arr->push_back (all_ev_app_stats);

        ev_sockaddr serverAddr;
        ev_socket::set_sockaddr (&serverAddr
                            , cs_grp_cfg["srv_ip"].get<std::string>().c_str()
                            , cs_grp_cfg["srv_port"].get<int>());

        ev_sockaddr clientAddr;
        ev_socket::set_sockaddr (&clientAddr
                        , cs_grp_cfg["clnt_ip_begin"].get<std::string>().c_str()
                        , cs_grp_cfg["clnt_port_begin"].get<int>());

        tls_client_socket* client_socket
            = (tls_client_socket*) new_tcp_connect (&clientAddr
                                                    , &serverAddr
                                                    , cs_grp_stats_arr);
        if (client_socket) 
        {
        }
        else
        {
            //todo error handling
        }
    }
}

tls_client_app::~tls_client_app()
{
}

void tls_client_app::run_iter()
{
    ev_app::run_iter ();

    //todo
}

ev_socket* tls_client_app::alloc_socket()
{
    return new tls_client_socket();
}

void tls_client_app::on_establish (ev_socket* ev_sock)
{
    printf ("on_establish\n");

    ev_sock->get_ssl ();
}

void tls_client_app::on_write (ev_socket* ev_sock)
{
    printf ("on_write\n");

    ev_sock->disable_wr_notification ();
}

void tls_client_app::on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status)
{
    printf ("on_wstatus\n");
}

void tls_client_app::on_read (ev_socket* ev_sock)
{
    printf ("on_read\n");

    ev_sock->read_next_data (m_read_buffer, 0, MAX_READ_BUFFER_LEN, true);

}

void tls_client_app::on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status)
{
    printf ("on_rstatus\n");

    if (bytes_read == 0) 
    {
        if (read_status == READ_STATUS_TCP_CLOSE) 
        {
            ev_sock->write_close();
        }
        else
        {
            ev_sock->abort ();
        }
    }
}

void tls_client_app::on_free (ev_socket* ev_sock)
{
    printf ("on_free\n");
    
    delete ev_sock;
}


//////////////////////////////tls_client_socket////////////////////////////////////
tls_client_socket::tls_client_socket(/* args */)
{
}

tls_client_socket::~tls_client_socket()
{
}
