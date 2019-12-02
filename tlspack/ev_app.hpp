#include "ev_socket.hpp"

class ev_app
{
public:
    ev_app(/* args */);
    ~ev_app();

    virtual int run_iter() = 0;
    virtual ev_socket* alloc_ev_socket() = 0;
    virtual void free_ev_socket(ev_socket* ev_sock) = 0;
    virtual void on_establish (ev_socket* ev_sock) = 0;
    virtual void on_write_next (ev_socket* ev_sock) = 0;
    virtual void on_write_status (ev_socket* ev_sock) = 0;
    virtual void on_read_next (ev_socket* ev_sock) = 0;
    virtual void on_read_status (ev_socket* ev_sock) = 0;
    virtual void on_delete (ev_socket* ev_sock) = 0;
};

