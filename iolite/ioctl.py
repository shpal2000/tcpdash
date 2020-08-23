__author__ = 'Shirish Pal'

import os
import subprocess
import sys
import argparse
import json
import jinja2
import time
import pdb

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
        'server_ip_prefix' : '139.2'
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


def add_cps_params (cmd_parser):
    cmd_parser.add_argument('--ecdsa_cert'
                                , action="store_true"
                                , default=False
                                , help = '0/1')

def process_cps_template (cmd_args):
    tlspack_cfg = jinja2.Template('''
    {
        "tgen_app" : "cps",
        "zones" : [
            {% set ns = namespace(cs_grp_count=0, srv_count=0) %}
            {%- for zone_id in range(1, PARAMS.zones+1) %}
                {
                    "zone_label" : "zone-{{zone_id}}-client",
                    "enable" : 1,
                    "app_list" : [
                        {
                            "app_type" : "tls_client",
                            "app_label" : "tls_client_1",
                            "enable" : 1,
                            "conn_per_sec" : {{PARAMS.cps}},
                            "max_pending_conn_count" : {{PARAMS.max_pipeline}},
                            "max_active_conn_count" : {{PARAMS.max_active}},
                            "total_conn_count" : {{PARAMS.total_conn_count}},
                            "cs_grp_list" : [
                                {% set ns.cs_grp_count = 0 %}
                                {%- for tls_ver in ['sslv3', 'tls1', 'tls1_1', 'tls1_2', 'tls1_3'] %}
                                    {%- if (tls_ver == 'sslv3' and PARAMS.sslv3) 
                                            or (tls_ver == 'tls1' and PARAMS.tls1) 
                                            or (tls_ver == 'tls1_1' and PARAMS.tls1_1) 
                                            or (tls_ver == 'tls1_2' and PARAMS.tls1_2) 
                                            or (tls_ver == 'tls1_3' and PARAMS.tls1_3) %}
                                        {{ "," if ns.cs_grp_count }}
                                        {% set ns.cs_grp_count = ns.cs_grp_count+1 %}
                                        {
                                            "cs_grp_label" : "cs_grp_{{loop.index}}",
                                            "enable" : 1,
                                            "srv_ip"   : "14.2{{zone_id}}.51.{{loop.index}}",
                                            "srv_port" : 443,
                                            "clnt_ip_begin" : "12.2{{zone_id}}.51.{{1+loop.index0*10}}",
                                            "clnt_ip_end" : "12.2{{zone_id}}.51.{{loop.index*10}}",
                                            "clnt_port_begin" : 5000,
                                            "clnt_port_end" : 65000,
                                            "cipher" : "{{PARAMS.cipher}}",
                                            "tls_version" : "{{tls_ver}}",
                                            "close_type" : "fin",
                                            "close_notify" : "no_send",
                                            "app_rcv_buff" : {{PARAMS.app_rcv_buff}},
                                            "app_snd_buff" : {{PARAMS.app_snd_buff}},
                                            "write_chunk" : {{PARAMS.app_next_write}},
                                            "tcp_rcv_buff" : {{PARAMS.tcp_rcv_buff}},
                                            "tcp_snd_buff" : {{PARAMS.tcp_snd_buff}},
                                            "cs_data_len" : {{PARAMS.app_cs_data_len}},
                                            "sc_data_len" : {{PARAMS.app_sc_data_len}},
                                            "cs_start_tls_len" : 0,
                                            "sc_start_tls_len" : 0
                                        }
                                    {%- endif %}
                                {%- endfor %}                         
                            ]
                        }
                    ],

                    "host_cmds" : [
                        "sudo ip link set dev {{PARAMS.na_iface}} up",
                        "sudo docker network connect {{PARAMS.na_macvlan}} {{PARAMS.runtag}}-zone-{{zone_id}}-client"
                    ],

                    "zone_cmds" : [
                        "ip link set dev {{PARAMS.iface_container}} up",
                        "ifconfig {{PARAMS.iface_container}} hw ether {{PARAMS.client_mac_seed}}:{{'{:02x}'.format(zone_id)}}",
                        "ip route add default dev {{PARAMS.iface_container}} table 200",
                        "ip -4 route add local 12.2{{zone_id}}.51.0/24 dev lo",
                        "ip rule add from 12.2{{zone_id}}.51.0/24 table 200",
                        "tcpdump -i {{PARAMS.iface_container}} {{PARAMS.tcpdump}} -w {{PARAMS.result_dir_container}}/zone-{{zone_id}}-client/init.pcap &"
                    ]
                }
                ,
                {
                    "zone_label" : "zone-{{zone_id}}-server",
                    "enable" : 1,
                    "iface" : "{{PARAMS.iface_container}}",
                    "tcpdump" : "{{PARAMS.tcpdump}}",
                    "app_list" : [
                        {
                            "app_type" : "tls_server",
                            "app_label" : "tls_server_1",
                            "enable" : 1,
                            "srv_list" : [
                                {% set ns.srv_count = 0 %}
                                {%- for tls_ver in ['sslv3', 'tls1', 'tls1_1', 'tls1_2', 'tls1_3'] %}
                                    {%- if (tls_ver == 'sslv3' and PARAMS.sslv3) 
                                            or (tls_ver == 'tls1' and PARAMS.tls1) 
                                            or (tls_ver == 'tls1_1' and PARAMS.tls1_1) 
                                            or (tls_ver == 'tls1_2' and PARAMS.tls1_2) 
                                            or (tls_ver == 'tls1_3' and PARAMS.tls1_3) %}
                                        {{ "," if ns.srv_count }}
                                        {% set ns.srv_count = ns.srv_count+1 %}
                                        {
                                            "srv_label" : "srv_{{loop.index}}",
                                            "enable" : 1,
                                            "srv_ip" : "14.2{{zone_id}}.51.{{loop.index}}",
                                            "srv_port" : 443,
                                            "srv_cert" : "{{PARAMS.server_cert}}",
                                            "srv_key" : "{{PARAMS.server_key}}",
                                            "cipher" : "{{PARAMS.cipher}}",
                                            "tls_version" : "{{tls_ver}}",
                                            "close_type" : "fin",
                                            "close_notify" : "no_send",
                                            "app_rcv_buff" : {{PARAMS.app_rcv_buff}},
                                            "app_snd_buff" : {{PARAMS.app_snd_buff}},
                                            "write_chunk" : {{PARAMS.app_next_write}},
                                            "tcp_rcv_buff" : {{PARAMS.tcp_rcv_buff}},
                                            "tcp_snd_buff" : {{PARAMS.tcp_snd_buff}},
                                            "cs_data_len" : {{PARAMS.app_cs_data_len}},
                                            "sc_data_len" : {{PARAMS.app_sc_data_len}},
                                            "cs_start_tls_len" : 0,
                                            "sc_start_tls_len" : 0
                                        }
                                    {%- endif %}
                                {%- endfor %}
                            ]
                        }
                    ],

                    "host_cmds" : [
                        "sudo ip link set dev {{PARAMS.nb_iface}} up",
                        "sudo docker network connect {{PARAMS.nb_macvlan}} {{PARAMS.runtag}}-zone-{{zone_id}}-server"
                    ],

                    "zone_cmds" : [
                        "ip link set dev {{PARAMS.iface_container}} up",
                        "ifconfig {{PARAMS.iface_container}} hw ether {{PARAMS.server_mac_seed}}:{{'{:02x}'.format(zone_id)}}",
                        "ip route add default dev {{PARAMS.iface_container}} table 200",
                        "ip -4 route add local 14.2{{zone_id}}.51.0/24 dev lo",
                        "ip rule add from 14.2{{zone_id}}.51.0/24 table 200",
                        "tcpdump -i {{PARAMS.iface_container}} {{PARAMS.tcpdump}} -w {{PARAMS.result_dir_container}}/zone-{{zone_id}}-server/init.pcap &"
                    ]
                }
                {{ "," if not loop.last }}
            {%- endfor %}
        ]
    }
    ''')
    if cmd_args.ecdsa_cert:
        cmd_args.server_cert = '/rundir/certs/server2.cert'
        cmd_args.server_key = '/rundir/certs/server2.key'
    else:
        cmd_args.server_cert = '/rundir/certs/server.cert'
        cmd_args.server_key = '/rundir/certs/server.key'
    return tlspack_cfg.render(PARAMS = cmd_args)

