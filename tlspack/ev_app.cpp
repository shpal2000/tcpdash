#include "ev_app.hpp"

ev_app::ev_app(/* args */)
{
    m_epoll_ctx = ev_socket::epoll_alloc (this, 1, 1);
}

ev_app::~ev_app()
{
    if (m_epoll_ctx)
    {
        ev_socket::epoll_free (m_epoll_ctx);
        m_epoll_ctx = nullptr;
    }
}
