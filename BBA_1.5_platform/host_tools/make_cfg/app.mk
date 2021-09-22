###############################################################
#			mkcfg
###############################################################

define tools/mkcfg/build
	
endef
define tools/mkcfg/install
	cp -pr  $(1)/enc $(3)/usr/bin
	cp -pr  $(1)/mkcfg $(3)/usr/bin
endef
define tools/mkcfg/clean
	$(HOSTMAKE) -C $(1) clean
endef
$(eval $(call app_definition,tools/mkcfg,private))




