__author__ = 'Shirish Pal'
 
import argparse
import os
import utils
import json

def get_command_line_params():
    arg_parser = argparse.ArgumentParser(description = \
                                            'show stats')
    utils.add_common_arguments (arg_parser)

    return arg_parser.parse_args()


if __name__ == '__main__':
    CmdArgs = get_command_line_params()

    result_dir = os.path.join(CmdArgs.rundir
                                , 'traffic'
                                , CmdArgs.cfg_id
                                , 'results'
                                # , CmdArgs.result_tag
                                , 'tmpzone-1-a')

    stats_file = os.path.join(result_dir, 'ev_sockstats.json')
    with open (stats_file, 'r') as f:
        stats_dict = json.load (f)
        print "\n{:<50} {:<10}".format('Stats','Count')
        stats_list = sorted(stats_dict.keys())
        for k in stats_list:
            v = stats_dict[k]
            if v:
                print "{:<50} {:<10}".format(k, v)
        print '\n'
