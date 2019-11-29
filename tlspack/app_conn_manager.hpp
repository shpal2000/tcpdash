#include "app_conn.hpp"

class app_conn_manager
{
public:
    app_conn_manager(/* args */);
    ~app_conn_manager();

    int run_loop ();

private:

private:
    std::queue<app_conn> m_free_conn_pool;
    std::queue<app_conn> m_active_conn_pool;
    std::queue<app_conn> m_cleanup_conn_pool;
    std::queue<app_conn> m_abort_conn_pool;
};
