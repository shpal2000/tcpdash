#include "ev_app.hpp"

class tlssrv_app : public ev_app
{
public:
    tlssrv_app(/* args */);
    ~tlssrv_app();

    int run_iter();
    tlssrv_socket* alloc_socket();
    void free_socket (tlssrv_socket* ev_sock);
    void on_establish (tlssrv_socket* ev_sock);
    void on_write (tlssrv_socket* ev_sock);
    void on_wstatus (tlssrv_socket* ev_sock);
    void on_read (tlssrv_socket* ev_sock);
    void on_rstatus (tlssrv_socket* ev_sock);
    void on_finish (tlssrv_socket* ev_sock);

private:
};
