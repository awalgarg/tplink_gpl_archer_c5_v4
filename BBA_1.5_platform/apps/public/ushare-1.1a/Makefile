ifeq (,$(wildcard config.mak))
$(error "config.mak is not present, run configure !")
endif
include config.mak

SUBDIRS =  src \

all:
	for subdir in $(SUBDIRS); do \
	  $(MAKE) -C $$subdir $@; \
	done

clean:
	for subdir in $(SUBDIRS); do \
	  $(MAKE) -C $$subdir $@; \
	done

distclean: clean
	for subdir in $(SUBDIRS); do \
	  $(MAKE) -C $$subdir $@; \
	done
	-$(RM) -f config.log
	-$(RM) -f config.mak
	-$(RM) -f config.h


install:
	for subdir in $(SUBDIRS); do \
	  $(MAKE) -C $$subdir $@; \
	done

.PHONY: clean distclean install
