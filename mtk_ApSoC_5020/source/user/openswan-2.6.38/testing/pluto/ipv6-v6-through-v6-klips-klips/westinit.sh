: ==== start ====
TESTNAME=ipv6-v6-through-v6-klips-klips
source /testing/pluto/bin/westlocal.sh

# confirm that the network is alive
ping6 -n -c 4 -I 2001:db8:0:1::254 2001:db8:0:2::254
# make sure that clear text does not get through
ip6tables -A INPUT -i eth1 -s 2001:db8:0:2::254 -j DROP
# confirm with a ping to east-in
ping6 -n -c 4 -I 2001:db8:0:1::254 2001:db8:0:2::254

ipsec setup start
/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add westnet-eastnet-6in6
ipsec whack --debug-control --debug-controlmore --debug-parsing --debug-crypt

echo done

