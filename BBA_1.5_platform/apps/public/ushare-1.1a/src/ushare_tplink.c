/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		ushare_tplink.c
 * brief	tplink 我司相关的一些实现.	
 * details	
 *
 * author	LI CHENGLONG
 * version	1.0.0
 * date		24Nov11
 *
 * history 	\arg  1.0.0, 24Nov11, LI CHENGLONG, Create the file.	
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include "list_template.h"

#include "upnp.h"
#include "upnptools.h"
#include "TimerThread.h"

#include "ushare.h"
#include "ushare_tplink.h"
#include "metadata.h"
#include "config.h"
#include "cfgparser.h"
#include "trace.h"
 
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define USB_DISK_DIRECTORY		"/var/usbdisk"
#define DEV_DIR_PREFIX			"/var/dev/"
#define USHARE_MAX_FOLDER_NUM	32
#define USBVM 					"usbvm"
#define FILE_BLKID_RESULT		"/var/run/blkid_info"
#define FORMAT_BLKID_COMMAND	"blkid %s > %s"
#define FILE_DM_STORAGE			"/var/run/dm_storage" 

/*added by LY to share all the volumes if user set shareAll flag from UI, in  2014.09.05*/
#define ROOT_DIRECTORY		"/"
#define ROOT_DISPLAYNAME	"Share"
#define ROOT_UUID			"nouuid"
/*end added by LY*/

/*添加目录时是否更新信息树, Added by LI CHENGLONG , 2011-Dec-18.*/
#define NO_SYNC_MTREE	0
/* Ended by LI CHENGLONG , 2011-Dec-18.*/

/*
 * brief	define the interval of sending ssdp, 1200 means 1200/2 = 600 seconds
 */
#define UPNP_SSDP_SEND_INTERVAL 1200

/* 
 * brief	the & in html style, Windows media server need convert it in Server name.
 */
#define HTML_SYMBOL_ADDRESS	"&amp;"
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/
/* Added by LI CHENGLONG , 2011-Nov-24.*/
extern ushare_t *ut;
extern TimerThread gTimerThread;
extern int uppn_setMPoolAttr(ThreadPoolAttr *newAttr);/*in upnp lib*/
extern void sync_metadata_list(struct ushare_t * ut);

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
/* 
 *--------------------------------------------------------------------------------------------------
 *以下是处理CMM的请求的函数声明
 *--------------------------------------------------------------------------------------------------	  
 */

/* 
 * fn		static BOOL usHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF) 
 * brief	根据初始化配置消息设置Ushare进程中的全局配置.	
 *
 * param[in]	pIF		指向DMS_INIT_INFO_MSG 类型消息的指针
 *
 * return		BOOL	布尔值
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF);

/* 
 * fn		static BOOL usHandleInitFolderReq(const DMS_FOLDER_INFO *pFI) 
 * brief	处理初始化时传递过来的共享目录消息.	
 * details	因为buf的大小原因,目录的信息需要分开发送.	
 *
 * param[in]	pFI		共享文件夹的信息.
 *
 * return		BOOL	布尔值
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
static BOOL usHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI);

/* 
 * fn		static BOOL usHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	添加一个共享目录.	
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder);

/* 
 * fn		static BOOL usHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	删除一个共享目录.		
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder);

/* 
 * fn		static BOOL usHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
 * brief	添加或删除共享目录时的处理函数.	
 * details	包括添加 和 删除 目录.	
 *
 * param[in]	pFolder		指向目录操作的指针.
 *
 * return		BOOL		布尔值
 * retval		TRUE		处理添加或删除目录消息成功
 *				FALSE		处理添加或删除目录消息失败
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
static BOOL usHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder);

/* 
 * fn		static BOOL usHandleReloadReq(const DMS_RELOAD_MSG *pReload) 
 * brief	当用户配置改变后发送该消息到ushare进程,这里是最终的处理函数.	
 *
 * param[in]	pReload			指向reload 消息结构.
 *
 * return		BOOL			布尔值
 * retval		TRUE			处理reload 消息成功
 *				FALSE			处理reload消息失败
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
static BOOL usHandleReloadReq(const DMS_RELOAD_MSG *pReload);

/* 
 * fn		static BOOL usHandleManualScanReq() 
 * brief	设手动扫描共享目录.	
 *
 * return	BOOL		布尔值
 * retval	TRUE		设置成功
 *			FALSE		设置失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleManualScanReq();

/* 
 * fn		static BOOL usHandleDeviceStateChange()
 * brief	when usb device's state changed,rebuild all the content list and metadata list
 *
 * param[in]	
 * param[out]	
 *
 * return	 BOOL
 * retval	TRUE :rebuild successful
 		FALSE:rebuild failed
 *
 * note		
 */
static BOOL usHandleDeviceStateChange();

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是一些用到的static函数声明
 *--------------------------------------------------------------------------------------------------
 */

/* 
 * fn		static BOOL ushareAutoScanWorker()
 * brief	ushare 进程中的自动扫描job	
 *
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  25Nov11, LI CHENGLONG	
 */
static BOOL ushareAutoScanWorker();
/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
/*Ushare 接收消息相关的结构, Added by LI CHENGLONG , 2011-Nov-24.*/
CMSG_FD g_usFd;
CMSG_BUFF g_usBuff;
/* Ended by LI CHENGLONG , 2011-Nov-24.*/

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是处理CMM的请求的函数实现
 *--------------------------------------------------------------------------------------------------
 */
 
/* 
 * fn		static BOOL usHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF) 
 * brief	根据初始化配置消息设置Ushare进程中的全局配置.	
 *
 * param[in]	pIF		指向USHARE_INIT_INFO_MSG 类型消息的指针
 *
 * return		BOOL	布尔值
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF)
{
	assert(pIF != NULL);

	if (pIF == NULL)
	{
		return FALSE;
	}

	/*just copy it, Added by LI CHENGLONG , 2011-Nov-24.*/
	memcpy(&(ut->initInfo), pIF, sizeof(DMS_INIT_INFO_MSG));
	ut->initInfo.serverState = pIF->serverState ? 1 : 0;
	ut->initInfo.scanFlag = pIF->scanFlag ? 1 : 0;
	ut->initInfo.scanInterval = pIF->scanInterval > 0 ? pIF->scanInterval : US_SCAN_DFINTERVAL;
	ut->initInfo.folderCnt = pIF->folderCnt > 0 ? pIF->folderCnt : 0;
	/*added by LY to set shareAll, in 2014.09.05*/
	ut->initInfo.shareAll = pIF->shareAll > 0 ? 1 : 0;
	/*end added by LY*/
	
 	/*timer thread  not executing, no need mutex, Added by LI CHENGLONG , 2011-Nov-25.*/
	ut->scan.scanFlag= ut->initInfo.scanFlag;
	ut->scan.scanInterval = ut->initInfo.scanInterval;
	ut->scan.counter = ut->initInfo.scanInterval;


	if (strlen(ut->initInfo.serverName) == 0)
	{
		snprintf(ut->initInfo.serverName, USHARE_MEDIA_SERVER_SERVERNAME_L, "%s", US_DEF_NAME);
	}
 	/* Ended by LI CHENGLONG , 2011-Nov-25.*/

	USHARE_DEBUG("init ut->initInfo and ut->scan data struct END.\n ");
	
	return TRUE;
}

