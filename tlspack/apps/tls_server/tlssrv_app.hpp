#include "ev_app.hpp"

class tlssrv_app : public ev_app
{
public:
    tlssrv_app(/* args */);
    ~tlssrv_app();

    int run_iter();
    ev_socket* alloc_ev_socket();
    void free_ev_socket (ev_socket* ev_sock);
    void on_establish (ev_socket* ev_sock);
    void on_write_next (ev_socket* ev_sock);
    void on_write_status (ev_socket* ev_sock);
    void on_read_next (ev_socket* ev_sock);
    void on_read_status (ev_socket* ev_sock);
    void on_delete (ev_socket* ev_sock);

private:
};
