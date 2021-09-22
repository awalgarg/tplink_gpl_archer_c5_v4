###############################################################
#			modelconfig
###############################################################

.PHONY:config config/prepare config/build config/install config/clean config/gpl config/gplclean config/menuconfig

config: tools/buildtools tools/mconf config/build config/install

config/prepare:
	mkdir -p $(CONFIG_BUILD_PATH)
	cp -pfr $(CONFIG_PATH)/* $(CONFIG_BUILD_PATH)

config/build:config/prepare config/silentconfig

ifeq ($(shell [ -f $(CONFIG_BUILD_PATH)/kernel.config.init ] && echo y),)
$(CONFIG_BUILD_PATH)/kernel.config.init:sdk/kernel/silentconfig
	touch $(CONFIG_BUILD_PATH)/kernel.config.init
endif


config/silentconfig:
	cp $(CONFIG_BUILD_PATH)/$(MODEL).config $(CONFIG_BUILD_PATH)/.config
	cd $(CONFIG_BUILD_PATH)/ && $(HOST_TOOLS_PATH)/mconf $(BUILD_PATH)/sysdeps/linux/Config.in silent
	cp $(CONFIG_BUILD_PATH)/.config $(CONFIG_BUILD_PATH)/$(MODEL).config

config/menuconfig:
	cp $(CONFIG_BUILD_PATH)/$(MODEL).config $(CONFIG_BUILD_PATH)/.config
	cd $(CONFIG_BUILD_PATH)/ && $(HOST_TOOLS_PATH)/mconf $(BUILD_PATH)/sysdeps/linux/Config.in
	cp $(CONFIG_BUILD_PATH)/.config $(CONFIG_BUILD_PATH)/$(MODEL).config
	
config/install:
	
	
config/uninstall:
	
	
config/clean: config/prepare
	rm -rf $(CONFIG_BUILD_PATH)/*
	
config/distclean:config/uninstall config/clean
	rm -rf $(CONFIG_BUILD_PATH)
	
#### $1: builddir
#### $2: srcdir
#### $3: gpldir
config/gpl:
	mkdir -p $(GPL_SRC_PATH)/build/config/$(MODEL)/$(SPEC)
	
	### only copy the desired model configs.
	for i in $$(find $(BUILD_PATH) -maxdepth 1 -mindepth 1);do \
		[ "$$(basename $$i)" = "config" ] || cp -pr $$i $(GPL_SRC_PATH)/build ; \
	done
	cp -pr $(BUILD_PATH)/config/$(MODEL)/$(SPEC)/* $(GPL_SRC_PATH)/build/config/$(MODEL)/$(SPEC)
	

config/gplclean:
	rm -rf $(GPL_SRC_PATH)/build
	


.PHONY:menuconfig
menuconfig:config/menuconfig