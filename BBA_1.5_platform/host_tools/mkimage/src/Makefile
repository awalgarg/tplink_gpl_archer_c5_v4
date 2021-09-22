CC=gcc
LD=ld

all:mkimage2 mkkernel2 


mkimage2: 
	$(CC) -g -o mkimage2 -DFULL_BUILD mkimage.c md5.c md5_interface.c
	
mkkernel2:
	$(CC) -g -o mkkernel2 mkimage.c md5.c md5_interface.c


clean:
	-rm -f mkimage2 mkkernel2 

