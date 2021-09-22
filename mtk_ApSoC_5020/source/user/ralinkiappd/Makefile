EXEC = ralinkiappd
OBJS = rtmpiapp.o

# If you want to debug daemon, add following line
CFLAGS += -DDBG
CFLAGS += -I./

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/$(EXEC)
clean:
	rm -f core *~ *.o ralinkiappd *.d

