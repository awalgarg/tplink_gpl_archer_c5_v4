###############################################################
#			libz
###############################################################
###libz
define apps/libz/build
	cd $(1) && test -e Makefile || \
	./configure --prefix=$(3)
	$(MAKE) -C $(1)
	$(MAKE) -C $(1) install
endef
define apps/libz/clean
	cd $(1) && $(MAKE) clean
endef
define apps/libz/install
	cp $(3)/lib/libz.so* $(2)/lib
endef
$(eval $(call app_definition,apps/libz,public))




