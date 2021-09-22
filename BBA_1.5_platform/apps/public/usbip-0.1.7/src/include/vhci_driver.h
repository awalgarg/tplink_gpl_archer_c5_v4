/*
 * Copyright (C) 2005-2006 Takahiro Hirofuchi
 */

#ifndef _VHCI_DRIVER_H
#define _VHCI_DRIVER_H

#include "usbip.h"


extern const char *portst_str[];


#define MAXNPORT 128

struct class_device {
	char clspath[SYSFS_PATH_MAX];
	char devpath[SYSFS_PATH_MAX];
};

struct usbip_imported_device {
	uint8_t port;
	uint32_t status;

	uint8_t busnum;
	uint8_t devnum;

	char remote_busid[SYSFS_BUS_ID_SIZE];


	struct sockaddr_storage ss;
	

	struct dlist *cdev_list;	/* list of class device */
	struct usb_device udev;
};

struct usbip_vhci_driver {
	char sysfs_mntpath[SYSFS_PATH_MAX];
	struct sysfs_device *hc_device; /* /sys/devices/platform/hc.0 */

	//struct dlist *idev_list;	/* list of usbip device */
	struct dlist *cdev_list;	/* list of class device */

	int nports;
	struct usbip_imported_device idev[MAXNPORT];
};



struct usbip_vhci_driver *usbip_vhci_driver_open(void);
void   usbip_vhci_driver_close(struct usbip_vhci_driver *driver);
int  usbip_vhci_refresh_device_list(struct usbip_vhci_driver *driver);

void usbip_vhci_imported_device_dump(struct usbip_vhci_driver *driver, 
		struct usbip_imported_device *dev);

int usbip_vhci_get_free_port(struct usbip_vhci_driver *driver);
int usbip_vhci_import_device(struct usbip_vhci_driver *driver,
		uint8_t port, int sockfd, struct usb_device *udev);
int usbip_vhci_detach_device(struct usbip_vhci_driver *driver, uint8_t port);
#endif
