#added by LY for media_server, in 2014.06.17
FFMPEG_CUSTOM_ENCODERS:= \
	ac3 \
	jpegls \
	mpeg1video \
	mpeg2video \
	mpeg4 \
	pcm_s16be \
	pcm_s16le \
	png \
	bmp \
	tiff \
	gif \
	vorbis \
	zlib \

FFMPEG_CUSTOM_DECODERS:= \
	ac3 \
	ape \
	atrac3 \
	flac \
	gif \
	tiff \
	bmp	\
	h264 \
	jpegls \
	mp2 \
	mp3 \
	mpeg1video \
	mpeg2video \
	mpeg4 \
	mpegvideo \
	pcm_s16be \
	pcm_s16le \
	png \
	vorbis \
	wmav1 \
	wmav2 \
	zlib \
	flv \

FFMPEG_CUSTOM_MUXERS:= \
	ac3 \
	aiff \
	adts \
	ffm \
	h264 \
	mp3 \
	mp4 \
	mpeg1video \
	mpeg2video \
	mpegts \
	ogg \
	rtp \
	gxf \
	ipod \
	dv \
	srt \
	swf \
	image2 \
	rm \
	wav \
	mov \
	avi \
	gif \
	
FFMPEG_CUSTOM_DEMUXERS:= \
	aac \
	ac3 \
	ape \
	avi \
	aiff \
	dv \
	flac \
	flv \
	ffm \
	h264 \
	image2 \
	matroska \
	mov \
	mp3 \
	mpegps \
	mpegts \
	mpegvideo \
	ogg \
	rm \
	rtsp \
	sdp \
	wav \
	gif \

FFMPEG_CUSTOM_PARSERS:= \
	aac \
	ac3 \
	flac \
	h264 \
	mpegaudio \
	mpeg4video \
	mpegvideo \
	png \
	vorbis \
	
FFMPEG_CUSTOM_PROTOCOLS:= \
	file http pipe rtp tcp udp

	
FFMPEG_ENABLE= \
	$(foreach c, $(2), \
		--enable-$(1)="$(c)" \
	)

 
#media_share:libz libffmpeg libexif libjpeg libid3tag libogg libflac libvorbis libsqlite libuuid libiconv libtiff minidlna
media_share:libz libffmpeg libexif libjpeg libid3tag libogg libflac libvorbis libsqlite libuuid libtiff minidlna
lite_media_share: libsqlite lite_minidlna

###libz
.PHONY:libz_configure
libz_configure:
	@echo media_server: configure libz...
	cd $(MEDIA_SERVER_PATH)/zlib-1.2.6 && ./configure --prefix=$(MEDIA_SERVER_INSTALL)
	
.PHONY:libz
libz: libz_configure
	@echo media_server:build libz...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/zlib-1.2.6
	$(MAKE) -C $(MEDIA_SERVER_PATH)/zlib-1.2.6 install