/* 
 * fn		static BOOL usHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI) 
 * brief	处理初始化时传递过来的共享目录消息.	
 * details	因为buf的大小原因,目录的信息需要分开发送.	
 *
 * param[in]	pFI			共享文件夹的信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
static BOOL usHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI)
{
	USHARE_FOLDER_INFO *pNew = NULL;
	
	assert(pFI != NULL);

	if (pFI == NULL)
	{
		return FALSE;
	}

	if (pFI->op != DMS_INIT_FOLDER)
	{
		log_error("not init folder req.");
		return FALSE;
	}
	
	pNew = (USHARE_FOLDER_INFO *)malloc(sizeof (USHARE_FOLDER_INFO));
	if (pNew == NULL)
	{
		log_error("malloc failed.");
		return FALSE;
	}

	memcpy(pNew, &pFI->folder, sizeof(pFI->folder));
	pNew->list.next = pNew->list.prev = NULL;
	
	list_add(&(pNew->list), &ut->initInfo.folderList);
	
	USHARE_DEBUG("handle init folder request END\n");

	return TRUE;

}
 
/* 
 * fn		static BOOL usHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	添加一个共享目录.	
 * details		
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
{
	BOOL addFlag = TRUE;
	struct list_head *pos = NULL;
	USHARE_FOLDER_INFO *pNew = NULL;
	USHARE_FOLDER_INFO *pEntry = NULL;

	assert(pFolder != NULL);

	if (pFolder == NULL)
	{
		return FALSE;
	}

	/*只有路径不同才可以添加, Added by LI CHENGLONG , 2011-Dec-26.*/
	list_for_each(pos, &ut->initInfo.folderList)
	{
		pEntry = list_entry(pos, USHARE_FOLDER_INFO, list);

		if ((0 == strcmp(pEntry->path, pFolder->folder.path))
			&& (0 == strcmp(pEntry->uuid, pFolder->folder.uuid)))
		{	
			addFlag = FALSE;
			break;
		}
	}

	if (addFlag)
	{
		/*update config first, Added by LI CHENGLONG , 2011-Dec-18.*/
		pNew = (USHARE_FOLDER_INFO *)malloc(sizeof (USHARE_FOLDER_INFO));
		if (pNew == NULL)
		{
			log_error("malloc failed!");
			return FALSE;
		}

		memcpy(pNew, &pFolder->folder, sizeof(pFolder->folder));
		pNew->list.next = pNew->list.prev = NULL;
		list_add(&(pNew->list), &ut->initInfo.folderList);	

		if(!ut->initInfo.serverState)
		{
			return TRUE;
		}
	    ushare_rebuild_all(REBUILD_CHANGE);
	}
	
	return TRUE;
}

/* 
 * fn		static BOOL usHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	删除一个共享目录.		
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
{
	struct list_head *pos = NULL;
	USHARE_FOLDER_INFO *pEntry = NULL;
	
	assert(pFolder != NULL);
	
	if (pFolder == NULL)
	{
		return FALSE;
	}

	/*find it out, where you are? ^_^, update config first, Added by LI CHENGLONG , 2011-Dec-18.*/
	list_for_each(pos, &ut->initInfo.folderList)
	{
		pEntry = list_entry(pos, USHARE_FOLDER_INFO, list);
		
		if ((0 == strcmp(pEntry->path, pFolder->folder.path))
			&& (0 == strcmp(pEntry->uuid, pFolder->folder.uuid)))
		{			
			list_del(pos);
			free(pEntry);
			break;
		}
	}

	if(!ut->initInfo.serverState)
	{
		return TRUE;
	}
	return ushare_rebuild_all(REBUILD_CHANGE);
	
	return TRUE;
}

/* 
 * fn		static BOOL usHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
 * brief	添加或删除共享目录时的处理函数.	
 * details	包括添加 和 删除 目录.	
 *
 * param[in]	pFolder	指向目录操作的指针.
 *
 * return		BOOL	布尔值
 * retval		TRUE	处理添加或删除目录消息成功
 *				FALSE	处理添加或删除目录消息失败
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
static BOOL usHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
{
	assert(pFolder != NULL);

	if (pFolder == NULL)
	{
		return FALSE;
	}


	switch (pFolder->op)
	{

	case DMS_ADD_FOLDER:
		return usHandleAddNewShareFolderReq(pFolder);
		break;
		
	case DMS_DEL_FOLDER:
		return usHandleDelShareFolderReq(pFolder);
		break;
		
	/*do not care about the others now, Added by LI CHENGLONG , 2011-Dec-16.*/
	default:
		USHARE_DEBUG("we don't care it,^_^!");
		break;
	}

	return TRUE;
}

/* 
 * fn		static BOOL usHandleReloadReq(const DMS_RELOAD_MSG *pReload) 
 * brief	当用户配置改变后发送该消息到ushare进程,这里是最终的处理函数.	
 *
 * param[in]	pReload		指向reload 消息结构.
 *
 * return		BOOL		布尔值
 * retval		TRUE		处理reload 消息成功
 *				FALSE		处理reload消息失败
 *
 * note	written by  16Dec11, LI CHENGLONG	
 */
static BOOL usHandleReloadReq(const DMS_RELOAD_MSG *pReload)
{
	assert(pReload != NULL);
	
	if (pReload == NULL)
	{
		return FALSE;
	}

//	sem_wait();
	ut->initInfo.serverState = pReload->serverState ? 1 : 0;
	ut->initInfo.scanFlag = pReload->scanFlag ? 1 : 0;
	ut->initInfo.scanInterval = pReload->scanInterval > 0 ?
								pReload->scanInterval : 
								US_SCAN_DFINTERVAL;
	
	ut->scan.scanFlag = ut->initInfo.scanFlag;
	ut->scan.scanInterval = ut->initInfo.scanInterval;
	ut->scan.counter = ut->scan.scanInterval;

//	sem_post();

	/*modified by LY to restart ushare if shareAll is changed, in 2014.09.10*/
	if (strcmp(pReload->serverName, ut->initInfo.serverName) != 0 || pReload->shareAll != ut->initInfo.shareAll)
	{
		/*如果名称不相同, Added by LI CHENGLONG , 2011-Dec-25.*/
		if (strcmp(pReload->serverName, ut->initInfo.serverName) != 0 && strlen(pReload->serverName) != 0)
		{
			snprintf(ut->initInfo.serverName, 
				 	 USHARE_MEDIA_SERVER_SERVERNAME_L, 
					 "%s", 
				 	 pReload->serverName);

			USHARE_DEBUG("name changed restart it...");
			if(pReload->serverState)
			{
				ushareSwitchOnOff(FALSE);			
				return ushareSwitchOnOff(TRUE);
			
			}
			else
			{
				return ushareSwitchOnOff(FALSE);
			}
			
		}
		else if (pReload->shareAll != ut->initInfo.shareAll)
		{
			ut->initInfo.shareAll = pReload->shareAll;
			USHARE_DEBUG("shareAll changed restart it...");
			if(pReload->serverState)
			{
				ushareSwitchOnOff(FALSE);			
				return ushareSwitchOnOff(TRUE);
			
			}
			else
			{
				return ushareSwitchOnOff(FALSE);
			}
		}
		else
		{
			return ushareSwitchOnOff(pReload->serverState > 0 ? TRUE : FALSE);
		}
	}
	else
	{
		return ushareSwitchOnOff(pReload->serverState > 0 ? TRUE : FALSE);
	}
	
}

