#include "ev_socket.hpp"

class socket_manager
{
public:
    socket_manager(int max_events, int event_timeout);
    ~socket_manager();

    int run_loop ();

private:

private:
    int m_epoll_id;
    int m_epoll_timeout;
    int m_max_epoll_events;
    struct epoll_event* m_epoll_event_arr;

    std::queue<ev_socket> m_free_queue;
    std::queue<ev_socket> m_active_queue;
    std::queue<ev_socket> m_cleanup_queue;
    std::queue<ev_socket> m_abort_queue;
};
