__author__ = 'Shirish Pal'
 
import os
import argparse
import cmds
import utils

supported_ciphers = [
    {'cipher_name' : 'AES128-SHA',
        'cipher' : '{AES128-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '10.2',
        'server_ip_prefix' : '100.2'
        },

    {'cipher_name' : 'AES256-SHA',
        'cipher' : '{AES256-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '11.2',
        'server_ip_prefix' : '101.2'
        },

    {'cipher_name' : 'DHE-RSA-AES128-SHA',
        'cipher' : '{DHE-RSA-AES128-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '12.2',
        'server_ip_prefix' : '102.2'
        },

    {'cipher_name' : 'DHE-RSA-AES256-SHA',
        'cipher' : '{DHE-RSA-AES256-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '13.2',
        'server_ip_prefix' : '103.2'
        },

    {'cipher_name' : 'DHE-RSA-AES128-GCM-SHA256',
        'cipher' : '{DHE-RSA-AES128-GCM-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '14.2',
        'server_ip_prefix' : '104.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-AES128-SHA',
        'cipher' : '{ECDHE-ECDSA-AES128-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '15.2',
        'server_ip_prefix' : '105.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-AES256-SHA',
        'cipher' : '{ECDHE-ECDSA-AES256-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '16.2',
        'server_ip_prefix' : '106.2'
        },

    {'cipher_name' : 'ECDHE-RSA-AES128-SHA',
        'cipher' : '{ECDHE-RSA-AES128-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '17.2',
        'server_ip_prefix' : '107.2'
        },

    {'cipher_name' : 'ECDHE-RSA-AES256-SHA',
        'cipher' : '{ECDHE-RSA-AES256-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '18.2',
        'server_ip_prefix' : '108.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-CHACHA20-POLY1305',
        'cipher' : '{ECDHE-ECDSA-CHACHA20-POLY1305}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '19.2',
        'server_ip_prefix' : '109.2'
        },

    {'cipher_name' : 'DHE-RSA-CHACHA20-POLY1305',
        'cipher' : '{DHE-RSA-CHACHA20-POLY1305}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '20.2',
        'server_ip_prefix' : '110.2'
        },	

    {'cipher_name' : 'CAMELLIA128-SHA',
        'cipher' : '{CAMELLIA128-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '21.2',
        'server_ip_prefix' : '111.2'
        },

    {'cipher_name' : 'CAMELLIA256-SHA',
        'cipher' : '{CAMELLIA256-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '22.2',
        'server_ip_prefix' : '112.2'
        },

    {'cipher_name' : 'DHE-RSA-CAMELLIA128-SHA',
        'cipher' : '{DHE-RSA-CAMELLIA128-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '23.2',
        'server_ip_prefix' : '113.2'
        },

    {'cipher_name' : 'DHE-RSA-CAMELLIA256-SHA',
        'cipher' : '{DHE-RSA-CAMELLIA256-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '24.2',
        'server_ip_prefix' : '114.2'
        },

    {'cipher_name' : 'AES128-SHA256',
        'cipher' : '{AES128-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '25.2',
        'server_ip_prefix' : '115.2'
        },

    {'cipher_name' : 'AES256-SHA256',
        'cipher' : '{AES256-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '26.2',
        'server_ip_prefix' : '116.2'
        },

    {'cipher_name' : 'DHE-RSA-AES128-SHA256',
        'cipher' : '{DHE-RSA-AES128-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '27.2',
        'server_ip_prefix' : '117.2'
        },

    {'cipher_name' : 'AES128-GCM-SHA256',
        'cipher' : '{AES128-GCM-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '28.2',
        'server_ip_prefix' : '118.2'
        },

    {'cipher_name' : 'AES256-GCM-SHA384',
        'cipher' : '{AES256-GCM-SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '29.2',
        'server_ip_prefix' : '119.2'
        },

    {'cipher_name' : 'ECDHE-RSA-AES128-GCM-SHA256',
        'cipher' : '{ECDHE-RSA-AES128-GCM-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '30.2',
        'server_ip_prefix' : '120.2'
        },

    {'cipher_name' : 'ECDHE-RSA-AES256-GCM-SHA384',
        'cipher' : '{ECDHE-RSA-AES256-GCM-SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '31.2',
        'server_ip_prefix' : '121.2'
        },

    {'cipher_name' : 'ECDHE-RSA-AES128-SHA256',
        'cipher' : '{ECDHE-RSA-AES128-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '32.2',
        'server_ip_prefix' : '122.2'
        },

    {'cipher_name' : 'ECDHE-RSA-AES256-SHA384',
        'cipher' : '{ECDHE-RSA-AES256-SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '33.2',
        'server_ip_prefix' : '123.2'
        },

    {'cipher_name' : 'DHE-RSA-AES256-SHA256',
        'cipher' : '{DHE-RSA-AES256-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '34.2',
        'server_ip_prefix' : '124.2'
        },

    {'cipher_name' : 'DHE-RSA-AES256-GCM-SHA384',
        'cipher' : '{DHE-RSA-AES256-GCM-SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '35.2',
        'server_ip_prefix' : '125.2'
        },

    {'cipher_name' : 'ECDHE-RSA-CHACHA20-POLY1305',
        'cipher' : '{ECDHE-RSA-CHACHA20-POLY1305}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '36.2',
        'server_ip_prefix' : '126.2'
        },

    {'cipher_name' : 'TLS_AES_128_GCM_SHA256',
        'cipher' : '{TLS_AES_128_GCM_SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : 0,
        'tls1_3' : '{tls1_3}',
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '37.2',
        'server_ip_prefix' : '127.2'
        },

    {'cipher_name' : 'TLS_AES_256_GCM_SHA384',
        'cipher' : '{TLS_AES_256_GCM_SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : 0,
        'tls1_3' : '{tls1_3}',
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '38.2',
        'server_ip_prefix' : '128.2'
        },

    {'cipher_name' : 'TLS_CHACHA20_POLY1305_SHA256',
        'cipher' : '{TLS_CHACHA20_POLY1305_SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : 0,
        'tls1_3' : '{tls1_3}',
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '39.2',
        'server_ip_prefix' : '129.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-AES128-GCM-SHA256',
        'cipher' : '{ECDHE-ECDSA-AES128-GCM-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '40.2',
        'server_ip_prefix' : '130.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-AES256-GCM-SHA384',
        'cipher' : '{ECDHE-ECDSA-AES256-GCM-SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '41.2',
        'server_ip_prefix' : '131.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-AES128-SHA256',
        'cipher' : '{ECDHE-ECDSA-AES128-SHA256}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '42.2',
        'server_ip_prefix' : '132.2'
        },

    {'cipher_name' : 'ECDHE-ECDSA-AES256-SHA384',
        'cipher' : '{ECDHE-ECDSA-AES256-SHA384}',
        'sslv3': 0,
        'tls1': 0,
        'tls1_1': 0,
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server2.cert',
        'srv_key' : '/rundir/certs/server2.key',
        'client_ip_prefix' : '43.2',
        'server_ip_prefix' : '133.2'
        },

    {'cipher_name' : 'RC4-MD5',
        'cipher' : '{RC4-MD5}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '44.2',
        'server_ip_prefix' : '134.2'
        },

    {'cipher_name' : 'RC4-SHA',
        'cipher' : '{RC4-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '45.2',
        'server_ip_prefix' : '135.2'
        },

    {'cipher_name' : 'DES-CBC-SHA',
        'cipher' : '{DES-CBC-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '46.2',
        'server_ip_prefix' : '136.2'
        },

    {'cipher_name' : 'DES-CBC3-SHA',
        'cipher' : '{DES-CBC3-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '47.2',
        'server_ip_prefix' : '137.2'
        },

    {'cipher_name' : 'SEED-SHA',
        'cipher' : '{SEED-SHA}',
        'sslv3' : '{sslv3}',
        'tls1' : '{tls1}',
        'tls1_1' : '{tls1_1}',
        'tls1_2' : '{tls1_2}',
        'tls1_3' : 0,
        'srv_cert' : '/rundir/certs/server.cert',
        'srv_key' : '/rundir/certs/server.key',
        'client_ip_prefix' : '48.2',
        'server_ip_prefix' : '138.2'}
]