/* 
 * fn		static BOOL usHandleManualScanReq() 
 * brief	设手动扫描共享目录.	
 *
 * return	BOOL		布尔值
 * retval	TRUE		设置成功
 *			FALSE		设置失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
static BOOL usHandleManualScanReq()
{
/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-18.
 *		  just rebuild all ,so what about big big disk? just pray ^_^.
 *		  maybe use sqlite... in the future.
 */
 	if (ut->initInfo.serverState)
 	{
		return ushare_rebuild_all(REBUILD_NOT_CHANGE);
 	}
	else
	{
		return TRUE;
	}

}


/* 
 * fn		static BOOL usHandleDeviceStateChange()
 * brief	when usb device's state changed,rebuild all the content list and metadata list
 *
 * param[in]	
 * param[out]	
 *
 * return	 BOOL
 * retval	TRUE :rebuild successful
 		FALSE:rebuild failed
 *
 * note		
 */
static BOOL usHandleDeviceStateChange()
{
	if (ut->initInfo.serverState) 
	{
		return ushare_rebuild_all(REBUILD_CHANGE);
	}
	else
	{
		return TRUE;
	}
}
/* 
 *--------------------------------------------------------------------------------------------------
 *以下是一些用到的static函数实现
 *--------------------------------------------------------------------------------------------------
 */
/* 
 * fn		static BOOL ushareAutoScanWorker()
 * brief	ushare 进程中的自动扫描job	
 *
 *
 * return		BOOL	布尔值
 * retval		TRUE	成功
 *				FALSE	失败
 *
 * note	written by  25Nov11, LI CHENGLONG	
 */
static BOOL ushareAutoScanWorker()
{
	int ret = 0;
	ThreadPoolJob job;

 	/*avoid competition, Added by LI CHENGLONG , 2011-Nov-25.*/
	//sem_wait();
	if (ut->scan.scanFlag == 1)
	{
		if (--ut->scan.counter == 0 && ut->initInfo.serverState)
		{	
			USHARE_DEBUG("ushare auto scan timeout, rescan it.\n");

			ushare_rebuild_all(REBUILD_NOT_CHANGE);
			
			ut->scan.counter = ut->scan.scanInterval;
		}
		
	}
	//sem_post();
 	/* Ended by LI CHENGLONG , 2011-Nov-25.*/

	/*do it again and again are you tired ^_^? Added by LI CHENGLONG , 2011-Dec-26.*/
	TPJobInit( &job, ( start_routine ) ushareAutoScanWorker, NULL);
	TPJobSetFreeFunction( &job, ( free_routine )NULL);
	TPJobSetPriority( &job, MED_PRIORITY );

	ret = TimerThreadSchedule(&gTimerThread,
							  1,
							  REL_SEC,
							  &job,
							  SHORT_TERM,
							  NULL);
	
	if (ret != 0)
	{
		return FALSE;
	}

	return TRUE;
	
}


/* 
 * fn		static int ushareConvertStr(const char *pServerName, char *pRevisedName)
 * brief	convert the '&' in pServerName to '&amp;' and save the result in pRevisedName
 * details	Added by zjj, 20130308, to resolve the windows media player cannot find the device
 *			if there is '&' in server name.
 *
 * param[in]	pServerName		the orignal server name.
 * param[out]	pRevisedName	the revised server name.
 *
 * return	int
 * retval	0--OK; 1--ERROR;
 *
 * note		pRevisedName is at least 5 times long as pServerName.
 */
static int ushareConvertStr(const char *pServerName, char *pRevisedName)
{
	int idx = 0;
	
	if ((NULL == pServerName) || (NULL == pRevisedName))
	{
		printf("%s(%d): invalid parameter.\n", __FUNCTION__, __LINE__);
		return 1;
	}

	while (*pServerName != '\0' && idx < USHARE_MEDIA_SERVER_SERVERNAME_L)
	{
		if ('&' == *pServerName)
		{
			strncpy(pRevisedName, HTML_SYMBOL_ADDRESS, sizeof(HTML_SYMBOL_ADDRESS));
			pRevisedName += 5;
		}
		else
		{
			*pRevisedName = *pServerName;
			pRevisedName++;
		}
		idx++;
		pServerName++;
	}

	return 0;
}

/* 
 * fn		static int ushareGetVolumeInfoFromDev(char *pVolumeDevPath, char *pUuid, 
 *																	char *pType, char *pLabel,
 *																	int lenUuid, int lenType,
 *																	int lenLabel)
 * brief	get some infomation with command blkid from volume device.
 * details	
 *
 * param[in]	pVolumeDevPath	the path of the volume device.
 * param[out]	pUuid			the uuid get from the volume path.
 * param[out]	pType			the type get from the volume path, like ntfs, vfat, etc.
 * param[out]	pLabel			the label of the volume, not use by now, some device may not set it.
 * param[in]	lenUuid			the length of the pUuid buffer.
 * param[in]	lenType			the length of the pType buffer.
 * param[in]	lenLabel		the length of the pLabel buffer.
 *
 * return	int
 * retval	0 -- OK; -1 -- ERROR;
 *
 * note		lenUuid at least 37, lenType at least 16, lenLabel at least 64.
 */
