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

        ev_app_cs_grp* next_cs_grp 
            = new ev_app_cs_grp (cs_grp_cfg["srv_ip"].get<std::string>().c_str()
                        , cs_grp_cfg["srv_port"].get<u_short>()
                        , cs_grp_cfg["clnt_ip_begin"].get<std::string>().c_str()
                        , cs_grp_cfg["clnt_ip_end"].get<std::string>().c_str()
                        , cs_grp_cfg["clnt_port_begin"].get<u_short>()
                        , cs_grp_cfg["clnt_port_end"].get<u_short>()
                        , cs_grp_stats_arr);

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
        ev_app_cs_grp* cs_grp = m_cs_groups [m_cs_group_index];

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

    ev_sock->get_ssl ();

    ev_sock->write_close();
}

void tls_client_app::on_write (ev_socket* ev_sock)
{
    // printf ("on_write\n");

    ev_sock->disable_wr_notification ();
}

void tls_client_app::on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status)
{
    // printf ("on_wstatus\n");
}

void tls_client_app::on_read (ev_socket* ev_sock)
{
    // printf ("on_read\n");

    ev_sock->read_next_data (m_read_buffer, 0, MAX_READ_BUFFER_LEN, true);

}

void tls_client_app::on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status)
{
    // printf ("on_rstatus\n");

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
    // printf ("on_free\n");
    
    delete ev_sock;
}


//////////////////////////////tls_client_socket////////////////////////////////////
tls_client_socket::tls_client_socket(/* args */)
{
}

tls_client_socket::~tls_client_socket()
{
}
