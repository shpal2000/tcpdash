#include "platform.hpp"

class ev_socket
{
private:
    int m_fd;
    uint16_t m_saved_lport;
    uint16_t m_saved_rport;

    td_sockaddr* m_local_addr;
    td_sockaddr* m_remote_addr;
    td_portpool* m_port_pool;

    td_sockstats_arr m_sockstats_arr;

public:
    ev_socket(/* args */);
    virtual ~ev_socket();

    virtual void on_establish () = 0;
    virtual void on_write_next () = 0;
    virtual void on_write_status () = 0;
    virtual void on_read_next () = 0;
    virtual void on_read_status () = 0;
    virtual void on_purge () = 0;
};
