#**************************************************************************** 
# 
#  Copyright(c) 2010-2011 Shenzhen TP-LINK Technologies Co.Ltd. 
#  All Rights Reserved 
#  YangXv <yangxu@tp-link.com.cn> 
# 
#***************************************************************************

# Top-level Makefile for all linux platforms 

ifndef MODEL
MODEL=C5V4
SPEC=SP
endif

export CONFIG_TP_IMAGE=y

#define basic variable
TOP_PATH = $(shell pwd)/..
BUILD_PATH = $(TOP_PATH)/build
TOOLS_PATH = $(TOP_PATH)/host_tools

ifneq ($(MAKECMDGOALS), sdk_config)

include .sdk_config/$(MODEL).config

ifeq ($(strip $(SUPPLIER_SDK)), )
$(error "SDK folder not specified, you have to make sdk_config first")
endif #ifeq ($(strip $(SUPPLIER_SDK)), )

SDK_PATH:=$(TOP_PATH)/../$(subst ",,$(SUPPLIER_SDK))
SDK_TP_PATH=$(SDK_PATH)/tplink
SDK_BUILD_PATH=$(SDK_TP_PATH)/build

export SDK_PATH SDK_TP_PATH SDK_BUILD_PATH

ifneq ($(SDK_PATH), $(wildcard $(SDK_PATH)))
$(error "SDK folder $(SDK_PATH) not found, please check or make sdk_config to specify again")
endif #ifneq ($(SDK_PATH), $(wildcard $(SDK_PATH)))

ifneq ($(MAKECMDGOALS), sdk_init)
include $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).config
include $(SDK_BUILD_PATH)/tplink.mak
endif #ifneq ($(MAKECMDGOALS), sdk_init)

endif #ifneq ($(MAKECMDGOALS), sdk_config)

ifeq ($(INCLUDE_VOIP), y)
include voip.mak
endif

FAKEROOT_PATH = $(TOOLS_PATH)/fakeroot

WEBFILE_PATH = $(TOP_PATH)/sysfiles/web/en
CONFFILE_PATH = $(TOP_PATH)/datas

PUBLIC_APPS_PATH = $(TOP_PATH)/apps/public
PRIVATE_APPS_PATH = $(TOP_PATH)/apps/private
OS_LIB_PATH = $(PUBLIC_APPS_PATH)/os_libs
PUBLIC_MODULES_PATH = $(SDK_TP_PATH)/modules/public
PRIVATE_MODULES_PATH = $(SDK_TP_PATH)/modules/private
KO_MODULES_PATH = $(SDK_TP_PATH)/modules/ko

####################################################
#
#Supplier PATH variable        
#
####################################################	
KERNELRELEASE=$(shell cat $(KERNELPATH)/include/config/kernel.release 2> /dev/null)
#KERNELVERSION=$(shell cat $(KERNELPATH)/include/linux/version.h 2> /dev/null | grep -o [0-9]\\\{1,\\\}$$)
INSTALL_MOD_PATH = $(KERNELPATH)/../

#Data PATH
ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5020)
SDK_DATAS_PATH = $(SDK_TP_PATH)/datas
else
SDK_DATAS_PATH = $(SDK_TP_PATH)/build
endif

#TG means target
TARGETS_PATH = $(SDK_TP_PATH)/output
BOOT_TG_PATH = $(TARGETS_PATH)/$(MODEL)/boot
KERNEL_TG_PATH = $(TARGETS_PATH)/$(MODEL)/kernel
MODULE_TG_PATH = $(TARGETS_PATH)/$(MODEL)/modules
DATAS_TG_PATH = $(TARGETS_PATH)/$(MODEL)/datas
ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5020)
FS_TG_PATH = $(TARGETS_PATH)/$(MODEL)/filesystem
else
FS_TG_PATH = $(PRODUCT_BUILD_PATH)/fs
endif
IMG_TG_PATH = $(TARGETS_PATH)/$(MODEL)/image
INSTALL_WIRELESS = $(TARGETS_PATH)/$(MODEL)/wireless

PRIVATE_LIB_BIN_PATH = $(PRIVATE_APPS_PATH)/bins/$(OBJ_DIR)/libs
PRIVATE_APPS_BIN_PATH = $(PRIVATE_APPS_PATH)/bins/$(OBJ_DIR)/apps

####################################################
#
#Supplier compiler variable        
#
####################################################	
CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
AR = $(TOOLPREFIX)ar
STRIP = $(TOOLPREFIX)strip
RANLIB = $(TOOLPREFIX)ranlib

MAKEARCH = $(MAKE) ARCH=$(ARCH)

####################################################
#
#PJSIP Variable
#
####################################################
PJSIP_CONFCACHE_EXIST = $(wildcard $(PUBLIC_APPS_PATH)/pjsip_1.10/config_cache_file) 

####################################################
#
#Function conditional compilation        
#
####################################################	

DFLAGS = 
WEBFLAGS = 
# DF_FLAGS will be exported, so function must be excuted immediately
DF_FLAGS := 

####################################################
#
#Config Variable 
#
####################################################	
include ./config.mak

ifeq ($(INCLUDE_ROUTER_MODE), y)
DFLAGS += -DINCLUDE_ROUTER_MODE
endif

ifeq ($(INCLUDE_SMART_DHCP), y)
DFLAGS += -DINCLUDE_SMART_DHCP
endif

DF_FLAGS += $(DFLAGS)

#NOTE:INCLUDE_VOIP in voip.mak
ifeq ($(INCLUDE_VOIP), y)
DFLAGS += $(VOIP_DFLAGS)
DF_FLAGS += $(VOIP_CFLAGS)
endif

####################################################
#
#Export some variable        
#
####################################################	
export ARCH
export TOOLPREFIX
export HOST
#For modules build & br2684ctl build
export KERNELPATH
#For install kernel modules
export INSTALL_MOD_PATH
#For code control, placed different target file
export OBJ_DIR
#For function conditional compilation
export DF_FLAGS

export TOP_PATH

export OS_LIB_PATH

export PRIVATE_LIB_BIN_PATH

export PRIVATE_APPS_BIN_PATH

#For complier
export CC
export LD
export AR
export STRIP
export RANLIB

export MAKEARCH

####################################################
#
# General target        
#
####################################################
.PHONY: boot_build kernel_build modules_build apps_build fs_build image_build
.PHONY: boot_clean kernel_clean modules_clean apps_clean fs_clean fakeroot_clean

all: boot_build kernel_build modules_build apps_build fs_build image_build

clean: boot_clean kernel_clean modules_clean apps_clean fs_clean fakeroot_clean

####################################################
#
#SDK configuration        
#
####################################################
.PHONY: sdk_config sdk_init model_init

sdk_config:
	mkdir -p $(BUILD_PATH)/.sdk_config
	if [ -f $(BUILD_PATH)/.sdk_config/$(MODEL).config ]; \
		then cp -pfT $(BUILD_PATH)/.sdk_config/$(MODEL).config $(BUILD_PATH)/.sdk_config/.config; \
	else \
		cp -pfT $(BUILD_PATH)/sysdeps/linux/defconfig $(BUILD_PATH)/.sdk_config/.config; \
	fi
	cd $(BUILD_PATH)/.sdk_config && $(BUILD_PATH)/scripts/config/mconf $(BUILD_PATH)/sysdeps/linux/SDK.in
	cp -pfT $(BUILD_PATH)/.sdk_config/.config $(BUILD_PATH)/.sdk_config/$(MODEL).config
	
sdk_init:
ifeq ($(SDK_TP_PATH), $(wildcard $(SDK_TP_PATH)))
	echo "SDK TP folder $(SDK_TP_PATH) already exist, you have to delete the folder to do a new sdk_init"
	exit 1
endif #ifneq ($(SDK_TP_PATH), $(wildcard $(SDK_TP_PATH)))
	#gen tplink.mak

model_init:
	#gen dirs
	mkdir -p $(SDK_BUILD_PATH)/$(MODEL)
	#gen menuconfig
	cp -pfT $(BUILD_PATH)/sysdeps/linux/defconfig $(BUILD_PATH)/.config; \
	#same as menuconfig
	cd $(BUILD_PATH) && $(BUILD_PATH)/scripts/config/mconf $(BUILD_PATH)/sysdeps/linux/Config.in
	cp -pfT $(BUILD_PATH)/.config $(SDK_BUILD_PATH)/$(MODEL)/$(MODEL).config
	#gen rcS
	#gen xml
	#mkdir -p $(SDK_DATAS_PATH)/$(MODEL)
	#cd $(SDK_DATAS_PATH)/$(MODEL) && ln -sf xml_Editor.exe ../../../../platform/datas/xml_Editor.exe
	#cp -pf $(CONFFILE_PATH)/Dev2DataModel.xml $(SDK_DATAS_PATH)/$(MODEL)

####################################################
#
#Compilation environment         
#
####################################################
.PHONY: env_build tools_build supplier_env_build supplier_tools env_clean supplier_env_clean

env_build: supplier_env_build
	mkdir -p $(BOOT_TG_PATH)
	mkdir -p $(KERNEL_TG_PATH)
	mkdir -p $(MODULE_TG_PATH)/kmdir/
	mkdir -p $(DATAS_TG_PATH)
	mkdir -p $(IMG_TG_PATH)
	mkdir -p $(INSTALL_WIRELESS)/sbin/
	mkdir -p $(INSTALL_WIRELESS)/lib/
env_clean: supplier_env_clean
tools_build: supplier_tools

####################################################
#
#Menuconfig         
#
####################################################
.PHONY: menuconfig

menuconfig: 
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).config $(BUILD_PATH)/.config
	#same as sdk_init
	cd $(BUILD_PATH) && $(BUILD_PATH)/scripts/config/mconf $(BUILD_PATH)/sysdeps/linux/Config.in
	cp -pfT $(BUILD_PATH)/.config $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).config
	
####################################################
#
#Build bootloader          
#
####################################################
.PHONY: supplier_boot_build supplier_boot_clean boot_path_check

ifeq ($(USE_SUPPLIER_BOOT_BUILD), y)
boot_build: supplier_boot_build
boot_clean: supplier_boot_clean
else #ifeq ($(USE_SUPPLIER_BOOT_BUILD), y)

boot_path_check:
	if [ -d $(BOOT_PATH) ]; \
	then \
		echo "BOOT_PATH: $(BOOT_PATH) not found, please check the BOOT_PATH defination"; \
		exit 1; \
	fi
	
boot_build: boot_path_check
	cd $(BOOT_PATH) && $(MAKE)
	cp -pfT $(BOOT_PATH)/boot.bin $(BOOT_TG_PATH)/boot.bin
	
boot_clean: boot_path_check
	cd $(BOOT_PATH) && $(MAKE) clean
	rm -f $(BOOT_TG_PATH)/boot.bin
endif #ifeq ($(USE_SUPPLIER_BOOT_BUILD), y)

####################################################
#
#Build linux kernel           
#
####################################################
.PHONY: kernel_menuconfig
.PHONY: supplier_kernel_build supplier_kernel_menuconfig supplier_kernel_modules supplier_kernel_clean kernel_path_check

ifeq ($(USE_SUPPLIER_KERNEL_BUILD), y)
kernel_build: supplier_kernel_build
kernel_menuconfig: supplier_kernel_menuconfig
kernel_modules: supplier_kernel_modules
kernel_clean: supplier_kernel_clean
else #ifeq ($(USE_SUPPLIER_KERNEL_BUILD), y)

kernel_path_check:
	@if [ ! -d $(KERNELPATH) ]; \
	then \
		echo "KERNELPATH: $(KERNELPATH) not found, please check the KERNELPATH defination"; \
		exit 1; \
	else \
		echo "KERNELPATH: $(KERNELPATH)"; \
	fi

kernel_build: kernel_path_check
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(MODEL).kconfig $(KERNELPATH)/.config
	cd $(KERNELPATH) && $(MAKE) V=2 linux.7z
	cp -pf $(KERNELPATH)/linux.7z $(KERNEL_TG_PATH)/
	cp -pf $(KERNELPATH)/vmlinux $(KERNEL_TG_PATH)/

kernel_menuconfig: kernel_path_check
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(MODEL).kconfig $(KERNELPATH)/.config
	cd $(KERNELPATH) && $(MAKE) menuconfig
	cp -pfT $(KERNELPATH)/.config $(SDK_BUILD_PATH)/$(MODEL)/$(MODEL).kconfig
	@echo "Replace Kernel Config File"

