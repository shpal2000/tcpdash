__author__ = 'Shirish Pal'
 
import os
import argparse
import cmds
import utils

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
                            "enable" : {sslv3},
                            "srv_ip"   : "14.2{subnet_id}.51.1",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.1",
                            "clnt_ip_end" : "12.2{subnet_id}.51.10",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "{cipher}",
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
                            "enable" : {tls1},
                            "srv_ip"   : "14.2{subnet_id}.51.2",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.11",
                            "clnt_ip_end" : "12.2{subnet_id}.51.20",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "{cipher}",
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
                            "enable" : {tls1_1},
                            "srv_ip"   : "14.2{subnet_id}.51.3",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.21",
                            "clnt_ip_end" : "12.2{subnet_id}.51.30",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "{cipher}",
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
                            "enable" : {tls1_2},
                            "srv_ip"   : "14.2{subnet_id}.51.4",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.31",
                            "clnt_ip_end" : "12.2{subnet_id}.51.40",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "{cipher}",
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
                        }},
                        {{
                            "cs_grp_label" : "cs_grp_5",
                            "enable" : {tls1_3},
                            "srv_ip"   : "14.2{subnet_id}.51.5",
                            "srv_port" : 443,
                            "clnt_ip_begin" : "12.2{subnet_id}.51.41",
                            "clnt_ip_end" : "12.2{subnet_id}.51.50",
                            "clnt_port_begin" : 5000,
                            "clnt_port_end" : 65000,
                            "cipher" : "{cipher}",
                            "tls_version" : "tls1_3",
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
                            "enable" : {sslv3},
                            "srv_ip" : "14.2{subnet_id}.51.1",
                            "srv_port" : 443,
                            "srv_cert" : "{server_cert}",
                            "srv_key" : "{server_key}",
                            "cipher" : "{cipher}",
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
                            "enable" : {tls1},
                            "srv_ip" : "14.2{subnet_id}.51.2",
                            "srv_port" : 443,
                            "srv_cert" : "{server_cert}",
                            "srv_key" : "{server_key}",
                            "cipher" : "{cipher}",
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
                            "enable" : {tls1_1},
                            "srv_ip" : "14.2{subnet_id}.51.3",
                            "srv_port" : 443,
                            "srv_cert" : "{server_cert}",
                            "srv_key" : "{server_key}",
                            "cipher" : "{cipher}",
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
                            "enable" : {tls1_2},
                            "srv_ip" : "14.2{subnet_id}.51.4",
                            "srv_port" : 443,
                            "srv_cert" : "{server_cert}",
                            "srv_key" : "{server_key}",
                            "cipher" : "{cipher}",
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
                        }},
                        {{
                            "srv_label" : "14.2{subnet_id}.51.5",
                            "enable" : {tls1_3},
                            "srv_ip" : "14.2{subnet_id}.51.5",
                            "srv_port" : 443,
                            "srv_cert" : "{server_cert}",
                            "srv_key" : "{server_key}",
                            "cipher" : "{cipher}",
                            "tls_version" : "tls1_3",
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

def add_arguments_cb(arg_parser):
    arg_parser.add_argument('--ecdsa_cert'
                                , action="store_true"
                                , default=False
                                , help = '0/1')


if __name__ == '__main__':
    CmdArgs = utils.get_arguments("cps test", add_arguments_cb)

    cfg_j_pre = '''{
        "zones" : [
'''

    cfg_j_post = '''
        ]
}'''

    cfg_j = ''
    
    for i in range(CmdArgs.zones):
        if i:
            cfg_j = cfg_j + ','
            
        m = vars (CmdArgs)
        m['zone_id'] = i+1
        m['subnet_id'] = i+1
        m['sslv3'] = 0
        m['tls1'] = 0
        m['tls1_1'] = 0
        m['tls1_2'] = 0
        m['tls1_3'] = 0
        m['server_cert'] = '/rundir/certs/server.cert'
        m['server_key'] = '/rundir/certs/server.key'

        if CmdArgs.sslv3:
            m['sslv3'] = 1
        if CmdArgs.tls1:
            m['tls1'] = 1
        if CmdArgs.tls1_1:
            m['tls1_1'] = 1
        if CmdArgs.tls1_2:
            m['tls1_2'] = 1
        if CmdArgs.tls1_3:
            m['tls1_3'] = 1
        if CmdArgs.ecdsa_cert:
            m['server_cert'] = '/rundir/certs/server2.cert'
            m['server_key'] = '/rundir/certs/server2.key'

        cfg_j += cfj_template.format(**m)

    cfg_j = cfg_j_pre + cfg_j + cfg_j_post

    traffic_dir = os.path.join(CmdArgs.rundir
                                , 'traffic'
                                , CmdArgs.cfg_id)
                                
    os.system ( 'rm -rf {}'.format(traffic_dir) )
    os.system ( 'mkdir -p {}'.format(traffic_dir) )

    with open(os.path.join(traffic_dir, 'config.json'), 'w') as f:
        f.write(cfg_j)

    cmds.start_traffic(CmdArgs.cfg_id, CmdArgs.result_tag)
