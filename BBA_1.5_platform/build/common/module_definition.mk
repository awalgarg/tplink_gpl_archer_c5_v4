#### $1:module name
#### $2:model fs path
#### $3:model os lib
define sdk_module_install
ifneq ($$(_sdk_modules_$(1)_gpl_private_build),y)
	mkdir -p $(2)/lib/modules/kmdir
	cp -pr $$(sdk/modules/external_modules_dir)/$(1)/*.ko $(2)/lib/modules/kmdir
$(call sdk/modules/$(1)/install,$$(_sdk_modules_$(1)_builddir),$(2),$(3))
else
	-cp -pr $$(_sdk_modules_$(1)_builddir)/$(shell basename $(MODEL_FS_PATH))/. $(2)
	-cp -pr $$(_sdk_modules_$(1)_builddir)/$(shell basename $(OSLIB_PATH))/. $(2)
endif
endef
#### $1:modules name
#### $2:public or private
define sdk_module_definition
####TODO: SHOULD take depends into accounts! by xjb.

.PHONY: sdk/modules/$(1) sdk/modules/$(1)/build sdk/modules/$(1)/clean sdk/modules/$(1)/install \
sdk/modules/$(1)/uninstall sdk/modules/$(1)/gpl sdk/modules/$(1)/gplclean 


###############################################################
#			public variables
###############################################################
#### Just a flag. We can filter .VARIABLES and get all module definitions.
sdk/modules/$(1)/flag:=

###############################################################
#			local variables
###############################################################

_sdk_modules_$(1)_srcdir:=$(dir $(lastword $(MAKEFILE_LIST)))

_sdk_modules_$(1)_builddir:=$$(sdk/modules/builddir)/$(1)
_sdk_modules_$(1)_gpldir:=$$(sdk/modules/gpldir)/$(1)


_sdk_modules_$(1)_gpl_targetfs:=$$(_sdk_modules_$(1)_gpldir)/$(shell basename $(MODEL_FS_PATH))
_sdk_modules_$(1)_gpl_targethost:=$$(_sdk_modules_$(1)_gpldir)/$(shell basename $(OSLIB_PATH))
_sdk_modules_$(1)_gpl_private_build:=$$(shell [ -f $$(_sdk_modules_$(1)_srcdir)/_private ] && echo y)
_sdk_modules_$(1)_gpl_targetfs_installdir:=$$(_sdk_modules_$(1)_gpl_targetfs)/lib/modules/kmdir
###############################################################
#			modules
###############################################################
## This target check INSTALL flag and build app.
sdk/modules/$(1):sdk/modules/$(1)/build sdk/modules/$(1)/install
	@echo "make $(1) done."

$(1)_%:sdk/modules/$(1)/%

$(1):sdk/modules/$(1)

###############################################################
#			prepare
###############################################################
## This target check INSTALL flag and build app.
sdk/modules/$(1)/prepare:
ifneq ($$(_sdk_modules_$(1)_gpl_private_build),y)
	@-$(CREATE_SHADOW) $$(_sdk_modules_$(1)_srcdir) $$(_sdk_modules_$(1)_builddir)
else
	mkdir -p $$(_sdk_modules_$(1)_builddir)
	cp -pr $$(_sdk_modules_$(1)_srcdir)/. $$(_sdk_modules_$(1)_builddir)
endif
###############################################################
#			build
###############################################################
####$1: src dir
sdk/modules/$(1)/build:
	@echo "Start building $(1)."
ifneq ($$(_sdk_modules_$(1)_gpl_private_build),y)
	cd $(KERNELPATH) && $(MAKE) M=$$(_sdk_modules_$(1)_builddir) CROSS_COMPILE=$(TOOLPREFIX)
	cd $(KERNELPATH) && $(MAKE) DEPMOD=true modules_install M=$$(_sdk_modules_$(1)_builddir) \
	INSTALL_MOD_PATH=$$(sdk/modules/outputdir)
	@echo "Building $(1) done."
else
	@echo "Nothing to be built."
endif
###############################################################
#			clean
###############################################################
####$1: src dir

sdk/modules/$(1)/clean:
	@echo "Start cleaning $(1)."
ifneq ($$(_sdk_modules_$(1)_gpl_private_build),y)
	cd $(KERNELPATH) && $(MAKE) clean M=$$(_sdk_modules_$(1)_builddir) CROSS_COMPILE=$(TOOLPREFIX)
	@echo "Cleaning $(1) done."
else
	@echo "Nothing to be cleaned."
endif
	
###############################################################
#			install
###############################################################	
####$1: src dir, $2: fs dir
sdk/modules/$(1)/install:
	@echo "Start installing $(1)."
$(call sdk_module_install,$(1),$(MODEL_FS_PATH),$(OSLIB_PATH))
	@echo "Installing $(1) done."


###############################################################
#			GPL
###############################################################
sdk/modules/$(1)/gpl:
	@echo "Start building $(1) GPL."
	mkdir -p $$(_sdk_modules_$(1)_gpldir)
ifeq ($(strip $(2)),private)
####only copy files to gpl fs.
$(call init_fs,$$(_sdk_modules_$(1)_gpl_targetfs))
#### make a flag.
	@echo "Making $(1) private flag."
	touch $$(_sdk_modules_$(1)_gpldir)/_private
	cp $$(_sdk_modules_$(1)_srcdir)/app.mk $$(_sdk_modules_$(1)_gpldir)
	cp $$(_sdk_modules_$(1)_srcdir)/Kconfig $$(_sdk_modules_$(1)_gpldir)
	#### using empty makefile here.
	echo >> $$(_sdk_modules_$(1)_gpldir)/Makefile
$(call sdk_module_install,$(1),$$(_sdk_modules_$(1)_gpl_targetfs),$$(_sdk_modules_$(1)_gpl_targethost))
else
####copy source code
	cp -fpR $$(_sdk_modules_$(1)_srcdir)/. $$(_sdk_modules_$(1)_gpldir)
endif
	@echo "Building $(1) GPL done."
	
###############################################################
#			GPL clean
###############################################################	
## This target make clean apps's build path.
####$1: src dir
sdk/modules/$(1)/gplclean:
	@echo "Start cleaning $(1) GPL"
####delete install flag
	-rm -rf $$(_sdk_modules_$(1)_gpldir)
	@echo "Cleaning $(1) GPL done."


endef    ####app_definition
