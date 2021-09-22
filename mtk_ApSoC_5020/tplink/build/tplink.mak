USE_SUPPLIER_BOOT_BUILD=y

USE_SUPPLIER_KERNEL_BUILD=y

USE_SUPPLIER_ROOTFS_BUILD=y

#USE_SUPPLIER_IMAGE_BUILD=y

ARCH=mips
GCC_PATH = /opt
TOOLROOTPATH :=$(GCC_PATH)/buildroot-gcc463/usr
export PATH :=$(TOOLROOTPATH)/bin:$(PATH)
SUPPLIER = mtk_ApSoC_5020
TOOLPREFIX = mipsel-linux-
HOST = mipsel-linux
KERNELVERSION = 3.10.14
OBJ_DIR = mtk_4.6.3
SLIB_PATH = $(GCC_PATH)/buildroot-gcc463/usr/mipsel-buildroot-linux-uclibc/sysroot/lib
KERNELPATH = $(SDK_PATH)/source/linux-3.10.14.x
SUPPLIER_TOOLS = $(SDK_TP_PATH)/tools
ifneq ($(shell uname -m), x86_64)
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs4.2
else
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs4.2.x86_64
endif

export SUPPLIER_TOOLS
export CROSS_COMPILE=$(TOOLPREFIX)

unexport TC_CFLAGS
TC_CFLAGS += -DTCSUPPORT_IGMPSNOOPING_ENHANCE
export TC_CFLAGS

export KERNELVERSION
export MT7612E_AP_DIR = $(KERNELPATH)/drivers/net/wireless/MT7612_ap
export MT7628_DIR = $(KERNELPATH)/drivers/net/wireless/mt_wifi
export MT7612E_AP_DIR = $(KERNELPATH)/drivers/net/wireless/MT7612_ap
export MT7610E_DIR = $(KERNELPATH)/drivers/net/wireless/MT7610_ap
export WIRELESSTOOLS =$(SDK_PATH)/source/user/wireless_tools.29
export WIRELESSTOOLSLIB = libiw.so.29

###############################################################################
supplier_env_build:supplier_pre_build

###############################################################################
.PHONY: supplier_pre_build
supplier_tools:
	echo "There's no supplier_tools by now."

supplier_pre_build:
	@if test -d /opt/buildroot-gcc463; \
	then \
		echo "toolchain buildroot-gcc463 exists." ; \
	else \
		echo "Uncompressing toolchain..." ; \
		cp $(SDK_PATH)/toolchain/buildroot-gcc463_32bits.tar.bz2 /opt/ ;\
		cd /opt && tar -jxf buildroot-gcc463_32bits.tar.bz2 && rm -f buildroot-gcc463_32bits.tar.bz2 ;\
	fi; 

###############################################################################
.PHONY: supplier_menuconfig
supplier_menuconfig: kernel_path_check
	@echo "profile menuconfig"
	@echo "config TC_CFLAGS which used by kernel"
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).profile $(PROFILE_DIR)/$(MODEL).profile
	cd $(SDK_PATH) && $(MAKE) PROFILE=$(MODEL)  CUSTOM=$(CUSTOM) menuconfig
	cp -pfT $(PROFILE_DIR)/$(MODEL).profile $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).profile 
	@echo "Replace TC_CFLAGS profile Config File"

boot_menuconfig:
	cp -f $(SDK_PATH)/Uboot/$(MODEL)_config $(SDK_PATH)/Uboot/.config
	cd $(SDK_PATH)/Uboot && make menuconfig
	cp -f $(SDK_PATH)/Uboot/.config $(SDK_PATH)/Uboot/$(MODEL)_config
	cp -f $(SDK_PATH)/Uboot/autoconf.h $(SDK_PATH)/Uboot/$(MODEL)_autoconf_h

