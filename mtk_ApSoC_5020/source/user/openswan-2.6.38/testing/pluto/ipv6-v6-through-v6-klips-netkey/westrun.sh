ipsec auto --up  westnet-eastnet-6in6
ping6 -n -c 4 2001:db8:0:1::254 2001:db8:0:2::254
ipsec look
echo done