def process_cps_stats(result_dir):
    ev_sockstats_client_list = []
    ev_sockstats_server_list = []

    result_dir_contents = []
    try:
        result_dir_contents = os.listdir(result_dir)
    except:
        pass

    for zone_dir in result_dir_contents:
        zone_dir_path = os.path.join(result_dir, zone_dir)
        if os.path.isdir(zone_dir_path):
            ev_sockstats_json_file = os.path.join (zone_dir_path
                                            , 'ev_sockstats.json')
            try:
                with open(ev_sockstats_json_file) as f:
                    stats_j = json.load(f)
                    if zone_dir.endswith('-client'):
                        ev_sockstats_client_list.append (stats_j)
                    if zone_dir.endswith('-server'):
                        ev_sockstats_server_list.append (stats_j)
            except:
                ev_sockstats_client_list = []
                ev_sockstats_server_list = []
                break

    if ev_sockstats_client_list:
        ev_sockstats = ev_sockstats_client_list.pop()
        while ev_sockstats_client_list:
            next_ev_sockstats = ev_sockstats_client_list.pop()
            for k, v in next_ev_sockstats.items():
                ev_sockstats[k] += v
        with open(os.path.join(result_dir, 'ev_sockstats_client.json'), 'w') as f:
            json.dump(ev_sockstats, f)

    if ev_sockstats_server_list:
        ev_sockstats = ev_sockstats_server_list.pop()
        while ev_sockstats_server_list:
            next_ev_sockstats = ev_sockstats_server_list.pop()
            for k, v in next_ev_sockstats.items():
                ev_sockstats[k] += v
        with open(os.path.join(result_dir, 'ev_sockstats_server.json'), 'w') as f:
            json.dump(ev_sockstats, f)



def add_bw_params (cmd_parser):
    cmd_parser.add_argument('--ecdsa_cert'
                                , action="store_true"
                                , default=False
                                , help = '0/1')