###############################################################################	
supplier_boot_build:
	@echo "start supplier_boot_build"
	test -d $(GCC_PATH)/buildroot-gcc342 || (cp $(SDK_PATH)/toolchain/buildroot-gcc342.tar.bz2 $(GCC_PATH)/ && \
	cd $(GCC_PATH)/ && tar -jxf buildroot-gcc342.tar.bz2 && rm -f buildroot-gcc342.tar.bz2)
	test -f $(BUILD_PATH)/config/$(MODEL)/$(SPEC)/$(MODEL)_autoconf_h && \
	cp $(BUILD_PATH)/config/$(MODEL)/$(SPEC)/$(MODEL)_autoconf_h $(SDK_PATH)/Uboot/$(MODEL)_autoconf_h || \
	echo "no $(MODEL)_autoconf_h file in $(SPEC) dir."
	test -f $(BUILD_PATH)/config/$(MODEL)/$(SPEC)/$(MODEL)_config && \
	cp $(BUILD_PATH)/config/$(MODEL)/$(SPEC)/$(MODEL)_config $(SDK_PATH)/Uboot/$(MODEL)_config  || \
	echo "no $(MODEL)_config file in $(SPEC) dir."
	cp $(SDK_PATH)/Uboot/$(MODEL)_config $(SDK_PATH)/Uboot/.config
	cp $(SDK_PATH)/Uboot/$(MODEL)_autoconf_h $(SDK_PATH)/Uboot/autoconf.h
	cd $(SDK_PATH)/Uboot && make 
	test -d $(BOOT_TG_PATH) || mkdir -p $(BOOT_TG_PATH)
	cp $(SDK_PATH)/Uboot/uboot.bin $(BOOT_TG_PATH)/boot_$(MODEL).bin
	
