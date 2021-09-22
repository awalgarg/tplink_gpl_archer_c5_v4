/*
 * ushare.h : GeeXboX uShare UPnP Media Server header.
 * Originally developped for the GeeXboX project.
 * Parts of the code are originated from GMediaServer from Oskar Liljeblad.
 * Copyright (C) 2005-2007 Benjamin Zores <ben@geexbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _USHARE_H_
#define _USHARE_H_

#include <upnp.h>
#include <upnptools.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

#ifdef HAVE_DLNA
#include <dlna.h>
#endif /* HAVE_DLNA */

#include <os_msg.h>
#include "content.h"
#include "buffer.h"
#include "list_template.h"


#define VIRTUAL_DIR "/web"
#define XBOX_MODEL_NAME "Windows Media Connect Compatible"
#define DEFAULT_UUID "898f9738-d930-4db4-a3cf"

#define UPNP_MAX_CONTENT_LENGTH 4096

#define STARTING_ENTRY_ID_DEFAULT 0
#define STARTING_ENTRY_ID_XBOX360 100000
#define STARTING_ENTRY_ID_STEP	2000
#define STARTING_ENTRY_ID_CYCLE	100
#define USHARE_MEDIA_SERVER_FOLDER_DISPNAME_L	32
#define USHARE_MEDIA_SERVER_SERVERNAME_L 		16
#define USHARE_MEDIA_SERVER_FOLDER_PATH_L		128

/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-16.
 *		 自动扫描的数据结构. 
 */
typedef struct _USHARE_SCAN_ST
{
	unsigned char 	scanFlag;
	unsigned int 	scanInterval;	
	unsigned int 	counter;/*current counter.*/
}USHARE_SCAN_ST;

/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-21.
 *		  folder info struct.
 */
typedef struct _USHARE_FOLDER_INFO
{
	char				dispName[USHARE_MEDIA_SERVER_FOLDER_DISPNAME_L];
	char 				path[USHARE_MEDIA_SERVER_FOLDER_PATH_L];
	char				uuid[32];
	int enable;	/*added by LY to record whether this item is enabled, in 20141203*/
	struct 	list_head 	list;
}USHARE_FOLDER_INFO;

/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-16.
 *		  
 */
typedef struct _USHARE_INIT_INFO
{
	unsigned char		scanFlag;				/*scan*/
	unsigned char		serverState;			/* ServerState */
	unsigned int		folderCnt;			/*how many folde is shared now*/	
	int			shareAll;				/*added by LY, in 2014.09.05*/
	unsigned int		scanInterval;		/*scan interval*/
	MANUFACT_SPEC_INFO	manuInfo;				/*oem等不同厂商的信息*/		
	char			serverName[USHARE_MEDIA_SERVER_SERVERNAME_L];
	/*folderlist Added by LI CHENGLONG , 2011-Dec-18.*/
	struct list_head 	folderList;
}USHARE_INIT_INFO;

#define UPNP_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <device>" \
"    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>" \
"    <friendlyName>%s: 1</friendlyName>" \
"    <manufacturer>GeeXboX Team</manufacturer>" \
"    <manufacturerURL>http://ushare.geexbox.org/</manufacturerURL>" \
"    <modelDescription>GeeXboX uShare : UPnP Media Server</modelDescription>" \
"    <modelName>%s</modelName>" \
"    <modelNumber>001</modelNumber>" \
"    <modelURL>http://ushare.geexbox.org/</modelURL>" \
"    <serialNumber>GEEXBOX-USHARE-01</serialNumber>" \
"    <UDN>uuid:%s</UDN>" \
"    <serviceList>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>" \
"        <SCPDURL>/web/cms.xml</SCPDURL>" \
"        <controlURL>/web/cms_control</controlURL>" \
"        <eventSubURL>/web/cms_event</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>" \
"        <SCPDURL>/web/cds.xml</SCPDURL>" \
"        <controlURL>/web/cds_control</controlURL>" \
"        <eventSubURL>/web/cds_event</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1</serviceType>\n" \
"        <serviceId>urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar</serviceId>\n" \
"        <SCPDURL>/web/msr.xml</SCPDURL>" \
"        <controlURL>/web/msr_control</controlURL>" \
"        <eventSubURL>/web/msr_event</eventSubURL>" \
"      </service>\n" \
"    </serviceList>" \
"    <presentationURL>/web/ushare.html</presentationURL>" \
"  </device>" \
"</root>"

typedef struct ushare_t {
  char *name;
  char *interface;
  char *model_name;
  content_list *contentlist;
 // struct rbtree *rb;
 // struct upnp_entry_t *root_entry;
  int nr_entries;
  int starting_id;
  int init;
  char *description;
  UpnpDevice_Handle dev;
  char *udn;
  char *ip;
  unsigned short port;
#ifdef INCLUDE_TELNET
  unsigned short telnet_port;
#endif /* INCLUDE_TELNET */
  struct buffer_t *presentation;
  bool use_presentation;
#ifdef INCLUDE_TELNET
  bool use_telnet;
#endif /* INCLUDE_TELNET */
#ifdef HAVE_DLNA
  bool dlna_enabled;
  dlna_t *dlna;
  dlna_org_flags_t dlna_flags;
#endif /* HAVE_DLNA */
  bool xbox360;
  bool verbose;
  bool daemon;
  bool override_iconv_err;
  char *cfg_file;
  unsigned int mediaFilesNum;

 /*tplink相关的一些配置, Added by LI CHENGLONG , 2011-Dec-16.*/
  USHARE_SCAN_ST		scan;
  USHARE_INIT_INFO 	initInfo;
  char *	record_buf;
 /* Ended by LI CHENGLONG , 2011-Dec-16.*/

  pthread_mutex_t termination_mutex;
  pthread_cond_t termination_cond;
}ushare_t;

struct action_event_t {
  struct Upnp_Action_Request *request;
  bool status;
  struct service_t *service;
};


#include "ushare_tplink.h"


inline void display_headers (void);
/* 
 * fn		int daemonize(int nochdir, int noclose)
 * brief	将当前进程放到后台运行.	
 *
 * param[in]	nochdir		是否改变后台进程的工作目录
 * param[in]	noclode		是否改变后台进程的标准输入输出	
 *
 * return		int
 * retval		-1		错误
 *				0		成功
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
int daemonize(int nochdir, int noclose);

int device_callback_event_handler (Upnp_EventType type, void *event, 
										 void *cookie __attribute__((unused)));
void UPnPBreak (int s __attribute__ ((unused)));
void reload_config (int s __attribute__ ((unused)));


struct ushare_t * ushare_new (void)__attribute__ ((malloc));

void ushare_free (struct ushare_t *ut);

bool has_iface (char *interface);


void setup_i18n(void);
void setup_iconv (void);

char *create_udn (char *interface);
char *get_iface_address (char *interface);


#endif /* _USHARE_H_ */
