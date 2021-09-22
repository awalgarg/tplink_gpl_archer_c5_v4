###############################################################
#			buildfs
###############################################################
tools/buildtools/priority:=0
define tools/buildtools/build

endef
define tools/buildtools/install
	cp -pr  $(1)/createShadow $(3)/usr/bin
endef
define tools/buildtools/clean

endef
$(eval $(call app_definition,tools/buildtools,private,static))




