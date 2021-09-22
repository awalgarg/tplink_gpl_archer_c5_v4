LIB=libmoca2
LIB_VERSION=1.0.0

all:

romfs:
	$(ROMFSINST) /lib/$(LIB)-$(LIB_VERSION).so
	$(ROMFSINST) /lib/libmoca2.a
	ln -sf $(LIB)-$(LIB_VERSION).so $(ROMFSDIR)/lib/$(LIB).so
	ln -sf $(LIB)-$(LIB_VERSION).so $(ROMFSDIR)/lib/$(LIB).so.1
	
