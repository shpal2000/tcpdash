#include "tlssrv_app.hpp"
#include "tlssrv_socket.hpp"

tlssrv_app::tlssrv_app(/* args */)
{
}

tlssrv_app::~tlssrv_app()
{
}

int tlssrv_app::run_iter()
{
    return 0;
}

ev_socket* tlssrv_app::alloc_socket()
{
    return new tlssrv_socket();
}

void tlssrv_app::free_socket(ev_socket* ev_sock)
{
    delete ev_sock;
}

void tlssrv_app::on_establish (ev_socket* ev_sock)
{

}

void tlssrv_app::on_write (ev_socket* ev_sock)
{

}

void tlssrv_app::on_wstatus (ev_socket* ev_sock)
{

}

void tlssrv_app::on_read (ev_socket* ev_sock)
{

}

void tlssrv_app::on_rstatus (ev_socket* ev_sock)
{

}

void tlssrv_app::on_finish (ev_socket* ev_sock)
{

}