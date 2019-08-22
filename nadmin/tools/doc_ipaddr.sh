#!/bin/bash

CNAME=$1
ACTION=$2
SUBN=$3

CPID=`docker inspect -f '{{.State.Pid}}' $CNAME`

ip netns exec $CPID ip -4 route $ACTION local $SUBN dev lo
ip netns exec $CPID ip rule $ACTION from $SUBN table 200

