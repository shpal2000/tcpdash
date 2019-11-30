#include "ev_socket.hpp"

ev_socket::ev_socket(/* args */)
{
    m_saved_lport = 0;
    m_saved_rport = 0;

    m_state = 0;
    m_error_state = 0;

    m_local_addr = nullptr;
    m_remote_addr = nullptr;

    m_port_pool = nullptr;
    m_sockstats_arr = nullptr;

}

ev_socket::~ev_socket()
{
}

ev_socket* ev_socket::accept_connection()
{
    // ev_socket* new_ev_socket = new ev_socket ();
    return nullptr;
}

epoll_ctx* ev_socket::create_epoll_ctx(int max_events, int epoll_timeout)
{
    epoll_ctx* epoll_ctx_ptr = new epoll_ctx(max_events, epoll_timeout);
    return epoll_ctx_ptr;
}

void ev_socket::free_epoll_ctx(epoll_ctx* epoll_ctx_ptr)
{
    delete epoll_ctx_ptr;
}

void ev_socket::epoll_loop(epoll_ctx* epoll_ctx_ptr)
{
    int event_count = epoll_wait (epoll_ctx_ptr->m_epoll_id
                        , epoll_ctx_ptr->m_epoll_event_arr
                        , epoll_ctx_ptr->m_max_epoll_events
                        , epoll_ctx_ptr->m_epoll_timeout);
    
    if (event_count > 0)
    {
        for (int event_index = 0; event_index < event_count; event_index++)
        {
            ev_socket* event_socket_ptr 
                = (ev_socket*) epoll_ctx_ptr->m_epoll_event_arr[event_index].data.ptr;

            if ( event_socket_ptr->is_set_state (STATE_TCP_LISTENING) )
            {
                event_socket_ptr->accept_connection ();
            } 
            else
            {

            }
        }
    }
}