def process_bw_template (cmd_args):

    tlspack_cfg = jinja2.Template('''
    {
        "tgen_app" : "bw",
        "zones" : [
            {% set ns = namespace(cs_grp_count=0, srv_count=0) %}
            {%- for zone_id in range(1, PARAMS.zones+1) %}
                {
                    "zone_label" : "zone-{{zone_id}}-client",
                    "enable" : 1,

                    "app_list" : [
                        {
                            "app_type" : "tls_client",
                            "app_label" : "tls_client_1",
                            "enable" : 1,
                            "conn_per_sec" : {{PARAMS.cps}},
                            "max_pending_conn_count" : {{PARAMS.max_pipeline}},
                            "max_active_conn_count" : {{PARAMS.max_active}},
                            "total_conn_count" : {{PARAMS.total_conn_count}},
                            "cs_grp_list" : [
                                {% set ns.cs_grp_count = 0 %}
                                {%- for tls_ver in ['sslv3', 'tls1', 'tls1_1', 'tls1_2', 'tls1_3'] %}
                                    {%- if (tls_ver == 'sslv3' and PARAMS.sslv3) 
                                            or (tls_ver == 'tls1' and PARAMS.tls1) 
                                            or (tls_ver == 'tls1_1' and PARAMS.tls1_1) 
                                            or (tls_ver == 'tls1_2' and PARAMS.tls1_2) 
                                            or (tls_ver == 'tls1_3' and PARAMS.tls1_3) %}
                                        {{ "," if ns.cs_grp_count }}
                                        {% set ns.cs_grp_count = ns.cs_grp_count+1 %}
                                        {
                                            "cs_grp_label" : "cs_grp_{{loop.index}}",
                                            "enable" : 1,
                                            "srv_ip"   : "24.2{{zone_id}}.51.{{loop.index}}",
                                            "srv_port" : 443,
                                            "clnt_ip_begin" : "22.2{{zone_id}}.51.{{1+loop.index0*10}}",
                                            "clnt_ip_end" : "22.2{{zone_id}}.51.{{loop.index*10}}",
                                            "clnt_port_begin" : 5000,
                                            "clnt_port_end" : 65000,
                                            "cipher" : "{{PARAMS.cipher}}",
                                            "tls_version" : "{{tls_ver}}",
                                            "close_type" : "reset",
                                            "close_notify" : "no_send",
                                            "app_rcv_buff" : {{PARAMS.app_rcv_buff}},
                                            "app_snd_buff" : {{PARAMS.app_snd_buff}},
                                            "write_chunk" : {{PARAMS.app_next_write}},
                                            "tcp_rcv_buff" : {{PARAMS.tcp_rcv_buff}},
                                            "tcp_snd_buff" : {{PARAMS.tcp_snd_buff}},
                                            "cs_data_len" : {{PARAMS.app_cs_data_len}},
                                            "sc_data_len" : {{PARAMS.app_sc_data_len}},
                                            "cs_start_tls_len" : 0,
                                            "sc_start_tls_len" : 0
                                        }
                                    {%- endif %}
                                {%- endfor %}                         
                            ]
                        }
                    ],

                    "host_cmds" : [
                        "sudo ip link set dev {{PARAMS.na_iface}} up",
                        "sudo docker network connect {{PARAMS.na_macvlan}} {{PARAMS.runtag}}-zone-{{zone_id}}-client"
                    ],

                    "zone_cmds" : [
                        "ip link set dev {{PARAMS.iface_container}} up",
                        "ifconfig {{PARAMS.iface_container}} hw ether {{PARAMS.client_mac_seed}}:{{'{:02x}'.format(zone_id)}}",
                        "ip route add default dev {{PARAMS.iface_container}} table 200",
                        "ip -4 route add local 22.2{{zone_id}}.51.0/24 dev lo",
                        "ip rule add from 22.2{{zone_id}}.51.0/24 table 200",
                        "tcpdump -i {{PARAMS.iface_container}} {{PARAMS.tcpdump}} -w {{PARAMS.result_dir_container}}/zone-{{zone_id}}-client/init.pcap &"
                    ]                    
                }
                ,
                {
                    "zone_label" : "zone-{{zone_id}}-server",
                    "enable" : 1,

                    "app_list" : [
                        {
                            "app_type" : "tls_server",
                            "app_label" : "tls_server_1",
                            "enable" : 1,
                            "srv_list" : [
                                {% set ns.srv_count = 0 %}
                                {%- for tls_ver in ['sslv3', 'tls1', 'tls1_1', 'tls1_2', 'tls1_3'] %}
                                    {%- if (tls_ver == 'sslv3' and PARAMS.sslv3) 
                                            or (tls_ver == 'tls1' and PARAMS.tls1) 
                                            or (tls_ver == 'tls1_1' and PARAMS.tls1_1) 
                                            or (tls_ver == 'tls1_2' and PARAMS.tls1_2) 
                                            or (tls_ver == 'tls1_3' and PARAMS.tls1_3) %}
                                        {{ "," if ns.srv_count }}
                                        {% set ns.srv_count = ns.srv_count+1 %}
                                        {
                                            "srv_label" : "srv_{{loop.index}}",
                                            "enable" : 1,
                                            "srv_ip" : "24.2{{zone_id}}.51.{{loop.index}}",
                                            "srv_port" : 443,
                                            "srv_cert" : "{{PARAMS.server_cert}}",
                                            "srv_key" : "{{PARAMS.server_key}}",
                                            "cipher" : "{{PARAMS.cipher}}",
                                            "tls_version" : "{{tls_ver}}",
                                            "close_type" : "reset",
                                            "close_notify" : "no_send",
                                            "app_rcv_buff" : {{PARAMS.app_rcv_buff}},
                                            "app_snd_buff" : {{PARAMS.app_snd_buff}},
                                            "write_chunk" : {{PARAMS.app_next_write}},
                                            "tcp_rcv_buff" : {{PARAMS.tcp_rcv_buff}},
                                            "tcp_snd_buff" : {{PARAMS.tcp_snd_buff}},
                                            "cs_data_len" : {{PARAMS.app_cs_data_len}},
                                            "sc_data_len" : {{PARAMS.app_sc_data_len}},
                                            "cs_start_tls_len" : 0,
                                            "sc_start_tls_len" : 0
                                        }
                                    {%- endif %}
                                {%- endfor %}
                            ]
                        }
                    ],

                    "host_cmds" : [
                        "sudo ip link set dev {{PARAMS.nb_iface}} up",
                        "sudo docker network connect {{PARAMS.nb_macvlan}} {{PARAMS.runtag}}-zone-{{zone_id}}-server"
                    ],

                    "zone_cmds" : [
                        "ip link set dev {{PARAMS.iface_container}} up",
                        "ifconfig {{PARAMS.iface_container}} hw ether {{PARAMS.server_mac_seed}}:{{'{:02x}'.format(zone_id)}}",
                        "ip route add default dev {{PARAMS.iface_container}} table 200",
                        "ip -4 route add local 24.2{{zone_id}}.51.0/24 dev lo",
                        "ip rule add from 24.2{{zone_id}}.51.0/24 table 200",
                        "tcpdump -i {{PARAMS.iface_container}} {{PARAMS.tcpdump}} -w {{PARAMS.result_dir_container}}/zone-{{zone_id}}-server/init.pcap &"
                    ]
                }
                {{ "," if not loop.last }}
            {%- endfor %}
        ]
    }
    ''')

    if cmd_args.ecdsa_cert:
        cmd_args.server_cert = '/rundir/certs/server2.cert'
        cmd_args.server_key = '/rundir/certs/server2.key'
    else:
        cmd_args.server_cert = '/rundir/certs/server.cert'
        cmd_args.server_key = '/rundir/certs/server.key'
    return tlspack_cfg.render(PARAMS = cmd_args)

