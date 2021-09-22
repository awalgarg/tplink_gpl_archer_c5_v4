#
#	Makefile -- Build instructions for user level apps
#

.EXPORT_ALL_VARIABLES:
.PHONY: config all romfs clean prune romfs.post

#
# Include architecture specific build rules.
#
ifndef ROOTDIR
ROOTDIR=..
endif

UCLINUX_BUILD_GLIBC_USER=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(BUSYBOX_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

VEND=$(ROOTDIR)/vendors

CC        = $(GLIBC_CROSS_COMPILE)gcc $(CPUFLAGS)
AS        = $(GLIBC_CROSS_COMPILE)as $(CPUFLAGS)
CXX       = $(GLIBC_CROSS_COMPILE)g++ $(CPUFLAGS)
AR        = $(GLIBC_CROSS_COMPILE)ar
LD        = $(GLIBC_CROSS_COMPILE)ld
OBJCOPY   = $(GLIBC_CROSS_COMPILE)objcopy
RANLIB    = $(GLIBC_CROSS_COMPILE)ranlib
STRIPTOOL = $(GLIBC_CROSS_COMPILE)strip

#
# must run the vendor build first
#
dir_v = $(VEND)/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/.
dir_p = $(ROOTDIR)/prop

dir_y = glibc
dir_n =
dir_  =
dir_post_inst_y =

dir_$(CONFIG_USER_IPTABLES_IPTABLES)        += busybox-1.23.0
dir_$(CONFIG_USER_IPTABLES_IP6TABLES)       += busybox-1.23.0
dir_$(CONFIG_USER_DROPBEAR)		    += dropbear-0.52

all: config
	$(MAKE) -j$(HOST_NCPU) $(sort $(dir_y) $(dir_v) $(dir_p)) || exit $$?

#
# add directory dependancies here
#
.PHONY: $(sort $(dir_y) $(dir_p))

$(sort $(dir_y) $(dir_p)):  config
	[ ! -d "$@" ] || ( touch $@/.sgbuilt_user && $(MAKE) -j1 -C $@ ) || exit $$?

%_only:
	touch $(@:_only=)/.sgbuilt_user && $(MAKE) -j1 -C $(@:_only=)

%_clean:
	$(MAKE) -j1 -C $(@:_clean=) clean; rm -f $(@:_clean=)/.sgbuilt_user; true

romfs:
	for i in $(sort $(dir_y)) $(dir_p) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done

romfs.post:
	@echo "#########################################################################################"
	@echo "dir_post_inst_y = "$(dir_post_inst_y)
	@echo "#########################################################################################"
	for i in $(dir_post_inst_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs.post || exit $$? ; \
	done

clean:
	-for i in $(dir_v) $(sort $(dir_y) $(dir_n) $(dir_)) $(dir_p) ; do \
		if [ -f $$i/.sgbuilt_user ]; then \
			$(MAKE) -C $$i clean ; \
			rm -f $$i/.sgbuilt_user; \
		fi; \
	done

prune:
	-for i in $(sort $(dir_n) $(dir_)) ; do \
		found=0; \
		for j in $(sort $(dir_y)) ; do \
			if [ $$i == $$j ]; then \
				found=1; \
			fi; \
		done; \
		if [ $$found == 0 ]; then \
			[ "$$i" = "openswan" ] && make prune_openswan; \
			rm -fr $$i; \
		fi; \
	done

