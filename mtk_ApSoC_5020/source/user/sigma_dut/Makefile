include Makefile.inc

DIRS= ${LIB} ${DUT} ${CA} ${TC}

all:
	for i in ${DIRS}; do \
		$(MAKE) -C $$i || exit 1; \
	done

clean:
	for i in ${DIRS}; do \
		$(MAKE) -C $$i clean || exit 1; \
	done

romfs:
	$(ROMFSINST) ./dut/wfa_dut /sbin/wfa_dut
	$(ROMFSINST) ./ca/wfa_ca /sbin/wfa_ca
	$(ROMFSINST) ./scripts/arp_neigh_loop /sbin/arp_neigh_loop
	$(ROMFSINST) ./scripts/getipconfig.sh /sbin/getipconfig.sh
	$(ROMFSINST) ./scripts/getpid.sh /sbin/getpid.sh
	$(ROMFSINST) ./scripts/getpstats.sh /sbin/getpstats.sh
	$(ROMFSINST) ./scripts/gpstats.sh /sbin/gpstats.sh
	$(ROMFSINST) ./scripts/stoping.sh /sbin/stoping.sh
	$(ROMFSINST) ./scripts/updatepid.sh /sbin/updatepid.sh
	$(ROMFSINST) ./scripts/wfaping6.sh /sbin/wfaping6.sh
	$(ROMFSINST) ./scripts/wfaping.sh /sbin/wfaping.sh
	$(ROMFSINST) ./scripts/wfa_test_cli.sh /sbin/wfa_test_cli.sh
#	$(ROMFSINST) ./ping /bin/ping
