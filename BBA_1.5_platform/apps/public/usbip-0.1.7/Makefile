APP_PATH	= src

usbip_server: usbipd

usbipd:
	cd $(APP_PATH) && \
	./configure --host=$(HOST) --enable-shared=no --enable-static=yes --without-PACKAGE && \
	make && cp cmd/usbipd .. && cd ..

clean:
	-rm -f usbipd
	if test -e $(APP_PATH)/Makefile; then make -C $(APP_PATH) clean; fi
