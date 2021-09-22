PWD=$(shell pwd)
ALSA_UTILS_VER=alsa-utils-1.0.27.2
SRC_DIR=$(PWD)/$(ALSA_UTILS_VER)

all:
	if [ ! -f $(SRC_DIR)/aplay/aplay ]; then \
		tar -jvxf $(PWD)/$(ALSA_UTILS_VER).tar.bz2 ;\
		cd $(SRC_DIR) && ./configure --host=$(ARCH) --srcdir=$(SRC_DIR) --disable-alsamixer --disable-alsaconf --disable-alsaloop --disable-xmlto --disable-alsatest --prefix="$(ROOTDIR)/user/alsa-lib-1.0.27.2/alsa_install" --with-alsa-prefix="$(ROOTDIR)/user/alsa-lib-1.0.27.2/alsa_install/lib" --with-alsa-inc-prefix="$(ROOTDIR)/user/alsa-lib-1.0.27.2/alsa_install/include" CC="$(CROSS_COMPILE)gcc $(CPUFLAGS)" LD=$(CROSS_COMPILE)ld AR=$(CROSS_COMPILE)ar CFLAGS="$(CFLAGS) -finline-functions" LDFLAGS="$(LDFLAGS)" ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/--fatal-warnings/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-Dlinux/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-D__linux__/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-Dunix/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-DEMBED/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-fomit-frame-pointer/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-pipe/\ /g' ;\
        find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-Wl,/\ /g' ;\
		if [ -d $(PWD)/patch ]; then \
			cp -fRa $(PWD)/patch/* $(SRC_DIR)/ ;\
		fi;\
	fi;\
	if [ -d $(SRC_DIR) ]; then \
		make -C $(SRC_DIR) ;\
	fi;\

clean:
	make -C $(SRC_DIR) clean
distclean:
	rm -rf $(SRC_DIR)   
romfs:
	$(ROMFSINST) -e CONFIG_USER_ALSA_UTIL_1_0_27_2 $(SRC_DIR)/aplay/aplay /sbin/aplay
	$(ROMFSINST) -e CONFIG_USER_ALSA_UTIL_1_0_27_2 $(SRC_DIR)/amixer/amixer /sbin/amixer
	$(ROMFSINST) -e CONFIG_USER_ALSA_UTIL_1_0_27_2 -s /sbin/aplay /sbin/arecord