def process_bw_stats(result_dir):
    ev_sockstats_client_list = []
    ev_sockstats_server_list = []

    result_dir_contents = []
    try:
        result_dir_contents = os.listdir(result_dir)
    except:
        pass

    for zone_dir in result_dir_contents:
        zone_dir_path = os.path.join(result_dir, zone_dir)
        if os.path.isdir(zone_dir_path):
            ev_sockstats_json_file = os.path.join (zone_dir_path
                                            , 'ev_sockstats.json')
            try:
                with open(ev_sockstats_json_file) as f:
                    stats_j = json.load(f)
                    if zone_dir.endswith('-client'):
                        ev_sockstats_client_list.append (stats_j)
                    if zone_dir.endswith('-server'):
                        ev_sockstats_server_list.append (stats_j)
            except:
                ev_sockstats_client_list = []
                ev_sockstats_server_list = []
                break
    if ev_sockstats_client_list:
        ev_sockstats = ev_sockstats_client_list.pop()
        while ev_sockstats_client_list:
            next_ev_sockstats = ev_sockstats_client_list.pop()
            for k, v in next_ev_sockstats.items():
                ev_sockstats[k] += v
        with open(os.path.join(result_dir, 'ev_sockstats_client.json'), 'w') as f:
            json.dump(ev_sockstats, f)

    if ev_sockstats_server_list:
        ev_sockstats = ev_sockstats_server_list.pop()
        while ev_sockstats_server_list:
            next_ev_sockstats = ev_sockstats_server_list.pop()
            for k, v in next_ev_sockstats.items():
                ev_sockstats[k] += v
        with open(os.path.join(result_dir, 'ev_sockstats_server.json'), 'w') as f:
            json.dump(ev_sockstats, f)



