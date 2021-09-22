###############################################################
#			buildfs
###############################################################

define tools/buildubifs/build

endef
define tools/buildubifs/install
	cp -pr  $(1)/makeUbiFs.sh $(3)/usr/bin
	cp -pr  $(1)/mkfs.ubifs $(3)/usr/bin
	cp -pr $(1)/ubinize $(3)/usr/bin
	cp -pr $(1)/liblzo2.so* $(3)/usr/bin
endef
define tools/buildubifs/clean

endef
$(eval $(call app_definition,tools/buildubifs,private))




