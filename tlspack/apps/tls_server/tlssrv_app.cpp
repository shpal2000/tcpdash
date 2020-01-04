#include "tlssrv_app.hpp"
#include "tlssrv_socket.hpp"

tlssrv_app::tlssrv_app(json cfg_json, int c_index)
{
    std::vector<ev_sockstats*> statsArr;
    statsArr.push_back ( new ev_sockstats() );

    auto srv_list = cfg_json["server"]["containers"][c_index]["srv_list"];
    for (auto it = srv_list.begin(); it != srv_list.end(); ++it)
    {
        auto srv_cfg = it.value ();
        ev_sockaddr serverAddr;
        ev_socket::set_sockaddr (&serverAddr
                                , srv_cfg["srv_ip"].get<std::string>().c_str()
                                , srv_cfg["srv_port"].get<int>());
        ev_socket* server;
        server = new_tcp_listen (&serverAddr, 100, &statsArr);
        server->set_status (0);
    }
}

tlssrv_app::~tlssrv_app()
{
}

void tlssrv_app::run_iter()
{
    ev_app::run_iter ();

    //todo
}

ev_socket* tlssrv_app::alloc_socket()
{
    return new tlssrv_socket();
}

void tlssrv_app::on_establish (ev_socket* ev_sock)
{
    ev_sock->get_ssl ();
}

void tlssrv_app::on_write (ev_socket* ev_sock)
{
    ev_sock->get_ssl ();
}

void tlssrv_app::on_wstatus (ev_socket* ev_sock
                            , int bytes_written
                            , int write_status)
{
    if (bytes_written && write_status){
        ev_sock->get_ssl ();
    }
}

void tlssrv_app::on_read (ev_socket* ev_sock)
{
    ev_sock->get_ssl ();
}

void tlssrv_app::on_rstatus (ev_socket* ev_sock
                            , int bytes_read
                            , int read_status)
{
    if (bytes_read && read_status) {
        ev_sock->get_ssl ();
    }
}

void tlssrv_app::on_free (ev_socket* ev_sock)
{
    delete ev_sock;
}