tls_client_template = '''
    {{{{
        "app_type" : "tls_client",
        "app_label" : "{cipher_name}-CLIENT",
        "enable" : {cipher},
        "conn_per_sec" : {{cps}},
        "max_pending_conn_count" : {{max_pipeline}},
        "max_active_conn_count" : {{max_active}},
        "total_conn_count" : 0,
        "cs_grp_list" : [
            {{{{
                "cs_grp_label" : "cs_grp_1",
                "enable" : {sslv3},
                "srv_ip"   : "{server_ip_prefix}{{subnet_id}}.51.1",
                "srv_port" : 443,
                "clnt_ip_begin" : "{client_ip_prefix}{{subnet_id}}.51.1",
                "clnt_ip_end" : "{client_ip_prefix}{{subnet_id}}.51.10",
                "clnt_port_begin" : 5000,
                "clnt_port_end" : 8000,
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "cs_grp_label" : "cs_grp_2",
                "enable" : {tls1},
                "srv_ip"   : "{server_ip_prefix}{{subnet_id}}.51.2",
                "srv_port" : 443,
                "clnt_ip_begin" : "{client_ip_prefix}{{subnet_id}}.51.11",
                "clnt_ip_end" : "{client_ip_prefix}{{subnet_id}}.51.20",
                "clnt_port_begin" : 5000,
                "clnt_port_end" : 8000,
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "cs_grp_label" : "cs_grp_3",
                "enable" : {tls1_1},
                "srv_ip"   : "{server_ip_prefix}{{subnet_id}}.51.3",
                "srv_port" : 443,
                "clnt_ip_begin" : "{client_ip_prefix}{{subnet_id}}.51.21",
                "clnt_ip_end" : "{client_ip_prefix}{{subnet_id}}.51.30",
                "clnt_port_begin" : 5000,
                "clnt_port_end" : 8000,
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "cs_grp_label" : "cs_grp_4",
                "enable" : {tls1_2},
                "srv_ip"   : "{server_ip_prefix}{{subnet_id}}.51.4",
                "srv_port" : 443,
                "clnt_ip_begin" : "{client_ip_prefix}{{subnet_id}}.51.31",
                "clnt_ip_end" : "{client_ip_prefix}{{subnet_id}}.51.40",
                "clnt_port_begin" : 5000,
                "clnt_port_end" : 8000,
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "cs_grp_label" : "cs_grp_5",
                "enable" : {tls1_3},
                "srv_ip"   : "{server_ip_prefix}{{subnet_id}}.51.5",
                "srv_port" : 443,
                "clnt_ip_begin" : "{client_ip_prefix}{{subnet_id}}.51.41",
                "clnt_ip_end" : "{client_ip_prefix}{{subnet_id}}.51.50",
                "clnt_port_begin" : 5000,
                "clnt_port_end" : 8000,
                "cipher" : "{cipher_name}",
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
            }}}}
        ]
    }}}}
'''

