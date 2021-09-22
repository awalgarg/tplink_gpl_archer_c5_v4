
CPUN=$(grep ^processor /proc/cpuinfo  | wc -l)
all:
	make -C wpa_supplicant -j ${CPUN} V=1

clean:
	make -C wpa_supplicant clean 

romfs:
	$(ROMFSINST) wpa_supplicant/wpa_supplicant /bin/wpa_supplicant
	$(ROMFSINST) wpa_supplicant/wpa_cli /bin/wpa_cli
	$(ROMFSINST) wpa_supplicant/wpa_passphrase /bin/wpa_passphrase

