: ==== start ====
TESTNAME=sourceip-02
source /testing/pluto/bin/eastlocal.sh

ipsec setup start

/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add westnet--eastnet-sourceip

