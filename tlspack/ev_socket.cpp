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

ev_socket* ev_socket::tcp_accept(epoll_ctx* epoll_ctxp)
{
    // ev_socket* new_ev_socket = new ev_socket ();
    return nullptr;
}

epoll_ctx* ev_socket::epoll_alloc(ev_app* app_ptr, int max_events, int epoll_timeout)
{
    epoll_ctx* epoll_ctxp = new epoll_ctx(app_ptr, max_events, epoll_timeout);
    return epoll_ctxp;
}

void ev_socket::epoll_free(epoll_ctx* epoll_ctxp)
{
    delete epoll_ctxp;
}

void ev_socket::epoll_process(epoll_ctx* epoll_ctxp)
{
    int event_count = epoll_wait (epoll_ctxp->m_epoll_id
                        , epoll_ctxp->m_epoll_event_arr
                        , epoll_ctxp->m_max_epoll_events
                        , epoll_ctxp->m_epoll_timeout);
    
    if (event_count > 0)
    {
        for (int event_index = 0; event_index < event_count; event_index++)
        {
            ev_socket* event_socket_ptr 
                = (ev_socket*) epoll_ctxp->m_epoll_event_arr[event_index].data.ptr;

            if ( event_socket_ptr->is_set_state (STATE_TCP_LISTENING) )
            {
                event_socket_ptr->tcp_accept (epoll_ctxp);
            } 
            else
            {

            }
        }
    }
}