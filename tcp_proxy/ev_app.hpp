#ifndef __EV_APP__H
#define __EV_APP__H

#include "ev_socket.hpp"

#define MAX_EPOLL_EVENT_COUNT 1
#define MAX_EPORL_TIMEOUT_MSEC  1

class ev_app
{
public:
    ev_app()
    {
        m_epoll_ctx = ev_socket::epoll_alloc (this
                                    , MAX_EPOLL_EVENT_COUNT
                                    , MAX_EPORL_TIMEOUT_MSEC);

        m_tick_sec = 0;
    }
    virtual ~ev_app()
    {
        if (m_epoll_ctx)
        {
            ev_socket::epoll_free (m_epoll_ctx);
            m_epoll_ctx = nullptr;
        }
    }

    virtual void run_iter(bool tick_sec)
    {
        if (tick_sec) {
            m_tick_sec++;
        }

        int is_debug = 0;

        switch (is_debug)
        {
        case 1:
            dump_active_conn_count ();
            break;
        case 2:
            dump_active_conns ();
            break;
            
        default:
            break;
        }
        ev_socket::epoll_process (m_epoll_ctx);
    }


    virtual ev_socket* alloc_socket() = 0;
    virtual void free_socket(ev_socket* ev_sock) = 0;

    ev_socket* new_tcp_connect (ev_sockaddr* laddr
                                , ev_sockaddr* raddr
                                , std::vector<ev_sockstats*>* statsArr
                                , ev_portq* portq
                                , ev_socket_opt* ev_sock_opt)
    {
        return ev_socket::new_tcp_connect (m_epoll_ctx
                                            , laddr
                                            , raddr
                                            , statsArr
                                            , portq
                                            , ev_sock_opt);
    }

    ev_socket* new_tcp_listen (ev_sockaddr* laddr
                                , int lqlen
                                , std::vector<ev_sockstats*>* statsArr
                                , ev_socket_opt* ev_sock_opt )
    {
        return ev_socket::new_tcp_listen (m_epoll_ctx
                                            , laddr
                                            , lqlen
                                            , statsArr
                                            , ev_sock_opt);
    }

    void add_to_active_list (ev_socket* ev_sock) {
        m_act_conns.add (ev_sock);
    }

    void remove_from_active_list (ev_socket* ev_sock) {
        m_act_conns.remove (ev_sock);
    }

    void dump_active_conn_count () {
        printf ("\ncount = %d\n", m_act_conns.m_count);
        fflush (stdout);
    }

    void dump_active_conns () {
        ev_socket* next = m_act_conns.m_head;

        while (next) {
            printf ("\nstate=0x%" PRIX64 ", estate=0x%" PRIX64 "\n"
                        , next->get_state()
                        , next->get_error_state());

            next = next->m_next;
        }

        fflush (stdout);
    }

private:
    epoll_ctx* m_epoll_ctx;
    ev_sock_ll m_act_conns;
    uint32_t m_tick_sec;
};

#endif