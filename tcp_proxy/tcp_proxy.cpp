#include "tcp_proxy.hpp"

tp_app::tp_app(json app_json, tp_stats* app_stats)
{
    m_app_stats_arr = new std::vector<ev_sockstats*> ();

    m_app_stats = app_stats;
    m_app_stats_arr->push_back (app_stats);

    //todo
    m_sock_opt.rcv_buff_len = 0;
    m_sock_opt.snd_buff_len = 0;

    tp_socket* srv_socket 
        = (tp_socket*) new_tcp_listen (&m_listen_addr
                                                , 10000
                                                , m_app_stats_arr
                                                , &m_sock_opt);
    if (srv_socket)
    {
        srv_socket->m_app = this;
    }
    else 
    {
        //todo error handling
    }
}

tp_app::~tp_app()
{

}

void tp_app::run_iter(bool tick_sec)
{
    ev_app::run_iter (tick_sec);

    if (tick_sec)
    {
        m_app_stats->tick_sec();
    }
}

ev_socket* tp_app::alloc_socket()
{
    return new tp_socket();
}

void tp_app::free_socket(ev_socket* ev_sock)
{
    delete ev_sock;
}

void tp_socket::on_establish ()
{
    m_lsock = (tp_socket*) get_parent();
    m_app = m_lsock->m_app;

    if (m_session == nullptr) // accepted socket; new session
    {
        // printf ("on_establish - server\n");

        m_session = new tp_session(this);

        if (m_session)
        {
            tp_socket* client_socket 
                    = (tp_socket*) m_app->new_tcp_connect (get_remote_addr()
                                                        , get_local_addr()
                                                        , m_app->m_app_stats_arr
                                                        , NULL
                                                        , &m_app->m_sock_opt);
            if (client_socket)
            {
                //todo
            }
            else
            {
                //todo error handling
            }
        }
        else
        {
            //todo error handling
        }
    }
    else //client session
    {
        // printf ("on_establish - client\n");

        m_session->m_client_sock = this;
    }
}

void tp_socket::on_write ()
{

}

void tp_socket::on_wstatus (int bytes_written, int write_status)
{

}

void tp_socket::on_read ()
{

}

void tp_socket::on_rstatus (int bytes_read, int read_status)
{

}

void tp_socket::on_finish ()
{
    
}