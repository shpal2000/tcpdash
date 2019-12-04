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

tlssrv_socket* tlssrv_app::alloc_socket()
{
    return new tlssrv_socket();
}

void tlssrv_app::free_socket(tlssrv_socket* ev_sock)
{
    delete ev_sock;
}

void tlssrv_app::on_establish (tlssrv_socket* ev_sock)
{

}

void tlssrv_app::on_write (tlssrv_socket* ev_sock)
{

}

void tlssrv_app::on_wstatus (tlssrv_socket* ev_sock)
{

}

void tlssrv_app::on_read (tlssrv_socket* ev_sock)
{

}

void tlssrv_app::on_rstatus (tlssrv_socket* ev_sock)
{

}

void tlssrv_app::on_finish (tlssrv_socket* ev_sock)
{

}