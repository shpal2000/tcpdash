#!/bin/bash

ENAME=$1
SNAME="${ENAME}Br"

ip link set dev $ENAME up
ovs-vsctl del-br $SNAME
ovs-vsctl add-br $SNAME
ovs-vsctl add-port $SNAME $ENAME 

ovs-vsctl show

