#include "ev_app.hpp"

class tlssrv_app : public ev_app
{
public:
    tlssrv_app(/* args */);
    ~tlssrv_app();

    int run_iter();
    ev_socket* alloc_socket();
    void free_socket (ev_socket* ev_sock);
    void on_establish (ev_socket* ev_sock);
    void on_write (ev_socket* ev_sock);
    void on_wstatus (ev_socket* ev_sock);
    void on_read (ev_socket* ev_sock);
    void on_rstatus (ev_socket* ev_sock);
    void on_finish (ev_socket* ev_sock);

private:
};
