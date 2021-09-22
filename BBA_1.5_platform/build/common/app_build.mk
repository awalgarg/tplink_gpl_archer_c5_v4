
## Used in app referencing CMM shared library such as cos.
define cmm_app_makefile
ifeq ($$(_CMM_APP_MAKEFILE_INIT),)
#### this flag is for cmm. For those 'INCLUDE_XXX=n', watch out!
DF_FLAGS:=$$(shell awk 'BEGIN{FS="=";ORS=" ";}/^INCLUDE_/{if ($$$$2=="y") {print "-D" $$$$1} else { if ($$$$2!="n"){print "-D" $$$$0}}}' $$(CONFIG_BUILD_PATH)/$(MODEL).config)
CFLAGS += $$(DF_FLAGS) -D__LINUX_OS_FC__
CONFIG_SRC_DEPENDS:=$$(CONFIG_BUILD_PATH)/$(MODEL).config

LDFLAGS += -lcmm

endif
$(top_app_makefile)
endef

## Used in apps which compiled with DF_FLAGS such as oslibs.
define app_makefile
ifeq ($$(_CMM_APP_MAKEFILE_INIT),)
#### this flag is for cmm. For those 'INCLUDE_XXX=n', watch out!
DF_FLAGS:=$$(shell awk 'BEGIN{FS="=";ORS=" ";}/^INCLUDE_/{if ($$$$2=="y") {print "-D" $$$$1} else { if ($$$$2!="n"){print "-D" $$$$0}}}' $$(CONFIG_BUILD_PATH)/$(MODEL).config)
CFLAGS += $$(DF_FLAGS) -D__LINUX_OS_FC__
CONFIG_SRC_DEPENDS:=$$(CONFIG_BUILD_PATH)/$(MODEL).config
endif
$(top_app_makefile)
endef


define top_app_makefile

ifeq ($$(_CMM_APP_MAKEFILE_INIT),)

DEBUG ?= n
OS	?= linux
LIBT ?= dynamic
#LIBT ?= static
FULL_COMPILE ?= y

CFLAGS += -I$$(OSLIB_PATH)/include
## -rpath-link should be specified because
## *.so will be searched in -L, but dependendences of *.so will only be searched in -rpath-link.
## Do not use -rpath, as the paths will be set into executables.
LDFLAGS += -L$$(OSLIB_PATH)/lib -Wl,-rpath-link=$(OSLIB_PATH)/lib -Wl,--as-needed

ifeq ($$(DEBUG),n)
## -fomit-frame-pointer optimizes stack.Be careful, all libraries and executables should keep the same use of -fomit-frame-pointer.
## -fstrength-reduce optimizes loops.
CFLAGS += -Wall -Os -DNDEBUG -fstrength-reduce -fomit-frame-pointer -Werror -Wno-unused -Wno-pointer-sign -Wno-address -Wno-unused-function -Wno-unused-variable -Wno-strict-aliasing -Wno-implicit-function-declaration
else 
CFLAGS += -Wall -g -rdynamic
endif
export LIBT
export OS
export FULL_COMPILE
export DEBUG

endif
$(sub_app_makefile)
endef


##$1:app name $2:app src files.
define sub_app_makefile
APPS+=$(1)
ifeq ($$(_CMM_APP_MAKEFILE_INIT),)
_CMM_APP_MAKEFILE_INIT := y
export CFLAGS
export LDFLAGS


	
%.d: %.c
	@$$(CC) -MM $$(CFLAGS) $$< > $$@.$$$$$$$$; \
	sed 's,\($$(notdir $$*)\)\.o[ :]*,\$$(patsubst %.d,%.o,$$@) : $$(CONFIG_SRC_DEPENDS) ,g' < $$@.$$$$$$$$ > $$@; \
	rm -f $$@.$$$$$$$$

%.dirbuild:
	$$(MAKE) -C $$(patsubst %.dirbuild, %, $$@)
	
%.dirclean:
	$$(MAKE) -C $$(patsubst %.dirclean, %, $$@) clean

endif

##many 'all' targets exist, but only the last one take effects.
all:$$(APPS)

##many 'clean' targets exist, but only the last one take effects.
.PHONY:clean
clean:$$(foreach app,$$(APPS),$$(app)_clean)


$(1)_OBJS_src:=$$(filter %.c %.o,$(2))
$(1)_OBJS_src:=$$(patsubst %.o,%.c,$$($(1)_OBJS_src))
$(1)_OBJS_depend:=$$(patsubst %.c,%.d,$$($(1)_OBJS_src))

$(1)_OBJS:=$$(patsubst %.c,%.o,$(2))

##we regard names withoud suffix as dirs.
$(1)_OBJS_dir:=$$(foreach obj,$$($(1)_OBJS),$$(if $$(suffix $$(obj)),,$$(obj)))

$(1)_OBJS_obj:=$$(filter-out $$($(1)_OBJS_dir),$$($(1)_OBJS))

$(1)_OBJS_dir:=$$(patsubst %,%.dirbuild,$$($(1)_OBJS_dir))
$(1)_OBJS_dirclean:=$$(patsubst %,%.dirclean,$$($(1)_OBJS_dir))
$(1)_OBJS:=$$($(1)_OBJS_dir) $$($(1)_OBJS_obj)

#.PHONY:$$($(1)_OBJS_dir) $$($(1)_OBJS_dirclean)
$$($(1)_OBJS_obj):%.o: %.c %.d $$(CONFIG_SRC_DEPENDS)
	$$(CC) $$(CFLAGS) $$(if $$(filter %.so,$(1)),-fPIC,) -c -o $$@ $$<
	
ifeq ($$(FULL_COMPILE), y)
sinclude $$($(1)_OBJS_depend)
endif


## TODO: for','in string
comma:=,
ifeq ($$($(1)_OBJS_obj),)
.PHONY:$(1)
endif
$(1): $$($(1)_OBJS)
ifneq ($$($(1)_OBJS_obj),)
ifeq ($$(filter %.ld,$(1)),$(1))
	$$(LD) -r -o $$@ $$($(1)_OBJS_obj)
else
ifeq ($$(filter %.a,$(1)),$(1))
	$$(AR) -rcs $$@ $$($(1)_OBJS_obj)
else
	$$(CC) -o $$@ $$($(1)_OBJS_obj) $$(LDFLAGS) $$(if $$(filter %.so,$(1)),-shared -Wl$$(comma)--no-undefined,)
endif ##($$(filter %.a,$(1)),$(1))
endif ##($$(filter %.ld,$(1)),$(1))
endif ##($$($(1)_OBJS_obj),)
.PHONY: $(1)_clean
$(1)_clean:$$($(1)_OBJS_dirclean)
	$$(RM) $$($(1)_OBJS_depend) $$($(1)_OBJS_obj) $(1)
	
endef