kernel_modules: kernel_path_check
	@echo "Making Kernel Modules"
	cp -pfT $(SDK_BUILD_PATH)/$(MODEL)/$(MODEL).kconfig $(KERNELPATH)/.config
	cd $(KERNELPATH) && $(MAKE) modules
	cd $(KERNELPATH) && $(MAKE) DEPMOD=true INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) modules_install
	cp -pfR $(INSTALL_MOD_PATH)/lib/modules/$(KERNELRELEASE)/* $(MODULE_TG_PATH)/kmdir/

kernel_clean: kernel_path_check
	cd $(KERNELPATH) && $(MAKE) clean
#rm -Rf $(INSTALL_MOD_PATH)/lib
	rm -Rf $(KERNEL_TG_PATH)/*
endif #ifeq ($(USE_SUPPLIER_KERNEL_BUILD), y)

####################################################
#
#Build all kernel modules
#
####################################################
.PHONY: kernel_modules public_modules supplier_modules
.PHONY: private_modules_clean public_modules_clean supplier_modules_clean

ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5020)
modules_build: kernel_modules public_modules supplier_modules
modules_clean: private_modules_clean public_modules_clean supplier_modules_clean
else
modules_build:supplier_modules
modules_clean:supplier_modules_clean
endif
	-rm -rf $(MODULE_TG_PATH)/* 
	-rm -rf $(MODULE_TG_PATH)/kmdir/* 

###############################################################################
#   Public Modules
###############################################################################
.PHONY: usbip_modules pptp_modules l2tp_modules
.PHONY: usbip_modules_clean pptp_modules_clean l2tp_modules_clean

public_modules: usbip_modules pptp_modules l2tp_modules
public_modules_clean: usbip_modules_clean pptp_modules_clean l2tp_modules_clean

ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
usbip_modules:
	cd $(PUBLIC_MODULES_PATH)/usbip-0.1.7 && $(MAKE)
	cp -f $(PUBLIC_MODULES_PATH)/usbip-0.1.7/usbip.ko $(MODULE_TG_PATH)

usbip_modules_clean:
	cd $(PUBLIC_MODULES_PATH)/usbip-0.1.7 && $(MAKE) clean
endif #ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)

ifeq ($(INCLUDE_PPTP), y)
pptp_modules:
	# less then 2.6.23
	if [ $(KERNELVERSION) -lt 132631 ]; \
	then \
		cd $(PUBLIC_MODULES_PATH)/accel-pptp-0.8.5 && $(MAKE); \
		cp -f $(PUBLIC_MODULES_PATH)/accel-pptp-0.8.5/pptp.ko $(MODULE_TG_PATH); \
	else \
		echo "Kernel version is not lower than 2.6.23, use pptp driver in kernel"; \
	fi

pptp_modules_clean:
	# less then 2.6.23
	if [ $(KERNELVERSION) -lt 132631 ]; \
	then \
		cd $(PUBLIC_MODULES_PATH)/accel-pptp-0.8.5 && $(MAKE) clean; \
	fi
endif #ifeq ($(INCLUDE_PPTP), y)

ifeq ($(INCLUDE_L2TP), y)
l2tp_modules:
	# less then 2.6.23
	if [ $(KERNELVERSION) -lt 132631 ]; \
	then \
		cd $(PUBLIC_MODULES_PATH)/pppol2tp-kmod-0.17 && $(MAKE); \
		cp -f $(PUBLIC_MODULES_PATH)/pppol2tp-kmod-0.17/pppol2tp.ko $(MODULE_TG_PATH); \
	else \
		echo "Kernel version is not lower than 2.6.23, use pppol2tp v1.0 driver in kernel"; \
	fi
	
l2tp_modules_clean:
	# less then 2.6.23
	if [ $(KERNELVERSION) -lt 132631 ]; \
	then \
		cd $(PUBLIC_MODULES_PATH)/pppol2tp-kmod-0.17 && $(MAKE) clean; \
	fi
endif #ifeq ($(INCLUDE_L2TP), y)

###############################################################################
#   Private Modules
###############################################################################
.PHONY: board_modules voip_modules stat_modules login_modules iqos_modules
.PHONY: board_modules_clean voip_modules_clean stat_modules_clean login_modules_clean iqos_modules_clean

private_modules: board_modules voip_modules stat_modules login_modules iqos_modules
private_modules_clean: board_modules_clean voip_modules_clean stat_modules_clean login_modules_clean iqos_modules_clean

board_modules:
	cd $(PRIVATE_MODULES_PATH)/board/ && $(MAKE)
	cd $(PRIVATE_MODULES_PATH)/board && cp tp_board.ko $(MODULE_TG_PATH)
	
board_modules_clean:
	cd $(PRIVATE_MODULES_PATH)/board/ && $(MAKE) clean

ifneq ($(strip $(INCLUDE_VOIP)),) 
voip_modules:
	cd $(PRIVATE_MODULES_PATH)/voip/ && $(MAKE)
	cd $(PRIVATE_MODULES_PATH)/voip && cp voip.ko $(MODULE_TG_PATH)
	
voip_modules_clean:
	cd $(PRIVATE_MODULES_PATH)/voip/ && $(MAKE) clean
endif

stat_modules:
	cd $(PRIVATE_MODULES_PATH)/ipt_STAT && $(MAKE)
	cd $(PRIVATE_MODULES_PATH)/ipt_STAT && cp ipt_STAT.ko $(MODULE_TG_PATH)

stat_modules_clean:
	cd $(PRIVATE_MODULES_PATH)/ipt_STAT && $(MAKE) clean
	
login_modules:
	cd $(PRIVATE_MODULES_PATH)/domain_login && $(MAKE)
	cd $(PRIVATE_MODULES_PATH)/domain_login && cp tp_domain.ko $(MODULE_TG_PATH)
	
login_modules_clean:
	cd $(PRIVATE_MODULES_PATH)/domain_login && $(MAKE) clean

ifeq ($(INCLUDE_APP_IQOS),y)
iqos_modules:
	cd $(PRIVATE_MODULES_PATH)/qos-appidentify && $(MAKE)
	cd $(PRIVATE_MODULES_PATH)/qos-appidentify && cp appid.ko $(MODULE_TG_PATH)
	
	cd $(PRIVATE_MODULES_PATH)/xt_app && $(MAKE)
	cd $(PRIVATE_MODULES_PATH)/xt_app && cp xt_app.ko $(MODULE_TG_PATH)
	
iqos_modules_clean:
	cd $(MODULES_PATH)/qos-appidentify && $(MAKE) clean
	cd $(MODULES_PATH)/xt_app && $(MAKE) clean
endif #ifeq ($(INCLUDE_APP_IQOS),y)

####################################################
#
#Build libs
#
####################################################

	
####################################################
#
#Build apps
#
####################################################

#openssl should before pjsip case pjsip need it
#libusb&libcompat should before handle_card&usb_modeswitch
.PHONY: public_apps_build supplier_apps_build

apps_build: public_apps_build supplier_apps_build

####################################################
#
#Build private apps
#
####################################################
.PHONY: cmm voip hotplug usbp handle_card

private_apps_build: cmm voip hotplug usbp handle_card wirelesstool

cmm: oslibs
	cd $(PRIVATE_APPS_PATH)/user/ && $(MAKE) apps_build xmode

hotplug:
ifeq ($(INCLUDE_USB_STORAGE), y)
	cd $(PRIVATE_APPS_PATH)/hotplug/ && $(MAKE)
else

ifeq ($(INCLUDE_USB_3G_DONGLE), y)
	cd $(PRIVATE_APPS_PATH)/hotplug/ && $(MAKE)
endif

endif

usbp:
ifeq ($(INCLUDE_USB_STORAGE), y)
	cd $(PRIVATE_APPS_PATH)/usbp/ && $(MAKE)
endif	

.PHONY: wirelesstool
wirelesstool:
ifeq ($(INCLUDE_LAN_WLAN), y) 
	mkdir -p $(INSTALL_WIRELESS)/sbin/
	mkdir -p $(INSTALL_WIRELESS)/lib/
ifeq ($(strip $(SUPPLIER)),mtk)
	cd $(TOP_PATH)/$(SUPPLIER) &&  make PROFILE=$(CPU_TYPE)_demo wirelesstool
endif
ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5020)
	@echo "build wireless tools"
	cd $(WIRELESSTOOLS) &&  make realclean && make all;\
	cp -f ated_tp $(INSTALL_WIRELESS)/sbin;\
	cp -f $(WIRELESSTOOLSLIB) $(INSTALL_WIRELESS)/lib;\
	cp -f iwpriv $(INSTALL_WIRELESS)/sbin;\
	cp -f iwconfig $(INSTALL_WIRELESS)/sbin;\
	cp -f iwlist $(INSTALL_WIRELESS)/sbin;\
    cp -f wlNetlinkTool $(INSTALL_WIRELESS)/sbin

	#@echo "build wireless tools"
	#cd $(WIRELESSTOOLS)/../sysstat-9.0.6 &&  make clean && make mpstat;\
	#cp -f mpstat $(INSTALL_WIRELESS)/sbin
ifeq ($(INCLUDE_ATED_IWPRIV), y)
	@echo "use ated_iwpriv instead of ated_tp"
	cd $(WIRELESSTOOLS) && cp -f ated_iwpriv $(INSTALL_WIRELESS)/sbin/ated_tp
endif
	
ifeq ($(INCLUDE_HARDWARE_DEBUG),y)
	@echo "build ated for QA"
	cd $(WIRELESSTOOLS)/../ated && make clean && make all;\
	cp -f ated $(INSTALL_WIRELESS)/sbin
endif

	echo "Build the rt2860apd for 8021x"
	cd $(WIRELESSTOOLS)/../8021x &&  make clean && make all;\
	cp -f rt2860apd $(INSTALL_WIRELESS)/sbin/;\
	ln -sf rt2860apd $(INSTALL_WIRELESS)/sbin/rtinicapd

	echo "Build wscd for wps while coexist of dual-band"
	cd $(WIRELESSTOOLS)/../wsc_upnp/ && chmod -R +rx ./ && make clean && make all;\
	cp -f wscd $(INSTALL_WIRELESS)/sbin/;\
	ln -sf wscd $(INSTALL_WIRELESS)/sbin/wscd_5G
endif

else	#INCLUDE_LAN_WLAN
	@echo no wifi
endif	#INCLUDE_LAN_WLAN


voip:
ifneq ($(strip $(INCLUDE_VOIP)),)
	cd $(PRIVATE_APPS_PATH)/voip / && $(MAKE) -B
else
	@echo no voip
endif

handle_card:
ifeq ($(INCLUDE_USB_3G_DONGLE), y)
	@echo "Building handle_card"
	$(MAKE) -C $(PRIVATE_APPS_PATH)/handle_card
else
	@echo no handle_card
endif #INCLUDE_USB_3G_DONGLE

####################################################
#
#Build public apps
#
####################################################
.PHONY: oslibs libusb libcompat openssl pjsip \
					br2684ctl busybox tc iptables ebtables pppd rip \
					upnplib upnp flex radvd dhcpv6 map samba ntfs-3g arpd \
					usbip vsftp dlna ip ipsectools \
					chat usb_modeswitch xl2tpd dropbear ividns mtd-utils
public_apps_build: oslibs libusb libcompat openssl pjsip \
					br2684ctl busybox tc iptables ebtables pppd rip \
					upnplib upnp flex radvd dhcpv6 map samba ntfs-3g arpd \
					usbip vsftp dlna ip ipsectools openvpn pptpd\
					chat usb_modeswitch xl2tpd dropbear ividns mtd-utils

oslibs: 
	cd $(PUBLIC_APPS_PATH)/os_libs && $(MAKE)

busybox:
	-cp -f $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/busybox.config $(PUBLIC_APPS_PATH)/busybox-1.23.2/.config
	cd $(PUBLIC_APPS_PATH)/busybox-1.23.2 && $(MAKE)

tc:
	# kernel version > 3.4
	#if [ $(KERNELVERSION) -gt 197632 ]; \
	#then \
		#cd $(PUBLIC_APPS_PATH)/iproute2-2.6.24-rc7/include/linux/ && mv types.h types.bak; \
		#cd $(PUBLIC_APPS_PATH)/iproute2-2.6.24-rc7 && $(MAKE); \
		#cd $(PUBLIC_APPS_PATH)/iproute2-2.6.24-rc7/include/linux/ && mv types.bak types.h; \
	#else \
		#cd $(PUBLIC_APPS_PATH)/iproute2-2.6.24-rc7 && $(MAKE); \
	#fi

iptables:
	cd $(PUBLIC_APPS_PATH)/iptables-1.4.17 && test -e Makefile || ./configure --host=$(HOST) --enable-static --disable-shared  
	cd $(PUBLIC_APPS_PATH)/iptables-1.4.17 && $(MAKE) AR=$(TOOLPREFIX)ar
	$(STRIP) $(PUBLIC_APPS_PATH)/iptables-1.4.17/iptables/xtables-multi

ebtables:
	cd $(PUBLIC_APPS_PATH)/ebtables-v2.0.10-4 && $(MAKE) static

pppd:
ifeq ($(INCLUDE_PPTPVPN_SERVER), y)
	cd $(PUBLIC_APPS_PATH)/ppp-2.4.5/pppd && $(MAKE) MPPE=y INCLUDE_RUSSIA_SPEC=$(INCLUDE_RUSSIA_SPEC) #INCLUDE_IPV6_PPPV6=$(INCLUDE_IPV6_PPPV6)
else
	cd $(PUBLIC_APPS_PATH)/ppp-2.4.5/pppd && $(MAKE) INCLUDE_RUSSIA_SPEC=$(INCLUDE_RUSSIA_SPEC) #INCLUDE_IPV6_PPPV6=$(INCLUDE_IPV6_PPPV6)
endif

arpd:
	#cd $(PUBLIC_APPS_PATH)/linux-atm && $(MAKE)

upnplib:
	$(MAKE) -C $(PUBLIC_APPS_PATH)/upnp_libs

upnp:
	$(MAKE) -C $(PUBLIC_APPS_PATH)/IGD

rip:
ifeq ($(INCLUDE_RIP)_$(INCLUDE_RIPNG), y_y)
	cd $(PUBLIC_APPS_PATH)/zebra-0.95a && ./configure --host=$(HOST) --disable-bgpd --disable-ospfd --disable-ospf6d host_alias=mips-linux CC=$(CC) --with-cflags="-s -O3 -fomit-frame-pointer -DTP_CMM_MSG -D__LINUX_OS_FC__ -I../../os_libs/include/"
	$(MAKE) -C $(PUBLIC_APPS_PATH)/zebra-0.95a	
else ifeq ($(INCLUDE_RIP), y)
	cd $(PUBLIC_APPS_PATH)/zebra-0.95a && ./configure --host=$(HOST) --disable-ipv6 --disable-ripngd --disable-bgpd --disable-ospfd --disable-ospf6d host_alias=mips-linux CC=$(CC) --with-cflags="-s -O3 -fomit-frame-pointer -DTP_CMM_MSG -D__LINUX_OS_FC__ -I../../os_libs/include/"
	$(MAKE) -C $(PUBLIC_APPS_PATH)/zebra-0.95a
else ifeq ($(INCLUDE_RIPNG), y)
	cd $(PUBLIC_APPS_PATH)/zebra-0.95a && ./configure --host=$(HOST) --disable-ripd --disable-bgpd --disable-ospfd --disable-ospf6d host_alias=mips-linux CC=$(CC) --with-cflags="-s -O3 -fomit-frame-pointer -DTP_CMM_MSG -D__LINUX_OS_FC__ -I../../os_libs/include/"
	$(MAKE) -C $(PUBLIC_APPS_PATH)/zebra-0.95a
else
	@echo no rip
endif

pjsip:
ifneq ($(strip $(INCLUDE_VOIP)),)
ifneq ($(strip $(PJSIP_CONFCACHE_EXIST)),)
	cd $(PUBLIC_APPS_PATH)/pjsip_1.10 && $(MAKE)
else
	cd $(PUBLIC_APPS_PATH)/pjsip_1.10 && ./configure --cache-file=./config_cache_file --srcdir=./ --prefix=$(TOOLROOTPATH) --exec-prefix=$(TOOLROOTPATH) --host=$(HOST) --target=mipsel-linux --disable-speex-aec --disable-l16-codec --disable-gsm-codec --disable-speex-codec --disable-ilbc-codec --disable-g7221-codec --disable-ssl --disable-oss --disable-sound && $(MAKE)
endif
else
	@echo no pjsip 
endif

usbip:
ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/usbip-0.1.7 clean
	cd $(PUBLIC_APPS_PATH)/usbip-0.1.7 && $(MAKE)
endif

vsftp:
ifeq ($(INCLUDE_USB_FTP_SERVER), y)
	cd $(PUBLIC_APPS_PATH)/vsftpd-2.3.2 && $(MAKE)
else 
	@echo no vsftp
endif

flex:
ifeq ($(INCLUDE_IPV6), y)
	cd $(PUBLIC_APPS_PATH)/flex-2.5.35 && test -e Makefile || ./configure --host=$(HOST)
	cd $(PUBLIC_APPS_PATH)/flex-2.5.35 && $(MAKE)
	mkdir -p $(PUBLIC_APPS_PATH)/lib
	cp -p $(PUBLIC_APPS_PATH)/flex-2.5.35/libfl.a $(PUBLIC_APPS_PATH)/lib
else 
	@echo no ipv6 flex
endif

radvd:
ifeq ($(INCLUDE_IPV6), y)
	cd $(PUBLIC_APPS_PATH)/radvd-1.5 && test -e Makefile || ./configure --host=$(HOST)
	#cd $(PUBLIC_APPS_PATH)/radvd-1.8.3 && export CFLAGS+="$(DF_FLAGS) "; $(MAKE)
	cd $(PUBLIC_APPS_PATH)/radvd-1.5 && export CFLAGS+="$(DF_FLAGS) "; $(MAKE)
else 
	@echo no ipv6 radvd
endif

dhcpv6:
ifeq ($(INCLUDE_IPV6), y)
	#cd $(PUBLIC_APPS_PATH)/dibbler-1.0.1 && test -e Makefile || \
	#( export CPPFLAGS+="$(DF_FLAGS) "; \
	# export CPPFLAGS+="-I$(OS_LIB_PATH)/include -D__LINUX_OS_FC__ -DCMM_MSG "; \
	# export LDFLAGS+="-L$(OS_LIB_PATH)/ -los -lpthread -lrt "; \
	# ./configure --prefix=$(PUBLIC_APPS_PATH)/dibbler-1.0.1/install \
	# --host=$(HOST)	--disable-dns-update --enable-auth --disable-debug\
	#)
	#$(MAKE) -C $(PUBLIC_APPS_PATH)/dibbler-1.0.1/ server client	
	#$(STRIP) $(PUBLIC_APPS_PATH)/dibbler-1.0.1/dibbler-client	
	#$(STRIP) $(PUBLIC_APPS_PATH)/dibbler-1.0.1/dibbler-server
	
	cd $(PUBLIC_APPS_PATH)/wide-dhcpv6-20080615 && export CFLAGS+="$(DF_FLAGS) "; $(MAKE)
else 
	@echo no ipv6 dhcpv6
endif

map:
ifeq ($(INCLUDE_IPV6_MAP), y)
	cd $(PUBLIC_APPS_PATH)/MAP/modules && $(MAKE) CROSS_COMPILE="$(TOOLPREFIX)"
	cd $(PUBLIC_APPS_PATH)/MAP/utils && $(MAKE) CROSS_COMPILE="$(TOOLPREFIX)"
else
	@echo no ipv6 map
endif

ividns:
ifeq ($(INCLUDE_IPV6), y)
	#cd $(PUBLIC_APPS_PATH)/ividns0.2 && $(MAKE) CROSS_COMPILE="$(TOOLPREFIX)"	
else 
	@echo no ipv6 ividns
endif

ip: 
#ifeq ($(INCLUDE_IPV6), y)
	cd $(PUBLIC_APPS_PATH)/iproute2-2.6.39 && $(MAKE)
#endif

#for samba and ntfs-3g, zjj, 20111129
samba:
ifeq ($(INCLUDE_USB_SAMBA_SERVER), y)
#	cd $(PUBLIC_APPS_PATH)/samba-3.0.14a/source && test -e Makefile || ./configure --host=$(HOST) --prefix=/var/samba 
	cd $(PUBLIC_APPS_PATH)/samba-3.0.14a/source && $(MAKE)
	$(STRIP) $(PUBLIC_APPS_PATH)/samba-3.0.14a/source/bin/*
else 
	@echo no samba
endif

#modified by LY on 2015.01.28, as ntfs-3g is not needed when use tntfs.ko provided by Tuxera 	
ntfs-3g:
ifeq ($(INCLUDE_USB_STORAGE), y)
ifneq ($(INCLUDE_USB_STORAGE_TUXERA_NTFS), y)
	cd $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15 && test -e Makefile || ./configure --build=i686-linux --host=$(HOST) --disable-nfconv
	cd $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15 && $(MAKE)
	$(STRIP) $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15/libntfs-3g/.libs/libntfs-3g.so.83.0.0
	$(STRIP) $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15/src/.libs/ntfs-3g
	$(STRIP) $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15/src/.libs/lowntfs-3g
endif
else 
	@echo no ntfs-3g+ntfsprogs
endif
#end modified by LY

#end by zjj

#for mediaServer, modify by zengdongbiao, 21May15.
export MEDIA_SERVER_PATH = $(PUBLIC_APPS_PATH)/media_server
export MEDIA_SERVER_INSTALL=$(MEDIA_SERVER_PATH)/media_server_install
dlna: 
ifeq ($(INCLUDE_USHARE), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/ffmpeg-0.6/
	$(MAKE) -C $(PUBLIC_APPS_PATH)/libdlna-0.2.3/
	$(MAKE) -C $(PUBLIC_APPS_PATH)/ushare-1.1a/
endif
ifeq ($(INCLUDE_MINIDLNA), y)
	cd $(PUBLIC_APPS_PATH)/media_server && $(MAKE) media_share 
	find $(MEDIA_SERVER_INSTALL)/lib -name "*.so*" -type f | xargs $(STRIP) --strip-unneeded
endif
ifeq ($(INCLUDE_LITE_MINIDLNA), y)
	cd $(PUBLIC_APPS_PATH)/media_server && $(MAKE) lite_media_share 
endif
#end modify by zengdongbiao

openssl:
ifeq ($(INCLUDE_SSL), y)
	cd $(PUBLIC_APPS_PATH)/openssl-0.9.8zh && test -e Makefile || ./Configure shared \
		no-threads no-zlib no-asm no-gmp no-krb5 no-hw no-cipher no-rfc3779 no-sse2 no-speed \
		no-md2 no-md4 no-mdc2 no-ripemd no-rc5 no-idea no-bf no-cast no-camellia no-ssl2 no-ssl3 \
		no-ecdsa linux-generic32 --prefix=$(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install \
	&& sed -i 's/-O3/-Os -DGNU/g' Makefile
	
	cd $(PUBLIC_APPS_PATH)/openssl-0.9.8zh \
    && $(MAKE) CC=$(CC) LD=$(LD) SHARED_LDFLAGS="-s" install \
	&& mkdir -p ./lib \
	&& cp -p ./libcrypto.so.0.9.8 ./lib/ \
	&& cp -p ./libssl.so.0.9.8 ./lib/ \
	&& cd ./lib/ \
	&& ln -sf libcrypto.so.0.9.8 libcrypto.so.0 \
	&& ln -sf libcrypto.so.0.9.8 libcrypto.so \
	&& ln -sf libssl.so.0.9.8 libssl.so.0 \
	&& ln -sf libssl.so.0.9.8 libssl.so
	$(STRIP) $(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install/bin/openssl
else
ifeq ($(INCLUDE_LOGIN_GDPR_ENCRYPT), y)
	cd $(PUBLIC_APPS_PATH)/openssl-0.9.8zh && test -e Makefile || ./Configure -fPIC no-shared \
		no-threads no-zlib no-asm no-gmp no-krb5 no-hw no-cipher no-rfc3779 no-sse2 no-speed \
		no-md2 no-md4 no-mdc2 no-ripemd no-rc5 no-idea no-bf no-cast no-camellia \
		no-ec no-dsa no-ecdsa no-store linux-mips \
	&& sed -i 's/-O3/-Os -DGNU/g' Makefile
	
	cd $(PUBLIC_APPS_PATH)/openssl-0.9.8zh \
	&& $(MAKE) CC=$(CC) LD=$(LD)\
	&& cp -p ./libcrypto.a $(SLIB_PATH)/../usr/lib/
endif
endif

ipsectools:
ifeq ($(INCLUDE_IPSEC), y)
# dbs added 2014-11-03
ifeq ($(strip $(SUPPLIER)),qualcomm)
	cd $(PUBLIC_APPS_PATH)/ipsectools && test -e Makefile || ./configure --host=$(HOST) --with-kernel-headers=$(KERNEL_SRC_PATH)/include \
	--with-openssl=$(PUBLIC_APPS_PATH)/openssl/ --enable-security-context=no --enable-shared=yes \
	--enable-dpd=yes CFLAGS="-Os -DGNU -D__EXPORTED_HEADERS__" 
# end 2014-10-03
else	
	#cd $(PUBLIC_APPS_PATH)/ipsectools && test -e Makefile || ./configure --host=$(HOST) --with-kernel-headers=$(KERNELPATH)/include \
	#--with-openssl=$(PUBLIC_APPS_PATH)/openssl/ --enable-security-context=no --enable-shared=yes \
	#--enable-dpd=yes CFLAGS="-Os -DGNU" 
endif	
	#cd $(PUBLIC_APPS_PATH)/ipsectools && $(MAKE)
	#$(STRIP) $(PUBLIC_APPS_PATH)/ipsectools/src/racoon/.libs/racoon
	#$(STRIP) $(PUBLIC_APPS_PATH)/ipsectools/src/setkey/.libs/setkey
	#$(STRIP) $(PUBLIC_APPS_PATH)/ipsectools/src/racoon/.libs/libracoon.so.0.0.0
	#$(STRIP) $(PUBLIC_APPS_PATH)/ipsectools/src/libipsec/.libs/libipsec.so.0.0.1
endif

##### added by CCy for vpn, 20150630  #####
.PHONY: lzo
lzo:
ifeq ($(INCLUDE_OPENVPN_SERVER), y)
	cd $(PUBLIC_APPS_PATH)/lzo-2.09 && test -e Makefile || ./configure --host=$(HOST) --prefix=$(PUBLIC_APPS_PATH)/lzo-2.09/install --enable-shared
	cd $(PUBLIC_APPS_PATH)/lzo-2.09 && $(MAKE) install-strip
endif
	
.PHONY: openvpn
openvpn: lzo
ifeq ($(INCLUDE_OPENVPN_SERVER), y)
	cd $(PUBLIC_APPS_PATH)/openvpn-2.3.6 && test -e Makefile || ./configure --host=$(HOST) --prefix=$(PUBLIC_APPS_PATH)/openvpn-2.3.6/install \
		IFCONFIG=/sbin/ifconfig ROUTE=/sbin/route IPROUTE=/usr/bin/ip NETSTAT=/bin/netstat \
		--enable-small --disable-selinux --disable-systemd --disable-plugins --disable-debug --disable-eurephia --disable-pkcs11 --enable-password-save --enable-management \
		--enable-lzo --disable-x509-alt-username-ssl --enable-fragment --disable-iproute2 --enable-pf --enable-ssl --enable-crypto --with-crypto-library=openssl \
		OPENSSL_CRYPTO_CFLAGS=-I$(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install/include OPENSSL_CRYPTO_LIBS="-L$(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install/lib -lcrypto" \
		OPENSSL_SSL_CFLAGS=-I$(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install/include OPENSSL_SSL_LIBS="-L$(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install/lib -lssl" \
		LZO_CFLAGS=-I$(PUBLIC_APPS_PATH)/lzo-2.09/install/include LZO_LIBS="-L$(PUBLIC_APPS_PATH)/lzo-2.09/install/lib -llzo2"
	cd $(PUBLIC_APPS_PATH)/openvpn-2.3.6 && $(MAKE) install-strip
endif

##### added by CCy for vpn, 20150630  #####
#because --sbindir is used for program when running, do not define --prefix and do not $(MAKE) install,
# or else install will install in the wrong directory.
.PHONY: pptpd
pptpd:
ifeq ($(INCLUDE_PPTPVPN_SERVER), y)
	cd $(PUBLIC_APPS_PATH)/pptpd-1.4.0 && test -e Makefile || ./configure --host=$(HOST) \
		ac_cv_header_libintl_h=no --sbindir=/usr/sbin --enable-bcrelay
	cd $(PUBLIC_APPS_PATH)/pptpd-1.4.0 && $(MAKE)
endif
###########################################

chat:
ifeq ($(INCLUDE_USB_3G_DONGLE), y)
	@echo building chat...
	cd $(PUBLIC_APPS_PATH)/ppp-2.4.5/chat && $(MAKE)
	$(STRIP) $(PUBLIC_APPS_PATH)/ppp-2.4.5/chat/chat
else
	@echo no chat
endif

ifeq ($(INCLUDE_USB_SYSFS), y)
LIBUSB_CFLAGS = "-DINCLUDE_USB_SYSFS -g -O2"
else
LIBUSB_CFLAGS = "-g -O2"
endif

libusb:
ifneq ($(strip $(filter y, $(INCLUDE_USB_3G_DONGLE) $(INCLUDE_USB_SYSFS))),)
	echo "Building usb library"
	cd $(PUBLIC_APPS_PATH)/libusb-1.0.8 && test -e Makefile || (export CFLAGS+=$(LIBUSB_CFLAGS); ./configure --build=i686-linux --host=$(HOST) --prefix=$(PUBLIC_APPS_PATH)/libusb-1.0.8/install)
	cd $(PUBLIC_APPS_PATH)/libusb-1.0.8 && $(MAKE)
	cd $(PUBLIC_APPS_PATH)/libusb-1.0.8 && $(MAKE) install
else
	@echo no libusb
endif

libcompat:
ifneq ($(strip $(filter y, $(INCLUDE_USB_3G_DONGLE) $(INCLUDE_USB_SYSFS))),)
	echo "Building usb compat"
	cd $(PUBLIC_APPS_PATH)/libusb-compat-0.1.3 && test -e Makefile || (export CFLAGS+=$(LIBUSB_CFLAGS); ./configure --build=i686-linux --host=$(HOST) --prefix=$(PUBLIC_APPS_PATH)/libusb-compat-0.1.3/install LIBUSB_1_0_CFLAGS=-I$(PUBLIC_APPS_PATH)/libusb-1.0.8/install/include/libusb-1.0 LIBUSB_1_0_LIBS=-L$(PUBLIC_APPS_PATH)/libusb-1.0.8/install/lib\ -lusb-1.0)
	cd $(PUBLIC_APPS_PATH)/libusb-compat-0.1.3 && $(MAKE)
	cd $(PUBLIC_APPS_PATH)/libusb-compat-0.1.3 && $(MAKE) install
else
	@echo no libcompat
endif

usb_modeswitch:
ifeq ($(INCLUDE_USB_3G_DONGLE), y)
	echo "Building usb_modeswitch"
	cd $(PUBLIC_APPS_PATH)/usb-modeswitch-1.2.3 && $(MAKE)
	$(STRIP) $(PUBLIC_APPS_PATH)/usb-modeswitch-1.2.3/usb_modeswitch
else
	@echo no usb_modeswitch
endif

xl2tpd:
ifeq ($(INCLUDE_L2TP), y)
	cd $(PUBLIC_APPS_PATH)/xl2tpd-1.1.12 && $(MAKE)
endif

dropbear:
ifeq ($(INCLUDE_PORTABLE_APP), y)
	cd $(PUBLIC_APPS_PATH)/dropbear && $(MAKE) MULTI=1 SCPPROGRESS=1
else
	@echo no dropbear
endif

.PHONY:lzo libz e2fsprogs
libz:
ifeq ($(INCLUDE_MTD_UTILS), y)
	cd $(PUBLIC_APPS_PATH)/zlib-1.2.6 && test -e Makefile || ./configure --prefix=$(PUBLIC_APPS_PATH)/zlib-1.2.6/install
	cd $(PUBLIC_APPS_PATH)/zlib-1.2.6 && $(MAKE)
	cd $(PUBLIC_APPS_PATH)/zlib-1.2.6 && $(MAKE) install
endif
e2fsprogs:
ifeq ($(INCLUDE_MTD_UTILS), y)
	cd $(PUBLIC_APPS_PATH)/e2fsprogs-1.41.3 && test -e Makefile || ./configure --prefix=$(PUBLIC_APPS_PATH)/e2fsprogs-1.41.3/install --host=$(HOST)
	cd $(PUBLIC_APPS_PATH)/e2fsprogs-1.41.3 && $(MAKE)
	cd $(PUBLIC_APPS_PATH)/e2fsprogs-1.41.3 && mkdir -p $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/include/uuid/ && cp -fpR lib/uuid/uuid.h $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/include/uuid/ && cp -fpR lib/libuuid.a $(PUBLIC_APPS_PATH)/zlib-1.2.6/install/lib
endif
mtd-utils:lzo libz e2fsprogs
ifeq ($(INCLUDE_MTD_UTILS), y)
	echo "Building mtd-utils"
	cd $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0 && $(MAKE) all LZOCFLAGS=-I$(PUBLIC_APPS_PATH)/lzo-2.09/install/include ZLIBCFLAGS=-I$(PUBLIC_APPS_PATH)/zlib-1.2.6/install/include  LZOLDFLAGS="-L$(PUBLIC_APPS_PATH)/lzo-2.09/install/lib -llzo2" ZLIBLDFLAGS=-L$(PUBLIC_APPS_PATH)/zlib-1.2.6/install/lib
else
	@echo no mtd-utils
endif 
###############################################################################
# clean application
###############################################################################
.PHONY: cmm_clean voip_clean pjsip_clean supplier_apps_clean
apps_clean: cmm_clean voip_clean pjsip_clean supplier_apps_clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/busybox-1.23.2 clean
	#$(MAKE) -C $(PUBLIC_APPS_PATH)/iproute2-2.6.24-rc7  clean
	-$(MAKE) -C $(PUBLIC_APPS_PATH)/iptables-1.4.17 clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/br2684ctl clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/ebtables-v2.0.10-4 clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/ppp-2.4.5/pppd clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/linux-atm clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/upnp_libs clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/IGD clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/os_libs clean


ifeq ($(INCLUDE_SSL), y)
	-$(MAKE) -C $(PUBLIC_APPS_PATH)/openssl-0.9.8zh clean
endif
	
ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/usbip-0.1.7 clean
endif

ifeq ($(INCLUDE_USB_FTP_SERVER), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/vsftpd-2.3.2 clean
endif

ifneq ($(findstring, $(INCLUDE_RIP)$(INCLUDE_RIPNG), y), )
	$(MAKE) -C $(PUBLIC_APPS_PATH)/zebra-0.95a clean
endif

ifeq ($(INCLUDE_IPV6), y)
	#$(MAKE) -C $(PUBLIC_APPS_PATH)/radvd-1.8.3 clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/radvd-1.5 clean
endif

ifeq ($(INCLUDE_IPV6), y)
	#$(MAKE) -C $(PUBLIC_APPS_PATH)/dibbler-1.0.1/ clean
endif

ifeq ($(INCLUDE_IPV6), y)
	cd $(PUBLIC_APPS_PATH)/flex-2.5.35 && test -e Makefile || ./configure --host=$(HOST)
	cd $(PUBLIC_APPS_PATH)/flex-2.5.35 && $(MAKE) clean
	rm -rf $(PUBLIC_APPS_PATH)/lib
endif
	
#ifeq ($(INCLUDE_IPV6), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/iproute2-2.6.39 clean
#endif

ifeq ($(INCLUDE_IPV6_MAP), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/MAP/modules clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/MAP/utils clean
endif 

ifeq ($(INCLUDE_IPV6), y)
	#$(MAKE) -C $(PUBLIC_APPS_PATH)/ividns0.2 clean
endif

ifeq ($(INCLUDE_USB_STORAGE), y)
	#for the clean of samba and ntfs-3g, zjj, 20111129
ifeq ($(INCLUDE_USB_SAMBA_SERVER), y)
	cd $(PUBLIC_APPS_PATH)/samba-3.0.14a/source && $(MAKE) clean
endif	#INCLUDE_USB_SAMBA_SERVER
	-cd $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15 && $(MAKE) clean
	cd $(PRIVATE_APPS_PATH)/usbp && $(MAKE) clean
	cd $(PRIVATE_APPS_PATH)/hotplug && $(MAKE) clean
	#end by zjj
else
ifeq ($(INCLUDE_USB_3G_DONGLE), y)
	cd $(PRIVATE_APPS_PATH)/hotplug && $(MAKE) clean
endif #INCLUDE_USB_3G_DONGLE
endif	#INCLUDE_USB_STORAGE

ifeq ($(INCLUDE_USB_3G_DONGLE), y)
	cd $(PUBLIC_APPS_PATH)/ppp-2.4.5/chat && $(MAKE) clean
	cd $(PUBLIC_APPS_PATH)/usb-modeswitch-1.2.3 && $(MAKE) clean
	cd $(PRIVATE_APPS_PATH)/handle_card && $(MAKE) clean
endif

ifneq ($(strip $(filter y, $(INCLUDE_USB_3G_DONGLE) $(INCLUDE_USB_SYSFS))),)
	-cd $(PUBLIC_APPS_PATH)/libusb-1.0.8 && $(MAKE) clean && rm -rf install/*
	-cd $(PUBLIC_APPS_PATH)/libusb-compat-0.1.3 && $(MAKE) clean && rm -rf install/*
endif

#for DLNA_CLEAN
ifeq ($(INCLUDE_USHARE), y)
	$(MAKE) -C $(PUBLIC_APPS_PATH)/ffmpeg-0.6 clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/libdlna-0.2.3 clean
	$(MAKE) -C $(PUBLIC_APPS_PATH)/ushare-1.1a clean
endif
ifeq ($(INCLUDE_MINIDLNA), y)
	cd $(PUBLIC_APPS_PATH)/media_server && $(MAKE) clean
endif
ifeq ($(INCLUDE_LITE_MINIDLNA), y)
	cd $(PUBLIC_APPS_PATH)/media_server && $(MAKE) lite_clean
endif
#end for DLNA_CLEAN

ifeq ($(INCLUDE_IPSEC), y)
	cd $(PUBLIC_APPS_PATH)/ipsectools && $(MAKE) clean
	cd $(PUBLIC_APPS_PATH)/openssl && $(MAKE) clean
	cd $(PUBLIC_APPS_PATH)/openssl && rm -rf ./lib
	rm -rf $(PUBLIC_APPS_PATH)/myipsec 
	rm -rf $(PUBLIC_APPS_PATH)/myssl
endif

#added by CCy for VPN, 2015-08-10
ifeq ($(INCLUDE_OPENVPN_SERVER), y)
	-cd $(PUBLIC_APPS_PATH)/openvpn-2.3.6 && $(MAKE)  clean && rm -rf ./install
	-cd $(PUBLIC_APPS_PATH)/lzo-2.09 && $(MAKE)  clean && rm -rf ./install
endif

ifeq ($(INCLUDE_PPTPVPN_SERVER), y)
	-$(MAKE) -C $(PUBLIC_APPS_PATH)/pptpd-1.4.0 clean
endif

ifeq ($(INCLUDE_L2TP), y)
	cd $(PUBLIC_APPS_PATH)/xl2tpd-1.1.12 && $(MAKE) clean
endif

voip_clean:
ifneq ($(strip $(INCLUDE_VOIP)),)
	$(MAKE) -C $(PRIVATE_APPS_PATH)/voip clean
else	
	@echo "skipped voip_clean"
endif

pjsip_clean:
ifneq ($(strip $(INCLUDE_VOIP)),)
	rm -f $(PUBLIC_APPS_PATH)/pjsip_1.10/config_cache_file
	$(MAKE) -C $(PUBLIC_APPS_PATH)/pjsip_1.10 clean
else	
	@echo "skipped sip_clean"
endif

cmm_clean:
	$(MAKE) -C $(PRIVATE_APPS_PATH)/user clean

####################################################
#
#Build filesystem, prepare basic fs first
#
####################################################
.PHONY: fs_clean fs_create fs_lib fs_bin fs_modules fs_supplier fs_sysfiles fs_dev fs_strip fs_rootfs
ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5020)
fs_build: fs_clean fs_create fs_lib fs_bin fs_modules fs_supplier fs_dev fs_strip fs_rootfs

fs_clean:
	rm -rf $(FS_TG_PATH)

fakeroot_clean:    
	if [ -e $(FAKEROOT_PATH)/Makefile ];  then make -C $(FAKEROOT_PATH) distclean; fi
	rm -f $(FAKEROOT_PATH)/fakeroot
	rm -f $(FAKEROOT_PATH)/.configured

fs_create: 
	cp -pRfT $(TOP_PATH)/targets/fs.dir $(FS_TG_PATH)
	find  $(FS_TG_PATH) -name ".svn" | xargs rm -rf
ifneq ($(INCLUDE_CWMP_SSL_VERIFY_PEER), y)
	@rm -fr $(FS_TG_PATH)/etc/cert
endif

	#for rcS, yuanshang, 20111026
	rm -fr $(FS_TG_PATH)/etc/init.d/*
	cp $(SDK_BUILD_PATH)/$(MODEL)/$(SPEC)/$(MODEL).rcS $(FS_TG_PATH)/etc/init.d/rcS
	chmod a+x $(FS_TG_PATH)/etc/init.d/rcS

ifneq ($(INCLUDE_VOIP), y)
	@rm -fr $(FS_TG_PATH)/etc/voiceNotify
endif	

else
fs_build:fs_lib fs_bin fs_modules fs_dev fs_rootfs fs_supplier
fs_clean:sdk/fs/clean
endif # mtk_ApSoC_5020

fs_lib:
	cp -P $(SLIB_PATH)/*.so* $(FS_TG_PATH)/lib/
	#cp -P $(PRIVATE_LIB_BIN_PATH)/* $(FS_TG_PATH)/lib/
	cp -P $(PUBLIC_APPS_PATH)/upnp_libs/*.so $(FS_TG_PATH)/lib/
	cp -P $(OS_LIB_PATH)/libos.so $(FS_TG_PATH)/lib

ifeq ($(strip $(SUPPLIER)),qualcomm)
	cp -rf $(TOOLS_TG_PATH)/lib/* $(FS_TG_PATH)/lib
endif

ifeq ($(INCLUDE_SSL), y)
	cp -p $(PUBLIC_APPS_PATH)/openssl-0.9.8zh/libcrypto.so.0.9.8 $(FS_TG_PATH)/lib/
	cd $(FS_TG_PATH)/lib/ && ln -sf libcrypto.so.0.9.8 libcrypto.so
	cp -p $(PUBLIC_APPS_PATH)/openssl-0.9.8zh/libssl.so.0.9.8 $(FS_TG_PATH)/lib/
	cd $(FS_TG_PATH)/lib/ && ln -sf libssl.so.0.9.8 libssl.so
endif
	
ifneq ($(strip $(INCLUDE_USB_VOICEMAIL)),)
	cp $(APP_PJSIP_DIR)/usbvm/lib/libusbvmcommon.so $(FS_TG_PATH)/lib/
endif

	#for the libs of samba and ntfs-3g, zjj, 20111129
	#modified by LY on 2015.01.28, as ntfs-3g is not needed when use tntfs.ko provided by Tuxera
ifeq ($(INCLUDE_USB_STORAGE), y)
ifneq ($(INCLUDE_USB_STORAGE_TUXERA_NTFS), y)
	cp -p $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15/libntfs-3g/.libs/libntfs-3g.so.83.0.0 $(FS_TG_PATH)/lib/
	cd $(FS_TG_PATH)/lib/ && ln -sf libntfs-3g.so.83.0.0 libntfs-3g.so.83
	cd $(FS_TG_PATH)/lib/ && ln -sf libntfs-3g.so.83.0.0 libntfs-3g.so
endif
endif


ifeq ($(INCLUDE_IPV6), y)
	#cp -p $(SLIB_PATH)/../usr/lib/libstdc++.so.6.0.16 $(FS_TG_PATH)/lib/	
	#cp -p $(SLIB_PATH)/../usr/lib/libstdc++.so.6.0.16 $(FS_TG_PATH)/lib/	
	#$(STRIP) $(FS_TG_PATH)/lib/libstdc++.so.6.0.16
	#cd $(FS_TG_PATH)/lib/ && ln -sf libstdc++.so.6.0.16 libstdc++.so.6
endif

ifeq ($(INCLUDE_USB_SAMBA_SERVER), y)
	cp -p $(PUBLIC_APPS_PATH)/samba-3.0.14a/source/bin/libbigballofmud.so $(FS_TG_PATH)/lib/
	cd $(FS_TG_PATH)/lib/ && ln -sf libbigballofmud.so libbigballofmud.so.0
endif
	#end modified by LY
	#end by zjj
 
ifneq ($(strip $(filter y, $(INCLUDE_USB_3G_DONGLE) $(INCLUDE_USB_SYSFS))),)
	cp --no-dereference -p $(PUBLIC_APPS_PATH)/libusb-1.0.8/install/lib/libusb-1.0.so* $(FS_TG_PATH)/lib/
	cp --no-dereference -p $(PUBLIC_APPS_PATH)/libusb-compat-0.1.3/install/lib/libusb-0.1.so* $(FS_TG_PATH)/lib/
	cp --no-dereference -p $(PUBLIC_APPS_PATH)/libusb-compat-0.1.3/install/lib/libusb.so $(FS_TG_PATH)/lib/
endif #INCLUDE_USB_3G_DONGLE
 
ifeq ($(INCLUDE_IPSEC), y)	
	#cp -p $(PUBLIC_APPS_PATH)/ipsectools/src/racoon/.libs/libracoon.so.0.0.0 $(FS_TG_PATH)/lib/
	#cd $(FS_TG_PATH)/lib/ && ln -sf libracoon.so.0.0.0 libracoon.so.0
	#cd $(FS_TG_PATH)/lib/ && ln -sf libracoon.so.0.0.0 libracoon.so
	#cp -p $(PUBLIC_APPS_PATH)/ipsectools/src/libipsec/.libs/libipsec.so.0.0.1 $(FS_TG_PATH)/lib/
	#cd $(FS_TG_PATH)/lib/ && ln -sf libipsec.so.0.0.1 libipsec.so.0
	#cd $(FS_TG_PATH)/lib/ && ln -sf libipsec.so.0.0.1 libipsec.so
endif
 
#added by zengdongbiao for minidlna, in 21May15.
ifeq ($(INCLUDE_MINIDLNA), y)
 ###copy media server related libs
	cp $(MEDIA_SERVER_INSTALL)/lib/libjpeg.so.9.0.0 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libid3tag.so.0.3.0 $(FS_TG_PATH)/lib
#	cp $(MEDIA_SERVER_INSTALL)/lib/libsqlite3.so.0.8.6 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libavformat.so.56.15.102 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libavutil.so.54.15.100 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libavcodec.so.56.13.100 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libavdevice.so.56.3.100 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libexif.so.12.3.3 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libFLAC.so.8.2.0 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libogg.so.0.6.0 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libvorbis.so.0.4.3 $(FS_TG_PATH)/lib
#	cp $(MEDIA_SERVER_INSTALL)/lib/libiconv.so.2.5.1 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libswscale.so.3.1.101 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libtiff.so.5.2.0 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libtiffxx.so.5.2.0 $(FS_TG_PATH)/lib
	cp $(MEDIA_SERVER_INSTALL)/lib/libz.so.1.2.6 $(FS_TG_PATH)/lib
###link media_server related libs
	cd $(FS_TG_PATH)/lib;
	pwd
	cd $(FS_TG_PATH)/lib/ && ln -sf libjpeg.so.9.0.0 libjpeg.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libjpeg.so.9.0.0 libjpeg.so.9
	cd $(FS_TG_PATH)/lib/ && ln -sf libid3tag.so.0.3.0 libid3tag.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libid3tag.so.0.3.0 libid3tag.so.0
#	cd $(FS_TG_PATH)/lib/ && ln -sf libsqlite3.so.0.8.6 libsqlite3.so
#	cd $(FS_TG_PATH)/lib/ && ln -sf libsqlite3.so.0.8.6 libsqlite3.so.0
	cd $(FS_TG_PATH)/lib/ && ln -sf libavformat.so.56.15.102 libavformat.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libavformat.so.56.15.102 libavformat.so.56
	cd $(FS_TG_PATH)/lib/ && ln -sf libavutil.so.54.15.100 libavutil.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libavutil.so.54.15.100 libavutil.so.54
	cd $(FS_TG_PATH)/lib/ && ln -sf libavcodec.so.56.13.100 libavcodec.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libavcodec.so.56.13.100 libavcodec.so.56
	cd $(FS_TG_PATH)/lib/ && ln -sf libavdevice.so.56.3.100 libavdevice.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libavdevice.so.56.3.100 libavdevice.so.56
	cd $(FS_TG_PATH)/lib/ && ln -sf libexif.so.12.3.3 libexif.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libexif.so.12.3.3 libexif.so.12
	cd $(FS_TG_PATH)/lib/ && ln -sf libFLAC.so.8.2.0 libFLAC.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libFLAC.so.8.2.0 libFLAC.so.8
	cd $(FS_TG_PATH)/lib/ && ln -sf libogg.so.0.6.0 libogg.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libogg.so.0.6.0 libogg.so.0
	cd $(FS_TG_PATH)/lib/ && ln -sf libvorbis.so.0.4.3 libvorbis.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libvorbis.so.0.4.3 libvorbis.so.0
#	cd $(FS_TG_PATH)/lib/ && ln -sf libiconv.so.2.5.1 libiconv.so
#	cd $(FS_TG_PATH)/lib/ && ln -sf libiconv.so.2.5.1 libiconv.so.2
	cd $(FS_TG_PATH)/lib/ && ln -sf libswscale.so.3.1.101 libswscale.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libswscale.so.3.1.101 libswscale.so.3
	cd $(FS_TG_PATH)/lib/ && ln -sf libz.so.1.2.6 libz.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libz.so.1.2.6 libz.so.1
	cd $(FS_TG_PATH)/lib/ && ln -sf libtiff.so.5.2.0 libtiff.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libtiff.so.5.2.0 libtiff.so.5
	cd $(FS_TG_PATH)/lib/ && ln -sf libtiffxx.so.5.2.0 libtiffxx.so
	cd $(FS_TG_PATH)/lib/ && ln -sf libtiffxx.so.5.2.0 libtiffxx.so.5
endif
#end added by zengdongbiao

ifeq ($(INCLUDE_MTD_UTILS), y)
	cp $(PUBLIC_APPS_PATH)/zlib-1.2.6/install/lib/libz.so* $(FS_TG_PATH)/lib
	cp -fpR $(PUBLIC_APPS_PATH)/e2fsprogs-1.41.3/lib/libuuid.a $(FS_TG_PATH)/lib
	cp -fpR  $(PUBLIC_APPS_PATH)/lzo-2.09/install/lib/liblzo2.so* $(FS_TG_PATH)/lib/
endif

fs_bin:
	#cp $(PRIVATE_APPS_BIN_PATH)/* $(FS_TG_PATH)/usr/bin
	
	cd $(PUBLIC_APPS_PATH)/busybox-1.23.2 && $(MAKE) install CONFIG_PREFIX=$(FS_TG_PATH)
	cp -p $(PUBLIC_APPS_PATH)/iproute2-2.6.39/tc/tc $(FS_TG_PATH)/usr/bin
	
	cp -p $(PUBLIC_APPS_PATH)/iptables-1.4.17/iptables/xtables-multi $(FS_TG_PATH)/usr/bin
	cd $(FS_TG_PATH)/usr/bin && ln -sf xtables-multi iptables
	
ifeq ($(INCLUDE_IPV6), y)
	cd $(FS_TG_PATH)/usr/bin && ln -sf xtables-multi ip6tables
endif

	cp -p $(PUBLIC_APPS_PATH)/ppp-2.4.5/pppd/pppd $(FS_TG_PATH)/usr/sbin
ifeq ($(INCLUDE_PPPOE_RELAY), y)
	cp -p $(PUBLIC_APPS_PATH)/ppp-2.4.5/pppd/pppoe-relay $(FS_TG_PATH)/usr/sbin
endif
	cp -p $(PUBLIC_APPS_PATH)/ebtables-v2.0.10-4/ebtables $(FS_TG_PATH)/usr/bin
	cp -p $(PUBLIC_APPS_PATH)/IGD/upnpd $(FS_TG_PATH)/usr/bin
	#cp -p $(PUBLIC_APPS_PATH)/linux-atm/arpd/atmarp $(FS_TG_PATH)/usr/sbin
	#cp -p $(PUBLIC_APPS_PATH)/linux-atm/arpd/atmarpd $(FS_TG_PATH)/usr/sbin
	
ifeq ($(INCLUDE_PORTABLE_APP), y)		
	cp -p $(PUBLIC_APPS_PATH)/dropbear/dropbearmulti $(FS_TG_PATH)/usr/bin
	cd $(FS_TG_PATH)/usr/bin && ln -s dropbearmulti dropbear
	cd $(FS_TG_PATH)/usr/bin && ln -s dropbearmulti dropbearkey
	cd $(FS_TG_PATH)/usr/bin && ln -s dropbearmulti scp
endif
	
ifeq ($(INCLUDE_L2TP), y)
	cp -p $(PUBLIC_APPS_PATH)/xl2tpd-1.1.12/xl2tpd $(FS_TG_PATH)/usr/sbin
endif

ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
	cp -p $(PUBLIC_APPS_PATH)/usbip-0.1.7/usbipd $(FS_TG_PATH)/usr/bin
endif

ifeq ($(INCLUDE_USB_FTP_SERVER), y)
	cp -p $(PUBLIC_APPS_PATH)/vsftpd-2.3.2/vsftpd $(FS_TG_PATH)/usr/bin
	cp -p $(PUBLIC_APPS_PATH)/vsftpd-2.3.2/vsftpd.conf $(FS_TG_PATH)/etc
	cp -p $(PUBLIC_APPS_PATH)/vsftpd-2.3.2/passwd $(FS_TG_PATH)/etc/vsftpd_passwd
endif
	
ifeq ($(INCLUDE_RIP)_$(INCLUDE_RIPNG), y_y)
	echo *****
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/ripd/ripd $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/ripngd/ripngd $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/zebra/zebra $(FS_TG_PATH)/usr/sbin
else ifeq ($(INCLUDE_RIP), y)
	echo ******************
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/ripd/ripd $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/zebra/zebra $(FS_TG_PATH)/usr/sbin
else ifeq ($(INCLUDE_RIPNG), y)
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/ripngd/ripngd $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/zebra-0.95a/zebra/zebra $(FS_TG_PATH)/usr/sbin
else
	@echo no rip
endif

ifneq ($(strip $(INCLUDE_VOIP)),)
	cp $(APP_VOIP_DIR)/bin/$(MODEL)/* $(FS_TG_PATH)/usr/bin
	cp $(APP_PJSIP_DIR)/pjsip-apps/bin/pjsua $(FS_TG_PATH)/usr/bin
endif	

ifeq ($(INCLUDE_IPV6), y)
	cp -p $(PUBLIC_APPS_PATH)/radvd-1.5/radvd $(FS_TG_PATH)/usr/sbin
endif

ifeq ($(INCLUDE_IPV6), y)
	cp -p $(PUBLIC_APPS_PATH)/wide-dhcpv6-20080615/dhcp6s $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/wide-dhcpv6-20080615/dhcp6c $(FS_TG_PATH)/usr/sbin
	#cp -p $(PUBLIC_APPS_PATH)/dibbler-1.0.1/dibbler-client $(FS_TG_PATH)/usr/sbin	
	#cp -p $(PUBLIC_APPS_PATH)/dibbler-1.0.1/dibbler-server $(FS_TG_PATH)/usr/sbin
endif

ifeq ($(INCLUDE_IPV6_MAP), y)
	cp -p $(PUBLIC_APPS_PATH)/MAP/utils/ivictl $(FS_TG_PATH)/usr/sbin
endif

ifeq ($(INCLUDE_IPV6), y)
	#cp -p $(PUBLIC_APPS_PATH)/ividns0.2/ividns $(FS_TG_PATH)/usr/sbin
endif
	
ifeq ($(INCLUDE_IPV6), y)
	cp -p $(PUBLIC_APPS_PATH)/iproute2-2.6.39/ip/ip $(FS_TG_PATH)/usr/bin
endif	

ifeq ($(INCLUDE_USB_STORAGE),y)
	#for the bins of hotplug, zjj, 20111129
	#cp $(PRIVATE_APPS_PATH)/usbp/usbp $(FS_TG_PATH)/sbin
	#cp $(PRIVATE_APPS_PATH)/hotplug/hotplug $(FS_TG_PATH)/sbin
	#end by zjj
	#for the bins of samba and ntfs-3g, zjj, 20111129
	#modified by LY on 2015.01.28, as ntfs-3g is not needed when use tntfs.ko provided by Tuxera
ifneq ($(INCLUDE_USB_STORAGE_TUXERA_NTFS), y)
	cp -p $(PUBLIC_APPS_PATH)/ntfs-3g_ntfsprogs-2012.1.15/src/.libs/ntfs-3g $(FS_TG_PATH)/usr/bin/ntfs-3g
endif
	#end modified by LY
ifeq ($(INCLUDE_USB_SAMBA_SERVER), y)
	cp -p $(PUBLIC_APPS_PATH)/samba-3.0.14a/source/bin/smbd $(FS_TG_PATH)/usr/bin
	cp -p $(PUBLIC_APPS_PATH)/samba-3.0.14a/source/bin/smbpasswd $(FS_TG_PATH)/usr/bin
	cp -p $(PUBLIC_APPS_PATH)/samba-3.0.14a/source/bin/smbcontrol $(FS_TG_PATH)/usr/bin
	cp -p $(PUBLIC_APPS_PATH)/samba-3.0.14a/source/bin/nmbd $(FS_TG_PATH)/usr/bin
endif #INCLUDE_USB_SAMBA_SERVER
	#end by zjj

#for mediaServer, modify by zengdongbiao, 21May15.
ifeq ($(INCLUDE_USB_MEDIA_SERVER), y)
ifeq ($(INCLUDE_USHARE), y)
	cp -p $(PUBLIC_APPS_PATH)/ushare-1.1a/src/ushare $(FS_TG_PATH)/usr/bin
	cp -p $(PUBLIC_APPS_PATH)/ushare-1.1a/scripts/ushare.conf $(FS_TG_PATH)/etc/ushare.conf
else
	cp -p $(MEDIA_SERVER_INSTALL)/sbin/minidlnad $(FS_TG_PATH)/usr/sbin
	cp -p $(MEDIA_SERVER_PATH)/minidlna-1.1.4/minidlna.conf $(FS_TG_PATH)/etc
endif #INCLUDE_USHARE
endif #INCLUDE_USB_MEDIA_SERVER
#end modify by zengdongbiao

else
ifeq ($(INCLUDE_USB_3G_DONGLE),y)
	cp $(PRIVATE_APPS_PATH)/hotplug/hotplug $(FS_TG_PATH)/sbin
endif #INCLUDE_USB_3G_DONGLE
endif #INCLUDE_USB_STORAGE
	
ifeq ($(INCLUDE_USB_3G_DONGLE),y)
	#cp $(PRIVATE_APPS_PATH)/handle_card/modswitch_conf/support_3g_list $(FS_TG_PATH)/etc
	cp $(PUBLIC_APPS_PATH)/usb-modeswitch-1.2.3/usb_modeswitch $(FS_TG_PATH)/usr/bin
	cp $(PUBLIC_APPS_PATH)/ppp-2.4.5/chat/chat $(FS_TG_PATH)/usr/bin
	#cp -pR $(PRIVATE_APPS_PATH)/handle_card/source/handle_card $(FS_TG_PATH)/usr/sbin/
	#cp -pR $(PRIVATE_APPS_PATH)/handle_card/modem_scan/modem_scan $(FS_TG_PATH)/usr/sbin/
	#cp -pR $(PRIVATE_APPS_PATH)/handle_card/chat_script/* $(FS_TG_PATH)/etc/ppp/
	#cp -pR $(PRIVATE_APPS_PATH)/handle_card/modswitch_conf/mode_switch.conf.bin $(FS_TG_PATH)/etc/
endif #INCLUDE_USB_3G_DONGLE
	
ifeq ($(INCLUDE_IPSEC),y)
	#cp -p $(PUBLIC_APPS_PATH)/ipsectools/src/racoon/.libs/racoon $(FS_TG_PATH)/usr/bin
	#cp -p $(PUBLIC_APPS_PATH)/ipsectools/src/setkey/.libs/setkey $(FS_TG_PATH)/usr/bin
	#copy ipsecVpn in user cmm Makefile
	#cp -p $(PRIVATE_APPS_PATH)/user/ipsecVpn/ipsecVpn $(FS_TG_PATH)/usr/sbin
endif

###### added by CCy for vpn ########
ifeq ($(INCLUDE_OPENVPN_SERVER), y)
	cp -p $(PUBLIC_APPS_PATH)/lzo-2.09/install/lib/*.so* $(FS_TG_PATH)/lib/
endif

ifeq ($(INCLUDE_OPENVPN_SERVER), y)
	cp -p $(PUBLIC_APPS_PATH)/openssl-0.9.8zh/install/bin/openssl $(FS_TG_PATH)/usr/sbin
	cp -pr $(PUBLIC_APPS_PATH)/openvpn-2.3.6/easy-rsa $(FS_TG_PATH)/etc
	cp -p $(PUBLIC_APPS_PATH)/openvpn-2.3.6/install/sbin/openvpn $(FS_TG_PATH)/usr/sbin
	cd $(FS_TG_PATH) && ln -sf /var/tmp/.rnd ./etc/.rnd
endif

ifeq ($(INCLUDE_PPTPVPN_SERVER), y)
	cp -p $(PUBLIC_APPS_PATH)/pptpd-1.4.0/bcrelay $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/pptpd-1.4.0/pptpctrl $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/pptpd-1.4.0/pptpd $(FS_TG_PATH)/usr/sbin
	cp -p $(PUBLIC_APPS_PATH)/pptpd-1.4.0/config/options.pptpd $(FS_TG_PATH)/etc/ppp
	cd $(FS_TG_PATH) && ln -sf /var/pptpd/chap-secrets ./etc/ppp/chap-secrets
endif
###### add ended #########################

ifeq ($(INCLUDE_MTD_UTILS), y)
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/flash_erase $(FS_TG_PATH)/sbin
	#cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/mkfs.jffs2 $(FS_TG_PATH)/sbin
	#cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/nandtest $(FS_TG_PATH)/sbin
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/ubi-utils/ubiattach $(FS_TG_PATH)/sbin
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/ubi-utils/ubidetach $(FS_TG_PATH)/sbin
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/ubi-utils/ubiformat $(FS_TG_PATH)/sbin
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/ubi-utils/ubimkvol $(FS_TG_PATH)/sbin
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/ubi-utils/ubirmvol $(FS_TG_PATH)/sbin
	cp -pr $(PUBLIC_APPS_PATH)/mtd-utils-1.5.0/ubi-utils/ubinfo $(FS_TG_PATH)/sbin
endif

ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5020)
	#cp $(TOP_PATH)/$(SUPPLIER)/apps/reg/reg $(FS_TG_PATH)/usr/bin

ifeq ($(INCLUDE_LAN_WLAN), y) 
	#cp $(INSTALL_WIRELESS)/sbin/iwpriv $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/sbin/iwconfig $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/sbin/iwlist $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/sbin/wlNetlinkTool $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/lib/libiw.so.29 $(FS_TG_PATH)/lib
ifeq ($(INCLUDE_ATED_CALIBRATE),y)
	mv $(FS_TG_PATH)/usr/bin/ated_calibrate $(FS_TG_PATH)/usr/bin/ated_tp
else
	#cp $(INSTALL_WIRELESS)/sbin/ated_tp $(FS_TG_PATH)/usr/bin
endif
ifeq ($(INCLUDE_HARDWARE_DEBUG),y)
	cp $(INSTALL_WIRELESS)/sbin/ated $(FS_TG_PATH)/usr/bin
endif
	#cp $(INSTALL_WIRELESS)/sbin/mpstat $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/sbin/rt2860apd $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/sbin/rtinicapd $(FS_TG_PATH)/usr/bin
	#cp $(INSTALL_WIRELESS)/sbin/wscd $(FS_TG_PATH)/usr/bin
ifeq ($(INCLUDE_LAN_WLAN_DUALBAND), y)
	#cp $(INSTALL_WIRELESS)/sbin/wscd_5G $(FS_TG_PATH)/usr/bin
endif

	cp $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/SingleSKU.dat $(FS_TG_PATH)/etc/
ifeq ($(INCLUDE_CPU_MT7628), y)
	cp $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/MT7628_EEPROM_20140317.bin $(FS_TG_PATH)/etc/
endif
ifeq ($(INCLUDE_CPU_MT7620A), y)
	cp $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/MT7620_AP_2T2R-4L_V15.BIN $(FS_TG_PATH)/etc/
endif

ifeq ($(INCLUDE_LAN_WLAN_DUALBAND), y)
	cp $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/SingleSKU_5G.dat $(FS_TG_PATH)/etc/
ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7610E), y)
	cp $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/MT7610E-V10-FEM-1ANT.bin $(FS_TG_PATH)/etc/
endif
ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7612E), y)
	cp $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/MT7612E_EEPROM.bin $(FS_TG_PATH)/etc/
endif
endif
	
#Build wscd with wifi modules
	#cp $(TOP_PATH)/$(SUPPLIER)/filesystem/usr/bin/wscd $(FS_TG_PATH)/usr/bin
	chmod +x $(FS_TG_PATH)/usr/bin/*
else	#INCLUDE_LAN_WLAN
	@echo no wifi
endif	#INCLUDE_LAN_WLAN

endif	#SUPPLIER-mtk_ApSoC_5020

fs_modules:
	-cp -pR $(MODULE_TG_PATH)/* $(FS_TG_PATH)/lib/modules
	-cp -pR $(KO_MODULES_PATH)/*.ko $(FS_TG_PATH)/lib/modules
	
ifeq ($(INCLUDE_USB_OVER_IP), y)
ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
	cp -p $(TOP_PATH)/$(SUPPLIER)/modules/public/usbip-0.1.7/usbip.ko $(FS_TG_PATH)/lib/modules
	cp -p $(TOP_PATH)/$(SUPPLIER)/modules/public/usbip-0.1.7/usbip_common_mod.ko $(FS_TG_PATH)/lib/modules
else   # INCLUDE_USB_OVER_IP_TPLINK
ifeq ($(INCLUDE_VOIP), y)
ifeq ($(strip $(CONFIG_NR_CPUS)), 2)
	cp -p $(TOP_PATH)/$(SUPPLIER)/modules/private/kcode/NetUSB_$(MODEL).ko $(FS_TG_PATH)/lib/modules/NetUSB.ko
	-cp -p $(TOP_PATH)/$(SUPPLIER)/modules/private/kcode/GPL_NetUSB_$(MODEL).ko $(FS_TG_PATH)/lib/modules/GPL_NetUSB.ko
else   # CONFIG_NR_CPUS
	cp -p $(TOP_PATH)/$(SUPPLIER)/modules/private/kcode/NetUSB_$(MODEL).ko $(FS_TG_PATH)/lib/modules/NetUSB.ko
	-cp -p $(TOP_PATH)/$(SUPPLIER)/modules/private/kcode/GPL_NetUSB_$(MODEL).ko $(FS_TG_PATH)/lib/modules/GPL_NetUSB.ko
endif  # CONFIG_NR_CPUS	
else   # INCLUDE_VOIP
	cp -p $(TOP_PATH)/$(SUPPLIER)/modules/private/kcode/NetUSB_$(MODEL).ko $(FS_TG_PATH)/lib/modules/NetUSB.ko
	-cp -p $(TOP_PATH)/$(SUPPLIER)/modules/private/kcode/GPL_NetUSB_$(MODEL).ko $(FS_TG_PATH)/lib/modules/GPL_NetUSB.ko
endif  # INCLUDE_VOIP	
endif  # INCLUDE_USB_OVER_IP_TPLINK
endif  # INCLUDE_USB_OVER_IP

ifeq ($(INCLUDE_IPV6_MAP), y)
	cp -p $(PUBLIC_APPS_PATH)/MAP/modules/ivi.ko $(FS_TG_PATH)/lib/modules/ivi.ko
endif

ifeq ($(INCLUDE_L2TP), y)
ifeq ($(KERNELVERSION), 2.6.22.15)
	cp -p $(TOP_PATH)/$(SUPPLIER)/modules/public/pppol2tp-kmod-0.17/pppol2tp.ko $(FS_TG_PATH)/lib/modules
else
	-cp -p $(KERNELPATH)/net/l2tp/l2tp_core.ko $(FS_TG_PATH)/lib/modules/pppol2tp.ko
	-cp -p $(KERNELPATH)/net/l2tp/l2tp_ppp.ko $(FS_TG_PATH)/lib/modules/l2tp_ppp.ko
	-rm $(FS_TG_PATH)/lib/modules/kmdir/kernel/net/l2tp/*.ko
endif
endif

ifeq ($(INCLUDE_PPTP), y)
	-cp -f $(KERNELPATH)/drivers/net/ppp/pptp.ko $(FS_TG_PATH)/lib/modules/
	-rm $(FS_TG_PATH)/lib/modules/kmdir/kernel/drivers/net/ppp/pptp.ko
endif

ifeq ($(strip $(SUPPLIER)),mtk_ApSoC_5030)
	#just for now, this should be done in modules_build
	cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/domain_login/*.ko $(FS_TG_PATH)/lib/modules/kmdir
	cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/gpioctrl/*.ko $(FS_TG_PATH)/lib/modules/kmdir
	cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/ipt_stat/*.ko $(FS_TG_PATH)/lib/modules/kmdir
	cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/cmgmt/*.ko $(FS_TG_PATH)/lib/modules/kmdir
	cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/ipv6_pass_through/*.ko $(FS_TG_PATH)/lib/modules/kmdir
	#cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/shortcut-fe/*.ko $(FS_TG_PATH)/lib/modules/kmdir
	#cp -p $(PRODUCT_BUILD_PATH)/$(SUPPLIER)/kernel/lib/modules/3.10.14/modules/wportal/*.ko $(FS_TG_PATH)/lib/modules/kmdir
endif
	-mv $(FS_TG_PATH)/lib/modules/kmdir/kernel/net/netfilter/nf_nat_*.ko $(FS_TG_PATH)/lib/modules/kmdir/kernel/net/ipv4/netfilter/

fs_sysfiles: 
	cp -pR $(WEBFILE_PATH)/common/* $(FS_TG_PATH)/web/
ifeq ($(INCLUDE_VOIP), y)
	find  $(FS_TG_PATH)/web/ -name ".svn" | xargs rm -rf
	cp -pR $(WEBFILE_PATH)/common_voice/* $(FS_TG_PATH)/web/
endif
ifeq ($(INCLUDE_WLAN_LQ), y)
	cp -pR $(WEBFILE_PATH)/common_spec/Lantiq/* $(FS_TG_PATH)/web/
endif
ifeq ($(WEB_INCLUDE_MOBILE_UI), y)
	find  $(FS_TG_PATH)/web/ -name ".svn" | xargs rm -rf
	cp -pR $(WEBFILE_PATH)/common_mobile/* $(FS_TG_PATH)/web/
endif
ifeq ($(INCLUDE_GERMANY_SPEC), y)
	find  $(FS_TG_PATH)/web/ -name ".svn" | xargs rm -rf
	if test -d $(WEBFILE_PATH)/common_spec/GERMANY/js/en; then \
	cp -f $(WEBFILE_PATH)/common_spec/GERMANY/js/en/* $(FS_TG_PATH)/web/js; \
	fi
	cp -pR $(WEBFILE_PATH)/common_spec/GERMANY/* $(FS_TG_PATH)/web/
	
	if test -d $(FS_TG_PATH)/web/js/en; then \
	rm -fr $(FS_TG_PATH)/web/js/en; \
	fi
endif

ifeq ($(INCLUDE_MULTI_LANGUAGE), y)
	-mkdir $(FS_TG_PATH)/web/locale/
	cp -pR $(WEBFILE_PATH)/common_language/language.js $(FS_TG_PATH)/web/locale/
ifeq ($(INCLUDE_LANGUAGE_EN_US), y)
	cp -pR $(WEBFILE_PATH)/common_language/en_US/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_DE_DE), y)
	cp -pR $(WEBFILE_PATH)/common_language/de_DE/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_IT_IT), y)
	cp -pR $(WEBFILE_PATH)/common_language/it_IT/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_RU_RU), y)
	cp -pR $(WEBFILE_PATH)/common_language/ru_RU/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_TR_TR), y)
	cp -pR $(WEBFILE_PATH)/common_language/tr_TR/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_PL_PL), y)
	cp -pR $(WEBFILE_PATH)/common_language/pl_PL/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_FR_FR), y)
	cp -pR $(WEBFILE_PATH)/common_language/fr_FR/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_ES_ES), y)
	cp -pR $(WEBFILE_PATH)/common_language/es_ES/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_ES_LA), y)
	cp -pR $(WEBFILE_PATH)/common_language/es_LA/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_PT_BR), y)
	cp -pR $(WEBFILE_PATH)/common_language/pt_BR/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_EL_GR), y)
	cp -pR $(WEBFILE_PATH)/common_language/el_GR/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_TH_TH), y)
	cp -pR $(WEBFILE_PATH)/common_language/th_TH/ $(FS_TG_PATH)/web/locale/
endif
ifeq ($(INCLUDE_LANGUAGE_PT_PT), y)
	cp -pR $(WEBFILE_PATH)/common_language/pt_PT/ $(FS_TG_PATH)/web/locale/
endif
endif

ifeq ($(INCLUDE_LOGIN_GDPR_ENCRYPT), y)
	cp -pR $(WEBFILE_PATH)/common_gdpr/* $(FS_TG_PATH)/web/
endif

# Added by chz 2017-11-07
ifneq ($(SPEC),)
	if test -d $(WEBFILE_PATH)/common_spec/$(MODEL)/$(SPEC)/; then \
	cp -pR $(WEBFILE_PATH)/common_spec/$(MODEL)/$(SPEC)/* $(FS_TG_PATH)/web/; \
	fi
endif

	if [ -f $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/file/reduced_data_model.xml ]; \
	then \
		cp -pf $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/file/oid_str.js $(FS_TG_PATH)/web/js; \
	else \
		cp -pf $(CONFFILE_PATH)/file/oid_str.js $(FS_TG_PATH)/web/js; \
	fi

	cd $(FS_TG_PATH)/web/js && echo $(DFLAGS) | sed 's/-D//g' | sed "s/\([^ ]\{1,\}\)/-e s\/\\\\\\\<\1\\\\\\\>\/\1=1\//g" | sed -e 's/=[0-9]\\/\\/g' -e 's/=\([0-9]\{1,\}\)=1/=\1/g' | xargs sed oid_str.js > temp1.js
	cd $(FS_TG_PATH)/web/js && echo $(WEBFLAGS) | sed 's/ /\n/g' | sed 's/^/var /g' | sed 's/$$/\n/g' > temp2.js && cat temp1.js >> temp2.js && mv temp2.js oid_str.js && rm temp1.js | sed -i "/^[ \t]*\/\*/,/.*\*\//d" oid_str.js

	cp -pR $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/default_config.xml $(FS_TG_PATH)/etc/default_config.xml
	cp -pR $(FS_TG_PATH)/etc/default_config.xml $(DATAS_TG_PATH)/default_config.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/default_config.xml

