/*
 * Copyright (C) 2005-2006 Takahiro Hirofuchi
 */

#ifndef _USBIP_COMMON_H
#define _USBIP_COMMON_H

#include <unistd.h>
#include <stdint.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include "sysfs/libsysfs.h"
#include <netdb.h>
#include <sys/socket.h>

//Delete by chz for TD-W8968V1
/* definition for LOGTYPE_NAS and declaration for msglogd */
//#include "libmsglog.h"
//end delete

//delete by chz. The ids file was not used. 2011-12-14
//#define USBIDS_FILE "/tmp/usb.ids"
//end delete

#define CLIENT 3

/* Add by chz for IPv6 host name, 2013-05-30 */
#define USBIP_MAXHOST 42
/* end add */

extern int usbip_use_syslog;
extern int usbip_use_stderr;
extern int usbip_use_debug ;

//#include <linux/usb_ch9.h>
enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,                  /* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,          /* usb 1.1 */
	USB_SPEED_HIGH,                         /* usb 2.0 */
	USB_SPEED_VARIABLE                      /* wireless (usb 2.5) */
};

/* FIXME: how to sync with drivers/usbip_common.h ? */
enum usbip_device_status{
	/* sdev is absent, add by tf */
	SDEV_ST_NULL = 0x00,
	/* sdev is available. */
	SDEV_ST_AVAILABLE = 0x01,
	/* sdev is now used. */
	SDEV_ST_USED,
	/* sdev is unusable because of a fatal error. */
	SDEV_ST_ERROR,

	/* vdev does not connect a remote device. */
	VDEV_ST_NULL,
	/* vdev is used, but the USB address is not assigned yet */
	VDEV_ST_NOTASSIGNED,
	VDEV_ST_USED,
	VDEV_ST_ERROR
};

/* increment here when new release */
/*#define USBIP_VERSION 0x01	*/
#if 0
#define USBIP_VERSION 0x106
#endif
#if CLIENT > 2
#define USBIP_TP_VERSION	0x0200
#else
#define USBIP_TP_VERSION	0x0106
#endif

#define USBIP_TP_MAGIC	0x5450	/* "TP" */
/*#define LOGTYPE_NAS		 7*/

#define err(fmt, args...)	do { \
	if(usbip_use_syslog) { \
		syslog(LOG_ERR, "usbip err: %13s:%4d (%-12s) " fmt "  [%s]", \
			__FILE__, __LINE__, __FUNCTION__,  ##args, strerror(errno)); \
	} \
	if(usbip_use_stderr) { \
		fprintf(stderr, "usbip err: %13s:%4d (%-12s) " fmt "  [%s]\n", \
			__FILE__, __LINE__, __FUNCTION__,  ##args, strerror(errno)); \
	} \
} while (0)

//modify by chz
/* log to webserver */
	/*
#define ps_err(fmt, args...)	do { \
	if(usbip_use_syslog) { \
		msglogd(LOG_ERR, LOGTYPE_NAS, "ps: " fmt "  [%s]", ##args, strerror(errno)); \
	} \
	if(usbip_use_stderr) { \
		fprintf(stderr, "usbip err: %13s:%4d (%-12s) " fmt "  [%s]\n", \
			__FILE__, __LINE__, __FUNCTION__,  ##args, strerror(errno)); \
	} \
} while (0)
*/
#define ps_err(fmt, args...)	do { \
		if(usbip_use_stderr) { \
			fprintf(stderr, "usbip err: %13s:%4d (%-12s) " fmt "  [%s]\n", \
				__FILE__, __LINE__, __FUNCTION__,  ##args, strerror(errno)); \
		} \
	} while (0)

//end modify

//modify by chz
//#if 0
#if 1
//end modify
#define notice(fmt, args...)	do { \
	if(usbip_use_syslog) { \
		syslog(LOG_DEBUG, "usbip: " fmt, ##args); \
	} \
	if(usbip_use_stderr) { \
		fprintf(stderr, "usbip: " fmt "\n",  ##args); \
	} \
} while (0)
#else	/* log to webserver */
#define notice(fmt, args...)	do { \
	if(usbip_use_syslog) { \
		msglogd(LOG_DEBUG, LOGTYPE_NAS,  "ps: " fmt, ##args); \
	} \
	if(usbip_use_stderr) { \
		fprintf(stderr, "usbip: " fmt "\n",  ##args); \
	} \
} while (0)
#endif

/* //modify by chz
#define ps_info(fmt, args...)	do { \
	if(usbip_use_syslog) { \
		//msglogd(LOG_INFO, LOGTYPE_NAS, "ps: " fmt, ##args); \
	} \
	if(usbip_use_stderr) { \
		fprintf(stderr, fmt "\n",  ##args); \
	} \
} while (0)
*/
#define ps_info(fmt, args...)	do { \
		if(usbip_use_stderr) { \
			fprintf(stderr, fmt "\n",  ##args); \
		} \
} while (0)
//end modify

#define info(fmt, args...)	do { \
	if(usbip_use_syslog) { \
		syslog(LOG_DEBUG, fmt, ##args); \
	} \
	if(usbip_use_stderr) { \
		fprintf(stderr, fmt "\n",  ##args); \
	} \
} while (0)


#define dbg(fmt, args...)	do { \
	if(usbip_use_debug) { \
		if(usbip_use_syslog) { \
			syslog(LOG_DEBUG, "usbip dbg: %13s:%4d (%-12s) " fmt, \
				__FILE__, __LINE__, __FUNCTION__,  ##args); \
		} \
		if(usbip_use_stderr) { \
			fprintf(stderr, "usbip dbg: %13s:%4d (%-12s) " fmt "\n", \
				__FILE__, __LINE__, __FUNCTION__,  ##args); \
		} \
	} \
} while (0)


#define BUG()	do { err("sorry, it's a bug"); abort(); } while(0)

struct usb_interface {
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t padding;	/* alignment */
} __attribute__((packed));

#if CLIENT > 2
#define STRING_MAXLEN 64  // added by zl 2011-2-12
#endif

struct usb_device {
	char path[SYSFS_PATH_MAX];
	char busid[SYSFS_BUS_ID_SIZE];
	
#if CLIENT > 2
	// added by zl 2011-2-12
	char product[STRING_MAXLEN];
	char manufacturer[STRING_MAXLEN];
	char serial[STRING_MAXLEN];
   	 // end----added
 #endif
    
	uint32_t busnum;
	uint32_t devnum;
	uint32_t speed;

	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;

	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bConfigurationValue;
	uint8_t bNumConfigurations;
	uint8_t bNumInterfaces;
} __attribute__((packed));

#define to_string(s)	#s

void dump_usb_interface(struct usb_interface *);
void dump_usb_device(struct usb_device *);
int read_usb_device(struct sysfs_device *sdev, struct usb_device *udev);
int read_attr_value(struct sysfs_device *dev, const char *name, const char *format);
int read_usb_interface(struct usb_device *udev, int i, struct usb_interface *uinf);

const char *usbip_speed_string(int num);
const char *usbip_status_string(int32_t status);

void usbip_product_name(char *buff, size_t size, uint16_t vendor, uint16_t product);
void usbip_class_name(char *buff, size_t size, uint8_t class, uint8_t subclass, uint8_t protocol);

void pack_uint32_t(int pack, uint32_t *num);
void pack_uint16_t(int pack, uint16_t *num);
void pack_usb_device(int pack, struct usb_device *udev);
void pack_usb_interface(int pack, struct usb_interface *uinf);

#endif
