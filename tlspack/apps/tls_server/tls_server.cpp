#include "tls_server.hpp"

tls_server_app::tls_server_app(json cfg_json, int c_index, int a_index)
{
    const char* app_stats_label = cfg_json["server"]
                                ["containers"]
                                [c_index]
                                ["apps"]
                                [a_index]
                                ["stats_label"].get<std::string>().c_str();

    tls_server_stats* app_stats = new tls_server_stats();
    m_stats_map.insert (app_stats_label, app_stats);

    auto srv_list =
        cfg_json["server"]["containers"][c_index]["apps"][a_index]["srv_list"];

    for (auto it = srv_list.begin(); it != srv_list.end(); ++it)
    {
        auto srv_cfg = it.value ();

        const char* srv_stats_label = srv_cfg["stats_label"]
                                    .get<std::string>()
                                    .c_str();

        tls_server_stats* srv_stats = new tls_server_stats();
        m_stats_map.insert ( srv_stats_label, srv_stats );

        std::vector<ev_sockstats*> *srv_stats_arr 
                        = new std::vector<ev_sockstats*> ();

        srv_stats_arr->push_back (app_stats);
        srv_stats_arr->push_back (srv_stats);

        ev_sockaddr serverAddr;
        ev_socket::set_sockaddr (&serverAddr
                                , srv_cfg["srv_ip"].get<std::string>().c_str()
                                , srv_cfg["srv_port"].get<int>());
    
        new_tcp_listen (&serverAddr, 100, srv_stats_arr);
    }
}

tls_server_app::~tls_server_app()
{
}

void tls_server_app::run_iter()
{
    ev_app::run_iter ();

    //todo
}

ev_socket* tls_server_app::alloc_socket()
{
    return new tls_server_socket();
}

void tls_server_app::on_establish (ev_socket* ev_sock)
{
    printf ("on_establish\n");
    
    ev_sock->disable_wr_notification ();
}

void tls_server_app::on_write (ev_socket* ev_sock)
{
    printf ("on_write\n");

    ev_sock->get_ssl ();
}

void tls_server_app::on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status)
{
    printf ("on_wstatus\n");

    if (bytes_written && write_status){
        ev_sock->get_ssl ();
    }
}

void tls_server_app::on_read (ev_socket* ev_sock)
{
    printf ("on_read\n");

    ev_sock->read_next_data (m_read_buffer, 0, MAX_READ_BUFFER_LEN, true);
}

void tls_server_app::on_rstatus (ev_socket* ev_sock
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

void tls_server_app::on_free (ev_socket* ev_sock)
{
    printf ("on_free\n");

    delete ev_sock;
}


//////////////////////////////tls_server_socket////////////////////////////////////
tls_server_socket::tls_server_socket(/* args */)
{
}

tls_server_socket::~tls_server_socket()
{
}
