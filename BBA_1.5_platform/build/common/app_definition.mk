####$1:app name 
define app/build
ifneq ($$(_$(1)_gpl_private_build),y)
$(call $(1)/build,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
endif
endef

####$1:app name 
define app/prepare
#### for private build, only copy files.
ifneq ($$(_$(1)_gpl_private_build),y)
ifeq ($$(_$(1)_prepare_option),static)
	mkdir -p $$($(1)/builddir)
	-cp -pR  $$($(1)/srcdir)/. $$($(1)/builddir)
else
	@-$(CREATE_SHADOW) $$($(1)/srcdir) $$($(1)/builddir)
endif
#### delete all svn temp files. Some ugly early versions of svn will create svn files all over the source tree.
	-find  $$($(1)/builddir) -name ".svn" -o -name ".git"| xargs rm -rf
$(call $(1)/prepare,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
else
	mkdir -p $$($(1)/builddir)
	-cp -pR  $$($(1)/srcdir)/. $$($(1)/builddir)
endif
endef

####$1:app name 
define app/install
ifneq ($$(_$(1)_gpl_private_build),y)
$(call $(1)/install,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
else
	-cp -pr $$($(1)/builddir)/oslib/. $(OSLIB_PATH)/
	-cp -pr $$($(1)/builddir)/fs/. $(MODEL_FS_PATH)/
	-cp -pr $$($(1)/builddir)/host/. $(HOST_FS_PATH)/
endif
####make a flag that this app has beed installed. for dependendences check.
	mkdir -p $$(dir $$($(1)/flag))
	echo > $$($(1)/flag)
endef



####$1:app name 
####$2:private or public(should be explictly indicated)
####$3:compile or static(default is compile)
define app_definition

.PHONY: $(1) $(1)/prepare $(1)/menuconfig $(1)/build $(1)/clean \
	$(1)/install $(1)/gpl $(1)/gplclean $(1)/distclean


###############################################################
#			public variables
###############################################################
_$(1)_basename:=$(shell basename $(1))
_$(1)_dirname:=$(shell dirname $(1))

### compile priority
$(1)/priority?=1000

$(1)/flag:=$(PRODUCT_BUILD_PATH)/flag/$(1)_INSTALL
$(1)/gplflag:=$(PRODUCT_BUILD_PATH)/flag/$(1)_GPL

#### Source code directory
$(1)/srcdir:=$(dir $(lastword $(MAKEFILE_LIST)))
#### Build directory


$(1)/builddir:=$$($$(_$(1)_dirname)/builddir)/$$(_$(1)_basename)/src_shadow
$(1)/configdir:=$$(CONFIG_BUILD_PATH)

#### GPL directory
#### keep original directory structure
$(1)/gpldir:=$$($$(_$(1)_dirname)/gpldir)/$(1)

ifeq ($$(shell basename $$(_$(1)_dirname)),tools)
$(1)/targetfs:=$(HOST_FS_PATH)
$(1)/targetoslib:=$(HOST_FS_PATH)
else

ifeq ($$(shell basename $$(_$(1)_dirname)),apps)
$(1)/targetfs:=$(MODEL_FS_PATH)
$(1)/targetoslib:=$(OSLIB_PATH)
else
$$(error app name is wrong!)
endif
endif


###############################################################
#			local variables
###############################################################


_$(1)_prepare_option:=$(3)
#### GPL target FS path
_$(1)_gpl_targetfs:=$$($(1)/gpldir)/$$(notdir $$($(1)/targetfs))

#### GPL target host FS path
_$(1)_gpl_targetoslib:=$$($(1)/gpldir)/$$(notdir $$($(1)/targetoslib))

#### GPL private building?
_$(1)_gpl_private_build:=$$(shell [ -f $$($(1)/srcdir)/_private ] && echo y)




###############################################################
#			app
###############################################################
## This target check INSTALL flag and build app.
$(1):_$(1)_deleteflag $$($(1)/flag)
	@echo "make $(1) done."
	
_$(1)_deleteflag:
	rm -rf $$($(1)/flag)
	
$$(_$(1)_basename):$(1)


#### For old targets.All app names should unique.
$(eval $(call old_app_targets,$(1)))

###############################################################
#			flag
###############################################################
$(1)/flag:$$($(1)/flag)

$$($(1)/flag):$$(foreach depend,$$($(1)/depends),$(PRODUCT_BUILD_PATH)/flag/$$(depend)_INSTALL)
$(call app/prepare,$(1))
$(call app/build,$(1))
$(call app/install,$(1))
	@echo "make $(1) flag done."

###############################################################
#			prepare
###############################################################
#### copy files into build dir.
$(1)/prepare:
	@echo "Start $(1) prepare."
####create shadow directory
$(call app/prepare,$(1))
#### $1 source dir
#### $2 build dir
$(call $(1)/prepare,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
	@echo "$(1) prepare done."
	
###############################################################
#			menuconfig
###############################################################
$(1)/menuconfig:$(1)/prepare
	@echo "Start $(1) menuconfig."
$(call $(1)/menuconfig,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
	@echo "$(1) menuconfig done."
###############################################################
#			build
###############################################################
####$1: src dir
$(1)/build:$(1)/prepare
	@echo "Start building $(1)."
####create shadow directory
$(call app/build,$(1))
	@echo "Building $(1) done."
	
###############################################################
#			install
###############################################################	
####$1: src dir, $2: fs dir, $3:os lib dir
$(1)/install:
	@echo "Start installing $(1)."
$(call app/install,$(1))
	@echo "Installing $(1) done."

###############################################################
#			uninstall
###############################################################
####$1: src dir, $2: fs dir, $3:os lib dir
$(1)/uninstall:
	@echo "Start uninstalling $(1)."
$(call $(1)/uninstall,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
####make a flag that this app has beed installed. for dependendences check.
	rm -rf $$($(1)/flag)
	@echo "Uninstalling $(1) done."


###############################################################
#			clean
###############################################################
## This target make clean apps's build path.
####$1: src dir
$(1)/clean:$(1)/uninstall
	@echo "Start cleaning $(1)"
$(call $(1)/clean,$$($(1)/builddir),$$($(1)/targetfs),$$($(1)/targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
	@echo "Cleaning $(1) done."

###############################################################
#			distclean
###############################################################
## This target delete app's build path.
####$1: src dir
$(1)/distclean:$(1)/uninstall
	@echo "Start deleting $(1). The whole build path will be deleted."
	rm -rf $$($(1)/builddir)
	@echo "Deleting $(1) done."



###############################################################
#			GPL
###############################################################
$(1)/gpl:$$($(1)/gplflag)


$$($(1)/gplflag):$$(foreach depend,$$($(1)/depends),$(PRODUCT_BUILD_PATH)/flag/$$(depend)_GPL)
	@echo "Start building $(1) GPL."
ifeq ($(strip $(2)),private)
####only copy files to gpl fs.
$(call init_fs,$$(_$(1)_gpl_targetfs))
$(call init_host_fs,$$(_$(1)_gpl_targetoslib))
$(call $(1)/install,$$($(1)/builddir),$$(_$(1)_gpl_targetfs),$$(_$(1)_gpl_targetoslib),$$($(1)/srcdir),$$($(1)/gpldir),$$($(1)/configdir))
#### make a flag.
	@echo "Making $(1) private flag."
	touch $$($(1)/gpldir)/_private
	cp -pr $$($(1)/srcdir)/app.mk $$($(1)/gpldir)
else
####copy source code
	mkdir -p $$($(1)/gpldir)
	cp -fpR $$($(1)/srcdir)/. $$($(1)/gpldir)
endif
#### $1: builddir
#### $2: srcdir
#### $3: gpldir
$(call $(1)/gpl,$$($(1)/srcdir),$$($(1)/gpldir))
	mkdir -p $$(dir $$($(1)/gplflag))
	touch $$($(1)/gplflag)
	
	@echo "Building $(1) GPL done."
###############################################################
#			GPL clean
###############################################################	
## This target make clean apps's build path.
####$1: src dir
$(1)/gplclean:
	@echo "Start cleaning $(1) GPL"
####delete install flag
	rm -rf $$($(1)/gpldir)
	rm -rf $$($(1)/gplflag)
	@echo "Cleaning $(1) GPL done."

endef    ####app_definition
