killall udhcpd
ifconfig p2p-p2p0-0 192.168.49.1
udhcpd ./udhcpd.conf&
