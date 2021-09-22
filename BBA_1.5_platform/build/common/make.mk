
HOSTMAKE=export CC=gcc;export LD=ld;export AR=ar;export STRIP=strip;export RANLIB=ranlib;export ARCH=$(shell arch);make

#### BE CAREFUL:if you call include in your makefile, MAKEFILE_LIST will be changed and you should
#### explictly specify $(1)
define include_sub_app_mk
ifeq ($(1),)
CURDIR:=$(dir $(lastword $(MAKEFILE_LIST)))
else
CURDIR:=$(1)
endif
subdir:=$$(shell find $$(CURDIR) -maxdepth 1 -mindepth 1 -type d)
$$(eval $$(foreach i,$$(subdir),sinclude $$(i)/app.mk))
endef
