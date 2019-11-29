#include "ev_socket.hpp"

class app_socket : public ev_socket
{
private:
    /* data */
public:
    app_socket(/* args */);
    virtual ~app_socket();

    virtual void on_establish ();
    virtual void on_write_next ();
    virtual void on_write_status ();
    virtual void on_read_next ();
    virtual void on_read_status ();
    virtual void on_purge ();
};
