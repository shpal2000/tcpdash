#include "tcp_proxy.hpp"

tp_app::tp_app(json app_json, tp_stats* app_stats)
{
    m_app_stats_arr = new std::vector<ev_sockstats*> ();

    m_app_stats = app_stats;
    m_app_stats_arr->push_back (app_stats);

    //todo
    m_sock_opt.rcv_buff_len = 0;
    m_sock_opt.snd_buff_len = 0;

    const char* srv_ip = "0.0.0.0";
    u_short srv_port = app_json["proxy_app_port"].get<u_short>();
    ev_socket::set_sockaddr (&m_listen_addr, srv_ip, htons(srv_port));

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
    if (m_session == nullptr) // accepted socket; new session
    {
        tp_socket* server_socket = this;
        tp_session* new_sess = new tp_session();

        if (new_sess)
        {
            tp_socket* client_socket 
                    = (tp_socket*) m_app->new_tcp_connect (get_remote_addr()
                                                        , get_local_addr()
                                                        , m_app->m_app_stats_arr
                                                        , NULL
                                                        , &m_app->m_sock_opt);
            if (client_socket)
            {
                server_socket->m_session = new_sess;
                client_socket->m_session = new_sess;

                m_session->m_server_sock = server_socket;
                m_session->m_client_sock = client_socket;

                server_socket->m_app = ((tp_socket*)get_parent())->m_app;
                client_socket->m_app = ((tp_socket*)get_parent())->m_app;
            }
            else
            {
                //todo error handling
                delete new_sess;
                server_socket->abort();
            }
        }
        else
        {
            //todo error handling
            server_socket->abort();
        }
    }
    else //client session
    {
        // printf ("on_establish - client\n");
        m_session->m_session_established = true;
    }
}

void tp_socket::on_write ()
{
    if (m_session->m_session_established)
    {
        if (m_session->m_client_sock == this)
        {
            if ( not m_session->m_server_rbuffs.empty () )
            {
                ev_buff* wr_buff = m_session->m_server_rbuffs.front ();
                m_session->m_server_rbuffs.pop ();
                write_next_data (wr_buff->m_buff, 0, wr_buff->m_data_len, false);
                m_session->m_client_current_wbuff = wr_buff;
            }
        }
        else
        {
            if ( not m_session->m_client_rbuffs.empty () )
            {
                ev_buff* wr_buff = m_session->m_client_rbuffs.front ();
                m_session->m_client_rbuffs.pop ();
                write_next_data (wr_buff->m_buff, 0, wr_buff->m_data_len, false);
                m_session->m_server_current_wbuff = wr_buff;
            }
        }   
    }
}

void tp_socket::on_wstatus (int bytes_written, int write_status)
{
    if (m_session->m_client_sock == this)
    {
        delete m_session->m_client_current_wbuff;
        m_session->m_client_current_wbuff = nullptr;
    }
    else
    {
        delete m_session->m_server_current_wbuff;
        m_session->m_server_current_wbuff = nullptr;
    }

    if (write_status == WRITE_STATUS_NORMAL)
    {
        //todo
    }
    else
    {
        //todo error handling
        abort();
    }
}

void tp_socket::on_read ()
{
    if (m_session->m_session_established)
    {
        ev_buff* rd_buff = new ev_buff(2048);
        if (rd_buff && rd_buff->m_buff)
        {
            if (m_session->m_client_sock == this)
            {
                m_session->m_client_current_rbuff = rd_buff;
            }
            else
            {
                m_session->m_server_current_rbuff = rd_buff;
            }

            read_next_data (rd_buff->m_buff, 0, rd_buff->m_buff_len, true);
        }
        else
        {
            //todo error handling
        }
    }
}

void tp_socket::on_rstatus (int bytes_read, int read_status)
{
    if (bytes_read == 0) 
    {
        if (read_status != READ_STATUS_TCP_CLOSE) {
            abort ();
        }
    } 
    else 
    {
        if (m_session->m_client_sock == this)
        { 
            m_session->m_client_current_rbuff->m_data_len = bytes_read;
            m_session->m_client_rbuffs.push (m_session->m_client_current_rbuff);
        } 
        else
        {
            m_session->m_server_current_rbuff->m_data_len = bytes_read;
            m_session->m_server_rbuffs.push (m_session->m_server_current_rbuff);
        }
    }
}

void tp_socket::on_finish ()
{
    if (m_session)
    {
        if (m_session->m_client_sock == this)
        {
            m_session->m_client_sock = nullptr;
        }
        else
        {
            m_session->m_server_sock = nullptr;
        }

        if (not m_session->m_client_sock && not m_session->m_server_sock)
        {
            delete m_session;
        }
    }
}