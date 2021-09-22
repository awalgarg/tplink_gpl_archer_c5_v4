CC=gcc
LD=ld

all:mkimage3 mkkernel3 


mkimage3: 
	$(CC) -g -o mkimage3 -DFULL_BUILD mkimage.c md5.c md5_interface.c
	
mkkernel3:
	$(CC) -g -o mkkernel3 mkimage.c md5.c md5_interface.c


clean:
	-rm -f mkimage3 mkkernel3 