static int ushareGetVolumeInfoFromDev(char *pVolumeDevPath, char *pUuid, 
														char *pType, char *pLabel,
														int lenUuid, int lenType,
														int lenLabel)
{
	FILE *pFileResult = NULL;
	int lockFd = -1;
	char *pVal = NULL;
	char lineBuf[BUFLEN_256] = {0};
	char uuidBuf[BUFLEN_64] = {0};
	char typeBuf[BUFLEN_32] = {0};
	char labelBuf[BUFLEN_64] = {0};
	int len = 0;
	int flag = 0;
	int ret = -1;

	if ((NULL == pVolumeDevPath) || (NULL == pUuid) || (NULL == pType) || (NULL == pLabel))
	{
		USHARE_ERROR("Invalid parameter.\n");
		return ret;
	}

	if (0 != strncmp(pVolumeDevPath, DEV_DIR_PREFIX, strlen(DEV_DIR_PREFIX)))
	{
		USHARE_ERROR("Invalid dev path! Prefix(%s) is required.\n", DEV_DIR_PREFIX);
		return ret;
	}

	if (0 != access(pVolumeDevPath, F_OK))
	{
		USHARE_ERROR("Dev file(%s) does not exist.\n", pVolumeDevPath);
		return ret;
	}

	if (-1 == (lockFd = open(FILE_DM_STORAGE, O_RDONLY)))
	{
		USHARE_ERROR("Failed to open lock file(%s).\n", FILE_DM_STORAGE);
		return ret;
	}
	/* get the file lock first. */
	flock(lockFd, LOCK_EX);
	
	snprintf(lineBuf, sizeof(lineBuf), FORMAT_BLKID_COMMAND, pVolumeDevPath, FILE_BLKID_RESULT);
	system(lineBuf);

	/* make sure the FILE_BLKID_RESULT has been created. */
	if (NULL == (pFileResult = fopen(FILE_BLKID_RESULT, "r")))
	{
		USHARE_ERROR("Failed to open the blkid result file for dev(%s).\n", pVolumeDevPath);
		flock(lockFd, LOCK_UN);
		close(lockFd);
		return ret;
	}

	/* A blkid result may like this:
	 * /var/dev/sda1: UUID="125C8C215C8C01A9" TYPE="ntfs"
	 * /var/dev/sdb2: LABEL="D??ó?í" UUID="84F7-B944" TYPE="vfat"
	 */
	memset(lineBuf, 0, sizeof(lineBuf));
	while (NULL != fgets(lineBuf, sizeof(lineBuf), pFileResult))
	{
		USHARE_DEBUG("lineBuf(%s).\n", lineBuf);
		if (0 == strncmp(lineBuf, pVolumeDevPath, strlen(pVolumeDevPath)))
		{
			flag = 1;
			break;
		}
		memset(lineBuf, 0, sizeof(lineBuf));
	}
	fclose(pFileResult);
	unlink(FILE_BLKID_RESULT);
	/* release the file lock. */
	flock(lockFd, LOCK_UN);
	close(lockFd);
	

	if (!flag)
	{
		USHARE_ERROR("Can not get block info for device(%s).\n", pVolumeDevPath);
		return ret;
	}

	if ('\n' == lineBuf[strlen(lineBuf) - 1])
	{
		lineBuf[strlen(lineBuf) - 1] = '\0';
	}

	pVal = NULL;
	/* hfs device may not set UUID. */
	if (NULL != (pVal = strstr(lineBuf, "UUID=")))
	{
		sscanf(pVal, "UUID=%s", uuidBuf);
		len = strlen(uuidBuf) - 2;
		strncpy(pUuid, &uuidBuf[1], len < lenUuid ? len : lenUuid);
		pUuid[lenUuid - 1] = '\0';
	}
	
	pVal = NULL;
	/* some volume may not set type. */
	if (NULL != (pVal = strstr(lineBuf, "TYPE=")))
	{
		sscanf(pVal, "TYPE=%s", typeBuf);
		len = strlen(typeBuf) - 2;
		strncpy(pType, &typeBuf[1], len < lenType? len : lenType);
		pType[lenType- 1] = '\0';
	}

	pVal = NULL;
	/* some volume may not set the LABEL. */
	if (NULL != (pVal = strstr(lineBuf, "LABEL=")))
	{
		sscanf(pVal, "LABEL=%s", labelBuf);
		len = strlen(labelBuf) - 2;
		strncpy(pLabel, &labelBuf[1], len < lenLabel? len : lenLabel);
		pLabel[lenLabel- 1] = '\0';
	}
	
	USHARE_DEBUG("Dev(%s), uuidBuf(%s), typeBuf(%s), labelBuf(%s).\n", pVolumeDevPath, uuidBuf, typeBuf, labelBuf);

	return 0;
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
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
BOOL ushareProcessInitInfo()
{
	int sRet = 0;
	fd_set usFds;
	BOOL iStep1_Done = FALSE;
	BOOL iStep2_Done = FALSE;
	unsigned int folderCnt = 0;


	while (!((iStep1_Done == TRUE) && (iStep2_Done == TRUE)))
	{	
		/*listen 消息 Added by LI CHENGLONG , 2011-Nov-02.*/
		memset(&g_usBuff, 0 , sizeof(CMSG_BUFF));

		FD_ZERO(&usFds);
   		FD_SET(g_usFd.fd, &usFds);

		USHARE_DEBUG("waiting for message...\n");
	    sRet = select(g_usFd.fd + 1, &usFds, NULL, NULL, NULL); 
		if (sRet == 0)
		{	
			/*timeout. Added by LI CHENGLONG , 2011-Nov-24.*/
			continue;
		}
		else if (sRet < 0)
		{
			if (sRet != EINTR && sRet != EAGAIN)
			{
				/*error and neither EINTR nor EAGAIN Added by LI CHENGLONG , 2011-Nov-24.*/
				return FALSE;
			}

			continue;
		}
		else if (FD_ISSET(g_usFd.fd, &usFds))
		{
			msg_recv(&g_usFd, &g_usBuff);

			switch (g_usBuff.type)
			{
			case CMSG_DLNA_MEDIA_SERVER_INIT:
				USHARE_DEBUG("received CMSG_DLNA_MEDIA_SERVER_INIT message.\n");
				if ((iStep1_Done == TRUE) || (iStep2_Done == TRUE))
				{
					/*order error in recving msg. Added by LI CHENGLONG , 2011-Dec-16.*/	
					break;
				}
				if (TRUE != usHandleInitInfoReq((DMS_INIT_INFO_MSG *)(g_usBuff.content)))
				{
					/*handle init info error, Added by LI CHENGLONG , 2011-Dec-16.*/
					return FALSE;
				}
				
				iStep1_Done = TRUE;
				if (ut->initInfo.folderCnt == 0)
				{
					iStep2_Done = TRUE;
				}

				break;

			case CMSG_DLNA_MEDIA_SERVER_OP_FOLDER:
				USHARE_DEBUG("received CMSG_DLNA_MEDIA_SERVER_OP_FOLDER message.\n");
				if (iStep1_Done != TRUE)
				{
					/*order error in recving msg. Added by LI CHENGLONG , 2011-Dec-16.*/
					return FALSE;
				}
				
				if (folderCnt >= ut->initInfo.folderCnt)
				{
					return FALSE;	
				}
				
				if (TRUE != usHandleInitFolderReq((DMS_OP_FOLDER_MSG *)(g_usBuff.content)))
				{
					return FALSE;
				}
				
				folderCnt++;
				
				if (folderCnt == ut->initInfo.folderCnt)
				{
					iStep2_Done = TRUE;
				}
				break;

			/*unknown msg, not supported yet Added by LI CHENGLONG , 2011-Nov-24.*/		
			default:
				break;
			}

		}

	}

	if ((iStep1_Done == TRUE) && (iStep2_Done == TRUE))
	{
		USHARE_DEBUG("handle initInfo end!\n");
		return TRUE;
	}

	/*should not be here, Added by LI CHENGLONG , 2011-Nov-24.*/
	return FALSE;
}

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
BOOL ushareProcessUserRequest()
{
	int sRet = 0;
	fd_set usFds;
	struct timeval selectTv;
	int devChangeFlag = 0;

	while(TRUE)
	{	
		/*listen 消息 Added by LI CHENGLONG , 2011-Nov-02.*/
		memset(&g_usBuff, 0 , sizeof(CMSG_BUFF));

		FD_ZERO(&usFds);
   		FD_SET(g_usFd.fd, &usFds);
		
		selectTv.tv_sec = 10;
		selectTv.tv_usec = 0;
		
		//USHARE_DEBUG("waiting for user request messages...\n");
	    sRet = select(g_usFd.fd + 1, &usFds, NULL, NULL, &selectTv); 
		if (sRet == 0)
		{	
			/*timeout. Added by LI CHENGLONG , 2011-Nov-24.*/
			//USHARE_DEBUG("reach timeout, continue...\n");

			if (devChangeFlag)
			{
				if(TRUE !=usHandleDeviceStateChange())	
				{
					log_error("handle device umount message error\n");
				}
				devChangeFlag = 0;
			}
			continue;
		}
		else if (sRet < 0)
		{
			if ((sRet != EINTR) && (sRet != EAGAIN))
			{
				/*error and not eintr,what happened! Added by LI CHENGLONG , 2011-Nov-24.*/
				return FALSE;
			}

			continue;
		}
		else if (FD_ISSET(g_usFd.fd, &usFds))
		{
			msg_recv(&g_usFd, &g_usBuff);

			switch (g_usBuff.type)
			{
			case CMSG_DLNA_MEDIA_SERVER_RELOAD:
				USHARE_DEBUG("received CMSG_DLNA_MEDIA_SERVER_RELOAD message.\n");
				if (TRUE != usHandleReloadReq((DMS_RELOAD_MSG *)(g_usBuff.content)))
				{
					/*need exit? just ignore it  Added by LI CHENGLONG , 2011-Dec-22.*/
					USHARE_DEBUG("handle reload message error\n");
				}
				devChangeFlag = 0;
				break;

			case CMSG_DLNA_MEDIA_SERVER_OP_FOLDER:
				USHARE_DEBUG("received CMSG_DLNA_MEDIA_SERVER_OP_FOLDER message.\n");
				if (TRUE != usHandleOperateFolderReq((DMS_OP_FOLDER_MSG *)(g_usBuff.content)))
				{
					/*need exit? just ignore it  Added by LI CHENGLONG , 2011-Dec-22.*/
					log_error("handle operatefolderreq message error\n");
				}
				devChangeFlag = 0;
				break;
				
			case CMSG_DLNA_MEDIA_SERVER_MANUAL_SCAN:
				USHARE_DEBUG("received CMSG_DLNA_MEDIA_SERVER_MANUAL_SCAN message.\n");
				if (TRUE != usHandleManualScanReq())
				{
					/*need exit? just ignore it Added by LI CHENGLONG , 2011-Dec-22.*/
					log_error("handle manual scan message error\n");
				}
				devChangeFlag = 0;
				break;
				
			case CMSG_DLNA_MEDIA_DEVICE_STATE_CHANGE:
				USHARE_DEBUG("received CMSG_DLNA_MEDIA_DEVICE_STATE_CHANGE message.\n");
				devChangeFlag = 1;
				break;
				
			/*unknown msg, not supported yet Added by LI CHENGLONG , 2011-Nov-24.*/
			default :
				USHARE_DEBUG("^_^, not supported yet now!\n");
				break;
				
			}
			
		}
	}

	/*ooh shit, Judgment Day ... should not be here, Added by LI CHENGLONG , 2011-Nov-24.*/
	return FALSE;		
}

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是与目录处理相关的一些函数实现.
 *--------------------------------------------------------------------------------------------------
 */
/* 
 * fn		BOOL ushare_rebuild_all() 
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
BOOL ushare_rebuild_all(int flag)
{
#if 0 /* for debug */
	int i = 0;
#endif /* for debug */

	free_metadata_list(ut, flag);
	/*free content_list, Added by LI CHENGLONG , 2011-Dec-18.*/
	content_free(ut->contentlist);
	ut->contentlist = NULL;

	USHARE_DEBUG("all free ok\n");
	build_content_list(ut);
	build_metadata_list(ut, flag);

#if 0 /*for debug*/
	USHARE_DEBUG("after rebuild all, content list count=%d.", ut->contentlist->count);
	for (i=0 ; i < ut->contentlist->count ; i++)
	{
		USHARE_DEBUG("ut->contentlist->content[i]=%s,", ut->contentlist->content[i]);
		USHARE_DEBUG("ut->contentlist->dispName[i]=%s\n", ut->contentlist->displayName[i]);
	}
#endif /* for debug */
	return TRUE;
}