def add_tproxy_params (cmd_parser):
    cmd_parser.add_argument('--runtag'
                                , action="store"
                                , required=True
                                , help = 'config id')

    cmd_parser.add_argument('--result_tag'
                                , action="store"
                                , default='latest'
                                , help = 'result tag')

    cmd_parser.add_argument('--rundir'
                                , action="store"
                                , default='/root/rundir'
                                , help = 'macvlan1 name')

    cmd_parser.add_argument('--rundir_container'
                                , action="store"
                                , default='/rundir'
                                , help = 'rundir path in container')

    cmd_parser.add_argument('--debug'
                                , action="store"
                                , type=int
                                , dest='is_debug'
                                , default=0
                                , help = '0;1;2')

    cmd_parser.add_argument('--host_src_dir'
                                , action="store"
                                , help = 'host src dir for debuging'
                                , default='/root/tcpdash')

    cmd_parser.add_argument('--issl_tool_vlan'
                                , action="store"
                                , type=int
                                , required=True
                                , help = '1-4095')

    cmd_parser.add_argument('--ta'
                                , action="store"
                                , required=True
                                , dest = 'ta_iface'
                                , help = 'ta host interface')

    cmd_parser.add_argument('--tb'
                                , action="store"
                                , required=True
                                , dest = 'tb_iface'
                                , help = 'tb host interface')

    cmd_parser.add_argument('--ta_macvlan'
                                , action="store"
                                , default=''
                                , help = 'ta host macvlan')

    cmd_parser.add_argument('--tb_macvlan'
                                , action="store"
                                , default=''
                                , help = 'tb host macvlan')

    cmd_parser.add_argument('--ta_iface_container'
                                , action="store"
                                , help = 'ta interface'
                                , default='eth1')

    cmd_parser.add_argument('--tb_iface_container'
                                , action="store"
                                , help = 'tb interface'
                                , default='eth2')

    cmd_parser.add_argument('--ta_subnet'
                                , action="store"
                                , help = 'ta subnet'
                                , required=True)

    cmd_parser.add_argument('--tb_subnet'
                                , action="store"
                                , help = 'tb subnet'
                                , required=True)

    cmd_parser.add_argument('--ta_tcpdump'
                                , action="store"
                                , help = 'ta tcpdump'
                                , default='-c 100')

    cmd_parser.add_argument('--tb_tcpdump'
                                , action="store"
                                , help = 'tb tcpdump'
                                , default='-c 100')

    cmd_parser.add_argument('--client_mac_seed'
                                , action="store"
                                , help = '5 bytes'
                                , default='02:42:ac:14:00')

    cmd_parser.add_argument('--server_mac_seed'
                                , action="store"
                                , help = '5 bytes'
                                , default='02:42:ac:15:00')


def process_tproxy_template (cmd_args):
    tlspack_cfg = jinja2.Template ('''{
        "tgen_app" : "tproxy",
        "zones" : [
            {
                "zone_label" : "zone-1-proxy",
                "enable" : 1,
            
                "app_list" : [
                    {
                        "app_type" : "tcp_proxy",
                        "app_label" : "tcp_proxy_1",
                        "enable" : 1,

                        "proxy_list" : [
                            {
                                "proxy_label" : "bae-issue",
                                "enable" : 1,

                                "proxy_ip" : "0.0.0.0",
                                "proxy_port" : 883,
                                "proxy_type_id" : 1,

                                "tcp_rcv_buff" : 0,
                                "tcp_snd_buff : 0
                            }
                        ]
                    }
                ]
                
                "host_cmds" : [
                    "sudo ip link set dev {{PARAMS.ta_iface}} up",
                    "sudo ip link set dev {{PARAMS.tb_iface}} up",
                    "sudo docker network connect {{PARAMS.ta_macvlan}} {{PARAMS.runtag}}-proxy-1",
                    "sudo docker network connect {{PARAMS.tb_macvlan}} {{PARAMS.runtag}}-proxy-1"
                ],

                "zone_cmds" : [
                    "sysctl net.ipv4.conf.all.rp_filter=0",
                    "sysctl net.ipv4.conf.default.rp_filter=0",

                    "ip link set dev {{PARAMS.ta_iface_container}} up",
                    "ifconfig {{PARAMS.ta_iface_container}} hw ether {{PARAMS.server_mac_seed}}:{{'{:02x}'.format(1)}}",
                    "sysctl net.ipv4.conf.{{PARAMS.ta_iface_container}}.rp_filter=0",
                    "ip link add link {{PARAMS.ta_iface_container}} name {{PARAMS.ta_iface_container}}.{{PARAMS.issl_tool_vlan}} type vlan id {{PARAMS.issl_tool_vlan}}",
                    "ip link set dev {{PARAMS.ta_iface_container}}.{{PARAMS.issl_tool_vlan}} up",
                    "ip addr add 1.1.1.1/24 dev {{PARAMS.ta_iface_container}}.{{PARAMS.issl_tool_vlan}}",
                    "arp -i {{PARAMS.ta_iface_container}}.{{PARAMS.issl_tool_vlan}} -s 1.1.1.254 {{PARAMS.client_mac_seed}}:{{'{:02x}'.format(1)}}",
                    "ip route add {{PARAMS.ta_subnet}} via 1.1.1.254 dev {{PARAMS.ta_iface_container}}.{{PARAMS.issl_tool_vlan}}",

                    "ip link set dev {{PARAMS.tb_iface_container}} up",
                    "ifconfig {{PARAMS.tb_iface_container}} hw ether {{PARAMS.client_mac_seed}}:{{'{:02x}'.format(1)}}",
                    "sysctl net.ipv4.conf.{{PARAMS.tb_iface_container}}.rp_filter=0",
                    "ip link add link {{PARAMS.tb_iface_container}} name {{PARAMS.tb_iface_container}}.{{PARAMS.issl_tool_vlan}} type vlan id {{PARAMS.issl_tool_vlan}}",
                    "ip link set dev {{PARAMS.tb_iface_container}}.{{PARAMS.issl_tool_vlan}} up",
                    "ip addr add 2.2.2.1/24 dev {{PARAMS.tb_iface_container}}.{{PARAMS.issl_tool_vlan}}",
                    "arp -i {{PARAMS.tb_iface_container}}.{{PARAMS.issl_tool_vlan}} -s 2.2.2.254 {{PARAMS.server_mac_seed}}:{{'{:02x}'.format(1)}}",
                    "ip route add {{PARAMS.tb_subnet}} via 2.2.2.254 dev {{PARAMS.tb_iface_container}}.{{PARAMS.issl_tool_vlan}}",

                    "iptables -t mangle -N DIVERT",
                    "iptables -t mangle -A PREROUTING -p tcp -m socket -j DIVERT",
                    "iptables -t mangle -A DIVERT -j MARK --set-mark 1",
                    "iptables -t mangle -A DIVERT -j ACCEPT",
                    "ip rule add fwmark 1 lookup 100",
                    "ip route add local 0.0.0.0/0 dev lo table 100",
                    "iptables -t mangle -A PREROUTING -i {{PARAMS.ta_iface_container}}.{{PARAMS.issl_tool_vlan}} -p tcp --dport 443 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 883",
                    "iptables -t mangle -A PREROUTING -i {{PARAMS.tb_iface_container}}.{{PARAMS.issl_tool_vlan}} -p tcp --dport 443 -j TPROXY --tproxy-mark 0x1/0x1 --on-port 883",

                    "tcpdump -i {{PARAMS.ta_iface_container}} {{PARAMS.ta_tcpdump}} -w {{PARAMS.result_dir_container}}/proxy-1/ta.pcap &",
                    "tcpdump -i {{PARAMS.tb_iface_container}} {{PARAMS.tb_tcpdump}} -w {{PARAMS.result_dir_container}}/proxy-1/tb.pcap &"
                ]
            }
        ]
    }
    ''')

    return tlspack_cfg.render(PARAMS = cmd_args)

