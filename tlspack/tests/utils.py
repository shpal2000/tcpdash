__author__ = 'Shirish Pal'

import os
import argparse
import cmds
import json

def add_common_arguments(arg_parser):

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

