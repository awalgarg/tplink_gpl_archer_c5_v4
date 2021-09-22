/*
 * Copyright (C) 2005-2006 Takahiro Hirofuchi
 */

#ifndef _USBIP_STUB_DRIVER_H
#define _USBIP_STUB_DRIVER_H

#include "usbip.h"

/* added by tf, 110510 */
typedef enum {
	USBIP_SERVER_OFF,
	USBIP_SERVER_ON,
	USBIP_SERVER_BUSY,
	STUB_OFFLINE
}USBIP_SERVER_STATE;

struct usbip_stub_driver {
	int ndevs;
	struct sysfs_driver *sysfs_driver;

	struct dlist *edev_list;	/* list of exported device */
};

/* added by tf 110512 , waiting list for Client */
struct waiting_list {
	struct waiting_list *next;
	/* Modify by chz for IPv6 host name, 2013-05-30 */
	//char hostname[16];
	char hostname[USBIP_MAXHOST];
	/* end modify */
	int status;
};

/* added by tf 110512 , device list of Server */
struct dev_list	{
	struct dev_list	*next;
	struct waiting_list	*client;
	char busid[8];	/* 8 is enough long for the length of busid */

	int status;
};


struct usbip_exported_device {
	struct sysfs_device *sudev;

	int32_t status;
	struct usb_device    udev;
	struct usb_interface uinf[];
};

/* added by tf 110512 ,  device list of Server */
extern struct dev_list	*devlist;

/* Modify by chz for IPv6 host name, 2013-05-30 */
//extern char host[NI_MAXHOST];	/* record ipaddr of Client */
extern char host[USBIP_MAXHOST];
/* end modify */


struct usbip_stub_driver *usbip_stub_driver_open(void);
void usbip_stub_driver_close(struct usbip_stub_driver *driver);
int usbip_stub_refresh_device_list(struct usbip_stub_driver *driver);

int usbip_stub_reserve_device(struct usbip_exported_device *edev);

int usbip_stub_export_device(struct usbip_exported_device *edev, int sockfd);

/* add for SWIG */
struct usbip_exported_device *usbip_stub_get_device(struct usbip_stub_driver *, int num);
#endif