def process_tproxy_stats (result_dir):
    pass




def add_c_arguments (arg_parser):
    try:
        with open('/root/rundir/sys/host') as f:
            host_info = json.load(f)
    except IOError:
            host_info = {'cores' : 40}

    arg_parser.add_argument('--rundir'
                                , action="store"
                                , default='/root/rundir'
                                , help = 'rundir path')

    arg_parser.add_argument('--rundir_container'
                                , action="store"
                                , default='/rundir'
                                , help = 'rundir path in container')

    arg_parser.add_argument('--na_macvlan'
                                , action="store"
                                , default=''
                                , help = 'na_macvlan name')

    arg_parser.add_argument('--na'
                                , action="store"
                                , required=True
                                , dest='na_iface'
                                , help = 'na_iface name')

    arg_parser.add_argument('--nb'
                                , action="store"
                                , required=True
                                , dest='nb_iface'
                                , help = 'nb_iface name')

    arg_parser.add_argument('--nb_macvlan'
                                , action="store"
                                , default=''
                                , help = 'nb_macvlan name')

    arg_parser.add_argument('--iface_container'
                                , action="store"
                                , default='eth1'
                                , help = 'iface_container name')

    arg_parser.add_argument('--runtag'
                                , action="store"
                                , required=True
                                , help = 'run id')

    arg_parser.add_argument('--result_tag'
                                , action="store"
                                , default='latest'
                                , help = 'result tag')

    arg_parser.add_argument('--zones'
                                , action="store"
                                , type=int
                                , default=host_info['cores']/2
                                , help = 'zones ')

    arg_parser.add_argument('--cps'
                                , action="store"
                                , type=int
                                , required=True
                                , help = 'tps : 1 - 10000')

    arg_parser.add_argument('--max_pipeline'
                                , action="store"
                                , type=int
                                , default=100
                                , help = 'max_pipeline : 1 - 10000')

    arg_parser.add_argument('--max_active'
                                , action="store"
                                , type=int
                                , default=100
                                , help = 'max_active : 1 - 2000000')

    arg_parser.add_argument('--server_count'
                                , action="store"
                                , type=int
                                , default=100
                                , help = 'server count')

    arg_parser.add_argument('--cipher'
                                , action="store"
                                , help = 'command name'
                                , required=True)

    arg_parser.add_argument('--sslv3'
                                , action="store_true"
                                , default=False
                                , help = '0/1')

    arg_parser.add_argument('--tls1'
                                , action="store_true"
                                , default=False
                                , help = '0/1')
                                

    arg_parser.add_argument('--tls1_1'
                                , action="store_true"
                                , default=False
                                , help = '0/1')

    arg_parser.add_argument('--tls1_2'
                                , action="store_true"
                                , default=False
                                , help = '0/1')

    arg_parser.add_argument('--tls1_3'
                                , action="store_true"
                                , default=False
                                , help = '0/1')

    arg_parser.add_argument('--debug'
                                , action="store"
                                , type=int
                                , dest='is_debug'
                                , default=0
                                , help = '0;1;2')

    arg_parser.add_argument('--host_src_dir'
                                , action="store"
                                , help = 'host src dir for debuging'
                                , default='/root/tcpdash')

    arg_parser.add_argument('--tcpdump'
                                , action="store"
                                , help = 'tcpdump options'
                                , default='-c 1000')

    arg_parser.add_argument('--total_conn_count'
                                , action="store"
                                , type=int
                                , default=0
                                , help = 'total connection counts')

    arg_parser.add_argument('--client_mac_seed'
                                , action="store"
                                , help = '5 bytes'
                                , default='02:42:ac:14:00')

    arg_parser.add_argument('--server_mac_seed'
                                , action="store"
                                , help = '5 bytes'
                                , default='02:42:ac:15:00')

    arg_parser.add_argument('--app_next_write'
                                , action="store"
                                , type=int
                                , default=0
                                , help = 'app_next_write')

    arg_parser.add_argument('--app_cs_data_len'
                                , action="store"
                                , type=int
                                , default=128
                                , help = 'app_cs_data_len')

    arg_parser.add_argument('--app_sc_data_len'
                                , action="store"
                                , type=int
                                , default=128
                                , help = 'app_sc_data_len')

    arg_parser.add_argument('--app_rcv_buff'
                                , action="store"
                                , type=int
                                , default=0
                                , help = 'app_rcv_buff')

    arg_parser.add_argument('--app_snd_buff'
                                , action="store"
                                , type=int
                                , default=0
                                , help = 'app_snd_buff')

    arg_parser.add_argument('--tcp_rcv_buff'
                                , action="store"
                                , type=int
                                , default=0
                                , help = 'tcp_rcv_buff')

    arg_parser.add_argument('--tcp_snd_buff'
                                , action="store"
                                , type=int
                                , default=0
                                , help = 'tcp_snd_buff')

    return arg_parser

