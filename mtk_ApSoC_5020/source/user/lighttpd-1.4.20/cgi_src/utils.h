#ifndef HAVE_UTILS_H
#define HAVE_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvram.h"
#include "linux_conf.h"
#include "user_conf.h"

#include <dirent.h>
#include <sys/stat.h>
#if defined CONFIG_USER_STORAGE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#define DATALEN		65
#define CMDLEN		256
#define IF_NAMESIZE     16
#define LOG_MAX 	16384
#if defined(RT2860_NEW_MBSS_SUPPORT) || defined(RTDEV_NEW_MBSS_SUPPORT) || defined(RT2860_16BSSID_SUPPORT) || defined(RTDEV_16BSSID_SUPPORT)
#define MBSSID_MAX_NUM 16
#else
#define MBSSID_MAX_NUM 8
#endif

#define SYSTEM_COMMAND_LOG	"/var/system_command.log"
#define LOGFILE	"/dev/console"
#define MOUNT_INFO      "/proc/mounts"

struct media_config {
	char path[40];
};

/*
 * FOR CGI
 */
inline int get_one_port(void);
int get_ifip(char *ifname, char *if_addr);
int get_nth_value(int index, char *value, char delimit, char *result, int len);
char* get_field(char *a_line, char *delim, int count);
int get_nums(char *value, char delimit);
int get_if_live(char *ifname);
int delete_nth_value(int index[], int count, char *value, char delimit);
void set_nth_value_flash(int nvram, int index, char *flash_key, char *value);
char *get_lanif_name(void);
char *get_wanif_name(void);
char *get_macaddr(char *ifname);
char *get_ipaddr(char *ifname);
char *get_netmask(char *ifname);
int get_index_routingrule(const char *rrs, char *dest, char *netmask, char *interface);
#if defined CONFIG_USER_STORAGE
FILE *fetch_media_cfg(struct media_config *cfg, int write);
#endif

int is_ipnetmask_valid(char *str);
int is_ip_valid(char *str);
int is_mac_valid(char *str);
int is_oneport_only(void);

char *strip_space(char *str);
char *racat(char *s, int i);
int port_secured(char *ifname);
void unencode(char *src, char *last, char *dest);
int netmask_aton(const char *ip);
void convert_string_display(char *str);
void do_system(char *fmt, ...);
void do_system_redirect(char *fmt, ...);
int check_semicolon(char *str);
void free_all(int num, char *fmt, ...);

unsigned int convert_rssi_signal_quality(long rssi);

/* 
 * FOR PROCESS
 */
void update_flash_8021x(int nvram);

/* 
 * FOR WEB
 */
void web_redirect(const char *url);
void web_redirect_wholepage(const char *url);
void web_back_parentpage(void);
void web_debug_header(void);
void web_debug_header_no_cache(void);
char *web_get(char *tag, char *input, int dbg);
#define WebTF(str, tag, nvram, entry, dbg) 	nvram_bufset(nvram, entry, web_get(tag, str, dbg))

#define WebNthTF(str, tag, nvram, index, entry, dbg) 	set_nth_value_flash(nvram, index, entry, web_get(tag, str, dbg))

/* Set the same value into all BSSID */
#define SetAllValues(bssid_num, nvram, flash_key, value)	do {	char buffer[24]; int i=1;                       \
									strcpy(buffer, value);                          \
									while (i++ < bssid_num)                         \
									sprintf(buffer, "%s;%s", buffer, value);    \
									nvram_bufset(nvram, #flash_key, buffer);        \
								} while(0)

#define DBG_MSG(fmt, arg...)	do {	FILE *log_fp = fopen(LOGFILE, "w+"); \
						if(log_fp == NULL) break;\
						fprintf(log_fp, "%s:%s:%d:" fmt "\n", __FILE__, __func__, __LINE__, ##arg); \
						fclose(log_fp); \
					} while(0)

void lookupAllList(char *dir_path);
void lookupSelectList(void);

//#if defined (CONFIG_RT2860V2_STA_DPB) || defined (CONFIG_RT2860V2_STA_ETH_CONVERT) || \
	defined (CONFIG_RLT_STA_SUPPORT) || defined (CONFIG_MT_STA_SUPPORT) || \
	defined (CONFIG_RT2860V2_STA) || defined (CONFIG_RT2860V2_STA_MODULE)
#if defined (RT2860_STA_SUPPORT)
#ifndef FALSE
#define FALSE	(0==1)
#endif

#ifndef TRUE
#define TRUE	(1==1)
#endif
        
int OidQueryInformation(unsigned long OidQueryCode, int socket_id, char *DeviceName, void *ptr, unsigned long PtrLength);
int OidSetInformation(unsigned long OidQueryCode, int socket_id, char *DeviceName, void *ptr, unsigned long PtrLength);
#endif /* RT2860_STA_SUPPORT */
#endif /* HAVE_UTILS_H */
