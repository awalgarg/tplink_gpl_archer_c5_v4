/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		ushare_tplink.h
 * brief	tplink 我司相关的一些实现.	
 * details	
 *
 * author	LI CHENGLONG(port from HouXB)
 * version	1.0.0
 * date		24Nov11
 *
 * history 	\arg  1.0.0, 24Nov11, LI CHENGLONG, Create the file.	
 */

#ifndef __USHARE_TPLINK_H__
#define __USHARE_TPLINK_H__

/**************************************************************************************************/
/*                                          INCLUDES                                              */
/**************************************************************************************************/
#include <os_msg.h>
#include "list_template.h"
#include "content.h"
#include "ushare.h"
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
#define BUFLEN_6		6
#define BUFLEN_8        8
#define BUFLEN_16       16
#define BUFLEN_18       18
#define BUFLEN_20		20
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

#define DEVELOP_DEBUG	0
#define US_SCAN_DFINTERVAL 3600			/*1 hour*/
#define US_DEF_NAME	"MediaShare"


/* 
 * brief	Add ":1" to the end of ushare servername make our dut compatible to xbox.	
 *			Add by zengdongbiao, 29May15.
 */
#define USHARE_SERVERNAME_SUFFIX	":1"


#if  DEVELOP_DEBUG == 1
#define USHARE_DEBUG(format, args ...)	 do{printf("[%s:%d]"format, __FUNCTION__, __LINE__, ##args);}while(0)
#else
#define USHARE_DEBUG(format, args ...)
#endif /*DEVELOP_DEBUG == 1*/

#define USHARE_ERROR(format, args ...)	 do{printf("[%s:%d]"format, __FUNCTION__, __LINE__, ##args);fflush(stdout);}while(0)
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
typedef	unsigned char	UBOOL8;

typedef signed char		SINT8;
typedef signed short	SINT16;
typedef signed int		SINT32;

typedef unsigned char	UINT8;
typedef unsigned short	UINT16;
typedef unsigned int	UINT32;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是一些public函数实现,ushare程序中其他地方会用到.
 *--------------------------------------------------------------------------------------------------
 */

/* 
 * fn		int ushareProcessInitInfo()
 * brief	ushare 进程接收初始化配置信息.	
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败	
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareProcessInitInfo();

/* 
 * fn		int ushareProcessUserRequest()
 * brief	ushare 进程监听所有的用户请求,并进行处理.	
 *
 * return	BOOL		布尔值
 * retval	FALSE		失败
 *			TRUE		成功
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareProcessUserRequest();

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是与目录处理相关的一些函数实现.
 *--------------------------------------------------------------------------------------------------
 */
/* 
 * fn		BOOL ushare_rebuild_all(int flag) 
 * brief	当收到手动扫描,或自动扫描,或/var/usbdisk下改变时就调用该函数.	
 * param[in]	flag	rebuild同时是否停止服务，0 为停止，1为不停止.
 * details	该函数释放content_list,释放metatree,释放redblack tree;	
 *
 * return	BOOL	布尔值		
 * retval	0		成功
 *			-1		失败
 *
 * note	written by  18Dec11, LI CHENGLONG	
 */
BOOL ushare_rebuild_all(int flag);


/* 
 * fn		content_list *ushare_sync_mtree(content_list *list)
 * brief	更新content_list 链表,媒体信息树和红黑树.	
 *
 * param[in]	list		content链表头.
 *
 * return		content_list*
 * retval		更新后的content链表头
 *
 * note	written by  18Dec11, LI CHENGLONG	
 */
content_list *ushare_sync_mtree(content_list *list);

/* 
 * fn		BOOL ushareRebuildMetaList()
 * brief	重新扫描共享目录, 重新建立多媒体信息树.	
 *
 * return		BOOL
 * retval		TRUE	成功
 *				FALSE	失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareRebuildMetaList();

/* 
 * fn		int ushare_entry_add(int index)
 * brief	将content_list中下标为index的content添加到信息树和redblack树中.	
 *
 * param[in]	list	content_list链表头.
 * param[in]	index	contnet在content_list中的下标.
 *
 * return		BOOL	布尔值
 * retval		TRUE	添加失败.
 *				FALSE	添加成功.
 *
 * note	written by  18Dec11, LI CHENGLONG	
 */
BOOL ushare_entry_add(content_list *list, int index);

/* 
 * fn		int ushare_entry_del(const char *fullpath)
 * brief	释放rootentry下的某个目录的所有媒体信息占用的内存.	
 *
 * param[in]	fullpath	该媒体目录的路径
 *
 * return		BOOL		布尔值
 * retval		TRUE		释放成功
 *				FALSE		释放失败
 *
 * note	written by  18Dec11, LI CHENGLONG	
 */
BOOL ushare_entry_del(const char *fullpath);

/* 
 * fn		int build_content_list(ushare_t *ut)
 * brief	根据配置创建content_list.	
 *
 * param[in/out]	ut		ushare全局配置指针.
 *
 * return			BOOL		布尔值
 * retval			TRUE		成功
 *					FALSE		失败
 *
 * note	written by  18Dec11, LI CHENGLONG	
 */