def get_arguments ():
    arg_parser = argparse.ArgumentParser(description = 'test commands')

    subparsers = arg_parser.add_subparsers(dest='cmd_name'
                                                    ,help='sub-command help')

    cps_parser = add_c_arguments (subparsers.add_parser('cps'
                                                    , help='cps help'))

    bw_parser = add_c_arguments (subparsers.add_parser('bw'
                                                    , help='bw help'))

    cipher_parser = add_c_arguments (subparsers.add_parser('cipher'
                                                    , help='cipher help'))

    conn_parser = add_c_arguments (subparsers.add_parser('active'
                                                    , help='active help'))

    stop_parser = subparsers.add_parser('stop', help='stop help')

    tproxy_parser = subparsers.add_parser('tproxy', help='tproxy help')

    stop_tproxy_parser = subparsers.add_parser('stop_tproxy', help='stop help')

    add_cps_params(cps_parser)

    add_bw_params(bw_parser)

    add_tproxy_params (tproxy_parser)

    stop_parser.add_argument('--runtag'
                                , action="store"
                                , required=True
                                , help = 'config id')


    stop_tproxy_parser.add_argument('--runtag'
                                , action="store"
                                , required=True
                                , help = 'config id')


    cmd_args = arg_parser.parse_args()


    if cmd_args.cmd_name in ['cps', 'bw', 'cipher', 'active', 'tproxy']:
        cmd_args.traffic_dir = os.path.join(cmd_args.rundir
                                            , 'traffic'
                                            , cmd_args.runtag).rstrip('/')

        cmd_args.result_dir = os.path.join (cmd_args.traffic_dir
                                            , 'results'
                                            ,  cmd_args.result_tag).rstrip('/')

        cmd_args.traffic_dir_container = os.path.join(cmd_args.rundir_container
                                            , 'traffic'
                                            , cmd_args.runtag).rstrip('/')

        cmd_args.result_dir_container = os.path.join (cmd_args.traffic_dir_container
                                            , 'results'
                                            ,  cmd_args.result_tag).rstrip('/')


    if cmd_args.cmd_name in ['cps', 'bw', 'cipher', 'active']:

        if not cmd_args.na_macvlan:
            cmd_args.na_macvlan = cmd_args.na_iface+'macvlan'
        if not cmd_args.nb_macvlan:
            cmd_args.nb_macvlan = cmd_args.nb_iface+'macvlan'

        cmd_args.cps = cmd_args.cps / cmd_args.zones
        cmd_args.max_active = cmd_args.max_active / cmd_args.zones
        cmd_args.max_pipeline = cmd_args.max_pipeline / cmd_args.zones
        cmd_args.server_count = cmd_args.server_count / cmd_args.zones

        supported_cipher_names = map(lambda x : x['cipher_name']
                                                , supported_ciphers)

        if cmd_args.cmd_name == 'cipher':
            selected_ciphers = map(lambda x : x.strip(), cmd_args.cipher.split(':'))
            for ciph in selected_ciphers:
                if ciph not in supported_cipher_names:
                    raise Exception ('unsupported cipher - ' + ciph)
        elif cmd_args.cmd_name == 'cps':
            if cmd_args.cipher not in supported_cipher_names:
                    raise Exception ('unsupported cipher - ' + cmd_args.cipher)

    elif cmd_args.cmd_name in ['tproxy']:
        if not cmd_args.ta_macvlan:
            cmd_args.ta_macvlan = cmd_args.ta_iface+'macvlan'
        if not cmd_args.tb_macvlan:
            cmd_args.tb_macvlan = cmd_args.tb_iface+'macvlan'

    else: #stop
        pass

    return cmd_args

