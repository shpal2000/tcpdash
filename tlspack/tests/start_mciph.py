__author__ = 'Shirish Pal'
 
import argparse
import os

cfj_template = '''
        {{
            "zone_label" : "zone-{zone_id}-a",
            "enable" : 1,
            "zone_type" : "single-interface",
            "iface" : "{iface}",
            "macvlan" : "{macvlan1}",
            "tcpdump" : "-c 1000",
            "subnets" : ["12.2{subnet_id}.51.0/24"],
            "app_list" : [
                {{
                    "app_type" : "tls_client",
                    "app_label" : "tls_client_1",
                    "enable" : 1,
                    "conn_per_sec" : {cps},
                    "max_pending_conn_count" : {max_pipeline},
                    "max_active_conn_count" : {max_active},
                    "total_conn_count" : 0,
                    "cs_grp_list" : [
                        {{
                            "cs_grp_label" : "cs_grp_1",
                            "enable" : 1,
                            "srv_ip"   : "14.2{subnet_id}.51.1",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.1",
                            "clnt_ip_end" : "12.2{subnet_id}.51.10",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "AES128-SHA",
                            "tls_version" : "sslv3",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }},
                        {{
                            "cs_grp_label" : "cs_grp_2",
                            "enable" : 1,
                            "srv_ip"   : "14.2{subnet_id}.51.2",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.11",
                            "clnt_ip_end" : "12.2{subnet_id}.51.20",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "AES128-SHA",
                            "tls_version" : "tls1",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }},
                        {{
                            "cs_grp_label" : "cs_grp_3",
                            "enable" : 1,
                            "srv_ip"   : "14.2{subnet_id}.51.3",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.21",
                            "clnt_ip_end" : "12.2{subnet_id}.51.30",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "AES128-SHA",
                            "tls_version" : "tls1_1",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }},
                        {{
                            "cs_grp_label" : "cs_grp_4",
                            "enable" : 1,
                            "srv_ip"   : "14.2{subnet_id}.51.4",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.31",
                            "clnt_ip_end" : "12.2{subnet_id}.51.40",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "AES128-SHA",
                            "tls_version" : "tls1_2",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }}
                    ]
                }}
            ]       
        }},

        {{
            "zone_label" : "zone-{zone_id}-b",
            "enable" : 1,
            "zone_type" : "single-interface",
            "iface" : "{iface}",
            "macvlan" : "{macvlan2}",
            "tcpdump" : "-c 1000",
            "subnets" : ["14.2{subnet_id}.51.0/24"],
            "app_list" : [
                {{
                    "app_type" : "tls_server",
                    "app_label" : "tls_server_1",
                    "enable" : 1,
                    "srv_list" : [
                        {{
                            "srv_label" : "14.2{subnet_id}.51.1",
                            "enable" : 1,
                            "srv_ip" : "14.2{subnet_id}.51.1",
                            "srv_port" : 443,
                            "srv_cert" : "/rundir/certs/server.cert",
                            "srv_key" : "/rundir/certs/server.key",
                            "cipher" : "AES128-SHA",
                            "tls_version" : "sslv3",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }},
                        {{
                            "srv_label" : "14.2{subnet_id}.51.2",
                            "enable" : 1,
                            "srv_ip" : "14.2{subnet_id}.51.2",
                            "srv_port" : 443,
                            "srv_cert" : "/rundir/certs/server.cert",
                            "srv_key" : "/rundir/certs/server.key",
                            "cipher" : "AES128-SHA",
                            "tls_version" : "tls1",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }},
                        {{
                            "srv_label" : "14.2{subnet_id}.51.3",
                            "enable" : 1,
                            "srv_ip" : "14.2{subnet_id}.51.3",
                            "srv_port" : 443,
                            "srv_cert" : "/rundir/certs/server.cert",
                            "srv_key" : "/rundir/certs/server.key",
                            "cipher" : "AES128-SHA",
                            "tls_version" : "tls1_1",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }},
                        {{
                            "srv_label" : "14.2{subnet_id}.51.4",
                            "enable" : 1,
                            "srv_ip" : "14.2{subnet_id}.51.4",
                            "srv_port" : 443,
                            "srv_cert" : "/rundir/certs/server.cert",
                            "srv_key" : "/rundir/certs/server.key",
                            "cipher" : "AES128-SHA",
                            "tls_version" : "tls1_2",
                            "close_type" : "fin",
                            "close_notify" : "no_send",
                            "write_chunk" : 1200,
                            "tcp_rcv_buff" : 0,
                            "tcp_snd_buff" : 0,
                            "cs_data_len" : 128,
                            "sc_data_len" : 128,
                            "cs_start_tls_len" : 0,
                            "sc_start_tls_len" : 0
                        }}
                    ]
                }}
            ]   
        }}
'''

def get_command_line_params():
    arg_parser = argparse.ArgumentParser(description = \
                                            'cps test')
 
    arg_parser.add_argument('--rundir'
                                , action="store"
                                , default='/root/rundir'
                                , help = 'macvlan1 name')

    arg_parser.add_argument('--macvlan1'
                                , action="store"
                                , default='ens160macvlan'
                                , help = 'macvlan1 name')

    arg_parser.add_argument('--macvlan2'
                                , action="store"
                                , default='ens192macvlan'
                                , help = 'macvlan2 name')

    arg_parser.add_argument('--iface'
                                , action="store"
                                , default='eth1'
                                , help = 'iface name')

    arg_parser.add_argument('--runid'
                                , action="store"
                                , default='tmprun'
                                , help = 'iface name')

    arg_parser.add_argument('--cipher'
                                , action="store"
                                , help = 'command name'
                                , required=True)

    arg_parser.add_argument('--cps'
                                , action="store"
                                , type=int
                                , default=10
                                , help = 'cps : 1 - 500')

    arg_parser.add_argument('--max_pipeline'
                                , action="store"
                                , type=int
                                , default=1
                                , help = 'max_pipeline : 1 - 1000')

    arg_parser.add_argument('--max_active'
                                , action="store"
                                , type=int
                                , default=100
                                , help = 'max_active : 1 - 1000')

    arg_parser.add_argument('--scale'
                                , action="store"
                                , type=int
                                , default=1
                                , help = 'cps scale : 1 - 20')

    return arg_parser.parse_args()


if __name__ == '__main__':
    CmdArgs = get_command_line_params()

    cfg_j_pre = '''{
        "zones" : [
'''

    cfg_j_post = '''
        ]
}'''

    cfg_j = ''

    for i in range(CmdArgs.scale):
        if i:
            cfg_j = cfg_j + ','
            
        m = vars (CmdArgs)
        m['zone_id'] = i+1
        m['subnet_id'] = i+1
        cfg_j += cfj_template.format(**m)

    cfg_j = cfg_j_pre + cfg_j + cfg_j_post

    traffic_dir = os.path.join(CmdArgs.rundir
                                , 'traffic'
                                , CmdArgs.runid)
    os.system ( 'rm -rf {}'.format(traffic_dir) )
    os.system ( 'mkdir {}'.format(traffic_dir) )

    with open(os.path.join(traffic_dir, 'config.json'), 'w') as f:
        f.write(cfg_j)

    os.system ( 'sudo docker run --name "{}-root" -it -d --volume=/root/rundir:/rundir tgen /rundir/bin/tlspack.exe start "{}" run5 /root/rundir 0'.format(CmdArgs.runid, CmdArgs.runid) )
