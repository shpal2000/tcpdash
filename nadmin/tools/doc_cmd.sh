#!/bin/bash

CNAME=$1

CPID=`docker inspect -f '{{.State.Pid}}' $CNAME`

CMDOUT=`ip netns exec $CPID ${@:2}`

echo "$CMDOUT"