supplier_boot_clean:
	cd $(SDK_PATH)/Uboot && make clean
	-rm -f $(BOOT_TG_PATH)/*

.PHONY: 
test_boot_build:
	cd $(HOSTTOOLS_DIR) && gcc -O2 -DGNU -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I. -o createimg createimg.c -lm
	$(HOSTTOOLS_DIR)/addvtoken --chip $(BRCM_CHIP) --flashtype NOR $(BOOT_TG_PATH)/cfe$(BRCM_CHIP).bin $(BOOT_TG_PATH)/boot.bin
	$(HOSTTOOLS_DIR)/createimg --endian $(BRCM_ENDIAN_FLAGS) --boardid=$(BRCM_BOARD_ID) --voiceboardid $(BRCM_VOICE_BOARD_ID) --numbermac=$(BRCM_NUM_MAC_ADDRESSES) --macaddr=00:0A:EB:13:09:69 --tp=$(BRCM_MAIN_TP_NUM) --psisize=$(BRCM_PSI_SIZE) --logsize=$(BRCM_LOG_SECTION_SIZE) --auxfsprcnt=$(BRCM_AUXFS_PERCENT) --gponsn=$(BRCM_GPON_SERIAL_NUMBER) --gponpw=$(BRCM_GPON_PASSWORD) --wholeflashfile=$(BOOT_TG_PATH)/boot.bin
	
###############################################################################
kernel_path_check:
	@if [ ! -d $(KERNELPATH) ]; \
	then \
		echo "KERNELPATH: $(KERNELPATH) not found, please check the KERNELPATH defination"; \
		exit 1; \
	else \
		echo "KERNELPATH: $(KERNELPATH)"; \
	fi

supplier_kernel_build: kernel_path_check
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/source.config $(KERNELPATH)/../.config
	rm -f $(KERNELPATH)/../.oldconfig
	cd $(KERNELPATH)/.. && chmod u+x config/setconfig && config/setconfig defaults && config/setconfig final
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/autoconf.h $(KERNELPATH)/../autoconf.h
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/config.arch $(KERNELPATH)/../config.arch
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).kconfig $(KERNELPATH)/.config
	#cd $(KERNELPATH)/.. && $(MAKE) linux
	cd $(KERNELPATH) && $(MAKE) V=2 linux.7z
	test -d $(KERNEL_TG_PATH) || mkdir -p $(KERNEL_TG_PATH)
	cp $(KERNELPATH)/linux.7z $(KERNEL_TG_PATH)/
	cp $(KERNELPATH)/vmlinux $(KERNEL_TG_PATH)/
	@if test -e $(KERNELPATH)/linux.7z; \
	then echo "Compile kernel success"; \
	else echo "Compile kernel error!"; exit 1; \
	fi

supplier_kernel_menuconfig: kernel_path_check
	@echo "kernel menuconfig"
	@echo "build kernel menuconfig"
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/source.config $(KERNELPATH)/../.config
	rm -f $(KERNELPATH)/../.oldconfig
	cd $(KERNELPATH)/.. && chmod u+x config/setconfig && config/setconfig defaults && config/setconfig final
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/autoconf.h $(KERNELPATH)/../autoconf.h
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/config.arch $(KERNELPATH)/../config.arch
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).kconfig $(KERNELPATH)/.config
	cd $(KERNELPATH)/.. && $(MAKE) menuconfig
	cp -pfT $(KERNELPATH)/.config $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).kconfig 
	cp -pfT $(KERNELPATH)/../config.arch $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/config.arch
	cp -pfT $(KERNELPATH)/../autoconf.h $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/autoconf.h
	cp -pfT $(KERNELPATH)/../.config $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/source.config
	@echo "Replace Kernel Config File"
	cd $(KERNELPATH)/.. && make dep

supplier_kernel_modules: kernel_path_check
	#cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).kconfig $(KERNELPATH)/.config
	$(MAKE)  -C $(KERNELPATH) modules
	$(MAKE)  -C $(KERNELPATH) modules_install
	test -d $(MODULE_TG_PATH) || mkdir -p $(MODULE_TG_PATH) && mkdir -p $(MODULE_TG_PATH)/kmdir/
	@echo "install kernel header"
	cd $(KERNELPATH) && make headers_install
	cp -pR $(INSTALL_MOD_PATH)/lib/modules/$(KERNELVERSION)/kernel $(MODULE_TG_PATH)/kmdir/

supplier_kernel_clean: kernel_path_check
	@echo "supplier_kernel_clean"
	cd $(KERNELPATH)/../ && $(MAKE) clean
	rm -Rf $(KERNEL_TG_PATH)/*

###############################################################################
supplier_modules:
	@echo "build supplier_modules"
	#cd $(SDK_PATH) && make PROFILE=$(MODEL) CUSTOM=CT modules 
	#cp -p $(FILESYSTEM_DIR)/lib/modules/*.ko $(MODULE_TG_PATH)/
	#cp -p $(FILESYSTEM_DIR)/lib/modules/usbhost/*.ko $(MODULE_TG_PATH)/
	
supplier_modules_clean:
	@echo "clean supplier_modules"
	#cd $(SDK_PATH) && make PROFILE=$(MODEL) CUSTOM=CT modules_clean 
	#rm -rf  $(MODULE_TG_PATH)/*

###############################################################################

supplier_apps_build:
	#cd $(SDK_PATH) && make PROFILE=$(MODEL) CUSTOM=CT app_bsp
	#cd $(SDK_PATH) && make PROFILE=$(MODEL) CUSTOM=CT apps
	
supplier_apps_clean: 
	#cd $(SDK_PATH) && make PROFILE=$(MODEL) CUSTOM=CT app_bsp_clean
	#cd $(SDK_PATH) && make PROFILE=$(MODEL) CUSTOM=CT apps_clean


###############################################################################	
fs_supplier:
	#if [ -d $(INSTALL_DIR)/lib/private ]; then cp -pfR $(INSTALL_DIR)/lib/private/* $(INSTALL_DIR)/lib; rm -Rf $(INSTALL_DIR)/lib/private; fi
	#if [ -d $(INSTALL_DIR)/lib/public ]; then cp -pfR $(INSTALL_DIR)/lib/public/* $(INSTALL_DIR)/lib; rm -Rf $(INSTALL_DIR)/lib/public; fi
	#cp -pfR $(INSTALL_DIR)/* $(FS_TG_PATH)

###############################################################################
supplier_fs_dev:
	#cd $(SDK_TP_PATH)/tools && ./buildDevs $(FS_TG_PATH)
	
supplier_fs_rootfs: fakeroot
	#$(HOSTTOOLS_DIR)/mksquashfs $(FS_TG_PATH) $(IMG_TG_PATH)/rootfs -noappend -all-root -b 65536 -comp xz
	
ifeq ($(INCLUDE_REDUCE_FLASH_SIZE), y)
	make compress_web
#	-rm -f $(FS_TG_PATH)/usr/bin/cli
#	-rm -f $(FS_TG_PATH)/usr/bin/diagTool
	-rm -f $(FS_TG_PATH)/usr/bin/reg
#	-rm -f $(FS_TG_PATH)/usr/bin/cwmp
	-rm -f $(FS_TG_PATH)/usr/bin/iwlist
	-rm -f $(FS_TG_PATH)/usr/bin/iwconfig
#	-rm -f $(FS_TG_PATH)/usr/bin/mldProxy
	-rm -f $(FS_TG_PATH)/usr/bin/wanType
	-rm -f $(FS_TG_PATH)/usr/bin/cmmsyslogc
#	-rm -f $(FS_TG_PATH)/usr/bin/ebtables
#	-rm -f $(FS_TG_PATH)/usr/bin/snmpd
#	-rm -f $(FS_TG_PATH)/usr/bin/tr143d
	-rm -f $(FS_TG_PATH)/usr/bin/rtinicapd
#	-rm -f $(FS_TG_PATH)/usr/sbin/bpalogin
	-rm -f $(FS_TG_PATH)/lib/modules/tp_board.ko
endif
	sed -i 's/\(X_TP_BuildSpec.*d=\)[^ ]*/\1$(SPEC)/' $(DATAS_TG_PATH)/reduced_data_model.xml
	
	cp -pR $(DATAS_TG_PATH)/reduced_data_model.xml $(DATAS_TG_PATH)/reduced_data_model_beta.xml
	cp -pR $(DATAS_TG_PATH)/reduced_data_model.xml $(DATAS_TG_PATH)/reduced_data_model_trans.xml
	sed -i 's/\(X_TP_IsBeta.*d=\)./\10/' $(DATAS_TG_PATH)/reduced_data_model.xml
	sed -i 's/\(X_TP_IsTrans.*d=\)./\10/' $(DATAS_TG_PATH)/reduced_data_model.xml
	sed -i 's/\(X_TP_IsBeta.*d=\)./\11/' $(DATAS_TG_PATH)/reduced_data_model_beta.xml
	sed -i 's/\(X_TP_IsTrans.*d=\)./\10/' $(DATAS_TG_PATH)/reduced_data_model_beta.xml
	sed -i 's/\(X_TP_IsBeta.*d=\)./\10/' $(DATAS_TG_PATH)/reduced_data_model_trans.xml
	sed -i 's/\(X_TP_IsTrans.*d=\)./\11/' $(DATAS_TG_PATH)/reduced_data_model_trans.xml
	