/* 
 * fn		BOOL ushareRebuildMetaList()
 * brief	重新扫描共享目录, 重新建立多媒体信息树.	
 *
 * return		BOOL	布尔值
 * retval		TRUE	成功
 *				FALSE	失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareRebuildMetaList()
{
	if (ut->contentlist)
	{
		free_metadata_list (ut, REBUILD_CHANGE);
		build_metadata_list (ut, REBUILD_CHANGE);
	}
	
	return TRUE;
}

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
BOOL build_content_list(ushare_t *ut)
{
	struct list_head *pos = NULL;
	USHARE_FOLDER_INFO *pEntry = NULL;
	
	assert(ut != NULL);

	if (ut == NULL)
	{
		return FALSE;
	}
	
	USHARE_DEBUG("build_content_list BEGIN...\n");

	/*modified by LY to add root directory to contentlist if shareAll is set*/
	if (ut->initInfo.shareAll)
	{
		USHARE_DEBUG("share all the volumes!\n");
		ut->contentlist = ushare_content_add(ut->contentlist, 
							ut->initInfo.shareAll,
							ROOT_DIRECTORY, 
							ROOT_DISPLAYNAME, 
							ROOT_UUID,
							NO_SYNC_MTREE);
	}
	else
	{
		USHARE_DEBUG("share the folderList\n");
		list_for_each(pos, &ut->initInfo.folderList)
		{
			pEntry = list_entry(pos, USHARE_FOLDER_INFO, list);

			/*added by LY to only add the item whose enable is set to be true, in 20141203*/
			if (!pEntry->enable)
			{
				continue;
			}
			
			//USHARE_DEBUG("add pEntry->path=%s, pEntry->dispName=%s\n", pEntry->path, pEntry->dispName);
			ut->contentlist = ushare_content_add(ut->contentlist,
								ut->initInfo.shareAll,
								pEntry->path, 
								pEntry->dispName, 
								pEntry->uuid,
								NO_SYNC_MTREE);
		/*now ut->contentlist maybe null,do not use ut->contentlist->count,
	  	Added by LI CHENGLONG , 2012-Jan-05.*/
		
		}
	}
	USHARE_DEBUG("build_content_list END...\n");
	/*end modified by LY*/

  	if (!ut->contentlist)
	{	
		USHARE_DEBUG("ut->conententlist=null\n");
		ut->contentlist = (content_list*) malloc (sizeof(content_list));
		ut->contentlist->content = NULL;
		ut->contentlist->displayName = NULL;
		ut->contentlist->count = 0;
	}

	return TRUE;
}

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
content_list *content_priv_del(content_list *list, const char *item)
/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-18.
 *		  1.find the entry.
 *		  2.call content_del in content.c
 */
{
	int cursor = 0;
	
	assert((item != NULL));

	if ((item == NULL))
	{
		return list;
	}

	USHARE_DEBUG("delte content %s in contentlist begin...\n", item);
	while (cursor < list->count)
	{
		/*比较路径名相同, Added by LI CHENGLONG , 2011-Dec-18.*/
		if (strcmp(item, list->content[cursor]) == 0)
		{
			USHARE_DEBUG("delte content %s in contentlist...\n", item);
			return content_del(list, cursor);			
		}
		cursor++;
	}

	return list;
}

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
 */
