# For common OSes, test's main() is defined in main.c.
# OS specific configuration may want to put it in different file.
# For example, see os-rtems.mak in current directory.
# Huanglei, 2011.10.28

include ../../build.mak
include $(PJDIR)/build/common.mak

RULES_MAK := $(PJDIR)/build/rules.mak

GCC-NOC = $(subst -c,,$(CC))
export DTMFRCV_SIP_LIB:=../lib/libdtmfrcvsip-$(TARGET_NAME)$(LIBEXT)
#DTMFRCV_COMMON_LIB:=../lib/libdtmfrcvcommon-$(TARGET_NAME).so

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
			$(CC_INC)../../usbvm/include
			
export _CXXFLAGS:= $(_CFLAGS) $(CC_CXXFLAGS) $(OS_CXXFLAGS) $(M_CXXFLAGS) \
		   $(HOST_CXXFLAGS) $(CXXFLAGS)


###############################################################################
# Defines for building dtmf receiver SIP library
#
export DTMFRCV_SIP_SRCDIR = ../src/dtmfrcv-sip
export DTMFRCV_SIP_OBJS += $(OS_OBJS) $(M_OBJS) $(CC_OBJS) $(HOST_OBJS) \
                                dtmfrcv_dtmfReceiver.o dtmfrcv_detect.o
ifneq ($(strip $(INCLUDE_PSTN_GATEWAY)),)
	DTMFRCV_SIP_CFLAGS := -DINCLUDE_PSTN_GATEWAY
endif			
ifneq ($(strip $(INCLUDE_USB_VOICEMAIL)),)
	DTMFRCV_SIP_CFALGS += -DINCLUDE_USB_VOICEMAIL
endif
                        

export DTMFRCV_SIP_CFLAGS += $(_CFLAGS) $(APP_CFLAGS)

###############################################################################
# Defines for building DTMF_RCV COMMON library
#



###############################################################################
# Main entry
#
#
TARGETS := dtmfrcv-sip \
#		dtmfrcv-common

.PHONY: $(TARGETS)

all: $(TARGETS)

#doc:
#	cd .. && rm -rf docs/html docs/latex && doxygen docs/doxygen.cfg
#	@if [ -n "$(WWWDIR)" ]; then \
#		echo "Copying to $(WWWDIR)/usbvm/docs/html.." ; \
#		cp -a ../docs/html/* $(WWWDIR)/usbvm/docs/html/ ; \
#	fi

dep: depend
distclean: realclean

.PHONY: dep depend pjsip dtmfrcv-sip clean realclean distclean\
	#dtmfrcv-common 

dtmfrcv-sip:
	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_SIP app=dtmfrcv-sip $(DTMFRCV_SIP_LIB)

#	$(DTMFRCV_COMMON_OBJDIR)/%.o: $(DTMFRCV_COMMON_SRCDIR)/%.c
#	$(CC) $(DTMFRCV_COMMON_CFLAGS) -shared -fPIC -c -o $@ $<
	
#dtmfrcv-common:$(DTMFRCV_COMMON_OBJS)
#	@echo $(GCC-NOC)
#	$(GCC-NOC) -shared -fPIC -o $(DTMFRCV_COMMON_LIB) $(DTMFRCV_COMMON_OBJS) 
	
clean:
	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_SIP app=dtmfrcv-sip $@
#	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_COMMON app=dtmfrcv-common $@
	rm -fr $(DTMFRCV_COMMON_OBJS) $(DTMFRCV_COMMON_LIB)

depend:
	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_SIP app=dtmfrcv-sip $@
#	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_COMMON app=dtmfrcv-common $@

realclean:
	$(subst @@,$(subst /,$(HOST_PSEP),.dtmfrcv-sip-$(TARGET_NAME).depend),$(HOST_RMR))
	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_SIP app=dtmfrcv-sip $@
#	$(subst @@,$(subst /,$(HOST_PSEP),.dtmfrcv-common-$(TARGET_NAME).depend),$(HOST_RMR))
#	$(MAKE) -f $(RULES_MAK) APP=DTMFRCV_COMMON app=dtmfrcv-common $@




