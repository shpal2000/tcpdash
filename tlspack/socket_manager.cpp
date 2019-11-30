#include "socket_manager.hpp"

socket_manager::socket_manager(int max_events, int epoll_timeout)
{
    m_epoll_id = epoll_create (1);
    m_max_epoll_events = max_events;
    m_epoll_timeout = epoll_timeout;
    m_epoll_event_arr = new struct epoll_event [max_events];
}

socket_manager::~socket_manager()
{
}

int socket_manager::run_loop ()
{
    int to_continue = 1;

    int event_count = epoll_wait (m_epoll_id
                        , m_epoll_event_arr
                        , m_max_epoll_events
                        , m_epoll_timeout);
    
    if (event_count > 0)
    {
        for (int event_index = 0; event_index < event_count; event_index++)
        {
            ev_socket* event_socket_ptr 
                = (ev_socket*) m_epoll_event_arr[event_index].data.ptr;

            if ( event_socket_ptr->is_set_state (STATE_TCP_LISTENING) )
            {
                event_socket_ptr->accept_connection ();
            } 
            else
            {

            }
        }
    }

    return to_continue;
}