content_list *ushare_content_add(content_list *list, int shareAll, const char *item, 
										const char *name, const char *pFolderUuid, char flag)
{
	int suffix = 0;
	int cursor = 0;
	struct stat st;
	int dirCount = 0;
	char *fullPath = NULL;
	char newName[BUFLEN_32] = {0};
	struct dirent	**nameList = NULL;
	char uuid[BUFLEN_64] = {0};
	char type[BUFLEN_16] = {0};
	char label[BUFLEN_64] = {0};
	char devPath[BUFLEN_32] = {0};
	
	/*list may be NULL so we don't care. Added by LI CHENGLONG , 2011-Dec-25.*/
	assert((item != NULL) && (name != NULL) && (pFolderUuid != NULL) && (NO_SYNC_MTREE == flag));

	if ((item == NULL) || (name == NULL))
	{
		return list;
	}

	if (list == NULL)
	{
		USHARE_DEBUG("add to a new created list\n");
	}
	
	/* 
	 * brief: Added by LI CHENGLONG, 2011-Dec-18.
	 *		  item 和 name的处理.
	 */
	dirCount = scandir(USB_DISK_DIRECTORY, &nameList, 0, alphasort);
	if (dirCount <= 0)
	{
		return list;
	}
	
	USHARE_DEBUG("after scandir %s, dirCount=%d\n", USB_DISK_DIRECTORY, dirCount);
	
	for (cursor = 0, suffix = 0; cursor < dirCount; cursor++)
	{
		if (nameList[cursor]->d_name[0] == '.')
		{
			free(nameList[cursor]);
			continue;
		}
		
		/*忽略掉语音信箱创建的链接. Added by LI CHENGLONG , 2012-Jan-05.*/
		if (strcmp(nameList[cursor]->d_name, USBVM) == 0)
		{
			free(nameList[cursor]);
			continue;
		}
		
		fullPath = (char *)	malloc(strlen(USB_DISK_DIRECTORY) + 
								   strlen(nameList[cursor]->d_name) +
								   strlen(item) + 2);

		if (fullPath == NULL)
		{
			log_error("malloc error");
			/* Add by chz to avoid memory leaks, 2012-10-30 */
			while (cursor < dirCount)
			{
				free(nameList[cursor++]);
			}
			free(nameList);
			/* end add */
			return list;
		}
		
		sprintf (fullPath, "%s/%s%s", USB_DISK_DIRECTORY, nameList[cursor]->d_name, item);
		
		if (stat (fullPath, &st) < 0)
		{
			free(nameList[cursor]);
			free(fullPath);
			continue;
		}

		/*when shareAll is set to be true, which means that we should scan all the volumes, 
		 *skip the step of checking the uuid,
		 *when shareAll is set to be false, which means that we should only scan the directories set by 
		 *user from UI, do the uuid checking, and only scan the volume that match the uuid of the directory.
		*/
		if (!shareAll)
		{
			memset(devPath, 0, sizeof(devPath));
			memset(uuid, 0, sizeof(uuid));
			memset(type, 0, sizeof(type));
			memset(label, 0, sizeof(label));
			snprintf(devPath, sizeof(devPath), "%s%s", DEV_DIR_PREFIX, nameList[cursor]->d_name);
			if (0 == ushareGetVolumeInfoFromDev(devPath, uuid, type, label, 
											sizeof(uuid), sizeof(type), sizeof(label)))
			{
				/* Share all the matched folder if uuid is empty, otherwise only the one with the same uuid. */
				if ((0 != strlen(pFolderUuid)) && (0 != strcmp(pFolderUuid, uuid)))
				{
					free(nameList[cursor]);
					free(fullPath);
					continue;
				}
			}
		}
		
		/*不处理链接, Added by LI CHENGLONG , 2012-Jan-05.*/
		if (S_ISDIR (st.st_mode) && !(S_ISLNK(st.st_mode)))
		{
			
			if (suffix == 0)
			{
				list = content_add(list, fullPath, name);			
			}
			else
			{
				/*如果存在多个同样的路径则 加字母后缀来区分, Added by LI CHENGLONG , 2011-Dec-18.*/
				snprintf(newName, BUFLEN_32, "%s_%c", name, 'A' + suffix - 1);
				list = content_add(list, fullPath, newName);
			}
			
			suffix++;
		}
		
	    free (nameList[cursor]);
    	free (fullPath);
	}

	/* Add by chz to avoid memory leaks, 2012-10-25 */
	free(nameList);
	/* end add */

	return list;
}

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
content_list *ushare_content_del(content_list *list, const char *item, const char *name, char flag)
/* 
 * brief: Added by LI CHENGLONG, 2011-Dec-18.
 *		  1.scan /var/usbdisk
 		  2.call content_priv_del
 		  3.sync? 
 */
{
	int cursor = 0;
	int dirCount = 0;
	char *fullPath = NULL;
	struct dirent	**nameList = NULL;

	assert((item != NULL) && (name != NULL) && (NO_SYNC_MTREE == flag));

	if (list == NULL)
	{
		return list;
	}

	if ((item == NULL) || (name == NULL))
	{
		return list;
	}

	dirCount = scandir(USB_DISK_DIRECTORY, &nameList, 0, alphasort); 
	if (dirCount <= 0)
	{
		return list;
	}

	USHARE_DEBUG("after scandir dirCount=%d\n", dirCount);

	for (cursor = 0; cursor < dirCount; cursor++)
	{
		if (nameList[cursor]->d_name[0] == '.')
		{
			free(nameList[cursor]);
			continue;
		}

		fullPath = (char *)	malloc(strlen(USB_DISK_DIRECTORY) + 
								   strlen(nameList[cursor]->d_name) +
								   strlen(item) + 2);

		if (fullPath == NULL)
		{
			log_error("malloc error");
			return list;
		}

		sprintf (fullPath, "%s/%s%s", USB_DISK_DIRECTORY, nameList[cursor]->d_name, item);		
		
		list = content_priv_del(list, fullPath);

	    free (nameList[cursor]);
    	free (fullPath);
	}

	/* Add by chz to avoid memory leaks, 2012-10-25 */
	free(nameList);
	/* end add */

	return list;
}

/* 
 * fn		BOOL ushareStartAutoScanWorker()
 * brief	Ushare 进程启动时开启该woker. 用于定时.	
 *
 * return	BOOL		布尔值
 * retval	FALSE		失败
 *			TRUE		成功
 *
 * note	written by  25Nov11, LI CHENGLONG	
 */
