#include "ev_app.hpp"

ev_app::ev_app()
{
    m_epoll_ctx = ev_socket::epoll_alloc (this, 1, 1);
    m_next_chunk_index = 0;
    m_next_chunk_shift = 0;
}

ev_app::~ev_app()
{
    if (m_epoll_ctx)
    {
        ev_socket::epoll_free (m_epoll_ctx);
        m_epoll_ctx = nullptr;
    }
}

void ev_app::run_iter (bool)
{
    ev_socket::epoll_process (m_epoll_ctx);
}