tls_server_template = '''
    {{{{
        "app_type" : "tls_server",
        "app_label" : "{cipher_name}-SERVER",
        "enable" : {cipher},
        "srv_list" : [
            {{{{
                "srv_label" : "{server_ip_prefix}{{subnet_id}}.51.1",
                "enable" : {sslv3},
                "srv_ip" : "{server_ip_prefix}{{subnet_id}}.51.1",
                "srv_port" : 443,
                "srv_cert" : "{srv_cert}",
                "srv_key" : "{srv_key}",
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "srv_label" : "{server_ip_prefix}{{subnet_id}}.51.2",
                "enable" : {tls1},
                "srv_ip" : "{server_ip_prefix}{{subnet_id}}.51.2",
                "srv_port" : 443,
                "srv_cert" : "{srv_cert}",
                "srv_key" : "{srv_key}",
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "srv_label" : "{server_ip_prefix}{{subnet_id}}.51.3",
                "enable" : {tls1_1},
                "srv_ip" : "{server_ip_prefix}{{subnet_id}}.51.3",
                "srv_port" : 443,
                "srv_cert" : "{srv_cert}",
                "srv_key" : "{srv_key}",
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "srv_label" : "{server_ip_prefix}{{subnet_id}}.51.4",
                "enable" : {tls1_2},
                "srv_ip" : "{server_ip_prefix}{{subnet_id}}.51.4",
                "srv_port" : 443,
                "srv_cert" : "{srv_cert}",
                "srv_key" : "{srv_key}",
                "cipher" : "{cipher_name}",
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
            }}}},
            {{{{
                "srv_label" : "{server_ip_prefix}{{subnet_id}}.51.5",
                "enable" : {tls1_3},
                "srv_ip" : "{server_ip_prefix}{{subnet_id}}.51.5",
                "srv_port" : 443,
                "srv_cert" : "{srv_cert}",
                "srv_key" : "{srv_key}",
                "cipher" : "{cipher_name}",
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
            }}}}
        ]
    }}}}
'''

