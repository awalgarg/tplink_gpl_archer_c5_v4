/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/miniupnpd-1.6/miniupnpdpath.h#1 $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2011 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __MINIUPNPDPATH_H__
#define __MINIUPNPDPATH_H__

#include "config.h"

/* Paths and other URLs in the miniupnpd http server */

#define ROOTDESC_PATH 		"/rootDesc.xml"

#ifdef ENABLE_WSC_SERVICE
#define WFA_ROOTDESC_PATH 	"/WFADeviceDesc.xml"
#define WSC_PATH			"/WFAWLANConfigSCPD.xml"
#define WSC_CONTROLURL		"/control"
#define WSC_EVENTURL		"/event"
#endif /* ENABLE_WSC_SERVICE */

#ifdef HAS_DUMMY_SERVICE
#define DUMMY_PATH			"/dummy.xml"
#endif

#define WANCFG_PATH			"/WANCfg.xml"
#define WANCFG_CONTROLURL	"/ctl/CmnIfCfg"
#define WANCFG_EVENTURL		"/evt/CmnIfCfg"

#define WANIPC_PATH			"/WANIPCn.xml"
#define WANIPC_CONTROLURL	"/ctl/IPConn"
#define WANIPC_EVENTURL		"/evt/IPConn"

#ifdef ENABLE_L3F_SERVICE
#define L3F_PATH			"/L3F.xml"
#define L3F_CONTROLURL		"/ctl/L3F"
#define L3F_EVENTURL		"/evt/L3F"
#endif

#ifdef ENABLE_6FC_SERVICE
#define WANIP6FC_PATH		"/WANIP6FC.xml"
#define WANIP6FC_CONTROLURL	"/ctl/IP6FCtl"
#define WANIP6FC_EVENTURL	"/evt/IP6FCtl"
#endif

#ifdef ENABLE_DP_SERVICE
/* For DeviceProtection introduced in IGD v2 */
#define DP_PATH				"/DP.xml"
#define DP_CONTROLURL		"/ctl/DP"
#define DP_EVENTURL			"/evt/DP"
#endif

#endif

