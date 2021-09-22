/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/miniupnpd-1.6/upnpdescstrings.h#1 $ */
/* miniupnp project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2012 Thomas Bernard
 * This software is subject to the coditions detailed in
 * the LICENCE file provided within the distribution */
#ifndef __UPNPDESCSTRINGS_H__
#define __UPNPDESCSTRINGS_H__

#include "config.h"

/* strings used in the root device xml description */
/*#define ROOTDEV_FRIENDLYNAME		OS_NAME " router"*/
#define ROOTDEV_MANUFACTURER		OS_NAME
#define ROOTDEV_MANUFACTURERURL		OS_URL
#define ROOTDEV_MODELNAME			OS_NAME " router"
#define ROOTDEV_MODELDESCRIPTION	OS_NAME " router"
#define ROOTDEV_MODELURL			OS_URL

#define WANDEV_FRIENDLYNAME			"WANDevice"
#define WANDEV_MANUFACTURER			"MiniUPnP"
#define WANDEV_MANUFACTURERURL		"http://miniupnp.free.fr/"
#define WANDEV_MODELNAME			"WAN Device"
#define WANDEV_MODELDESCRIPTION		"WAN Device"
#define WANDEV_MODELNUMBER			UPNP_VERSION
#define WANDEV_MODELURL				"http://miniupnp.free.fr/"
#define WANDEV_UPC					"MINIUPNPD"

#define WANCDEV_FRIENDLYNAME		"WANConnectionDevice"
#define WANCDEV_MANUFACTURER		WANDEV_MANUFACTURER
#define WANCDEV_MANUFACTURERURL		WANDEV_MANUFACTURERURL
#define WANCDEV_MODELNAME			"MiniUPnPd"
#define WANCDEV_MODELDESCRIPTION	"MiniUPnP daemon"
#define WANCDEV_MODELNUMBER			UPNP_VERSION
#define WANCDEV_MODELURL			"http://miniupnp.free.fr/"
#define WANCDEV_UPC					"MINIUPNPD"

#ifdef ENABLE_WSC_SERVICE
#define WLANDEV_FRIENDLYNAME		"RalinkAPS"
#define WLANDEV_MANUFACTURER		"Ralink Technology, Corp."
#define WLANDEV_MANUFACTURERURL		"http://www.ralinktech.com.tw"
#define WLANDEV_MODELNAME			"Ralink Wireless Access Point"
#define WLANDEV_MODELDESCRIPTION	"Ralink AP WPS device"
//Ralink :  need to modified
#define WLANDEV_MODELNUMBER			"RT3062"
#define WLANDEV_MODELURL			"http://www.ralinktech.com.tw"
//#define WLANDEV_UPC					"MINIUPNPD"
#endif /* ENABLE_WSC_SERVICE */

#endif