#5030 SDK，使用Nand Flash时，需要将生产配置文件加密拷贝到etc目录下，同default_config.xml处理方式
ifeq ($(INCLUDE_MTD_UTILS), y)
	cp -pR $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/online_config.xml $(FS_TG_PATH)/etc/online_config.xml
	cp -pR $(FS_TG_PATH)/etc/online_config.xml $(DATAS_TG_PATH)/online_config.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/online_config.xml
endif
	
	if [ -f $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/file/reduced_data_model.xml ]; \
	then \
		$(TOOLS_PATH)/mkcfg -m $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/file/reduced_data_model.xml -s $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/reduced_data_model.xml -o $(FS_TG_PATH)/etc/reduced_data_model.xml; \
	else \
		$(TOOLS_PATH)/mkcfg -m $(CONFFILE_PATH)/file/reduced_data_model.xml -s $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/reduced_data_model.xml -o $(FS_TG_PATH)/etc/reduced_data_model.xml; \
	fi
	cp -pR $(FS_TG_PATH)/etc/reduced_data_model.xml $(DATAS_TG_PATH)/reduced_data_model.xml
	$(TOOLS_PATH)/enc -e -i $(FS_TG_PATH)/etc/reduced_data_model.xml
	
	if [ -f $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/flash_config.xml ]; \
	then \
		cp -pR $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/flash_config.xml $(DATAS_TG_PATH)/flash_config.xml; \
	else \
		cp -pR $(SDK_DATAS_PATH)/$(MODEL)/$(SPEC)/default_config.xml $(DATAS_TG_PATH)/flash_config.xml; \
	fi
	
