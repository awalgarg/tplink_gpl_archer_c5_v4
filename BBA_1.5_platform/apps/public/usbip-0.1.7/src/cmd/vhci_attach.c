/*
 * Copyright (C) 2005-2006 Takahiro Hirofuchi
 */

#include "usbip.h"
#include "usbip_network.h"
#include <ctype.h>

static const char version[] = "$Id: vhci_attach.c 97 2006-03-31 16:08:40Z taka-hir $";


/* IPv6 Ready */
int tcp_connect(char *hostname, char *service)
{
	struct addrinfo hints, *res, *res0;
	int sockfd;
	int err;


	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	/* get all possible addresses */
	err = getaddrinfo(hostname, service, &hints, &res0);
	if(err) {
		err("%s %s: %s", hostname, service, gai_strerror(err));
		return -1;
	}

	/* try all the addresses */
	for(res = res0; res; res = res->ai_next) {
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

		err = getnameinfo(res->ai_addr, res->ai_addrlen,
				hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
		if(err) {
			err("%s %s: %s", hostname, service, gai_strerror(err));
			continue;
		}

		dbg("trying %s port %s\n", hbuf, sbuf);

		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(sockfd < 0) {
			err("socket");
			continue;
		}
		
		/* should set TCP_NODELAY for usbip */
		usbip_set_nodelay(sockfd);
		/* TODO: write code for heatbeat */
		usbip_set_keepalive(sockfd);
		
		err = connect(sockfd, res->ai_addr, res->ai_addrlen);
		if(err < 0) {
			close(sockfd);
			continue;
		}

		/* connected */
		dbg("connected to %s:%s", hbuf, sbuf);
		freeaddrinfo(res0);
		return sockfd;
	}


	dbg("%s:%s, %s", hostname, service, "no destination to connect to");
	freeaddrinfo(res0);

	return -1;
}



static int query_exported_devices(int sockfd)
{
	int ret;
	struct op_devlist_reply rep;
	uint16_t code = OP_REP_DEVLIST;

	bzero(&rep, sizeof(rep));

	ret = usbip_send_op_common(sockfd, OP_REQ_DEVLIST, 0);
	if(ret < 0) {
		err("send op_common");
		return -1;
	}

	ret = usbip_recv_op_common(sockfd, &code);
	if(ret < 0) {
		err("recv op_common");
		return -1;
	}

	ret = usbip_recv(sockfd, (void *) &rep, sizeof(rep));
	if(ret < 0) {
		err("recv op_devlist");
		return -1;
	}

	PACK_OP_DEVLIST_REPLY(0, &rep);
	dbg("exportable %d devices", rep.ndev);

	for(unsigned int i=0; i < rep.ndev; i++) {
		char product_name[100];
		char class_name[100];
		struct usb_device udev;

		bzero(&udev, sizeof(udev));

		ret = usbip_recv(sockfd, (void *) &udev, sizeof(udev));
		if(ret < 0) {
			err("recv usb_device[%d]", i);
			return -1;
		}
		pack_usb_device(0, &udev);

		usbip_product_name(product_name, sizeof(product_name),
				udev.idVendor, udev.idProduct);
		usbip_class_name(class_name, sizeof(class_name), udev.bDeviceClass,
				udev.bDeviceSubClass, udev.bDeviceProtocol);

		info("%10s: %s", udev.busid, product_name);
		info("%10s: %s", " ", udev.path);
		info("%10s: %s", " ", class_name);

		for(int j=0; j < udev.bNumInterfaces; j++) {
			struct usb_interface uinf;

			ret = usbip_recv(sockfd, (void *) &uinf, sizeof(uinf));
			if(ret < 0) {
				err("recv usb_interface[%d]", j);
				return -1;
			}

			pack_usb_interface(0, &uinf);
			usbip_class_name(class_name, sizeof(class_name), uinf.bInterfaceClass,
					uinf.bInterfaceSubClass, uinf.bInterfaceProtocol);

			info("%10s: %2d - %s", " ", j, class_name);
		}

		info(" ");
	}

	return rep.ndev;
}

static int import_device(int sockfd, struct usb_device *udev)
{
	int ret;
	struct usbip_vhci_driver *driver;

	driver = usbip_vhci_driver_open();
	if(!driver) {
		err("open vhci_driver");
		return -1;
	}

	ret = usbip_vhci_get_free_port(driver);
	if(ret < 0) {
		err("no free port");
		usbip_vhci_driver_close(driver);
		return -1;
	}

	ret = usbip_vhci_import_device(driver, (uint8_t) ret, sockfd, udev);
	if(ret < 0) 
		err("import device");

	usbip_vhci_driver_close(driver);

	return ret;
}

static int query_import_device(int sockfd, char *busid)
{
	int ret;
	struct op_import_request request;
	struct op_import_reply   reply;
	uint16_t code = OP_REP_IMPORT;

	bzero(&request, sizeof(request));
	bzero(&reply, sizeof(reply));

	ret = usbip_send_op_common(sockfd, OP_REQ_IMPORT, 0);
	if(ret < 0) {
		err("send op_common");
		return -1;
	}

	memcpy(&request.busid, busid, SYSFS_BUS_ID_SIZE);

	PACK_OP_IMPORT_REQUEST(0, &request);

	ret = usbip_send(sockfd, (void *) &request, sizeof(request));
	if(ret < 0) {
		err("send op_import_request");
		return -1;
	}

	ret = usbip_recv_op_common(sockfd, &code);
	if(ret < 0) {
		err("recv op_common");
		return -1;
	}

	ret = usbip_recv(sockfd, (void *) &reply, sizeof(reply));
	if(ret < 0) {
		err("recv op_import_reply");
		return -1;
	}

	PACK_OP_IMPORT_REPLY(0, &reply);

	if(strncmp(reply.udev.busid, busid, SYSFS_BUS_ID_SIZE)) {
		err("recv different busid %s", reply.udev.busid);
		return -1;
	}

	ret = import_device(sockfd, &reply.udev);
	if(ret < 0) {
		err("attach device");
	}

	return 0;
}

static void attach_device(char *host, char *busid)
{
	int sockfd;
	int ret;

	sockfd = tcp_connect(host, "3240");
	if(sockfd < 0) {
		err("tcp connect");
		return;
	}

	ret = query_import_device(sockfd, busid);
	if(ret < 0) {
		err("query");
	}

	close(sockfd);

	return;
}

static void detach_port(char *port)
{
	struct usbip_vhci_driver *driver;
	uint8_t portnum;

	for(unsigned int i=0; i < strlen(port); i++)
		if(!isdigit(port[i])) {
			err("invalid port %s", port);
			return;
		}

	/* check max port */

	portnum = atoi(port);

	driver = usbip_vhci_driver_open();
	if(!driver) {
		err("open vhci_driver");
		return;
	}

	usbip_vhci_detach_device(driver, portnum);

	usbip_vhci_driver_close(driver);
}

static void show_exported_devices(char *host)
{
	int ret;
	int sockfd;

	sockfd = tcp_connect(host, USBIP_PORT_STRING);
	if(sockfd < 0) {
		info("- %s failed", host);
		return;
	}

	info("- %s", host);

	ret = query_exported_devices(sockfd);
	if(ret < 0) {
		err("query");
	}

	close(sockfd);
}


const char help_message[] = "\
Usage: usbip [options]				\n\
	-a, --attach [host] [bus_id]		\n\
		Attach a remote USB device.	\n\
						\n\
	-d, --detach [ports]			\n\
		Detach an imported USB device.	\n\
						\n\
	-l, --list [hosts]			\n\
		List exported USB devices.	\n\
						\n\
	-p, --port				\n\
		List virtual USB port status. 	\n\
						\n\
	-D, --debug				\n\
		Print debugging information.	\n\
						\n\
	-v, --version				\n\
		Show version.			\n\
						\n\
	-h, --help 				\n\
		Print this help.		\n";

static void show_help(void)
{
	printf("%s", help_message);
}

static void show_port_status(void) 
{
	struct usbip_imported_device *idev;
	struct usbip_vhci_driver *driver;

	driver = usbip_vhci_driver_open();
	if(!driver) {
		return;
	}

	for(int i = 0; i < driver->nports; i++) {
		idev = &driver->idev[i];

		usbip_vhci_imported_device_dump(driver, idev);
	}

	usbip_vhci_driver_close(driver);
}

#define _GNU_SOURCE
#include <getopt.h>
static const struct option longopts[] = {
	{"attach",	no_argument,	NULL, 'a'},
	{"detach",	no_argument,	NULL, 'd'},
	{"port",	no_argument,	NULL, 'p'},
	{"list",	no_argument,	NULL, 'l'},
	{"version",	no_argument,	NULL, 'v'},
	{"help",	no_argument,	NULL, 'h'},
	{"debug",	no_argument,	NULL, 'D'},
	{"syslog",	no_argument,	NULL, 'S'},
	{NULL,		0,		NULL,  0}
};

int main(int argc, char *argv[])
{
	int ret;

	enum {
		cmd_attach = 1,
		cmd_detach,
		cmd_port,
		cmd_list,
		cmd_help,
		cmd_version
	} cmd = 0;

	usbip_use_stderr = 1;

	//delete by chz. The ids file was not used. 2011-12-14
	/*
 	ret = names_init(USBIDS_FILE);
 	if(ret)
 		err("open usb.ids");
	*/
	//end delete
	
	while(1) {
		int c;
		int index = 0;

		c = getopt_long(argc, argv, "adplvhDS", longopts, &index);

		if(c == -1)
			break;

		switch(c) {
			case 'a':
				if(!cmd)
					cmd = cmd_attach;
				else
					cmd = cmd_help;
				break;
			case 'd':
				if(!cmd)
					cmd = cmd_detach;
				else
					cmd = cmd_help;
				break;
			case 'p':
				if(!cmd)
					cmd = cmd_port;
				else
					cmd = cmd_help;
				break;
			case 'l':
				if(!cmd)
					cmd = cmd_list;
				else
					cmd = cmd_help;
				break;
			case 'v':
				if(!cmd)
					cmd = cmd_version;
				else
					cmd = cmd_help;
				break;
			case 'h':
				cmd = cmd_help;
				break;
			case 'D':
				usbip_use_debug = 1;
				break;
			case 'S':
				usbip_use_syslog = 1;
				break;
			case '?':
				break;

			default:
				err("getopt");
		}
	}


	switch(cmd) {
		case cmd_attach:
			if(optind == argc - 2)
				attach_device(argv[optind], argv[optind+1]);
			else
				show_help();
			break;
		case cmd_detach:
			while(optind < argc)
				detach_port(argv[optind++]);
			break;
		case cmd_port:
			show_port_status();
			break;
		case cmd_list:
			while(optind < argc)
				show_exported_devices(argv[optind++]);
			break;
		case cmd_version:
			printf("%s\n", version);
			break;
		case cmd_help:
			show_help();
			break;
		default:
			info("unknown cmd");
			show_help();
	}


	names_deinit();

	return 0;
}
