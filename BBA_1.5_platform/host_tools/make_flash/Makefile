
EXE=make_flash

OBJS=make_flash.o md5.o md5_interface.o

LDFLAGS += -lm
CFLAGS += -std=gnu99

$(EXE):$(OBJS)
	$(CC) $^ -o $(EXE) $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	@rm -rf $(EXE) $(OBJS)