def start_traffic (cfgid, result_tag, is_debug, host_src_dir):
    return os.system ( 'sudo docker run --name "{}-root" -it -d --volume=/root/rundir:/rundir tlspack/tgen:latest tlspack.exe start "{}" "{}" /root/rundir {} {}'.format(cfgid, cfgid, result_tag, is_debug, host_src_dir) )

def stop_traffic (cfgid):
    os.system ( 'sudo docker run --rm -it --volume=/root/rundir:/rundir tlspack/tgen:latest tlspack.exe stop "{}"'.format(cfgid) )

def start_tproxy (cfgid, result_tag, is_debug, host_src_dir):
    return os.system ( 'sudo docker run --name "{}-root" -it -d --volume=/root/rundir:/rundir tlspack/tgen:latest tcp_proxy.exe start "{}" "{}" /root/rundir {} {}'.format(cfgid, cfgid, result_tag, is_debug, host_src_dir) )

def stop_tproxy (cfgid):

    os.system ( 'sudo docker run --rm -it --volume=/root/rundir:/rundir tlspack/tgen:latest tcp_proxy.exe stop "{}"'.format(cfgid) )

if __name__ == '__main__':

    try:
        CmdArgs = get_arguments ()
    except Exception as er:
        print er
        sys.exit(1)

    if CmdArgs.cmd_name in ['cps', 'bw', 'cipher', 'active', 'tproxy']:
        if CmdArgs.cmd_name == 'cps':
            traffic_s = process_cps_template(CmdArgs)
        elif CmdArgs.cmd_name == 'bw':
            traffic_s = process_bw_template(CmdArgs)
        elif CmdArgs.cmd_name == 'tproxy':
            traffic_s = process_tproxy_template(CmdArgs)

        try:
            traffic_j = json.loads (traffic_s)
        except:
            print traffic_s
            raise

        traffic_s = json.dumps(traffic_j, indent=4)
        print traffic_s


        mon_dir = os.path.join (CmdArgs.traffic_dir, 'mon')

        os.system ( 'rm -rf {}'.format(CmdArgs.traffic_dir) )
        os.system ( 'mkdir -p {}'.format(CmdArgs.traffic_dir) )
        os.system ( 'mkdir -p {}'.format(mon_dir) )

        with open(os.path.join(CmdArgs.traffic_dir, 'config.json'), 'w') as f:
            f.write(traffic_s)


        if CmdArgs.cmd_name in ['cps', 'bw', 'cipher', 'active']:
            start_status = start_traffic(CmdArgs.runtag, CmdArgs.result_tag, CmdArgs.is_debug, CmdArgs.host_src_dir)
        elif CmdArgs.cmd_name == 'tproxy':
            start_status = start_tproxy(CmdArgs.runtag, CmdArgs.result_tag, CmdArgs.is_debug, CmdArgs.host_src_dir)

        if start_status == 0:
            try:
                pid = os.fork()
                if pid > 0:
                    mon_file = os.path.join (mon_dir, CmdArgs.runtag)
                    with open (mon_file, 'w') as f:
                        f.write(str(pid))
                    sys.exit(0)
            except Exception as er:
                print er
                sys.exit(1)
            
            mon_file = os.path.join (mon_dir, CmdArgs.runtag)
            devnull = open(os.devnull, 'w')
            while True:
                time.sleep (1)

                subprocess.call(['rsync', '-av', '--delete'
                                    , CmdArgs.traffic_dir.rstrip('/')
                                    , '/var/www/html/tmp']
                                    , stdout=devnull, stderr=devnull)
                
                if not os.path.exists (mon_file):
                    sys.exit(0)

                if CmdArgs.cmd_name == 'cps':
                    process_cps_stats (CmdArgs.result_dir)
                elif CmdArgs.cmd_name == 'bw':
                    process_bw_stats (CmdArgs.result_dir)
                elif CmdArgs.cmd_name == 'tproxy':
                    process_tproxy_stats (CmdArgs.result_dir);



    elif CmdArgs.cmd_name == 'stop':
        mon_file = os.path.join ('/root/rundir/mon', CmdArgs.runtag)
        os.system ( 'rm -f {}'.format(mon_file) )
        cfg_file = os.path.join ('/root/rundir/traffic', CmdArgs.runtag, 'config.json')
        with open (cfg_file) as f:
            cfg_j = json.load(f)
        if cfg_j['tgen_app'] in ['cps', 'bw', 'cipher', 'active']:
            stop_traffic (CmdArgs.runtag)
        elif cfg_j['tgen_app'] == 'tproxy':
            stop_tproxy (CmdArgs.runtag)