BOOL build_content_list(ushare_t *ut);

/* 
 * fn		content_list *content_priv_del(content_list *list, const char *item)
 * brief	从list链表中删除路径名为item的共享目录.	
 * details	content.c中并未实现通过路径名删除共享目录的方法,这里通过封装提供一个这样方便的接口.	
 *
 * param[in]	list		content链表
 * param[in]	item		路径字符串	
 *
 * return		content_list*	
 * retval		返回更新后的链表头.
 *
 * note	written by  18Dec11, LI CHENGLONG	
 */
content_list *content_priv_del(content_list *list, const char *item);

/* 
 * fn 		content_list *ushare_content_add(content_list *list, 
 											 const char *item, 
 											 const char *name, 
 											 const char *pFolderUuid,
 											 char flag)
 * brief	根据item和name添加一条共享路径.	
 * details	在/var/usbdisk目录下扫描所有存在的共享路径,添加到共享链表中，要求UUID必须匹配。
 *
 * param[in]	list			目录共享链表.
 * param[in]	shareAll		to indicate whether to share all the volumes or share the folders set by user
 * param[in]	item			共享目录根路径.
 * param[in]	name			共享目录名称.
 * param[in]	pFolderUuid		共享目录所在分区的uuid.
 * param[in]	flag			是否要更新媒体信息.
 *
 * return		content_list*
 * retval		返回更新后的链表头.
 *
 * note	written by  16Dec11, LI CHENGLONG	
 *	modified by LY to add shareAll flag to work with newUI,in 2014.09.10
 */
content_list *ushare_content_add(content_list *list, int shareAll, const char *item, 
										const char *name, const char *pFolderUuid, char flag);

/* 
 * fn		content_list *ushare_content_del(content_list *list, 
 											 const char *item, 
 											 const char *name, 
 											 char flag)
 * brief	根据item和name删除一条共享路径.	
 * details	在/var/usbdisk目录下扫描所有存在的共享路径,从共享链表中删除所有路径.
 *
 * param[in]	list			目录共享链表.
 * param[in]	item			共享目录根路径.
 * param[in]	name			共享目录名称.
 * param[in]	flag			是否要更新媒体信息.
 *
 * return		content_list*
 * retval		返回更新后的链表头.
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
content_list *ushare_content_del(content_list *list, const char *item, const char *name, char flag);

/* 
 * fn		BOOL ushareStartAutoScanWorker()
 * brief	Ushare 进程启动时开启该woker. 用于定时.	
 *
 * return	BOOL	布尔值	
 * retval	FALSE	失败
 *			TRUE	成功
 *
 * note	written by  25Nov11, LI CHENGLONG	
 */
BOOL ushareStartAutoScanWorker();

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是与开启关闭相关的一些函数声明.
 *--------------------------------------------------------------------------------------------------
 */

/* 
 * fn	 	BOOL ushareSwitchOn() 
 * brief	开启media server功能.
 *
 * return	BOOL	布尔值
 * retval	TRUE	开启成功	
 *			FALSE	开启失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareSwitchOn();

/* 
 * fn	 	BOOL ushareSwitchOff() 
 * brief	关闭media server功能.	
 *
 * return	BOOL	布尔值
 * retval	TRUE	关闭成功
 *			FALSE 	关闭失败	
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareSwitchOff();

/* 
 * fn		BOOL ushareSwitchOnOff(BOOL onOff)
 * brief	开启或关闭MediaServer功能.	
 *
 * param[in]	onOff	开启或关闭MediaServer功能.
 *
 * return		BOOL	布尔值
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareSwitchOnOff(BOOL onOff);

/* 
 * fn		BOOL ushareParseConfig(int argc, char **argv) 
 * brief	ushare进程解析配置,包括命令行, 配置文件, 初始化消息.	
 * details	经过这一步解析,所有的配置都保存在ut结构中,后续程序运行就根据ut来操作.	
 *
 * param[in]	argc		直接是main函数的参数个数
 * param[in]	argv		直接是main函数的参数数组	
 *
 * return		BOOL		布尔值
 * retval		TRUE		解析成功
 *				FALSE		解析过程中出现错误
 *
 * note	written by  25Nov11, LI CHENGLONG	
 */
BOOL ushareParseConfig(int argc, char **argv);

/* 
 * fn		BOOL ushareInitUPnPEnv() 
 * brief	ushare进程初始化upnp的环境,包括upnpinit调用等,整个ushare进程运行中只初始化一次.	
 *
 * return		BOOL	布尔值
 * retval		TRUE	初始化成功
 *				FALSE	初始化失败
 *
 * note	written by  25Nov11, LI CHENGLONG	
 */
BOOL ushareInitUPnPEnv();

/* 
 * fn		int daemonize(int nochdir, int noclose)
 * brief	将当前进程放到后台运行.	
 *
 * param[in]	nochdir		是否改变后台进程的工作目录
 * param[in]	noclode		是否改变后台进程的标准输入输出	
 *
 * return		int		错误码
 * retval		-1		失败
 *				0		成功
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
int daemonize(int nochdir, int noclose);

#endif	/* __USHARE_TPLINK_H__ */