cfj_template_part1 = '''
        {{
            "zone_label" : "zone-{zone_id}-a",
            "enable" : 1,
            "zone_type" : "single-interface",
            "iface" : "{iface}",
            "macvlan" : "{macvlan1}",
            "tcpdump" : "-c 1000",
            "subnets" : ["10.2{subnet_id}.51.0/24" , "11.2{subnet_id}.51.0/24" , "12.2{subnet_id}.51.0/24" , 
                        "13.2{subnet_id}.51.0/24" , "14.2{subnet_id}.51.0/24" , "15.2{subnet_id}.51.0/24" , 
                        "16.2{subnet_id}.51.0/24" , "17.2{subnet_id}.51.0/24" , "18.2{subnet_id}.51.0/24" , 
                        "19.2{subnet_id}.51.0/24" , "20.2{subnet_id}.51.0/24" , "21.2{subnet_id}.51.0/24" , 
                        "22.2{subnet_id}.51.0/24" , "23.2{subnet_id}.51.0/24" , "24.2{subnet_id}.51.0/24" , 
                        "25.2{subnet_id}.51.0/24" , "26.2{subnet_id}.51.0/24" , "27.2{subnet_id}.51.0/24" , 
                        "28.2{subnet_id}.51.0/24" , "29.2{subnet_id}.51.0/24" , "30.2{subnet_id}.51.0/24" , 
                        "31.2{subnet_id}.51.0/24" , "32.2{subnet_id}.51.0/24" , "33.2{subnet_id}.51.0/24" , 
                        "34.2{subnet_id}.51.0/24" , "35.2{subnet_id}.51.0/24" , "36.2{subnet_id}.51.0/24" , 
                        "37.2{subnet_id}.51.0/24" , "38.2{subnet_id}.51.0/24" , "39.2{subnet_id}.51.0/24" , 
                        "40.2{subnet_id}.51.0/24" , "41.2{subnet_id}.51.0/24" , "42.2{subnet_id}.51.0/24" , 
                        "43.2{subnet_id}.51.0/24" , "44.2{subnet_id}.51.0/24" , "45.2{subnet_id}.51.0/24" , 
                        "46.2{subnet_id}.51.0/24" , "47.2{subnet_id}.51.0/24" , "48.2{subnet_id}.51.0/24" ],
            "app_list" : [
'''

cfj_template_part3 = '''
            ]       
        }},

        {{
            "zone_label" : "zone-{zone_id}-b",
            "enable" : 1,
            "zone_type" : "single-interface",
            "iface" : "{iface}",
            "macvlan" : "{macvlan2}",
            "tcpdump" : "-c 1000",
            "subnets" : ["100.2{subnet_id}.51.0/24" , "101.2{subnet_id}.51.0/24" , "102.2{subnet_id}.51.0/24" , 
                        "103.2{subnet_id}.51.0/24" , "104.2{subnet_id}.51.0/24" , "105.2{subnet_id}.51.0/24" , 
                        "106.2{subnet_id}.51.0/24" , "107.2{subnet_id}.51.0/24" , "108.2{subnet_id}.51.0/24" , 
                        "109.2{subnet_id}.51.0/24" , "110.2{subnet_id}.51.0/24" , "111.2{subnet_id}.51.0/24" , 
                        "112.2{subnet_id}.51.0/24" , "113.2{subnet_id}.51.0/24" , "114.2{subnet_id}.51.0/24" , 
                        "115.2{subnet_id}.51.0/24" , "116.2{subnet_id}.51.0/24" , "117.2{subnet_id}.51.0/24" , 
                        "118.2{subnet_id}.51.0/24" , "119.2{subnet_id}.51.0/24" , "120.2{subnet_id}.51.0/24" , 
                        "121.2{subnet_id}.51.0/24" , "122.2{subnet_id}.51.0/24" , "123.2{subnet_id}.51.0/24" , 
                        "124.2{subnet_id}.51.0/24" , "125.2{subnet_id}.51.0/24" , "126.2{subnet_id}.51.0/24" , 
                        "127.2{subnet_id}.51.0/24" , "128.2{subnet_id}.51.0/24" , "129.2{subnet_id}.51.0/24" , 
                        "130.2{subnet_id}.51.0/24" , "131.2{subnet_id}.51.0/24" , "132.2{subnet_id}.51.0/24" , 
                        "133.2{subnet_id}.51.0/24" , "134.2{subnet_id}.51.0/24" , "135.2{subnet_id}.51.0/24" , 
                        "136.2{subnet_id}.51.0/24" , "137.2{subnet_id}.51.0/24" , "138.2{subnet_id}.51.0/24" ],
            "app_list" : [
'''

cfj_template_part5 = '''
            ]   
        }}
'''

def add_arguments_cb(arg_parser):
    pass


if __name__ == '__main__':
    CmdArgs = utils.get_arguments("mixed cipher test", add_arguments_cb)

    cfg_j_pre = '''{
        "zones" : [
'''

    cfg_j_post = '''
        ]
}'''

    cfg_j = ''
    
    cfj_template_part2 = ''
    cfj_template_part4 = ''
    for i in range(len(supported_ciphers)):
        if i:
            cfj_template_part2 += ','
            cfj_template_part4 += ','
        cfj_template_part2 += tls_client_template.format(**supported_ciphers[i])
        cfj_template_part4 += tls_server_template.format(**supported_ciphers[i])


    cfj_template = cfj_template_part1 + cfj_template_part2 + cfj_template_part3 + cfj_template_part4 + cfj_template_part5

    for i in range(CmdArgs.zones):
        if i:
            cfg_j = cfg_j + ','
        m = vars (CmdArgs)
        m['zone_id'] = i+1
        m['subnet_id'] = i+1        
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
