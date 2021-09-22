###############################################################
#			e2fsprogs
###############################################################
###libz
define apps/e2fsprogs/build
	cd $(1) && test -e Makefile || \
	./configure --prefix=$(3) --host=$(HOST)
	$(MAKE) -C $(1)
endef
define apps/e2fsprogs/clean
	cd $(1) && $(MAKE) clean
endef
define apps/e2fsprogs/install
	mkdir -p $(3)/include/uuid/
	cp -fpR $(1)/lib/uuid/uuid.h $(3)/include/uuid/
	cp -fpR $(1)/lib/libuuid.a $(3)/lib
endef
$(eval $(call app_definition,apps/e2fsprogs,public))




