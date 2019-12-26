ip route add 190.0.13.0/24 via 190.0.5.2 dev sat2p1
# gnome-terminal  -x bash -c "
# ip netns exec sat2 ip route add 190.0.13.0/24 via 190.0.5.2 dev sat2p1;ip route show;exec bash;"