BOOL ushareStartAutoScanWorker()
{
	int ret = 0;
	ThreadPoolJob job;
	
	TPJobInit( &job, ( start_routine ) ushareAutoScanWorker, NULL);
	TPJobSetFreeFunction( &job, ( free_routine )NULL);
	TPJobSetPriority( &job, MED_PRIORITY );
		
	ret = TimerThreadSchedule(&gTimerThread,
							  1,
							  REL_SEC,
							  &job,
							  SHORT_TERM,
							  NULL);
	
	if (ret != 0)
	{
		return FALSE;
	}

	return TRUE;
}

/* 
 *--------------------------------------------------------------------------------------------------
 *以下是与开启关闭相关的一些函数实现.
 *--------------------------------------------------------------------------------------------------
 */

/* 
 * fn	 	BOOL ushareSwitchOn() 
 * brief	开启media server功能.
 *
 * return	BOOL		布尔值
 * retval	TRUE		开启成功	
 *			FALSE		开启失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareSwitchOn()
{
	int len = 0;
	int res = 0;
	char modelDesc[100];
	char presentUrl[64];
	char ipa[16] = {0};
	char serverName[USHARE_MEDIA_SERVER_SERVERNAME_L * 5] = {0};

	if (ut->ip)
	{
		/*store old ip, Added by LI CHENGLONG , 2012-May-25.*/
		snprintf(ipa, sizeof(ipa), "%s", ut->ip);

		free(ut->ip);
		ut->ip = NULL;
		ut->ip = get_iface_address (ut->interface); 
	}
	else
	{
		memset(ipa, 0, sizeof(ipa));
		ut->ip = get_iface_address (ut->interface);
	}

	/*new interface not up, Added by LI CHENGLONG , 2012-May-25.*/
	if (!ut->ip)
	{
		ushare_free (ut);
		return FALSE;
	}
	
	/*ip更改,重新初始化upnp库, Added by LI CHENGLONG , 2012-May-25.*/
	if(strncmp(ipa, ut->ip, 16))
	{
		UpnpUnRegisterRootDevice (ut->dev);
		UpnpFinish ();
		ushareInitUPnPEnv();
	}


	snprintf(modelDesc, sizeof(modelDesc), 
			 "%s %s: DLNA Media Server", 
			 ut->initInfo.manuInfo.modelName, 
			 ut->initInfo.manuInfo.devModelVersion);

	snprintf(presentUrl, sizeof(presentUrl), "http://%s", ut->ip);
	
	/* Added by zjj, 20130308, to solve the WMP cannot find the device if the severName 
		contains '&'. */
	if (ushareConvertStr(ut->initInfo.serverName, serverName))
	{
		USHARE_DEBUG("convert string failed.\n");
		return FALSE;
	}
#if 0
	/* 
	 * brief	check if server name ends with ':1', if not true, add ':1' to the tail. 
	 *			Note sizeof(serverName)= 15*5 + 1 + 4, 15 for name, 1 for '\0', 4 is enough for ":1" 
	 *			Add by zengdongbiao, 29May15. 
	 */
	if (((':' != serverName[strlen(serverName) - 2]) || ('1' != serverName[strlen(serverName) - 1]))
		&& ((strlen(serverName) + strlen(USHARE_SERVERNAME_SUFFIX)) <= sizeof(serverName) - 1))
	{
		/*add ":1" to the end of the dlna server name to make our dut compatible to xbox */
		strncat(serverName, USHARE_SERVERNAME_SUFFIX, strlen(USHARE_SERVERNAME_SUFFIX));
	}
	/*end add by zengdongbiao*/
#endif
	
#ifdef HAVE_DLNA
	  if (ut->dlna_enabled)
	  {
		len = 0;
		ut->description =
		  dlna_dms_description_get (serverName,
									ut->initInfo.manuInfo.manufacturer,
									ut->initInfo.manuInfo.devManufacturerURL,
									modelDesc,
									ut->model_name,
									"1.0",
									ut->initInfo.manuInfo.devManufacturerURL,
									"USHARE-01",
									ut->udn,
									presentUrl,
									"/web/cms.xml",
									"/web/cms_control",
									"/web/cms_event",
									"/web/cds.xml",
									"/web/cds_control",
									"/web/cds_event");
		if (!ut->description)
		  return FALSE;
	  }
	  else
	  {
#endif /* HAVE_DLNA */ 
	  len = strlen (UPNP_DESCRIPTION) + strlen (serverName)
		+ strlen (ut->model_name) + strlen (ut->udn) + 1;
	  ut->description = (char *) malloc (len * sizeof (char));
	  memset (ut->description, 0, len);
	  sprintf (ut->description, UPNP_DESCRIPTION, serverName, ut->model_name, ut->udn);
#ifdef HAVE_DLNA
	  }
#endif /* HAVE_DLNA */

  res = UpnpRegisterRootDevice2 (UPNPREG_BUF_DESC, ut->description, 0, 1,
                                 device_callback_event_handler,
                                 NULL, &(ut->dev));
  if (res != UPNP_E_SUCCESS)
  {
    USHARE_DEBUG("Cannot register UPnP device\n");
    free (ut->description);
    return FALSE;
  }

  res = UpnpUnRegisterRootDevice (ut->dev);
  if (res != UPNP_E_SUCCESS)
  {
    log_error ("Cannot unregister UPnP device\n");
    free (ut->description);
    return FALSE;
  }

  res = UpnpRegisterRootDevice2 (UPNPREG_BUF_DESC, ut->description, 0, 1,
                                 device_callback_event_handler,
                                 NULL, &(ut->dev));
  if (res != UPNP_E_SUCCESS)
  {
    log_error ("Cannot register UPnP device\n");
    free (ut->description);
    return FALSE;
  }

  USHARE_DEBUG ("Sending UPnP advertisement for device ...\n");
  UpnpSendAdvertisement (ut->dev, UPNP_SSDP_SEND_INTERVAL);

  USHARE_DEBUG ("Listening for control point connections ...\n");

  if (ut->description)
    free (ut->description);

  return TRUE;	
}

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
BOOL ushareSwitchOff()
{	
	if (UPNP_E_SUCCESS != UpnpUnRegisterRootDevice (ut->dev))
	{
		USHARE_DEBUG("ushare:debug:switch off new folder %d, %s .\n", __LINE__, __FUNCTION__);	
		return FALSE;
	}
	USHARE_DEBUG("ushare switch off.......\n");
	
	free_metadata_list(ut, REBUILD_CHANGE);
	content_free(ut->contentlist);
	ut->contentlist = NULL;
	
	return TRUE;
}

/* 
 * fn		BOOL ushareSwitchOnOff(BOOL onOff)
 * brief	开启或关闭MediaServer功能.	
 *
 * param[in]	onOff		开启或关闭MediaServer功能.
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  24Nov11, LI CHENGLONG	
 */
