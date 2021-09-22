
###############################################################
#			mconf
###############################################################

define tools/mconf/build
	cd $(1)/config && $(HOSTMAKE)
endef
define tools/mconf/install
	cp -pr  $(1)/config/mconf $(3)/usr/bin
endef
define tools/mconf/clean
	$(HOSTMAKE) -C $(1)/config clean
endef
$(eval $(call app_definition,tools/mconf,public))




