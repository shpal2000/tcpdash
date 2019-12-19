#include "tlssrv_app.hpp"
#include "tlssrv_socket.hpp"

tlssrv_app::tlssrv_app(/* args */)
{
    ev_sockaddr serverAddr;
    std::vector<ev_sockstats*> statsArr;
    statsArr.push_back ( new ev_sockstats() );
    ev_socket::set_sockaddr (&serverAddr, "10.0.0.4", 6000);
    ev_socket* server = new_tcp_listen (&serverAddr, 100, &statsArr);
    server->set_status (0);
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