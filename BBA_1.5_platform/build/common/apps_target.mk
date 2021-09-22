###############################################################
#			apps_target
###############################################################

#### $1 should be:app/priority, such as cmm/0, oslibs/1
define priority_sort
$(shell echo $(1) | awk 'BEGIN{RS=" ";ORS="\n";}{print $$0}' \
| sort -n -k 2 -t/ | awk -F"/" 'BEGIN{ORS=" ";}{print $$1}')
endef

###$1:app name
###$2:app sub starget
define old_app_target
$(shell basename $(1))_$(2):$(1)/$(2)

endef

### $1:target name
define old_app_targets
$(foreach target,prepare menuconfig build install clean uninstall distclean gpl gplclean,\
$(call old_app_target,$(1),$(target)))
endef

#### $1:target name
#### $2:config name prefix, app:BUILD_APP,sdk app: BUILD_SDK_APP
define apps_target
$(call apps_target_0,$(1))
$(call include_sub_app_mk)

_$(1)_list:=$$(patsubst $(2)%,%,$$(filter $(2)%,$$(.VARIABLES)))

$(call apps_target_1,$(1),$$(_$(1)_list))
endef

#### $1:target name
define tools_target
$(call apps_target_0,$(1))

$(call include_sub_app_mk)

#### NOTE:USE $$ here.
_$(1)_list:=$$(patsubst $(1)/%/flag,%,$$(filter $(1)/%/flag,$$(.VARIABLES)))

$(call apps_target_1,$(1),$$(_$(1)_list))
endef

#### $1:target name
define apps_target_0
_$(1)_src_dir:=$(dir $(lastword $(MAKEFILE_LIST)))
_$(1)_dirname:=$$(shell basename $$(_$(1)_src_dir))
_$(1)_dirdir:=$(shell dirname $(1))
ifeq ($$(_$(1)_dirdir),.)
$(1)/builddir:=$(PRODUCT_BUILD_PATH)/$$(_$(1)_dirname)
$(1)/gpldir:=$(GPL_SRC_PATH)/$$(_$(1)_dirname)
else
$(1)/builddir:=$$($$(_$(1)_dirdir)/builddir)/$$(_$(1)_dirname)
$(1)/gpldir:=$$($$(_$(1)_dirdir)/gpldir)/$$(_$(1)_dirname)
endif
$(1)/srcdir:=$$(_$(1)_src_dir)
endef
#### $1:target name
#### $2:sub target list
define apps_target_1

_$(1)_list:=$(2)

_$(1)_force:=
_$(1)_priority:=$$(foreach app,$$(_$(1)_list),$$(app)/$$($(1)/$$(app)/priority))
_$(1)_queue:=$$(call priority_sort,$$(_$(1)_priority))

_$(1)_force_in_queue:=$$(filter $$(_$(1)_queue),$$(_$(1)_force))
_$(1)_queue_without_force:=$$(filter-out $$(_$(1)_force_in_queue),$$(_$(1)_queue))
_$(1)_list_sort:=$$(_$(1)_force_in_queue) $$(_$(1)_queue_without_force)

$(call top_target,$(1),$$(_$(1)_list_sort),flag,$$(_$(1)_src_dir))
endef





#### $1: top target name
#### $2: sub target list
#### $3: flag
define top_target
.PHONY:$(1) $(1)/build $(1)/clean $(1)/distclean  $(1)/install $(1)/unistall $(1)/gpl $(1)/gplclean $(1)/distclean 

$(1)_%:$(1)/%
#### For apps and tools, only need to compile the flag.
#### If compile every target, due to depends of targets,
#### some apps will be compiled many times.
ifeq ($(3),flag)
$(1):$$(foreach target,$(2),$(1)/$$(target)/flag)
else
$(1):$$(foreach target,$(2),$(1)/$$(target))
endif

$(1)/build:$$(foreach target,$(2),$(1)/$$(target)/build)

$(1)/clean:$$(foreach target,$(2),$(1)/$$(target)/clean)

$(1)/distclean:$$(foreach target,$(2),$(1)/$$(target)/distclean)
	rm -rf $$($(1)/builddir)

$(1)/install:$$(foreach target,$(2),$(1)/$$(target)/install)

$(1)/unistall:$$(foreach target,$(2),$(1)/$$(target)/unistall)

$(1)/gpl:$$(foreach target,$(2),$(1)/$$(target)/gpl)
	mkdir -p $$($(1)/gpldir)
	cp -pr $$($(1)/srcdir)/app.mk $$($(1)/gpldir)


$(1)/gplclean:$$(foreach target,$(2),$(1)/$$(target)/gplclean)
	rm -rf $$($(1)/gpldir)
endef
