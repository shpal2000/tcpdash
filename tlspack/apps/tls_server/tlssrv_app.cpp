#include "tlssrv_app.hpp"
#include "tlssrv_socket.hpp"

tlssrv_app::tlssrv_app(/* args */)
{
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
    tlssrv_socket* app_socket =  (tlssrv_socket*) ev_sock;
}

void tlssrv_app::on_write (ev_socket* ev_sock)
{

}

void tlssrv_app::on_wstatus (ev_socket* ev_sock, int bytes_written)
{

}

void tlssrv_app::on_read (ev_socket* ev_sock)
{

}

void tlssrv_app::on_rstatus (ev_socket* ev_sock, int bytes_read)
{

}

void tlssrv_app::on_free (ev_socket* ev_sock)
{
    delete ev_sock;
}