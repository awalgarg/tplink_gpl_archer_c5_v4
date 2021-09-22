EXEC = mtkiappd
OBJS = rtmpiapp.o mt_iapp_util.o

# If you want to debug daemon, add following line
CFLAGS += -DDBG
CFLAGS += -I./

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/$(EXEC)
clean:
	rm -f core *~ *.o mtkiappd *.d

