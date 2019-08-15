TCPDASH_S1_NAME=$1
TCPDASH_C1_NAME=$2

TCPDASH_S_NIC=$3
TCPDASH_C_NIC=$4

TCPDASH_S1_PID=$(docker inspect -f '{{.State.Pid}}' "$TCPDASH_S1_NAME")
TCPDASH_C1_PID=$(docker inspect -f '{{.State.Pid}}' "$TCPDASH_C1_NAME")

$(ln -s /proc/"$TCPDASH_S1_PID"/ns/net /var/run/netns/"$TCPDASH_S1_PID")
$(ln -s /proc/"$TCPDASH_C1_PID"/ns/net /var/run/netns/"$TCPDASH_C1_PID")

ovs-vsctl del-br cAppVs
ovs-vsctl del-br sAppVs

ovs-vsctl add-br cAppVs
ovs-vsctl add-br sAppVs

mkdir -p  /var/run/netns

$(ovs-vsctl add-port cAppVs "$TCPDASH_C_NIC")
$(ovs-vsctl set interface "$TCPDASH_C_NIC" type=patch)
$(ip link set dev "$TCPDASH_C_NIC" up)
ip  link add cApp1VsEth type veth peer name cApp1PrEth
ovs-vsctl add-port cAppVs cApp1VsEth
ip link set dev cApp1VsEth up
$(ip link set cApp1PrEth netns "$TCPDASH_C1_PID")
$(ip netns exec $TCPDASH_C1_PID ip link set dev cApp1PrEth up)
$(ip netns exec $TCPDASH_C1_PID ip -4 route add local 12.20.50.0/24 dev lo)
$(ip netns exec $TCPDASH_C1_PID ip rule add from 12.20.50.0/24 table 200)
$(ip netns exec $TCPDASH_C1_PID ip route add default dev cApp1PrEth table 200)

$(ovs-vsctl add-port sAppVs "$TCPDASH_S_NIC")
$(ovs-vsctl set interface "$TCPDASH_S_NIC" type=patch)
$(ip link set dev "$TCPDASH_S_NIC" up)
ip  link add sApp1VsEth type veth peer name sApp1PrEth
ovs-vsctl add-port sAppVs sApp1VsEth
ip link set dev sApp1VsEth up
$(ip link set sApp1PrEth netns "$TCPDASH_S1_PID")
$(ip netns exec "$TCPDASH_S1_PID" ip link set dev sApp1PrEth up)
$(ip netns exec "$TCPDASH_S1_PID" ip -4 route add local 12.20.60.0/24 dev lo)
$(ip netns exec "$TCPDASH_S1_PID" ip rule add from 12.20.60.0/24 table 200)
$(ip netns exec "$TCPDASH_S1_PID" ip route add default dev sApp1PrEth table 200)

$(ovs-vsctl set interface "$TCPDASH_C_NIC" options:peer="$TCPDASH_S_NIC")
$(ovs-vsctl set interface "$TCPDASH_S_NIC" options:peer="$TCPDASH_C_NIC")