# remove unnecessary files to reduce fs size
	@rm -f $(FS_TG_PATH)/web/img/icons-01.png
ifneq ($(INCLUDE_USB_OVER_IP), y)
	@rm -f $(FS_TG_PATH)/web/main/printSrv.htm
	@rm -f $(FS_TG_PATH)/web/main/printSrv.mobile.htm
endif
ifneq ($(INCLUDE_LED_SCHEDULE), y)
	@rm -f $(FS_TG_PATH)/web/main/ledSchedule.htm
endif
ifneq ($(INCLUDE_IPTV), y)
ifneq ($(INCLUDE_RUSSIA_SPEC), y)
	@rm -f $(FS_TG_PATH)/web/main/iptv.htm
endif
endif
ifneq ($(INCLUDE_VOIP), y)
	@rm -rf $(FS_TG_PATH)/etc/mail
	@rm -f $(FS_TG_PATH)/web/main/voice.mobile.htm
	@rm -f $(FS_TG_PATH)/web/main/voip.mobile.htm
	@rm -f $(FS_TG_PATH)/web/main/voipDECT.mobile.htm
	@rm -f $(FS_TG_PATH)/web/main/voipSummary.mobile.htm
	@rm -f $(FS_TG_PATH)/web/main/voipTelBook.mobile.htm
	@rm -f $(FS_TG_PATH)/web/main/voipTelDev.mobile.htm
	@rm -f $(FS_TG_PATH)/web/main/voipTelNum.mobile.htm
