# For common OSes, test's main() is defined in main.c.
# OS specific configuration may want to put it in different file.
# For example, see os-rtems.mak in current directory.

include ../../build.mak
include $(PJDIR)/build/common.mak

RULES_MAK := $(PJDIR)/build/rules.mak


export CMSIP_LIB:=../lib/libcmsip-$(TARGET_NAME)$(LIBEXT)


###############################################################################
# Gather all flags.
#
export _CFLAGS 	:= $(CC_CFLAGS) $(OS_CFLAGS) $(HOST_CFLAGS) $(M_CFLAGS) \
		   $(CFLAGS) $(CC_INC)../include 

export _CXXFLAGS:= $(_CFLAGS) $(CC_CXXFLAGS) $(OS_CXXFLAGS) $(M_CXXFLAGS) \
		   $(HOST_CXXFLAGS) $(CXXFLAGS)

###############################################################################
# Defines for building CMSIP library
#
export CMSIP_SRCDIR = ../src
export CMSIP_OBJS += $(OS_OBJS) $(M_OBJS) $(CC_OBJS) $(HOST_OBJS) \
					cmsip_transport.o cmsip_util.o	
#Add by yuchuwei
#for Telefonica
ifneq ($(strip $(INCLUDE_TFC_ES)),)
	CMSIP_CFLAGS := -DINCLUDE_TFC_ES
endif
ifneq ($(strip $(INCLUDE_PSTN_GATEWAY)),)
	CMSIP_CFLAGS += -DINCLUDE_PSTN_GATEWAY
endif
ifneq ($(strip $(INCLUDE_USB_VOICEMAIL)),)
	CMSIP_CFLAGS += -DINCLUDE_USB_VOICEMAIL
endif
export CMSIP_CFLAGS += $(_CFLAGS) -I$(OS_LIB_PATH)/include 


###############################################################################
# Main entry
#
#
TARGETS := cmsip 

.PHONY: $(TARGETS)

all: $(TARGETS)

doc:
	cd .. && rm -rf docs/html docs/latex && doxygen docs/doxygen.cfg
	@if [ -n "$(WWWDIR)" ]; then \
		echo "Copying to $(WWWDIR)/cmsip/docs/html.." ; \
		cp -a ../docs/html/* $(WWWDIR)/cmsip/docs/html/ ; \
	fi

dep: depend
distclean: realclean

.PHONY: dep depend cmsip clean realclean distclean

cmsip:
	$(MAKE) -f $(RULES_MAK) APP=CMSIP app=cmsip $(CMSIP_LIB)

clean:
	$(MAKE) -f $(RULES_MAK) APP=CMSIP app=cmsip $@

depend:
	$(MAKE) -f $(RULES_MAK) APP=CMSIP app=cmsip $@

realclean:
	$(subst @@,$(subst /,$(HOST_PSEP),.cmsip-$(TARGET_NAME).depend),$(HOST_RMR))
	$(MAKE) -f $(RULES_MAK) APP=CMSIP app=cmsip $@



