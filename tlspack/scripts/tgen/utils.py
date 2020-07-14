__author__ = 'Shirish Pal'

import os
import argparse
import cmds
import json

def get_arguments (description, add_arguments_cb):
    arg_parser 
        = argparse.ArgumentParser(description = description)

    add_arguments_cb (arg_parser)

    with open('/root/rundir/sys/host') as f:
        host_info = json.load(f)

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

    arg_parser.add_argument('--cipher'
                                , action="store"
                                , help = 'command name'
                                , required=True)

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

    return cmd_args
