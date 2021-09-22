ifeq ($(INCLUDE_TTNET), y)
	CFLAGS_pptp.o += -DINCLUDE_TTNET
endif

ifneq ($(KERNELRELEASE),)
obj-m += pptp.o
else
default:
	make -C $(KERNELPATH) LANG=C M=`pwd` modules
endif

clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