endif 
ifneq ($(INCLUDE_APP_IQOS), y)
	@rm -rf $(FS_TG_PATH)/etc/iqos
	@rm -rf $(FS_TG_PATH)/usr/bin/iqos-db-loader
	@rm -rf $(FS_TG_PATH)/usr/bin/iqos-db-parser
	@rm -f $(FS_TG_PATH)/web/main/qosSettings.htm
	@rm -f $(FS_TG_PATH)/web/main/qosDatabase.htm
	@rm -f $(FS_TG_PATH)/web/main/qosSettings.mobile.htm
endif
ifneq ($(INCLUDE_QOS), y)
	@rm -f $(FS_TG_PATH)/web/main/qos.htm
	@rm -f $(FS_TG_PATH)/web/main/qosList.htm
	@rm -f $(FS_TG_PATH)/web/main/qosQueueEdit.htm
	@rm -f $(FS_TG_PATH)/web/main/qosClsEdit.htm
	@rm -f $(FS_TG_PATH)/web/main/qosAppEdit.htm
endif
ifneq ($(INCLUDE_CLOUD), y)
	@rm -f $(FS_TG_PATH)/web/main/cloudBasic.htm
	@rm -f $(FS_TG_PATH)/web/main/qsCloud.htm
endif
ifneq ($(INCLUDE_YANDEX_DNS), y)
	@rm -f $(FS_TG_PATH)/web/main/yandexDns.htm
