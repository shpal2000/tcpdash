#!/bin/bash

CNAME=$1
ENAME=$2

mkdir -p /var/run/netns

SNAME="${ENAME}Br"
SPORT="${ENAME}Sw${CNAME}"
APORT="${ENAME}Ap${CNAME}"

CPID=`docker inspect -f '{{.State.Pid}}' $CNAME`

ovs-vsctl del-port $SNAME $SPORT
ip link del $SPORT type veth peer name $APORT

ip link add $SPORT type veth peer name $APORT
ip link set dev $SPORT up
ip link set dev $APORT up
ovs-vsctl add-port $SNAME $SPORT
ln -sf /proc/$CPID/ns/net /var/run/netns/$CPID
ip link set $APORT netns $CPID
ip netns exec $CPID ip link set dev $APORT up
ip netns exec $CPID ip route add default dev $APORT table 200

