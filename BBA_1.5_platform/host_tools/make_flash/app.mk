###############################################################
#			mkflash
###############################################################

define tools/mkflash/build
	cd $(1) && $(HOSTMAKE)
endef
define tools/mkflash/install
	cp -pr  $(1)/make_flash $(3)/usr/bin/mkflash
endef
define tools/mkflash/clean
	$(HOSTMAKE) -C $(1) clean
endef
$(eval $(call app_definition,tools/mkflash,private))