ifneq ($(shell id -u), 0)
	find $(FS_TG_PATH)/dev/ ! -type d | xargs rm -f
	cp -pR $(DATAS_TG_PATH)/reduced_data_model.xml $(FS_TG_PATH)/etc/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	cd $(SUPPLIER_TOOLS) && $(FAKEROOT_PATH)/fakeroot ./buildFs $(FS_TG_PATH) $(ROOTFSTOOLS) $(MODEL) $(SUPPLIER) 0 $(ROOTFS_BLOCK_SIZE)	
	
	find $(FS_TG_PATH)/dev/ ! -type d | xargs rm -f
	cp -pR $(DATAS_TG_PATH)/reduced_data_model_beta.xml $(FS_TG_PATH)/etc/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	cd $(SUPPLIER_TOOLS) && $(FAKEROOT_PATH)/fakeroot ./buildFs $(FS_TG_PATH) $(ROOTFSTOOLS) $(MODEL) $(SUPPLIER) 1 $(ROOTFS_BLOCK_SIZE)

	find $(FS_TG_PATH)/dev/ ! -type d | xargs rm -f
	cp -pR $(DATAS_TG_PATH)/reduced_data_model_trans.xml $(FS_TG_PATH)/etc/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	cd $(SUPPLIER_TOOLS) && $(FAKEROOT_PATH)/fakeroot ./buildFs $(FS_TG_PATH) $(ROOTFSTOOLS) $(MODEL) $(SUPPLIER) 2 $(ROOTFS_BLOCK_SIZE)

