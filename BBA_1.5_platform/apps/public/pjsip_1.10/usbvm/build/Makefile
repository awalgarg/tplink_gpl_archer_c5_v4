# For common OSes, test's main() is defined in main.c.
# OS specific configuration may want to put it in different file.
# For example, see os-rtems.mak in current directory.
# Written by sirrain zhang, 0816

include ../../build.mak
include $(PJDIR)/build/common.mak

RULES_MAK := $(PJDIR)/build/rules.mak

export USBVM_SIP_LIB:=../lib/libusbvmsip-$(TARGET_NAME)$(LIBEXT)
export USBVM_COMMON_LIB:=../lib/libusbvmcommon$(LIBEXT2)

###############################################################################
# Gather all flags.
#
export _CFLAGS 	:= $(CC_CFLAGS) $(OS_CFLAGS) $(HOST_CFLAGS) $(M_CFLAGS) \
		   $(CFLAGS) $(CC_INC)../include \
		   $(CC_INC)../../pjlib/include \
		   $(CC_INC)../../pjlib-util/include \
		   $(CC_INC)../../pjnath/include \
		   $(CC_INC)../../pjmedia/include \
		   $(CC_INC)../../pjsip/include \
			$(CC_INC)../../cmsip/include \
			$(CC_INC)../../dtmfrcv/include		#added by huanglei

ifneq ($(strip $(INCLUDE_USBVM_MODULE)),)
export _CFLAGS += -DINCLUDE_USBVM_MODULE -I$(TP_MODULES_PATH)/voip
endif

export _CXXFLAGS:= $(_CFLAGS) $(CC_CXXFLAGS) $(OS_CXXFLAGS) $(M_CXXFLAGS) \
		   $(HOST_CXXFLAGS) $(CXXFLAGS)


###############################################################################
# Defines for building USBVM SIP library
#
export USBVM_SIP_SRCDIR = ../src/usbvm-sip
export USBVM_SIP_OBJS += $(OS_OBJS) $(M_OBJS) $(CC_OBJS) $(HOST_OBJS) \
			 usbvm_remotePlay.o usbvm_remoteRecord.o \
#Added by yuchuwei
ifneq ($(strip $(INCLUDE_PSTN_GATEWAY)),)
	USBVM_SIP_CFLAGS := -DINCLUDE_PSTN_GATEWAY
endif
ifneq ($(strip $(INCLUDE_USB_VOICEMAIL)),)
	USBVM_SIP_CFLAGS += -DINCLUDE_USB_VOICEMAIL
endif

ifneq ($(strip $(INCLUDE_PJSIP_MAIL)),)
	USBVM_SIP_CFLAGS += -D__LINUX_OS_FC__
	USBVM_SIP_CFLAGS += -DINCLUDE_MAIL
	USBVM_SIP_CFLAGS += -I$(PJDIR)/../../private/user/include
endif

export USBVM_SIP_CFLAGS += $(_CFLAGS) $(APP_CFLAGS)

###############################################################################
# Defines for building USBVM COMMON library
#

export USBVM_COMMON_SRCDIR = ../src/usbvm-common
export USBVM_COMMON_OBJS = usbvm_g711.o usbvm_speechEncDec.o usbvm_recFileMang.o usbvm_voiceNotify.o

#Added by yuchuwei
ifneq ($(strip $(INCLUDE_PSTN_GATEWAY)),)
	USBVM_COMMON_CFLAGS := -DINCLUDE_PSTN_GATEWAY
endif
ifneq ($(strip $(INCLUDE_USB_VOICEMAIL)),)
	USBVM_COMMON_CFLAGS += -DINCLUDE_USB_VOICEMAIL
endif

ifneq ($(strip $(INCLUDE_PJSIP_MAIL)),)
	USBVM_COMMON_CFLAGS += -DINCLUDE_MAIL
endif

export USBVM_COMMON_CFLAGS += $(_CFLAGS) $(APP_CFLAGS)


###############################################################################
# Main entry
#
#
TARGETS := usbvm-sip usbvm-common

.PHONY: $(TARGETS)

all: $(TARGETS)

doc:
	cd .. && rm -rf docs/html docs/latex && doxygen docs/doxygen.cfg
	@if [ -n "$(WWWDIR)" ]; then \
		echo "Copying to $(WWWDIR)/usbvm/docs/html.." ; \
		cp -a ../docs/html/* $(WWWDIR)/usbvm/docs/html/ ; \
	fi

dep: depend
distclean: realclean

.PHONY: dep depend pjsip usbvm-sip usbvm-common clean realclean distclean

usbvm-sip:
	$(MAKE) -f $(RULES_MAK) APP=USBVM_SIP app=usbvm-sip $(USBVM_SIP_LIB)
	
usbvm-common:
	$(MAKE) -f $(RULES_MAK) APP=USBVM_COMMON app=usbvm-common $(USBVM_COMMON_LIB)
	$(STRIP) --strip-unneeded $(USBVM_COMMON_LIB)
	
clean:
	$(MAKE) -f $(RULES_MAK) APP=USBVM_SIP app=usbvm-sip $@
	$(MAKE) -f $(RULES_MAK) APP=USBVM_COMMON app=usbvm-common $@

depend:
	$(MAKE) -f $(RULES_MAK) APP=USBVM_SIP app=usbvm-sip $@
	$(MAKE) -f $(RULES_MAK) APP=USBVM_COMMON app=usbvm-common $@

realclean:
	$(subst @@,$(subst /,$(HOST_PSEP),.usbvm-sip-$(TARGET_NAME).depend),$(HOST_RMR))
	$(MAKE) -f $(RULES_MAK) APP=USBVM_SIP app=usbvm-sip $@
	$(subst @@,$(subst /,$(HOST_PSEP),.usbvm-common-$(TARGET_NAME).depend),$(HOST_RMR))
	$(MAKE) -f $(RULES_MAK) APP=USBVM_COMMON app=usbvm-common $@




