#ifndef __SESSION_STATES_H
#define __SESSION_STATES_H

#define TD_NO_ERROR                                         0
#define TD_PROGRAM_ERROR                                    1

#define TD_SOCKET_CREATE_FAILED                             2
#define TD_SOCKET_BIND_FAILED                               3
#define TD_SOCKET_CONNECT_ESTABLISH_FAILED                  4
#define TD_SOCKET_CONNECT_ESTABLISH_FAILED2                 5
#define TD_PROGRAM_ERROR_TcpNewConnection                   6

#define STATE_TCP_SOCK_CREATE                               0x0000000000000001
#define STATE_TCP_SOCK_BIND                                 0x0000000000000002
#define STATE_TCP_CONN_INIT                                 0x0000000000000004
#define STATE_TCP_CONN_IN_PROGRESS                          0x0000000000000008
#define STATE_TCP_CONN_IN_PROGRESS2                         0x0000000000000010
#define STATE_TCP_CONN_ESTABLISHED                          0x0000000000000020
#define STATE_TCP_SOCK_FD_CLOSE                             0x0000000000000040

#endif 

