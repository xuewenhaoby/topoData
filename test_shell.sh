ip netns add sat1
ip netns exec sat1 ip link set dev lo up
ip netns exec sat1 sysctl net.ipv4.conf.all.forwarding=1


ip netns add sat2
ip netns exec sat2 ip link set dev lo up
ip netns exec sat2 sysctl net.ipv4.conf.all.forwarding=1


ip netns add sat3
ip netns exec sat3 ip link set dev lo up
ip netns exec sat3 sysctl net.ipv4.conf.all.forwarding=1

ip netns add sat4
ip netns exec sat4 ip link set dev lo up
ip netns exec sat4 sysctl net.ipv4.conf.all.forwarding=1

ip netns add sat5
ip netns exec sat5 ip link set dev lo up
ip netns exec sat5 sysctl net.ipv4.conf.all.forwarding=1

ip netns add sat6
ip netns exec sat6 ip link set dev lo up
ip netns exec sat6 sysctl net.ipv4.conf.all.forwarding=1

ip netns add sat11
ip netns exec sat11 ip link set dev lo up
ip netns exec sat11 sysctl net.ipv4.conf.all.forwarding=1

ip netns add sat12
ip netns exec sat12 ip link set dev lo up
ip netns exec sat12 sysctl net.ipv4.conf.all.forwarding=1

ip netns add sat13
ip netns exec sat13 ip link set dev lo up
ip netns exec sat13 sysctl net.ipv4.conf.all.forwarding=1


ip link add sat1p0 type veth peer name sat2p1
ip link set sat1p0 netns sat1
ip link set sat2p1 netns sat2
ip netns exec sat1 ip link set dev sat1p0 up
ip netns exec sat2 ip link set dev sat2p1 up
ip netns exec sat1 ip addr add 1.0.1.2/24 broadcast 1.0.1.255 dev sat1p0
ip netns exec sat2 ip addr add 1.0.1.1/24 broadcast 1.0.1.255 dev sat2p1

ip link add sat2p0 type veth peer name sat3p1
ip link set sat2p0 netns sat2
ip link set sat3p1 netns sat3
ip netns exec sat2 ip link set dev sat2p0 up
ip netns exec sat3 ip link set dev sat3p1 up
ip netns exec sat2 ip addr add 1.0.2.2/24 broadcast 1.0.2.255 dev sat2p0
ip netns exec sat3 ip addr add 1.0.2.1/24 broadcast 1.0.2.255 dev sat3p1


ip link add sat3p0 type veth peer name sat4p1
ip link set sat3p0 netns sat3
ip link set sat4p1 netns sat4
ip netns exec sat3 ip link set dev sat3p0 up
ip netns exec sat4 ip link set dev sat4p1 up
ip netns exec sat3 ip addr add 1.0.3.2/24 broadcast 1.0.3.255 dev sat3p0
ip netns exec sat4 ip addr add 1.0.3.1/24 broadcast 1.0.3.255 dev sat4p1

ip link add sat11p0 type veth peer name sat3p4
ip link set sat11p0 netns sat11
ip link set sat3p4 netns sat3
ip netns exec sat11 ip link set dev sat11p0 up
ip netns exec sat3 ip link set dev sat3p4 up
ip netns exec sat11 ip addr add 1.0.11.2/24 broadcast 1.0.11.255 dev sat11p0
ip netns exec sat3 ip addr add 1.0.11.1/24 broadcast 1.0.11.255 dev sat3p4

ip link add sat4p0 type veth peer name sat5p1
ip link set sat4p0 netns sat4
ip link set sat5p1 netns sat5
ip netns exec sat4 ip link set dev sat4p0 up
ip netns exec sat5 ip link set dev sat5p1 up
ip netns exec sat4 ip addr add 1.0.4.2/24 broadcast 1.0.4.255 dev sat4p0
ip netns exec sat5 ip addr add 1.0.4.1/24 broadcast 1.0.4.255 dev sat5p1

ip link add sat5p0 type veth peer name sat6p1
ip link set sat5p0 netns sat5
ip link set sat6p1 netns sat6
ip netns exec sat5 ip link set dev sat5p0 up
ip netns exec sat6 ip link set dev sat6p1 up
ip netns exec sat5 ip addr add 1.0.5.2/24 broadcast 1.0.5.255 dev sat5p0
ip netns exec sat6 ip addr add 1.0.5.1/24 broadcast 1.0.5.255 dev sat6p1

ip link add sat11p2 type veth peer name sat12p1
ip link set sat11p2 netns sat11
ip link set sat12p1 netns sat12
ip netns exec sat11 ip link set dev sat11p2 up
ip netns exec sat12 ip link set dev sat12p1 up
ip netns exec sat11 ip addr add 1.0.12.2/24 broadcast 1.0.12.255 dev sat11p2
ip netns exec sat12 ip addr add 1.0.12.1/24 broadcast 1.0.12.255 dev sat12p1

ip link add sat12p0 type veth peer name sat13p1
ip link set sat12p0 netns sat12
ip link set sat13p1 netns sat13
ip netns exec sat12 ip link set dev sat12p0 up
ip netns exec sat13 ip link set dev sat13p1 up
ip netns exec sat12 ip addr add 1.0.13.2/24 broadcast 1.0.13.255 dev sat12p0
ip netns exec sat13 ip addr add 1.0.13.1/24 broadcast 1.0.13.255 dev sat13p1

ip link add sat13p3 type veth peer name sat5p4
ip link set sat13p3 netns sat13
ip link set sat5p4 netns sat5
ip netns exec sat13 ip link set dev sat13p3 up
ip netns exec sat5 ip link set dev sat5p4 up
ip netns exec sat13 ip addr add 1.0.14.2/24 broadcast 1.0.14.255 dev sat13p3
ip netns exec sat5 ip addr add 1.0.14.1/24 broadcast 1.0.14.255 dev sat5p4



# ip netns exec sat1 echo 1 >/proc/sys/net/ipv4/conf/sat1p0/proxy_arp
# ip netns exec sat2 echo 1 >/proc/sys/net/ipv4/conf/sat2p1/proxy_arp

# ip netns exec sat2 echo 1 >/proc/sys/net/ipv4/conf/sat2p0/proxy_arp

# ip netns exec sat3 echo 1 >/proc/sys/net/ipv4/conf/sat3p1/proxy_arp







