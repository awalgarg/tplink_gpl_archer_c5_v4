###############################################################
#			mtd-utils
###############################################################
apps/mtd-utils/depends=apps/lzo apps/libz apps/e2fsprogs
define apps/mtd-utils/build
	cd $(1) && $(MAKE) all LZOCFLAGS=-I$(3)/include ZLIBCFLAGS=-I$(3)/include  LZOLDFLAGS=-L$(3)/lib ZLIBLDFLAGS=-L$(3)/lib
endef
define apps/mtd-utils/clean
	cd $(1) && $(MAKE) clean
endef	
define apps/mtd-utils/install
	cp -pr $(1)/flash_erase $(2)/sbin
	#cp -pr $(1)/mkfs.jffs2 $(2)/sbin
	#cp -pr $(1)/nandtest $(2)/sbin
	cp -pr $(1)/ubi-utils/ubiattach $(2)/sbin
	cp -pr $(1)/ubi-utils/ubidetach $(2)/sbin
	cp -pr $(1)/ubi-utils/ubiformat $(2)/sbin
	cp -pr $(1)/ubi-utils/ubimkvol $(2)/sbin
	cp -pr $(1)/ubi-utils/ubirmvol $(2)/sbin
	cp -pr $(1)/ubi-utils/ubinfo $(2)/sbin
endef	
$(eval $(call app_definition,apps/mtd-utils,public))