endif

.PHONY: tp_fs_dev supplier_fs_dev
fs_dev: tp_fs_dev supplier_fs_dev

tp_fs_dev:
ifeq ($(USE_TP_DEV_BUILD), y)
	cd $(TOOLS_PATH) && $(FAKEROOT_PATH)/fakeroot ./buildDevs $(FS_TG_PATH) 
endif #ifeq ($(USE_TP_DEV_BUILD), y)

fs_strip:
	find  $(FS_TG_PATH) -name ".svn" | xargs rm -rf
	find  $(FS_TG_PATH) -name ".git*" | xargs rm -rf
	find  $(FS_TG_PATH) -name "log.txt" | xargs rm -rf
	find $(FS_TG_PATH)/lib -type f -name "*.so*" -type f | xargs $(STRIP) --strip-unneeded 
	find $(FS_TG_PATH)/lib -type f -name "*.ko" | xargs $(STRIP) --strip-unneeded
	find $(FS_TG_PATH)/bin/ -type f ! -name "*.sh" | xargs $(STRIP)
	find $(FS_TG_PATH)/sbin/ -type f ! -name "*.sh" | xargs $(STRIP)
	find $(FS_TG_PATH)/usr/bin/ -type f ! -name "*.sh" | xargs $(STRIP)
	find $(FS_TG_PATH)/usr/sbin/ -type f ! -name "*.sh" | xargs $(STRIP)
	
