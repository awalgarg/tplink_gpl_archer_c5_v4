: ==== start ====
TESTNAME=ipv6-v6-through-v6-klips-klips
source /testing/pluto/bin/eastlocal.sh

ipsec setup start
ipsec auto --add westnet-eastnet-6in6
ipsec whack --debug-control --debug-controlmore --debug-crypt
/testing/pluto/bin/wait-until-pluto-started
