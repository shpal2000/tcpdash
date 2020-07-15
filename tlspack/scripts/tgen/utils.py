__author__ = 'Shirish Pal'

import os
import argparse
import cmds
import json

supported_ciphers = ["AES128-SHA",	
"AES256-SHA",	
"DHE-RSA-AES128-SHA",	
"DHE-RSA-AES256-SHA",	
"DHE-RSA-AES128-GCM-SHA256",	
"ECDHE-ECDSA-AES128-SHA",	
"ECDHE-ECDSA-AES256-SHA",	
"ECDHE-RSA-AES128-SHA",	
"ECDHE-RSA-AES256-SHA",	
"ECDHE-ECDSA-CHACHA20-POLY1305",	
"DHE-RSA-CHACHA20-POLY1305",	
"CAMELLIA128-SHA",	
"CAMELLIA256-SHA",	
"DHE-RSA-CAMELLIA128-SHA",	
"DHE-RSA-CAMELLIA256-SHA",	
"AES128-SHA256",	
"AES256-SHA256",	
"DHE-RSA-AES128-SHA256",	
"AES128-GCM-SHA256",	
"AES256-GCM-SHA384",	
"ECDHE-RSA-AES128-GCM-SHA256",	
"ECDHE-RSA-AES256-GCM-SHA384",	
"ECDHE-RSA-AES128-SHA256",	
"ECDHE-RSA-AES256-SHA384",	
"DHE-RSA-AES256-SHA256",	
"DHE-RSA-AES256-GCM-SHA384",	
"ECDHE-RSA-CHACHA20-POLY1305",	
"TLS_AES_128_GCM_SHA256",	
"TLS_AES_256_GCM_SHA384",	
"TLS_CHACHA20_POLY1305_SHA256",	
"ECDHE-ECDSA-AES128-GCM-SHA256",	
"ECDHE-ECDSA-AES256-GCM-SHA384",	
"ECDHE-ECDSA-AES128-SHA256",	
"ECDHE-ECDSA-AES256-SHA384",	
"RC4-MD5",	
"RC4-SHA",	
"DES-CBC-SHA",	
"DES-CBC3-SHA",	
"SEED-SHA"]

def get_arguments (description, add_arguments_cb):
    arg_parser = argparse.ArgumentParser(description = description)

    add_arguments_cb (arg_parser)

    try:
        with open('/root/rundir/sys/host') as f:
            host_info = json.load(f)
    except IOError:
            host_info = {'cores' : 40}

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

    arg_parser.add_argument('--cfg_id'
                                , action="store"
                                , required=True
                                , help = 'run id')

    arg_parser.add_argument('--result_tag'
                                , action="store"
                                , default='tmp'
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
                                , help = 'cps : 1 - 10000')

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

    cmd_args = arg_parser.parse_args()

    cmd_args.cps = cmd_args.cps / cmd_args.zones
    cmd_args.max_active = cmd_args.max_active / cmd_args.zones
    cmd_args.max_pipeline = cmd_args.max_pipeline / cmd_args.zones

    cmd_args.sslv3 = 1 if cmd_args.sslv3 else 0
    cmd_args.tls1 = 1 if cmd_args.tls1 else 0
    cmd_args.tls1_1 = 1 if cmd_args.tls1_1 else 0
    cmd_args.tls1_2 = 1 if cmd_args.tls1_2 else 0
    cmd_args.tls1_3 = 1 if cmd_args.tls1_3 else 0

    cmd_args_map = vars (cmd_args)
    selected_ciphers = map(lambda x : x.strip(), cmd_args.cipher.split(':'))
    for ciph in supported_ciphers:
        if ciph in selected_ciphers:
            cmd_args_map[ciph] = 1
        else:
            cmd_args_map[ciph] = 0

    return cmd_args
