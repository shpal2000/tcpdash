__author__ = 'Shirish Pal'
 
import os

def start_traffic (cfgid, result_tag):
    os.system ( 'sudo docker run --name "{}-root" -it -d --volume=/root/rundir:/rundir tgen /rundir/bin/tlspack.exe start "{}" "{}" /root/rundir 0'.format(cfgid, cfgid, result_tag) )

def stop_traffic (cfgid):
    os.system ( 'sudo docker run --rm -it --volume=/root/rundir:/rundir tgen /rundir/bin/tlspack.exe stop "{}"'.format(cfgid) )