.PHONY:compress_web
compress_web:
	$(TOOLS_PATH)/fsCompress/doCompress.sh --webdir $(FS_TG_PATH)/web --compress-web --no-dsl --no-usb --no-hnat --no-5g 
	
ifeq ($(USE_SUPPLIER_ROOTFS_BUILD), y)
fs_rootfs: supplier_fs_rootfs
else #ifeq ($(USE_SUPPLIER_ROOTFS_BUILD), y)
fs_rootfs: fakeroot
	cd $(TOOLS_PATH) && $(FAKEROOT_PATH)/fakeroot ./mksquashfs4 $(FS_TG_PATH) $(IMG_TG_PATH)/rootfs.$(MODEL) -comp lzma -nopad -noappend
endif #ifeq ($(USE_SUPPLIER_ROOTFS_BUILD), y)

$(FAKEROOT_PATH)/.configured:
	(tar -jkxf fakeroot.tar.bz2 2> /dev/null || true)
	cd $(FAKEROOT_PATH); CFLAGS="-O2 -DGNU -I./" CC=gcc LD=ld AR=ar RANLIB=ranlib STRIP=strip ./configure --prefix=/usr --disable-nls
	touch $@
	make -C $(FAKEROOT_PATH)

fakeroot: $(FAKEROOT_PATH)/.configured
	@sed -e 's,^PREFIX=.*,PREFIX=$(FAKEROOT_PATH),g' -e 's,^BINDIR=.*,BINDIR=$(FAKEROOT_PATH),g' -e 's,^PATHS=.*,PATHS=$(FAKEROOT_PATH)/.libs:/lib:/usr/lib,g' $(FAKEROOT_PATH)/scripts/fakeroot > $(FAKEROOT_PATH)/fakeroot
	@chmod u+x $(FAKEROOT_PATH)/fakeroot
	
