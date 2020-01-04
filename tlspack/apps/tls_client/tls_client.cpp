#include "tls_client.hpp"

tls_client_app::tls_client_app(json cfg_json, int c_index)
{
    std::vector<ev_sockstats*> statsArr;
    statsArr.push_back ( new ev_sockstats() );

    auto cs_grp_list = cfg_json["client"]["containers"][c_index]["cs_grp_list"];
    for (auto it = cs_grp_list.begin(); it != cs_grp_list.end(); ++it)
    {
        auto cs_grp = it.value ();

        ev_sockaddr serverAddr;
        ev_socket::set_sockaddr (&serverAddr
                                , cs_grp["srv_ip"].get<std::string>().c_str()
                                , cs_grp["srv_port"].get<int>());

        ev_sockaddr clientAddr;
        ev_socket::set_sockaddr (&clientAddr
                            , cs_grp["clnt_ip_begin"].get<std::string>().c_str()
                            , cs_grp["clnt_port_begin"].get<int>());

        ev_socket* client;
        client = new_tcp_connect (&clientAddr, &serverAddr, &statsArr);
        client->set_status (0);
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
    fflush (NULL);
    ev_sock->get_ssl ();
}

void tls_client_app::on_write (ev_socket* ev_sock)
{
    ev_sock->get_ssl ();
}

void tls_client_app::on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status)
{
    if (bytes_written && write_status){
        ev_sock->get_ssl ();
    }
}

void tls_client_app::on_read (ev_socket* ev_sock)
{
    ev_sock->get_ssl ();
}

void tls_client_app::on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status)
{
    if (bytes_read && read_status) {
        ev_sock->get_ssl ();
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
