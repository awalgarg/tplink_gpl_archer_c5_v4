PWD=$(shell pwd)
ALSA_LIB_VER=alsa-lib-1.0.27.2
ALSA_INSTALL_DIR=$(PWD)/alsa_install
SRC_DIR=$(PWD)/$(ALSA_LIB_VER)

all:
	if [ ! -d $(ALSA_INSTALL_DIR) ]; then \
		tar -jvxf $(PWD)/$(ALSA_LIB_VER).tar.bz2 ;\
		cd $(SRC_DIR) && ./configure --host=$(ARCH) --srcdir=$(SRC_DIR) --disable-python --prefix="$(ALSA_INSTALL_DIR)" --with-alsa-devdir="/dev" --with-pkgconfdir="alsa_install/lib/pkgconfig" CC="$(CROSS_COMPILE)gcc $(CPUFLAGS)" LD=$(CROSS_COMPILE)ld AR=$(CROSS_COMPILE)ar CFLAGS="$(CFLAGS) -finline-functions" LDFLAGS="$(LDFLAGS) -lm" ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/--fatal-warnings/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-Dlinux/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-D__linux__/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-Dunix/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-DEMBED/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-fomit-frame-pointer/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-pipe/\ /g' ;\
		find $(SRC_DIR) -name "Makefile" | xargs sed -i 's/-Wl,/\ /g' ;\
		sed -i 's/#define HAVE_WORDEXP_H 1/#undef HAVE_WORDEXP_H/g' $(SRC_DIR)/include/config.h ;\
		sed -i 's/#define HAVE___THREAD 1/#undef HAVE___THREAD/g' $(SRC_DIR)/include/config.h ;\
		sed -i 's/#define HAVE___THREAD 1/#undef HAVE___THREAD/g' $(SRC_DIR)/include/config.h ;\
		sed -i 's/^#define ALSA_CONFIG_DIR .*/#define ALSA_CONFIG_DIR "\/etc_ro\/alsa"/g' $(SRC_DIR)/include/config.h ;\
		sed -i 's/^#define ALSA_PLUGIN_DIR .*/#define ALSA_PLUGIN_DIR "\/bin\/alsa"/g' $(SRC_DIR)/include/config.h ;\
		if [ -d $(PWD)/patch ]; then \
			cp -fRa $(PWD)/patch/* $(SRC_DIR)/ ;\
		fi;\
	fi;\
	
	if [ -d $(SRC_DIR) ]; then \
		make -C $(SRC_DIR) ;\
		make -C $(SRC_DIR) install ;\
	fi;\

clean:
	make -C $(SRC_DIR) clean
	rm -rf $(ALSA_INSTALL_DIR)
distclean:
	rm -rf $(SRC_DIR)	
	rm -rf $(ALSA_INSTALL_DIR)
romfs:
	$(ROMFSINST) -e CONFIG_USER_ALSA_UTIL_1_0_27_2 alsa_install/share/alsa /etc_ro/alsa
