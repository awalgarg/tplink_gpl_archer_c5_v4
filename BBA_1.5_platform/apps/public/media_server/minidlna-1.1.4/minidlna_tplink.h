/*  
 *
 * file   minidlna_tplink.h
 * brief  tplink
 * details  
 *
 * author liweijie(port from Lichenglong)
 * version  1.0.0
 * date   19Jun14
 *
 * history  \arg  1.0.0, 19Jun14, Liweijie, Create the file.  
 */

#ifndef __MINIDLNA_TPLINK_H__
#define __MINIDLNA_TPLINK_H__

/**************************************************************************************************/
/*                                          INCLUDES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

#ifndef TRUE
#define TRUE 1
#endif
 
#ifndef FALSE
#define FALSE 0
#endif

#define BUFLEN_4        4
#define BUFLEN_6        6
#define BUFLEN_8        8
#define BUFLEN_16       16
#define BUFLEN_18       18
#define BUFLEN_20       20
#define BUFLEN_24       24
#define BUFLEN_32       32
#define BUFLEN_40       40
#define BUFLEN_48       48
#define BUFLEN_64       64
#define BUFLEN_128      128
#define BUFLEN_256      256
#define BUFLEN_264      264
#define BUFLEN_512      512
#define BUFLEN_1024     1024


#define DEVELOP_DEBUG 0
#define MEDIA_SERVER_DEF_NAME "MediaShare"
#define MINIDLNA_MEDIA_SERVER_FOLDER_DISPNAME_L  32
#define MINIDLNA_MEDIA_SERVER_SERVERNAME_L    16
#define MINIDLNA_MEDIA_SERVER_FOLDER_PATH_L   512

#define USB_DISK_DIRECTORY	"/var/usbdisk"
#define USBVM 	"usbvm"
#define USB_DISK_MEDIA_SERVER_FILE_NAME ".media_server"

#define DEV_DIR_PREFIX			"/var/dev/"
#define FILE_BLKID_RESULT		"/var/run/blkid_info"
#define FORMAT_BLKID_COMMAND	"blkid %s > %s"
#define FILE_DM_STORAGE			"/var/run/dm_storage" 

/*added by LY to share all the volumes if user set shareAll flag from UI, in  2014.09.05*/
#define ROOT_DIRECTORY		"/"
#define ROOT_DISPLAYNAME	"Share"
#define ROOT_UUID			"nouuid"
/*end added by LY*/

#if  DEVELOP_DEBUG == 1
#define MINIDLNA_DEBUG(format, args ...)   do{printf("[%s:%d]"format, __FUNCTION__, __LINE__, ##args);}while(0)
#else
#define MINIDLNA_DEBUG(format, args ...)
#endif /*DEVELOP_DEBUG == 1*/

#define  MINIDLNA_ERROR(format, args ...)   do{printf("[%s:%d]"format, __FUNCTION__, __LINE__, ##args);fflush(stdout);}while(0)

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
typedef int			  BOOL;
typedef unsigned char UBOOL8;

typedef signed char   SINT8;
typedef signed short  SINT16;
typedef signed int    SINT32;

typedef unsigned char UINT8;
typedef unsigned short  UINT16;
typedef unsigned int  UINT32;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/* 
 * brief	数据库文件名列表。若数据库文件名不能使用，则在此列表中寻找下一个可用的数据库文件名。	
 */
extern const char *g_databaseNames[];

/* 
 * brief	数据库文件名，默认为"files.db"。
 */
extern char *g_dbNameStr;

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
 
/* 
 * fn		BOOL minidlnaProcessInitInfo()
 * brief	minidlna 进程接收初始化配置信息.	
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败	
 *
 * note	written by  19Jun14, liweijie
 */
BOOL minidlnaProcessInitInfo();

/* 
 * fn		BOOL minidlnaProcessConfigureRequest()
 * brief	minidlna 进程监听所有的用户请求,并进行处理.	
 *
 * return	BOOL		布尔值
 * retval	FALSE		失败
 *			TRUE		成功
 *
 * note written by  19Jun14, liweijie 
 */
BOOL minidlnaProcessConfigureRequest();

/* 
 * fn		BOOL minidlnaSwitchOnOff(BOOL onOff)
 * brief	开启或关闭MediaServer功能.	
 *
 * param[in]	onOff		开启或关闭MediaServer功能.
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  22Jun14, liweijie	
 */
BOOL minidlnaSwitchOnOff(BOOL onOff);

void free_media_dirs_realative();

/* 
 * fn		int minidlnaModifyFriendlyName(char *FriendlyName, 
 										  int FriendlyNameSize)
 * brief	Modify friendly_name.
 * details	将friendly_name中的一些特殊字符(&, <, > 等)替换掉。
 *
 * param[in]	FriendlyName	friendly_name
 * param[in]	FriendlyNameSize	The size of friendly_name.
 * return		
 * retval		0	successful.
 * 				-1	The size of friendly_name is not enough.
 *				-2	The lenght of friendly_name is longer than the 	ORIGINAL_FRIENDLYNAME_MAX_LEN.			
 *
 * note			written by zengdongbiao, 07Apr15.
 */
int minidlnaModifyFriendlyName(char *friendlyName, 
							  int friendlyNameSize);



#endif  /* __MINIDLNA_TPLINK_H__ */
