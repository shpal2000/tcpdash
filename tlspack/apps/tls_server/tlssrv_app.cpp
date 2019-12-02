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

ev_socket* tlssrv_app::alloc_ev_socket()
{
    return new tlssrv_socket();
}

void tlssrv_app::free_ev_socket(ev_socket* ev_sock)
{
    delete ev_sock;
}

void tlssrv_app::on_establish (ev_socket* ev_sock)
{

}

void tlssrv_app::on_write_next (ev_socket* ev_sock)
{

}

void tlssrv_app::on_write_status (ev_socket* ev_sock)
{

}

void tlssrv_app::on_read_next (ev_socket* ev_sock)
{

}

void tlssrv_app::on_read_status (ev_socket* ev_sock)
{

}

void tlssrv_app::on_delete (ev_socket* ev_sock)
{

}