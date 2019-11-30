#include "ev_socket.hpp"

class socket_manager
{
public:
    socket_manager(/* args */);
    ~socket_manager();

    int run_loop ();

private:

private:
    std::queue<ev_socket> m_free_queue;
    std::queue<ev_socket> m_active_queue;
    std::queue<ev_socket> m_cleanup_queue;
    std::queue<ev_socket> m_abort_queue;
};
