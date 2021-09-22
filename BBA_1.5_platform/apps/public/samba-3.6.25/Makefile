#**************************************************************************** 
# 
#  Add by zengdongbiao for samba-3.6.25. 
# 
#***************************************************************************

export DF_FLAGS

SAMBA_CONF_ENV:=ac_cv_lib_attr_getxattr=no \
				ac_cv_search_getxattr=no \
				ac_cv_file__proc_sys_kernel_core_pattern=yes \
				libreplace_cv_HAVE_C99_VSNPRINTF=yes \
				libreplace_cv_HAVE_GETADDRINFO=yes \
				libreplace_cv_HAVE_IFACE_IFCONF=yes \
				LINUX_LFS_SUPPORT=yes \
				samba_cv_CC_NEGATIVE_ENUM_VALUES=yes \
				samba_cv_HAVE_GETTIMEOFDAY_TZ=yes \
				samba_cv_HAVE_IFACE_IFCONF=yes \
				samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes \
				samba_cv_HAVE_SECURE_MKSTEMP=yes \
				samba_cv_HAVE_WRFILE_KEYTAB=no \
				samba_cv_USE_SETREUID=yes \
				samba_cv_USE_SETRESUID=yes \
				samba_cv_have_setreuid=yes \
				samba_cv_have_setresuid=yes \
				ac_cv_header_zlib_h=no \
				samba_cv_zlib_1_2_3=no
#--with-codepagedir=/etc/samba \	
#--with-shared-modules=pdb_tdbsam,pdb_wbc_sam,auth_winbind,auth_wbc,auth_domain \
.PHONY: samba_configure
samba_configure:
	@echo samba_configure: configure samba ...
	[ -f source3/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export GCC=$(TOOLPREFIX)gcc; \
	export CXX=$(TOOLPREFIX)g++; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	export STRIP=$(TOOLPREFIX)strip; \
	export NM=$(TOOLPREFIX)nm; \
	export OBJCOPY=$(TOOLPREFIX)objcopy; \
	export OBJDUMP=$(TOOLPREFIX)objdump; \
	export SIZE=$(TOOLPREFIX)size; \
	export CFLAGS+="-Os -pipe -msoft-float -DMAX_DEBUG_LEVEL=-1 -D__location__=\\\"\\\" -ffunction-sections -fdata-sections "; \
	export CXXFLAGS+="-Os -pipe -msoft-float -DMAX_DEBUG_LEVEL=-1 -D__location__=\\\"\\\" -ffunction-sections -fdata-sections "; \
	export LDFLAGS+="-Wl,--gc-sections -lrt"; \
	cd source3; \
	$(SAMBA_CONF_ENV) ./configure \
		--target=$(HOST) --host=$(HOST) \
		--build=i686-linux \
		--prefix=/var/samba \
		--disable-avahi \
		--disable-cups \
		--disable-pie \
		--disable-relro \
		--disable-static \
		--disable-swat \
		--disable-shared-libs \
		--with-included-iniparser \
		--with-included-popt \
		--with-privatedir=/var/samba/private \
		--with-sendfile-support \
		--without-acl-support \
		--without-cluster-support \
		--without-ads \
		--without-krb5 \
		--without-ldap \
		--without-pam \
		--without-winbind \
		--without-libtdb \
		--without-libtalloc \
		--without-libnetapi \
		--without-libsmbclient \
		--without-libsmbsharemodes \
		--without-libtevent \
		--without-libaddns \
	)

.PHONY: samba
samba:samba_configure
	@echo samba: build samba ...
	cd source3 && $(MAKE) DYNEXP= PICFLAG= MODULES=
	$(STRIP) source3/bin/samba_multicall

.PHONY:distclean
distclean:
	-rm -f auth/auth_sam_reply.o
	-rm -f source3/multi.o
	cd source3 && if test -e Makefile; then make distclean; fi ;

.PHONY:clean
clean:
	-rm -f auth/auth_sam_reply.o
	-rm -f source3/multi.o
	cd source3 && if test -e Makefile; then make clean; fi ;
	
	