###ffmepg
.PHONY:libffmpeg_configure
libffmpeg_configure:
	@echo media_server: configure libffmpeg ...
	[ -f $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2/config.mak ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export CXX=$(TOOLPREFIX)g++; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	export STRIP=$(TOOLPREFIX)strip; \
	export CFLAGS+=-fPIC; \
	export CFLAGS+=" -I$(MEDIA_SERVER_INSTALL)/include "; \
	export LDFLAGS+="-L$(MEDIA_SERVER_INSTALL)/lib -Wl,-rpath-link,$(MEDIA_SERVER_INSTALL)/lib"; \
	cd $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2; \
	./configure \
		--prefix=$(MEDIA_SERVER_INSTALL) \
		--libdir=$(MEDIA_SERVER_INSTALL)/lib \
		--shlibdir=$(MEDIA_SERVER_INSTALL)/lib \
		--incdir=$(MEDIA_SERVER_INSTALL)/include \
		--cc=$(TOOLPREFIX)gcc \
		--host-cc=gcc \
		--enable-cross-compile --target-os=linux --arch=$(KERNELARCH) \
		--enable-shared --enable-static \
		--disable-debug \
		--enable-gpl --enable-version3 --enable-nonfree \
		--disable-asm \
		--disable-dxva2 --enable-pthreads --enable-small --disable-stripping \
		--disable-outdevs \
		--disable-runtime-cpudetect  \
		--disable-ffmpeg --disable-ffserver --disable-ffplay --disable-ffprobe \
		--disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages \
		--disable-avfilter --disable-postproc \
		--disable-dwt --disable-error-resilience \
		--disable-pixelutils --disable-faan --disable-lzo --disable-lsp \
		--disable-dxva2 --disable-vaapi --disable-vda --disable-vdpau \
		--disable-xlib --disable-lzma --disable-iconv --disable-bzlib \
		--disable-everything \
		$(call FFMPEG_ENABLE,encoder,$(FFMPEG_CUSTOM_ENCODERS)) \
		$(call FFMPEG_ENABLE,decoder,$(FFMPEG_CUSTOM_DECODERS)) \
		$(call FFMPEG_ENABLE,muxer,$(FFMPEG_CUSTOM_MUXERS)) \
		$(call FFMPEG_ENABLE,demuxer,$(FFMPEG_CUSTOM_DEMUXERS)) \
		$(call FFMPEG_ENABLE,parser,$(FFMPEG_CUSTOM_PARSERS)) \
		$(call FFMPEG_ENABLE,protocol,$(FFMPEG_CUSTOM_PROTOCOLS)) \
	)

.PHONY:libffmpeg
libffmpeg:libffmpeg_configure
	@echo media_server: build libffmpeg ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2
	$(MAKE) -C $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2 install

.PHONY:libffmpeg_install
libffmpeg_install:
	@echo media_server: install libffmpeg ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2 install

.PHONY:libffmpeg_clean
libffmpeg_clean:
	@echo media_server: libffmpeg clean
	cd $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2 && if test -e config.mak; then make distclean; fi ;

