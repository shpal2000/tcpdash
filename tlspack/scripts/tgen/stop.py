__author__ = 'Shirish Pal'
 
import argparse
import os
import cmds

def get_command_line_params():
    arg_parser = argparse.ArgumentParser(description = \
                                            'stop test')
    arg_parser.add_argument('--cfg_id'
                                , action="store"
                                , required=True
                                , help = 'config id')

    return arg_parser.parse_args()


if __name__ == '__main__':
    CmdArgs = get_command_line_params()
    cmds.stop_traffic (CmdArgs.cfg_id)