####################################################
#
#Build image
#
####################################################
.PHONY: mkkernel_build supplier_mkkernel_build supplier_image_build

ifeq ($(USE_SUPPLIER_IMAGE_BUILD), y)
mkkernel_build: supplier_mkkernel_build
else #ifeq ($(USE_SUPPLIER_IMAGE_BUILD), y)
mkkernel_build:
	$(TOOLS_PATH)/mkkernel3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL) -v $(KERNEL_TG_PATH)/vmlinux -i $(IMG_TG_PATH)
endif #ifeq ($(USE_SUPPLIER_IMAGE_BUILD), y)

ifeq ($(USE_SUPPLIER_IMAGE_BUILD), y)
image_build: supplier_image_build
else #ifeq ($(USE_SUPPLIER_IMAGE_BUILD), y)
ifneq ($(INCLUDE_CUSTOMIZED_SP), "")
image_build:
	$(TOOLS_PATH)/mkimage3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL) -v $(KERNEL_TG_PATH)/vmlinux -c $(DATAS_TG_PATH)/flash_config.xml -i $(IMG_TG_PATH) -x $(CSP)
	$(TOOLS_PATH)/mkimage3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model_beta.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL).beta -v $(KERNEL_TG_PATH)/vmlinux -c $(DATAS_TG_PATH)/flash_config.xml -i $(IMG_TG_PATH) -x $(CSP)
	$(TOOLS_PATH)/mkimage3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model_trans.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL).trans -v $(KERNEL_TG_PATH)/vmlinux -c $(DATAS_TG_PATH)/flash_config.xml -i $(IMG_TG_PATH) -x $(CSP)
else
image_build:
	$(TOOLS_PATH)/mkimage3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL) -v $(KERNEL_TG_PATH)/vmlinux -c $(DATAS_TG_PATH)/flash_config.xml -i $(IMG_TG_PATH) 
	$(TOOLS_PATH)/mkimage3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model_beta.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL).beta -v $(KERNEL_TG_PATH)/vmlinux -c $(DATAS_TG_PATH)/flash_config.xml -i $(IMG_TG_PATH) 
	$(TOOLS_PATH)/mkimage3 -s $(FLASH_SIZE) -l $(BOOT_MAX_SIZE) -m $(KERNEL_MAX_SIZE) -n $(MISC_MAX_SIZE) -e $(CPU_ENDIAN_TYPE) -p $(DATAS_TG_PATH)/reduced_data_model_trans.xml -b $(BOOT_TG_PATH)/boot_$(MODEL).bin -k $(KERNEL_TG_PATH)/linux.7z -f $(IMG_TG_PATH)/rootfs.$(MODEL).trans -v $(KERNEL_TG_PATH)/vmlinux -c $(DATAS_TG_PATH)/flash_config.xml -i $(IMG_TG_PATH) 
endif
endif #ifeq ($(USE_SUPPLIER_IMAGE_BUILD), y)
	

####################################################
#
# Clear object               
#
###################################################
.PHONY: dlna_distclean
#for mediaServer, modify by zengdongbiao, 21May15
dlna_distclean: 
ifeq ($(INCLUDE_MINIDLNA), y)
	cd $(PUBLIC_APPS_PATH)/media_server && $(MAKE) distclean
endif
ifeq ($(INCLUDE_LITE_MINIDLNA), y)
	cd $(PUBLIC_APPS_PATH)/media_server && $(MAKE) lite_distclean
endif
#end modify by zengdongbiao
