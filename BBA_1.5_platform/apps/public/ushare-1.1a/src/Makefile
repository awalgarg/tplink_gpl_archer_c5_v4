ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

CURRDIR=$(shell pwd)
EXTRALIBS=-lthreadutil -lpthread -lupnp -lixml -ldlna -lavformat -lavcodec -lavutil -lm -los -lrt
OPTFLAGS+=-I$(OS_LIB_PATH)/include -I$(CURRDIR) -I$(CURRDIR)/../ \
		  -I$(CURRDIR)/../../ffmpeg-0.6 -I$(CURRDIR)/../../libdlna-0.2.3/src -I$(CURRDIR)/../../upnp_libs/inc

LDFLAGS+=-L$(CURRDIR)/../../libdlna-0.2.3/src -L$(CURRDIR)/../../upnp_libs/libs -L$(CURRDIR)/../../ffmpeg-0.6/libavcodec/ \
		-L$(CURRDIR)/../../ffmpeg-0.6/libavformat/ -L$(CURRDIR)/../../ffmpeg-0.6/libavutil/ \
		-L$(OS_LIB_PATH)/

CFLAGS += $(DF_FLAGS)
PROG = ushare
CONFIG_TELNET=n

EXTRADIST = ushare.1 \
	cds.h \
	cms.h \
	msr.h \
	http.h \
	presentation.h \
	metadata.h \
	mime.h \
	services.h \
	buffer.h \
	util_iconv.h \
	content.h \
	cfgparser.h \
	trace.h \
	redblack.h \
	osdep.h \
	ushare.h \
	gettext.h \
	minmax.h \
	template_list.h \
	ushare_tplink.h \


SRCS = \
        cds.c \
        cms.c \
        msr.c \
        http.c \
        presentation.c \
        metadata.c \
        mime.c \
        services.c \
        buffer.c \
        util_iconv.c \
        content.c \
        cfgparser.c \
        trace.c \
        redblack.c \
        osdep.c \
        ushare.c \
		ushare_tplink.c


ifeq ($(CONFIG_TELNET), y)
	EXTRADIST += ctrl_telnet.h
    SRCS += ctrl_telnet.c

    CFLAGS += -DINCLUDE_TELNET
endif


OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(PROG) 

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(EXTRALIBS) -o $@
	$(STRIP) $@
	@chmod 777 $@

clean:
	-$(RM) -f *.o $(PROG)
	-$(RM) -f .depend

distclean:

install: $(PROG)
	$(INSTALL) -d $(bindir)
	$(INSTALL) $(PROG) $(bindir)
	$(STRIP) $(INSTALLSTRIP) $(bindir)/$(PROG)

depend:
	$(CC) -I.. -MM $(CFLAGS) $(OPTFLAGS) $(SRCS) 1>.depend

.PHONY: clean distclean install depend

dist-all:
	cp $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist-all

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