BOOL ushareSwitchOnOff(BOOL onOff)
{
	if (onOff == TRUE)
	{
/*解决网页开启，但不扫描的问题, added by Zhu Haiming, 16Jul13.*/	
		BOOL ret = ushareSwitchOn();
  		ushare_rebuild_all(REBUILD_CHANGE);
		return ret;
/*end*/
	}
	else if (onOff == FALSE)
	{
		return ushareSwitchOff();
	}

	return FALSE;
}

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
BOOL ushareParseConfig(int argc, char **argv)
{
	setup_i18n ();
	setup_iconv ();
	/* Parse args before cfg file, as we may override the default file */
	if (parse_command_line (ut, argc, argv) < 0)
	{
	  ushare_free (ut);
	  return FALSE;
	}
	if (parse_config_file (ut) < 0)
	{
	  /* fprintf here, because syslog not yet ready */
	  fprintf (stderr, "Warning: can't parse file \"%s\".\n",
			   ut->cfg_file ? ut->cfg_file : SYSCONFDIR "/" USHARE_CONFIG_FILE);
	}
	
	if (ut->contentlist)
	{
		free(ut->contentlist);
		ut->contentlist = NULL;
	}
	
	/* Delete by chz, build the list in ushareSwitchOn(), not here. 2013-01-04 */
	/*
	build_content_list(ut);
	*/
	/* end delete */

	if (!ut->contentlist)
	{	
		USHARE_DEBUG("ut->conententlist=null\n");
		ut->contentlist = (content_list*) malloc (sizeof(content_list));
		ut->contentlist->content = NULL;
		ut->contentlist->displayName = NULL;
		ut->contentlist->count = 0;
	}	

	if (ut->xbox360)
	{
		char *name;

		name = malloc (strlen (XBOX_MODEL_NAME) + strlen (ut->model_name) + 4);
		sprintf (name, "%s (%s)", XBOX_MODEL_NAME, ut->model_name);
		free (ut->model_name);
		ut->model_name = strdup (name);
		free (name);

		ut->starting_id = STARTING_ENTRY_ID_XBOX360;
	}
	if (ut->daemon)
	{
		/* starting syslog feature as soon as possible */
		start_log ();
	}	

	if (!has_iface (ut->interface))
	{
		ushare_free (ut);
		return FALSE;
	}

	ut->udn = create_udn (ut->interface);
	if (!ut->udn)
	{
		ushare_free (ut);
		return FALSE;
	}

	ut->ip = get_iface_address (ut->interface);	
	if (!ut->ip)
	{
		ushare_free (ut);
		return EXIT_FAILURE;
	}

	if (ut->daemon)
	{
		int err;
		err = daemon (0, 0);
		if (err == -1)
		{
			log_error ("Error: failed to daemonize program : %s\n", strerror (err));
			ushare_free (ut);
			return FALSE;
		}
	}
	else
	{
		display_headers ();
	}	

	signal (SIGINT, UPnPBreak);
	signal (SIGHUP, reload_config);	
	
#ifdef INCLUDE_TELNET
	  if (ut->use_telnet)
	  {
		if (ctrl_telnet_start (ut->telnet_port) < 0)
		{
		  ushare_free (ut);
		  return FALSE;
		}
		
		ctrl_telnet_register ("kill", ushare_kill, "Terminates the uShare server");
	  }
#endif /*INCLUDE_TELNET*/

	return TRUE;
}

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
BOOL ushareInitUPnPEnv()
{
	int res;
	ThreadPoolAttr newAttr;
	
	if (!(ut && ut->udn && ut->ip))
	{
		if (ut->udn)
		{
			USHARE_DEBUG("ut->udn=%s\n", ut->udn);
		}
		if (ut->ip)
		{
			USHARE_DEBUG("ut->ip=%s", ut->ip);
		}
		return FALSE;
	}

	/*set threadpool attr here!*/
	newAttr.minThreads = 2;
	#if defined(INCLUDE_VOIP)&&defined(SUPPORT_SDRAM_32M)
	newAttr.maxThreads = 3;
	#else
	newAttr.maxThreads = 5;
	#endif
	newAttr.maxIdleTime = 5000;
	newAttr.maxJobsTotal = 100;
	newAttr.jobsPerThread = 1;
	newAttr.starvationTime = 1000;
	newAttr.schedPolicy = 0;
	if(uppn_setMPoolAttr(&newAttr) != UPNP_E_SUCCESS)
	{
		log_error("can't set miniserverpool attr in ushare\n");
	}

	USHARE_DEBUG("Initializing UPnP subsystem ...\n");
	res = UpnpInit (ut->ip, ut->port);
	if (res != UPNP_E_SUCCESS)
	{
		log_error ("Cannot initialize UPnP subsystem\n");
		return FALSE;
	}

	if (UpnpSetMaxContentLength (UPNP_MAX_CONTENT_LENGTH) != UPNP_E_SUCCESS)
	{
		USHARE_DEBUG ("Could not set Max content UPnP\n");
	}
	
	if (ut->xbox360)
	{
		USHARE_DEBUG ("Starting in XboX 360 compliant profile ...\n");
	}
	
#ifdef HAVE_DLNA
	if (ut->dlna_enabled)
	{
		USHARE_DEBUG ("Starting in DLNA compliant profile ...\n");
		ut->dlna = dlna_init ();
		dlna_set_verbosity (ut->dlna, ut->verbose ? 1 : 0);
		/*实际上这里不应该设置检查后缀名，否则有可能导致不能正确解析无后缀名或错后缀名的文件. 
		Added by LI CHENGLONG , 2011-Dec-07.*/
		dlna_set_extension_check (ut->dlna, 1);
		/*这里注册所有能够解析的文件类型, Added by LI CHENGLONG , 2011-Dec-07.*/
		dlna_register_all_media_profiles (ut->dlna);
	}
#endif /* HAVE_DLNA */

	ut->port = UpnpGetServerPort();
	USHARE_DEBUG ("UPnP MediaServer listening on %s:%d\n",
	UpnpGetServerIpAddress (), ut->port);

	UpnpEnableWebserver (TRUE);

	res = UpnpSetVirtualDirCallbacks (&virtual_dir_callbacks);
	if (res != UPNP_E_SUCCESS)
	{
		USHARE_DEBUG ("Cannot set virtual directory callbacks\n");
		return FALSE;
	}

	res = UpnpAddVirtualDir (VIRTUAL_DIR);
	if (res != UPNP_E_SUCCESS)
	{
		USHARE_DEBUG ("Cannot add virtual directory for web server\n");
		return FALSE;
	}	

	return TRUE;
}

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
int daemonize(int nochdir, int noclose)
{
	int fd;
	
	switch (fork()) 
	{
	/*fork error*/	
	case -1:		
		return (-1);
	/*child*/	

	case 0:
		break;
		
	/*parents*/	
	default:
		exit(0);
	}
	
	/*run the program in new session*/
	if (setsid() == -1)
	{
		return (-1);
	}

	/*change dir or not*/
	if (!nochdir)
	{
		chdir("/");
	}

	/*append stard description file to /dev/null,except stard out*/
	if (!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1) 
	{
		(void)dup2(fd, 0); //input
		//(void)dup2(fd, 1); //output
		//(void)dup2(fd, 2); //error
		if (fd > 2)
		{
			(void)close (fd);		
		}
	}
	
	return (0);	
}
/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