###exif
.PHONY:libexif_configure
libexif_configure:
	@echo media_server: configure libexif
	[ -f $(MEDIA_SERVER_PATH)/libexif-0.6.21/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/libexif-0.6.21; \
	chmod 777 ./configure; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--disable-docs \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
	
.PHONY:libexif
libexif: libexif_configure 
	@echo media_server: build libexif ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libexif-0.6.21
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libexif-0.6.21 install
	
###jpeg
.PHONY:libjpeg_configure
libjpeg_configure:
	@echo media_server: configure libjpeg
	[ -f $(MEDIA_SERVER_PATH)/jpeg-9/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/jpeg-9; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
		
.PHONY:libjpeg
libjpeg: libjpeg_configure
	@echo media_server: build libjpeg ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/jpeg-9
	$(MAKE) -C $(MEDIA_SERVER_PATH)/jpeg-9 install
	
###id3tag
.PHONY:libid3tag_configure
libid3tag_configure:
	@echo media_server: configure libid3tag
	[ -f $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	export CFLAGS+=-I$(MEDIA_SERVER_INSTALL)/include; \
	export LDFLAGS+=-L$(MEDIA_SERVER_INSTALL)/lib; \
	cd $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
	
.PHONY:libid3tag
libid3tag: libid3tag_configure
	@echo media_server: build libid3tag ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b install
	
.PHONY:libid3tag_distclean
libid3tag_distclean:
	@echo media_server: libid3tag distclean
	cd $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b && if test -e Makefile; then make distclean; fi ;

###libogg
.PHONY:libogg_configure
libogg_configure:
	@echo media_server: configure libogg
	[ -f $(MEDIA_SERVER_PATH)/libogg-1.1.4/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/libogg-1.1.4; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
	
.PHONY:libogg
libogg: libogg_configure
	@echo media_server: build libogg ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libogg-1.1.4
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libogg-1.1.4 install

###libflac
.PHONY:libflac_configure
libflac_configure:
	@echo media_server: configure libflac
	[ -f $(MEDIA_SERVER_PATH)/flac-1.2.1/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/flac-1.2.1; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
				--disable-cpplibs \
	)
	
.PHONY:libflac
libflac: libflac_configure
	@echo media_server: build libflac ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/flac-1.2.1
	$(MAKE) -C $(MEDIA_SERVER_PATH)/flac-1.2.1 install

###libvorbis
.PHONY:libvorbis_configure
libvorbis_configure:
	@echo media_server: configure libvorbis
	[ -f $(MEDIA_SERVER_PATH)/libvorbis-1.2.3/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	export CFLAGS+=-I$(MEDIA_SERVER_INSTALL)/include; \
	export LDFLAGS+="-lm -L$(MEDIA_SERVER_INSTALL)/lib -logg "; \
	cd $(MEDIA_SERVER_PATH)/libvorbis-1.2.3; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--with-ogg-libraries=$(MEDIA_SERVER_INSTALL)/lib \
				--with-ogg-includes=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
	
.PHONY:libvorbis
libvorbis: libvorbis_configure
	@echo media_server: build libvorbis ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libvorbis-1.2.3
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libvorbis-1.2.3 install

###libsqlite
#.PHONY:libsqlite_configure
#libsqlite_configure:
#	@echo media_server: configure libsqlite
#	[ -f $(MEDIA_SERVER_PATH)/sqlite-3.7.12.1/Makefile ] || \
#	( \
#	export CC=$(TOOLPREFIX)gcc; \
#	export LD=$(TOOLPREFIX)ld; \
#	export AR=$(TOOLPREFIX)ar; \
#	export RANLIB=$(TOOLPREFIX)ranlib; \
#	export CFLAGS+=-DSQLITE_ENABLE_MEMORY_MANAGEMENT; \
#	cd $(MEDIA_SERVER_PATH)/sqlite-3.7.12.1; \
#	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
#				--libexecdir=$(MEDIA_SERVER_INSTALL) \
#				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
#				--includedir=$(MEDIA_SERVER_INSTALL)/include \
#				--build=i686-linux --host=$(KERNELARCH)-linux \
#	)
	
#.PHONY:libsqlite
#libsqlite: libsqlite_configure
#	@echo media_server: build libsqlite ...
#	$(MAKE) -C $(MEDIA_SERVER_PATH)/sqlite-3.7.12.1
#	$(MAKE) -C $(MEDIA_SERVER_PATH)/sqlite-3.7.12.1 install

## compile options for sqlite.
SQLITE_COMPILE_OPTIONS = \
	-DSQLITE_ENABLE_MEMORY_MANAGEMENT -DSQLITE_OMIT_COMPILEOPTION_DIAGS \
	-DSQLITE_OMIT_ALTERTABLE -DSQLITE_OMIT_ANALYZE -DSQLITE_OMIT_ATTACH \
	-DSQLITE_OMIT_CAST -DSQLITE_OMIT_EXPLAIN -DSQLITE_OMIT_FOREIGN_KEY \
	-DSQLITE_OMIT_REINDEX -DSQLITE_OMIT_TEMPDB -DSQLITE_OMIT_TRIGGER \
	-DSQLITE_OMIT_VACUUM -DSQLITE_OMIT_AUTOVACUUM -DSQLITE_OMIT_VIEW \
	-DSQLITE_DISABLE_LFS -DSQLITE_DISABLE_DIRSYNC \
	-DSQLITE_DISABLE_FTS3_UNICODE -DSQLITE_DISABLE_FTS4_DEFERRED \
	-DSQLITE_OMIT_AUTHORIZATION -DSQLITE_OMIT_WAL \
	-DSQLITE_OMIT_TRACE \
	
###libsqlite 
.PHONY:libsqlite_configure
libsqlite_configure:
	@echo media_server: configure libsqlite
	[ -f $(MEDIA_SERVER_PATH)/sqlite-src-3081001/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	export CFLAGS+="$(SQLITE_COMPILE_OPTIONS) "; \
	cd $(MEDIA_SERVER_PATH)/sqlite-src-3081001; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
				--disable-largefile \
	)
	
.PHONY:libsqlite
libsqlite: libsqlite_configure
	@echo media_server: build libsqlite ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/sqlite-src-3081001 sqlite3.c

.PHONY:libsqlite_distclean
libsqlite_distclean: 
	@echo media_server: libsqlite_distclean
	cd $(MEDIA_SERVER_PATH)/sqlite-src-3081001 && if test -e Makefile; then make distclean; fi ;


###libuuid
.PHONY:libuuid_configure
libuuid_configure:
	@echo media_server: configure libuuid
	[ -f $(MEDIA_SERVER_PATH)/libuuid-1.0.2/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/libuuid-1.0.2; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
	
.PHONY:libuuid
libuuid: libuuid_configure
	@echo media_server: build libuuid ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libuuid-1.0.2
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libuuid-1.0.2 install

###libiconv
.PHONY:libiconv_configure
libiconv_configure:
	@echo media_server: configure libiconv
	[ -f $(MEDIA_SERVER_PATH)/libiconv-1.14/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/libiconv-1.14; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
	)
	
.PHONY:libiconv
libiconv: libiconv_configure
	@echo media_server: build libiconv ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libiconv-1.14
	$(MAKE) -C $(MEDIA_SERVER_PATH)/libiconv-1.14 install

##--with-jpeg-include-dir=$(MEDIA_SERVER_INSTALL)/include \
#--with-zlib-include-dir=$(MEDIA_SERVER_INSTALL)/include \
.PHONY:libtiff_configure
libtiff_configure:
	@echo media_server: configure libtiff
	[ -f $(MEDIA_SERVER_PATH)/tiff-4.0.3/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export CXX=$(TOOLPREFIX)g++; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export RANLIB=$(TOOLPREFIX)ranlib; \
	cd $(MEDIA_SERVER_PATH)/tiff-4.0.3; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
				--with-zlib-lib-dir=$(MEDIA_SERVER_INSTALL)/lib \
				--with-jpeg-lib-dir=$(MEDIA_SERVER_INSTALL)/lib \
	)

.PHONY:libtiff
libtiff: libtiff_configure
	@echo media_server: build libtiff ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/tiff-4.0.3
	$(MAKE) -C $(MEDIA_SERVER_PATH)/tiff-4.0.3 install

###minidlna
CFLAGS += \
	-I$(MEDIA_SERVER_INSTALL)/include \
	-I$(MEDIA_SERVER_INSTALL)/include/FLAC \
	-I$(MEDIA_SERVER_INSTALL)/include/libavcodec \
	-I$(MEDIA_SERVER_INSTALL)/include/libavformat \
	-I$(MEDIA_SERVER_INSTALL)/include/libavutil \
	-I$(MEDIA_SERVER_INSTALL)/include/libexif \
	-I$(MEDIA_SERVER_INSTALL)/include/uuid \
	-I$(MEDIA_SERVER_INSTALL)/include/vorbis \
	-I$(MEDIA_SERVER_PATH)/../os_libs/include  

.PHONY: minidlna_prepare
minidlna_prepare:
	@echo minidlna prepare
	cp -p $(MEDIA_SERVER_PATH)/../os_libs/include/* $(MEDIA_SERVER_INSTALL)/include/;
	cp -p $(MEDIA_SERVER_PATH)/../os_libs/libos.so $(MEDIA_SERVER_INSTALL)/lib/;
	cp -f $(MEDIA_SERVER_PATH)/sqlite-src-3081001/sqlite3.c $(MEDIA_SERVER_PATH)/minidlna-1.1.4/;
	cp -f $(MEDIA_SERVER_PATH)/sqlite-src-3081001/sqlite3.h $(MEDIA_SERVER_PATH)/minidlna-1.1.4/; 
	cp -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/configure_minidlna $(MEDIA_SERVER_PATH)/minidlna-1.1.4/configure; 
	cp -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile_minidlna.am $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile.am; 
	cp -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile_minidlna.in $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile.in; 
	
#--with-libiconv-prefix="$(MEDIA_SERVER_INSTALL)/lib" \	
.PHONY:minidlna_configure
minidlna_configure:
	@echo media_server: configure minidlna
	[ -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export CXX=$(TOOLPREFIX)g++; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export LDFLAGS+="-ldl -L$(MEDIA_SERVER_INSTALL)/lib -lz -Wl,-rpath-link,$(MEDIA_SERVER_INSTALL)/lib -L$(MEDIA_SERVER_PATH)/../os_lib/"; \
	export CFLAGS+="-DUSE_UPDATE -DUSE_ALBUM_RESPONSE $(SQLITE_COMPILE_OPTIONS) -I$(MEDIA_SERVER_INSTALL)/include "; \
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
				--with-db-path="/var/run/minidlna" \
				--with-log-path="/var/log" \
				--without-libiconv-prefix \
	)

.PHONY:minidlna
minidlna: minidlna_prepare minidlna_configure
	@echo media_server: build minidlna ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/minidlna-1.1.4 $(CFLAGS)
	$(MAKE) -C $(MEDIA_SERVER_PATH)/minidlna-1.1.4 install

.PHONY: minidlna_install
minidlna_install:
	$(MAKE) -C $(MEDIA_SERVER_PATH)/minidlna-1.1.4 install
	
.PHONY: minidlna_clean
minidlna_clean:
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && make clean;

.PHONY: minidlna_distclean
minidlna_distclean:
	@echo media_server: minidlna distclean
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && if test -e Makefile; then make distclean; fi ;

	
.PHONY: lite_minidlna_prepare
lite_minidlna_prepare:
	@echo lite_minidlna prepare
	mkdir -p $(MEDIA_SERVER_INSTALL)/include;
	mkdir -p $(MEDIA_SERVER_INSTALL)/lib;
	cp -p $(MEDIA_SERVER_PATH)/../os_libs/include/* $(MEDIA_SERVER_INSTALL)/include/;
	cp -p $(MEDIA_SERVER_PATH)/../os_libs/libos.so $(MEDIA_SERVER_INSTALL)/lib/;
	cp -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/configure_lite_minidlna $(MEDIA_SERVER_PATH)/minidlna-1.1.4/configure; 
	cp -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile_lite_minidlna.am $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile.am; 
	cp -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile_lite_minidlna.in $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile.in; 

.PHONY:lite_minidlna_configure 
lite_minidlna_configure:
	@echo media_server: configure lite_minidlna
	[ -f $(MEDIA_SERVER_PATH)/minidlna-1.1.4/Makefile ] || \
	( \
	export CC=$(TOOLPREFIX)gcc; \
	export CXX=$(TOOLPREFIX)g++; \
	export LD=$(TOOLPREFIX)ld; \
	export AR=$(TOOLPREFIX)ar; \
	export CFLAGS+="-I$(MEDIA_SERVER_INSTALL)/include -I$(MEDIA_SERVER_PATH)/../os_libs/include "; \
	export LDFLAGS+="-ldl -L$(MEDIA_SERVER_INSTALL)/lib -Wl,-rpath-link,$(MEDIA_SERVER_INSTALL)/lib -L$(MEDIA_SERVER_PATH)/../os_lib/"; \
	export CFLAGS+="-DLITE_MINIDLNA $(SQLITE_COMPILE_OPTIONS) "; \
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4; \
	./configure --prefix=$(MEDIA_SERVER_INSTALL) \
				--libexecdir=$(MEDIA_SERVER_INSTALL) \
				--libdir=$(MEDIA_SERVER_INSTALL)/lib \
				--includedir=$(MEDIA_SERVER_INSTALL)/include \
				--build=i686-linux --host=$(KERNELARCH)-linux \
				--with-db-path="/var/run/minidlna" \
				--with-log-path="/var/log" \
				--without-libiconv-prefix \
	)


.PHONY:lite_minidlna
lite_minidlna: lite_minidlna_prepare lite_minidlna_configure
	@echo media_server: build minidlna ...
	$(MAKE) -C $(MEDIA_SERVER_PATH)/minidlna-1.1.4 
	$(MAKE) -C $(MEDIA_SERVER_PATH)/minidlna-1.1.4 install

.PHONY: lite_minidlna_install
lite_minidlna_install:
	$(MAKE) -C $(MEDIA_SERVER_PATH)/minidlna-1.1.4 install

.PHONY: lite_minidlna_clean
lite_minidlna_clean:
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && make clean;

.PHONY: lite_minidlna_distclean
lite_minidlna_distclean:
	@echo media_server: minidlna clean
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && if test -e Makefile; then make distclean; fi ;

	
.PHONY:distclean
distclean:
	@echo media_server: libz distclean
	cd $(MEDIA_SERVER_PATH)/zlib-1.2.6 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libffmpeg distclean
	cd $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2 && if test -e config.mak; then make distclean; fi ;
	
	@echo media_server: libexif distclean
	cd $(MEDIA_SERVER_PATH)/libexif-0.6.21 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libjpeg distclean
	cd $(MEDIA_SERVER_PATH)/jpeg-9 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libid3tag distclean
	cd $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b && if test -e Makefile; then make distclean; fi ;

	@echo media_server: libogg distclean
	cd $(MEDIA_SERVER_PATH)/libogg-1.1.4 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libflac distclean
	cd $(MEDIA_SERVER_PATH)/flac-1.2.1 && if test -e Makefile; then make distclean; fi ;

	@echo media_server: libvorbis distclean
	cd $(MEDIA_SERVER_PATH)/libvorbis-1.2.3 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libsqlite clean
	cd $(MEDIA_SERVER_PATH)/sqlite-src-3081001 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libuuid clean
	cd $(MEDIA_SERVER_PATH)/libuuid-1.0.2 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libiconv distclean
	cd $(MEDIA_SERVER_PATH)/libiconv-1.14 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: libtiff distclean
	cd $(MEDIA_SERVER_PATH)/tiff-4.0.3 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: minidlna distclean
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: install file clean
	cd $(MEDIA_SERVER_PATH) && $(RM) -rf $(MEDIA_SERVER_INSTALL)/


.PHONY:lite_distclean
lite_distclean:
	@echo media_server: libsqlite distclean
	cd $(MEDIA_SERVER_PATH)/sqlite-src-3081001 && if test -e Makefile; then make distclean; fi ;


	@echo media_server: minidlna distclean
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && if test -e Makefile; then make distclean; fi ;
	
	@echo media_server: install file clean
	cd $(MEDIA_SERVER_PATH) && $(RM) -rf $(MEDIA_SERVER_INSTALL)/
	
.PHONY:clean
clean:
	@echo media_server: libz clean
	cd $(MEDIA_SERVER_PATH)/zlib-1.2.6 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libffmpeg clean
	cd $(MEDIA_SERVER_PATH)/ffmpeg-2.5.2 && if test -e config.mak; then make clean; fi ;
	
	@echo media_server: libexif clean
	cd $(MEDIA_SERVER_PATH)/libexif-0.6.21 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libjpeg clean
	cd $(MEDIA_SERVER_PATH)/jpeg-9 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libid3tag clean
	cd $(MEDIA_SERVER_PATH)/libid3tag-0.15.1b && if test -e Makefile; then make clean; fi ;

	@echo media_server: libogg clean
	cd $(MEDIA_SERVER_PATH)/libogg-1.1.4 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libflac clean
	cd $(MEDIA_SERVER_PATH)/flac-1.2.1 && if test -e Makefile; then make clean; fi ;

	@echo media_server: libvorbis clean
	cd $(MEDIA_SERVER_PATH)/libvorbis-1.2.3 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libsqlite clean
	cd $(MEDIA_SERVER_PATH)/sqlite-src-3081001 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libuuid clean
	cd $(MEDIA_SERVER_PATH)/libuuid-1.0.2 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libiconv clean
	cd $(MEDIA_SERVER_PATH)/libiconv-1.14 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: libtiff clean
	cd $(MEDIA_SERVER_PATH)/tiff-4.0.3 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: minidlna clean
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: install file clean
	cd $(MEDIA_SERVER_PATH) && $(RM) -rf $(MEDIA_SERVER_INSTALL)/


.PHONY:lite_clean
lite_clean:
	@echo media_server: libsqlite clean
	cd $(MEDIA_SERVER_PATH)/sqlite-src-3081001 && if test -e Makefile; then make clean; fi ;

	@echo media_server: minidlna clean
	cd $(MEDIA_SERVER_PATH)/minidlna-1.1.4 && if test -e Makefile; then make clean; fi ;
	
	@echo media_server: install file clean
	cd $(MEDIA_SERVER_PATH) && $(RM) -rf $(MEDIA_SERVER_INSTALL)/
#end added by LY for media_server