else
	find $(FS_TG_PATH)/dev/ ! -type d | xargs rm -f
	cp -pR $(DATAS_TG_PATH)/reduced_data_model.xml $(FS_TG_PATH)/etc/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	cd $(SUPPLIER_TOOLS) && ./buildFs $(FS_TG_PATH) $(ROOTFSTOOLS) $(MODEL) $(SUPPLIER) 0 $(ROOTFS_BLOCK_SIZE)	

	find $(FS_TG_PATH)/dev/ ! -type d | xargs rm -f
	cp -pR $(DATAS_TG_PATH)/reduced_data_model_beta.xml $(FS_TG_PATH)/etc/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	cd $(SUPPLIER_TOOLS) && ./buildFs $(FS_TG_PATH) $(ROOTFSTOOLS) $(MODEL) $(SUPPLIER) 1 $(ROOTFS_BLOCK_SIZE)

	find $(FS_TG_PATH)/dev/ ! -type d | xargs rm -f
	cp -pR $(DATAS_TG_PATH)/reduced_data_model_trans.xml $(FS_TG_PATH)/etc/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	cd $(SUPPLIER_TOOLS) && ./buildFs $(FS_TG_PATH) $(ROOTFSTOOLS) $(MODEL) $(SUPPLIER) 2 $(ROOTFS_BLOCK_SIZE)
endif
	#$(ROOTFSTOOLS) $(FS_TG_PATH) $(FS_TG_PATH)/rootfs -noappend -all-root -b 65536 -comp xz

###############################################################################
supplier_mkkernel_build:
#TEST_TG_PATH=$(TARGETS_PATH)/$(MODEL)/test
#$(SUPPLIER_TOOLS)/mkkernel3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(FS_TG_PATH)/../rootfs.$(MODEL) -v $(KERNEL_TG_PATH)/vmlinux -i $(IMG_TG_PATH)

supplier_image_build: 
	test -d $(IMG_TG_PATH)/ || mkdir -p $(IMG_TG_PATH)/

	#@test -f $(FS_TG_PATH)/../flash_config_$(MODEL).xml && USING_FLASH_CONFIG="-l $(FS_TG_PATH)/../flash_config_$(MODEL).xml" || echo "$(FS_TG_PATH)/../flash_config_$(MODEL).xml not found, do not include flash config in flash.bin.";\
	#$(SUPPLIER_TOOLS)/mkimage_c20v4 -m $(KERNEL_MAX_SIZE) -t 2 -p $(FS_TG_PATH)/../reduced_data_model_plaintext_$(MODEL).xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(FS_TG_PATH)/rootfs.$(MODEL) -v $(KERNEL_TG_PATH)/vmlinux -i $(IMG_TG_PATH)/ -r $(SPEC) $$USING_FLASH_CONFIG -s $(FLASH_SIZE) -u $(BUILD_PATH)/config/$(MODEL)/$(SPEC)/supportlist.bin 
	$(SUPPLIER_TOOLS)/mkimage_c20v4 -m $(KERNEL_MAX_SIZE) -t 2 -p $(DATAS_TG_PATH)/reduced_data_model.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(FS_TG_PATH)/../rootfs.$(MODEL) -v $(KERNEL_TG_PATH)/vmlinux -i $(IMG_TG_PATH)/ -r EU $$USING_FLASH_CONFIG -s $(FLASH_SIZE)
	
###############################################################################
.PHONY: kernel_prepare driver_clean 

kernel_clean: kernel_prepare



kernel_prepare:


driver_clean:
# this will be done in kernel_clean
#$(MAKE) -C $(SDK_PATH)/bcmdrivers clean
