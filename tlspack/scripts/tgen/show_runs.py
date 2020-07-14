__author__ = 'Shirish Pal'
 
import argparse
import os


def get_command_line_params():
    arg_parser = argparse.ArgumentParser(description = \
                                            'stop test')
    arg_parser.add_argument('--runid'
                                , action="store"
                                , default='tmprun'
                                , help = 'iface name')

    return arg_parser.parse_args()


if __name__ == '__main__':
    CmdArgs = get_command_line_params()

    os.system ( 'sudo docker run --rm -it --volume=/root/rundir:/rundir tgen /rundir/bin/tlspack.exe stop "{}"'.format(CmdArgs.runid) )