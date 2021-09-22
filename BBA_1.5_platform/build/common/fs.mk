
####init fs directory structure. $1: fs dir
define init_fs
	@echo "Initialize file system enviroment."
	@mkdir -p $(1)/lib/modules
	@mkdir -p $(1)/etc/init.d
	@mkdir -p $(1)/usr/sbin
	@mkdir -p $(1)/usr/bin
	@mkdir -p $(1)/sbin
	@mkdir -p $(1)/bin
	@mkdir -p $(1)/web
endef

####init fs directory structure. $1: fs dir
define init_host_fs
	@echo "Initialize host file system enviroment."
	@mkdir -p $(1)/lib
	@mkdir -p $(1)/etc/
	@mkdir -p $(1)/usr/sbin
	@mkdir -p $(1)/usr/bin
	@mkdir -p $(1)/sbin
	@mkdir -p $(1)/bin
	@mkdir -p $(1)/include
	@mkdir -p $(1)/install
endef



define strip_libs
	@echo "Striping all libraries."
#### strip all dynamic libs. Be careful for links.
	@for lib in $$(find $(1)/lib -name "*.so*" -type f);do \
		$(STRIP) --strip-unneeded $$lib || echo "strip $$lib error, continue."; \
	done
	
	@for lib in $$(find $(1)/lib -name "*.ko" -type f);do \
		$(STRIP) --strip-unneeded $$lib || echo "strip $$lib error, continue."; \
	done
	@echo "Strip all libraries done."
endef	

define strip_bins
	@echo "Striping all binaries."
#### strip all executives.
	@-for exec in $$(find $(1) -type f);do \
	file $$exec | grep 'ELF.*executable' > /dev/null && { $(STRIP) $$exec || echo "strip $$exec error, continue." }; \
	done
	@echo "Striping all binaries done."
endef