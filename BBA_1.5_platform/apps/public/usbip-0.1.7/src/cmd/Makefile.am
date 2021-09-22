bin_PROGRAMS = usbip usbipd
usbip_SOURCES = vhci_attach.c usbip_network.c usbip_network.h
usbipd_SOURCES = stub_server.c usbip_network.c usbip_network.h
AM_CPPFLAGS = -I../include -Wall -std=gnu99
#INCLUDES = -I../include
LDADD = ../lib/libusbip.la ../lib/sysfs/libsysfs.la
EXTRA_CFLAGS = @EXTRA_CFLAGS@
AM_CFLAGS = -Wall -W -Wstrict-prototypes $(EXTRA_CFLAGS)
