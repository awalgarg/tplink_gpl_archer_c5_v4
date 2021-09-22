###############################################################
#			buildfs
###############################################################

define tools/buildsquashfs/build

endef
define tools/buildsquashfs/install
	cp -pr  $(1)/makeSquashFs.sh $(3)/usr/bin
	cp -pr  $(1)/mksquashfs4.2 $(3)/usr/bin
	cp -pr  $(1)/unsquashfs $(3)/usr/bin
endef
define tools/buildsquashfs/clean

endef
$(eval $(call app_definition,tools/buildsquashfs,private))




