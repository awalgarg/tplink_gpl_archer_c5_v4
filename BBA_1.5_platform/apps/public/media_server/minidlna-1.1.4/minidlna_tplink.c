/* 
 *
 * file		minidlna_tplink.c
 * brief	tplink implementation of minidlna controlling
 * details	
 *
 * author	Li weijie 
 * version	1.0.0
 * date		19Jun14
 *
 * history 	\arg  1.0.0, 19Jun14, Liweijie, Create the file.	
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

#include <os_msg.h>

#include "minidlna.h"
#include "minidlna_tplink.h"
#include "upnpglobalvars.h"
#include "scanner.h"

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

/* 
 * fn		static BOOL mdHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF) 
 * brief	根据初始化配置消息设置minidlna进程中的全局配置.	
 *
 * param[in]	pIF		指向DMS_INIT_INFO_MSG 类型消息的指针
 *
 * return		BOOL	布尔值
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  19Jun14, Liweijie	
 */
static BOOL mdHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF);

/* 
 * fn		static BOOL mdHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI) 
 * brief	处理初始化时传递过来的共享目录消息.	
 * details	因为buf的大小原因,目录的信息需要分开发送.	
 *
 * param[in]	pFI			共享文件夹的信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI);

/* 
 * fn		static BOOL mdHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	添加一个共享目录.	
 * details		
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  19Jun14, liweijie
 */
static BOOL mdHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder);

/* 
 * fn		static BOOL mdHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	删除一个共享目录.		
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder);

/* 
 * fn		static BOOL mdHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
 * brief	添加或删除共享目录时的处理函数.	
 * details	包括添加 和 删除 目录.	
 *
 * param[in]	pFolder	指向目录操作的指针.
 *
 * return		BOOL	布尔值
 * retval		TRUE	处理添加或删除目录消息成功
 *				FALSE	处理添加或删除目录消息失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder);

/* 
 * fn		static BOOL mdHandleReloadReq(const DMS_RELOAD_MSG *pReload) 
 * brief	当用户配置改变后发送该消息到minidlna进程,这里是最终的处理函数.	
 *
 * param[in]	pReload		指向reload 消息结构.
 *
 * return		BOOL		布尔值
 * retval		TRUE		处理reload 消息成功
 *				FALSE		处理reload消息失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleReloadReq(const DMS_RELOAD_MSG *pReload);

/* 
 * fn		static BOOL mdHandleManualScanReq()
 * brief	设手动扫描共享目录.	
 *
 * return	BOOL		布尔值
 * retval	TRUE		设置成功
 *			FALSE		设置失败
 *
 * note	written by  19Jun14, liweijie
 */
static BOOL mdHandleManualScanReq();

/* 
 * fn		static BOOL mdHandleDeviceStateChange()
 * brief	when usb device's state changed,rebuild the sqlite database
 *
 * return	 BOOL
 * retval	 TRUE :rebuild successful
 		     FALSE:rebuild failed
 *
 * note	written by 19Jun14, liweijie	
 */
static BOOL mdHandleDeviceStateChange();


/* 
 * fn		BOOL mdConvertMediaDirsToAbsolute()
 * brief	convert the media_dirs_relative to media_dirs,make it can scan work~	
 *
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  22Jun14, liweijie	
 */
static BOOL mdConvertMediaDirsToAbsolute();

/* 
 * fn		void printMediaDirs(struct media_dir_s *print)
 * brief	Debug function for show media dirs 	
 *
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  22Jun14, liweijie	
 */
void printMediaDirs(struct media_dir_s *print);


/* 
 * fn		static int mdGetVolumeInfoFromDev(char *pVolumeDevPath, char *pUuid, 
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
static int mdGetVolumeInfoFromDev(char *pVolumeDevPath, char *pUuid, 
														char *pType, char *pLabel,
														int lenUuid, int lenType,
														int lenLabel);

/* 
* fn	static int modifyStr(char *strName, 
							 int strNameSize, 
							 const char * before, 
							 const char * after)
 * brief	Modify strName, replace "before" with "after".
 * details	用after替代before。
 *
 * param[in]	strName			The string to modify.
 * param[in]	strNameSize		The size of strName.
 * param[in]	before			The string going to be replaced.
 * param[in]	after			The string to replace "before" string.
 * return		
 * retval		0	successful.
 * retval		-1	The size of strName is not enough.
 *
 * note			written by zengdongbiao, 07Apr15.
 */
static int modifyStr(char *strName, 
					 int strNameSize, 
					 const char * before, 
					 const char * after);


/* 
 * fn		static int insertMediaDirsList(struct media_dir_s *new_dir)
 * brief	Insert new_dir to media_dirs list.
 * details	If one share folder contains another, would only insert the bigger share folder.
 *
 * param[in]	new_dir		struct media_dir_s of new share folder.
 *
 * return	
 * retval	0
 *
 * note		written by zengdongbiao, 12May15.
 */
static int insertMediaDirsList(struct media_dir_s *new_dir);


/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/* minidlna 接收消息相关的结构 */
CMSG_FD g_usFd;
CMSG_BUFF g_usBuff;

/* 
 * brief	数据库文件名列表。若数据库文件名不能使用，则在此列表中寻找下一个可用的数据库文件名。	
 */
const char *g_databaseNames[] = 
{
	"files.db",	
	"files1.db",
	"files2.db",
	"files3.db",
	"files4.db",
	"files5.db",
	"files6.db",
	"files7.db",
	"files8.db",
	"files9.db",
	"files10.db",
	NULL
};

/* 
 * brief	数据库文件名，默认为"files.db"。
 */
char *g_dbNameStr = NULL;

static DMS_INIT_INFO_MSG mdInitInfo;
static struct media_dir_s * media_dirs_realative = NULL;


/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
 
/* 
 * fn		static BOOL mdHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF) 
 * brief	根据初始化配置消息设置minidlna进程中的全局配置.	
 *
 * param[in]	pIF		指向DMS_INIT_INFO_MSG 类型消息的指针
 *
 * return		BOOL	布尔值
 * retval		TRUE	设置成功
 *				FALSE	设置失败
 *
 * note	written by  19Jun14, Liweijie	
 */
static BOOL mdHandleInitInfoReq(const DMS_INIT_INFO_MSG *pIF)
{
	if (pIF == NULL)
	{
		return FALSE;
	}

	memset(&mdInitInfo, 0, sizeof(DMS_INIT_INFO_MSG));
	
	memcpy(&mdInitInfo, pIF, sizeof(DMS_INIT_INFO_MSG));
	mdInitInfo.serverState = pIF->serverState ? 1 : 0;
	mdInitInfo.scanFlag = pIF->scanFlag ? 1 : 0;
	mdInitInfo.scanInterval = pIF->scanInterval > 0 ? pIF->scanInterval : 0;
	mdInitInfo.folderCnt = pIF->folderCnt > 0 ? pIF->folderCnt : 0;
	/*added by LY to set shareAll, in 2014.09.05*/
	mdInitInfo.shareAll = pIF->shareAll > 0 ? 1 : 0;
	/*end added by LY*/	
	
	if (strlen(mdInitInfo.serverName) == 0)
	{
		snprintf(mdInitInfo.serverName, MINIDLNA_MEDIA_SERVER_SERVERNAME_L, "%s", MEDIA_SERVER_DEF_NAME);
	}
	
	/* copy servername to friendly name, add by liweijie, 2014-06-22 */
	strncpyt(friendly_name, mdInitInfo.serverName, FRIENDLYNAME_MAX_LEN);
	
	MINIDLNA_DEBUG("init data struct END. servername %s\n", friendly_name);
	
	/* If friendly_name contains special chars(such as '&'), we would replace them, add by zengdongbiao, 07Apr15.  */
	minidlnaModifyFriendlyName(friendly_name, FRIENDLYNAME_MAX_LEN);
	/* End add by zengdongbiao */
	
	return TRUE;
}

/* 
 * fn		static BOOL mdHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI) 
 * brief	处理初始化时传递过来的共享目录消息.	
 * details	因为buf的大小原因,目录的信息需要分开发送.	
 *
 * param[in]	pFI			共享文件夹的信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  19Jun14, liweijie
 */
static BOOL mdHandleInitFolderReq(const DMS_OP_FOLDER_MSG *pFI)
{
	struct media_dir_s *media_dir;

	if (pFI == NULL)
	{
		return FALSE;
	}

	if (pFI->op != DMS_INIT_FOLDER)
	{
		MINIDLNA_DEBUG("not init folder req.");
		return FALSE;
	}
		
	/* add file to media_dirs */
	MINIDLNA_DEBUG("Add Media directory \"%s\" to media_dirs_realative\n", pFI->folder.path);

	if (pFI->folder.path)
	{
		media_dir = calloc(1, sizeof(struct media_dir_s));
		media_dir->path = strdup(pFI->folder.path);
		/*add by zhu haiming for uuid.*/
		strncpy(media_dir->uuid, pFI->folder.uuid, sizeof(media_dir->uuid));
		media_dir->types = ALL_MEDIA;
        media_dir->genable = FALSE;
		if (media_dirs_realative)
		{
			struct media_dir_s *all_dirs = media_dirs_realative;
			while( all_dirs->next )
			{
				all_dirs = all_dirs->next;
			}
			all_dirs->next = media_dir;
		}
		else
		{
			media_dirs_realative = media_dir;
		}
	}

	return TRUE;

}
 
 /* 
  * fn		 static BOOL mdHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
  * brief	 添加一个共享目录.	 
  * details 	 
  *
  * param[in]	 pFolder	 指向一个共享目录信息.
  *
  * return		 BOOL		 布尔值
  * retval		 TRUE		 成功
  * 			 FALSE		 失败
  *
  * note written by  19Jun14, liweijie
  */
static BOOL mdHandleAddNewShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
{
	BOOL addFlag = TRUE;
	struct media_dir_s *media_path = NULL;

	MINIDLNA_DEBUG("Add folder operate~\n");

	if (pFolder == NULL)
	{
		return FALSE;
	}

	/* 只有路径不同才可以添加 */
	media_path = media_dirs_realative;
	while (media_path)
	{
		if (strcmp(media_path->path, pFolder->folder.path) == 0 && 
			(0 == strcmp(media_path->uuid, pFolder->folder.uuid)))
		{	
			addFlag = FALSE;
			break;
		}
		media_path = media_path->next;
	}

	if (addFlag)
	{
		media_path = calloc(1, sizeof(struct media_dir_s));
		media_path->path = strdup(pFolder->folder.path);
		strncpy(media_path->uuid, pFolder->folder.uuid, sizeof(media_path->uuid));
		media_path->types = ALL_MEDIA;
		media_path->genable = FALSE;

		/*inset node to font head */
		media_path->next = media_dirs_realative;
		media_dirs_realative = media_path;
	}

	printMediaDirs(media_dirs_realative);
	
	return TRUE;

}

/* 
 * fn		static BOOL mdHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder) 
 * brief	删除一个共享目录.		
 *
 * param[in]	pFolder		指向一个共享目录信息.
 *
 * return		BOOL		布尔值
 * retval		TRUE		成功
 *				FALSE		失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleDelShareFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
{
	struct media_dir_s *media_pre = NULL;
	struct media_dir_s *media_path = media_dirs_realative;

	MINIDLNA_DEBUG("Del folder operate~\n");	

	if (pFolder == NULL || media_dirs_realative == NULL)
	{
		return FALSE;
	}
	
	for (media_pre = media_dirs_realative; media_pre != NULL; media_path = media_pre, media_pre = media_pre->next)
	{	
		if ((strcmp(media_pre->path, pFolder->folder.path) == 0) 
						&& (0 == strcmp(media_pre->uuid, pFolder->folder.uuid)))
		{
			if (media_pre == media_dirs_realative)
			{
				media_dirs_realative = media_pre->next;
			}
			else
			{
				media_path->next = media_pre->next;
			}
			free(media_pre->path);
			free(media_pre);
			printMediaDirs(media_dirs_realative);
			return TRUE;
		}
	}

	return FALSE;
}

/* 
 * fn		static BOOL mdHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
 * brief	添加或删除共享目录时的处理函数.	
 * details	包括添加 和 删除 目录.	
 *
 * param[in]	pFolder	指向目录操作的指针.
 *
 * return		BOOL	布尔值
 * retval		TRUE	处理添加或删除目录消息成功
 *				FALSE	处理添加或删除目录消息失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleOperateFolderReq(const DMS_OP_FOLDER_MSG *pFolder)
{
	int ret = 0;
	
	if (pFolder == NULL)
	{
		return FALSE;
	}

	switch (pFolder->op)
	{
	case DMS_ADD_FOLDER:
		mdHandleAddNewShareFolderReq(pFolder);
		break;
		
	case DMS_DEL_FOLDER:
		mdHandleDelShareFolderReq(pFolder);
		break;
		
	default:
		MINIDLNA_DEBUG("we don't care it,^_^!");
		break;
	}

	/* Quit inotify thread before quit scanner thread.  */
	minidlnaQuitInotifyThread();
		
	/* quit scanner thread */
	minidlnaQuitScannerThread();

	/* close sqlite io and we will check if database can access */
	sqlite3_close(db);
	
	//convert to scan dir
	mdConvertMediaDirsToAbsolute();

	/* If media server is not opening, do not scan media files. Add by zengdongbiao, 20Apr15. */
	if ( mdInitInfo.serverState == 0)
	{
		return TRUE;
	}
	/* End add by zengdongbiao.  */
	
	//check_db(db, 0);
	ret = minidlna_startScannerThread();
	if (ret == 0)
	{
		/* Start inotify thread */
		minidlnaStartInotifyThread();
	}
	
	updateID++;

	return TRUE;
}

/* 
 * fn		static BOOL mdHandleReloadReq(const DMS_RELOAD_MSG *pReload) 
 * brief	当用户配置改变后发送该消息到minidlna进程,这里是最终的处理函数.	
 *
 * param[in]	pReload		指向reload 消息结构.
 *
 * return		BOOL		布尔值
 * retval		TRUE		处理reload 消息成功
 *				FALSE		处理reload消息失败
 *
 * note	written by  19Jun14, liweijie	
 */
static BOOL mdHandleReloadReq(const DMS_RELOAD_MSG *pReload)
{	
	if (pReload == NULL)
	{
		return FALSE;
	}

	/*  Check if shareAll changed */
	if (pReload->shareAll != mdInitInfo.shareAll)
	{
		MINIDLNA_DEBUG("shareAll changed: %d to %d\n", mdInitInfo.shareAll, pReload->shareAll);
		mdInitInfo.shareAll = pReload->shareAll;

		/* If media server state is on, we need restart it.  */
		if(mdInitInfo.serverState > 0)
		{
			MINIDLNA_DEBUG("shareAll changed restart it...\n");
			
			minidlnaSwitchOnOff(FALSE);	

			//convert to scan dir
			mdConvertMediaDirsToAbsolute();
		
			minidlnaSwitchOnOff(TRUE);
		}
	}

	/*  Check if media server name changed */
	if (strcmp(pReload->serverName, mdInitInfo.serverName) != 0 && strlen(pReload->serverName) != 0)
	{
		MINIDLNA_DEBUG("media server name changed: %s to %s\n", mdInitInfo.serverName, pReload->serverName);
							
		snprintf(mdInitInfo.serverName, MINIDLNA_MEDIA_SERVER_SERVERNAME_L, "%s", pReload->serverName);
		strncpyt(friendly_name, mdInitInfo.serverName, FRIENDLYNAME_MAX_LEN);
		
		/* If friendly_name contains special chars(such as '&'), we would replace them, add by zengdongbiao, 07Apr15.  */
		minidlnaModifyFriendlyName(friendly_name, FRIENDLYNAME_MAX_LEN);
		/* End add by zengdongbiao */

		/* If media server state is on, we need restart it.  */
		if (mdInitInfo.serverState > 0)
		{
			MINIDLNA_DEBUG("media server name changed restart.....\n");
			
			minidlnaSwitchOnOff(FALSE);
			sleep(1);
			minidlnaSwitchOnOff(TRUE);
		}
	}

	/*  Check if severSate changed */
	if (mdInitInfo.serverState != pReload->serverState)
	{
		MINIDLNA_DEBUG("media server state changed: %d to %d\n", 
							mdInitInfo.serverState, (pReload->serverState ? 1 : 0));
	    mdInitInfo.serverState = pReload->serverState ? 1 : 0;
			
		/* 每次开启media server 都需要转换共享条目  */
		if (pReload->serverState > 0)
		{
			//convert to scan dir
			mdConvertMediaDirsToAbsolute();
		}
		
		minidlnaSwitchOnOff(pReload->serverState > 0 ? TRUE : FALSE);
	}

	mdInitInfo.scanFlag = pReload->scanFlag ? 1 : 0;
	mdInitInfo.scanInterval = pReload->scanInterval > 0 ? pReload->scanInterval : 0;

	return TRUE;
	
}


/* 
 * fn		static BOOL mdHandleManualScanReq()
 * brief	设手动扫描共享目录.	
 *
 * return	BOOL		布尔值
 * retval	TRUE		设置成功
 *			FALSE		设置失败
 *
 * note	written by  19Jun14, liweijie
 */
static BOOL mdHandleManualScanReq()
{
	int ret = -1;
	/* char cmd[PATH_MAX*2]; */

	/* Quit inotify thread before quit scanner thread.  */
	minidlnaQuitInotifyThread();
	
	/* quit scanner thread */
	minidlnaQuitScannerThread();

	/* set user_version to zero, make it rescan */
	sql_exec(db, "pragma user_version = %d;", 0);
		
	sqlite3_close(db);
	
	/* remove db to let rescan */
	/*
	snprintf(cmd, sizeof(cmd), "rm -rf %s/files.db %s/art_cache", db_path, db_path);
	if (system(cmd) != 0)
		MINIDLNA_DEBUG("Failed to clean old file cache! %s \n", cmd);
	*/
	
	/* create media dirs absolute path */
	mdConvertMediaDirsToAbsolute();
	
	/* Open sqlite database and rescan media files, modify by zengdongbiao, 27Mar15 */
	ret = minidlna_startScannerThread();
	if (ret == 0)
	{
		/* Start inotify thread */
		minidlnaStartInotifyThread();
	}
	/* End modify by zendongbiao.  */
	
    updateID++;

	return TRUE;
}


/* 
 * fn		static BOOL mdHandleDeviceStateChange()
 * brief	when usb device's state changed,rebuild the sqlite database
 *
 * return	 BOOL
 * retval	 TRUE :rebuild successful
 		     FALSE:rebuild failed
 *
 * note	written by 19Jun14, liweijie	
 */
static BOOL mdHandleDeviceStateChange()
{
	int ret = -1;

	/* Quit inotify thread before quit scanner thread.  */
	minidlnaQuitInotifyThread();
		
	/* close scanner thread */
	minidlnaQuitScannerThread();

	/* close sqlite io and we will check if database can access */
	sqlite3_close(db);

	/* create media dirs absolute path */
	mdConvertMediaDirsToAbsolute();
	
	/* Open sqlite database and rescan media files, modify by zengdongbiao, 27Mar15 */
	ret = minidlna_startScannerThread();
	if (ret == 0)
	{
		/* Start inotify thread */
		minidlnaStartInotifyThread();
	}
	/* End modify by zendongbiao.  */
	
	updateID++;
	
	return TRUE;
}


/* 
 * fn		BOOL mdConvertMediaDirsToAbsolute()
 * brief	convert the media_dirs_relative to media_dirs,make it can scan work~	
 *
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  22Jun14, liweijie	
 */
static BOOL mdConvertMediaDirsToAbsolute()
{
	char path[PATH_MAX];/* PATH_MAX=4096  */
	char uuid[BUFLEN_64] = {0};
	char type[BUFLEN_16] = {0};
	char label[BUFLEN_64] = {0};
	char devPath[BUFLEN_32] = {0};
	
	DIR *pDir = NULL;
	struct dirent *pResult = NULL;
	struct dirent entryItem;
	
	struct media_dir_s *media_path, *last_path, *media_dir;

	/* clear media_dirs */
	media_path = media_dirs;
	while (media_path)
	{
		if (media_path->path)
		{
			DPRINTF(E_WARN, L_GENERAL, "media_path->path: %s\n", media_path->path);
		}
		free(media_path->path);
		last_path = media_path;
		media_path = media_path->next;
		last_path->next = NULL;
		free(last_path);
	}

	/* set media_dirs to empty */
	media_dirs = NULL;
	
	/* find path to access, add by liweijie, 2014-11-03 */
	pDir = opendir(USB_DISK_DIRECTORY);
	if (NULL == pDir)
	{
		DPRINTF(E_ERROR, L_SCANNER, _("Scanning dir NULL\n"));
	}

	while (!readdir_r(pDir, &entryItem, &pResult) && (NULL != pResult))
	{
		/* escape dir . and usbvm and files */
		if (entryItem.d_type != DT_DIR)
		{
			continue;
		}
		
		if (entryItem.d_name[0] == '.')
		{
			continue;
		}
			
		if (strcmp(entryItem.d_name, USBVM) == 0)
		{
			continue;
		}

		/*when shareAll is set to be true, which means that we should scan all the volumes, 
		 *skip the step of checking the uuid,
		 *when shareAll is set to be false, which means that we should only scan the directories set by 
		 *user from UI, do the uuid checking, and only scan the volume that match the uuid of the directory.
		 */
		if (!mdInitInfo.shareAll)
		{
			memset(devPath, 0, sizeof(devPath));
			memset(uuid, 0, sizeof(uuid));
			memset(type, 0, sizeof(type));
			memset(label, 0, sizeof(label));
			snprintf(devPath, sizeof(devPath), "%s%s", DEV_DIR_PREFIX, entryItem.d_name);
			if (0 == mdGetVolumeInfoFromDev(devPath, uuid, type, label, 
											sizeof(uuid), sizeof(type), sizeof(label)))
			{
				media_path = media_dirs_realative;
				while(media_path)
				{
					memset(path, 0, sizeof(path));
					snprintf (path, sizeof(path), "%s/%s%s", USB_DISK_DIRECTORY, entryItem.d_name, media_path->path);
					/* Donot add '/' at the end of path */
					if (path[strlen(path) - 1] == '/')
					{
						path[strlen(path) - 1] = '\0';
					}
					
					/* path is not exist, Could't add it to scanner path */
					/* Share all the matched folder if uuid is empty, otherwise only the one with the same uuid. */
					if ((strlen(media_path->uuid) != 0) && ((access(path, F_OK) != 0) || (strcmp(media_path->uuid, uuid) != 0)))
					{
						MINIDLNA_DEBUG("Media directory %s not accessible, don't add!\n", path);
						media_path = media_path->next;
						continue;
					}
						
					media_dir = calloc(1, sizeof(struct media_dir_s));
					media_dir->path = strdup(path);
					media_dir->types = media_path->types;
		    		media_dir->genable = media_path->genable;

					/* if one share folder contains another, another will be removed. Modify by zengdongbiao, 12May15. */
					insertMediaDirsList(media_dir);
					//if (media_dirs)
					//{
					//	struct media_dir_s *all_dirs = media_dirs;
					//	while( all_dirs->next )
					//	{
					//		all_dirs = all_dirs->next;
					//	}
					//	all_dirs->next = media_dir;
					//}
					//else
					//{
					//	media_dirs = media_dir;
					//}
					/* End modify by zengdongbiao.  */
					
					media_path = media_path->next;
					MINIDLNA_DEBUG("path:%s\n",path);
				}
			}
		}
		else 
		{
			memset(path, 0, sizeof(path));
			/* Donot add '/' at the end of path */
			//snprintf (path, sizeof(path), "%s/%s%s", USB_DISK_DIRECTORY, entryItem.d_name, ROOT_DIRECTORY);
			snprintf (path, sizeof(path), "%s/%s", USB_DISK_DIRECTORY, entryItem.d_name);
			
			if (access(path, F_OK) != 0)
			{
				MINIDLNA_DEBUG("Media directory %s not accessible, don't add!\n", path);
			}
			else 
			{
				media_dir = calloc(1, sizeof(struct media_dir_s));
				media_dir->path = strdup(path);
				media_dir->types = ALL_MEDIA;
				media_dir->genable = FALSE;
				
				if (media_dirs)
				{
					struct media_dir_s *all_dirs = media_dirs;
					while( all_dirs->next )
					{
						all_dirs = all_dirs->next;
					}
					all_dirs->next = media_dir;
				}
				else
				{
					media_dirs = media_dir;
				}
				
				MINIDLNA_DEBUG("path:%s\n",path);
			}
		}
	}

	/* close dir, add by liweijie, 2014-11-03 */
	if (NULL != pDir)
	{
		if (-1 == closedir(pDir))
		{
			DPRINTF(E_ERROR, L_SCANNER, _("Close Dir fail, error: %d!\n"), errno);
		}
	}

	//MINIDLNA_DEBUG("realative dirs to show:\n");
	//printMediaDirs(media_dirs_realative);
	//MINIDLNA_DEBUG("scan dirs to show:\n");
	//printMediaDirs(media_dirs);
	
	return TRUE;
}

/* 
 * fn		static int mdGetVolumeInfoFromDev(char *pVolumeDevPath, char *pUuid, 
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
static int mdGetVolumeInfoFromDev(char *pVolumeDevPath, char *pUuid, 
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
		MINIDLNA_ERROR("Invalid parameter.\n");
		return ret;
	}

	if (0 != strncmp(pVolumeDevPath, DEV_DIR_PREFIX, strlen(DEV_DIR_PREFIX)))
	{
		MINIDLNA_ERROR("Invalid dev path! Prefix(%s) is required.\n", DEV_DIR_PREFIX);
		return ret;
	}

	if (0 != access(pVolumeDevPath, F_OK))
	{
		MINIDLNA_ERROR("Dev file(%s) does not exist.\n", pVolumeDevPath);
		return ret;
	}

	if (-1 == (lockFd = open(FILE_DM_STORAGE, O_RDONLY)))
	{
		MINIDLNA_ERROR("Failed to open lock file(%s).\n", FILE_DM_STORAGE);
		return ret;
	}
	/* get the file lock first. */
	flock(lockFd, LOCK_EX);
	
	snprintf(lineBuf, sizeof(lineBuf), FORMAT_BLKID_COMMAND, pVolumeDevPath, FILE_BLKID_RESULT);
	system(lineBuf);

	/* make sure the FILE_BLKID_RESULT has been created. */
	if (NULL == (pFileResult = fopen(FILE_BLKID_RESULT, "r")))
	{
		MINIDLNA_ERROR("Failed to open the blkid result file for dev(%s).\n", pVolumeDevPath);
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
		MINIDLNA_DEBUG("lineBuf(%s).\n", lineBuf);
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
		MINIDLNA_ERROR("Can not get block info for device(%s).\n", pVolumeDevPath);
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
	
	//MINIDLNA_ERROR("Dev(%s), uuidBuf(%s), typeBuf(%s), labelBuf(%s).\n", pVolumeDevPath, uuidBuf, typeBuf, labelBuf);

	return 0;
}


/* 
 * fn		void printMediaDirs(struct media_dir_s *print)
 * brief	Debug function for show media dirs 	
 *
 *
 * return		BOOL		布尔值
 * retval		TRUE		设置成功
 *				FALSE		设置失败
 *
 * note	written by  22Jun14, liweijie	
 */
void printMediaDirs(struct media_dir_s *print)
{
	struct media_dir_s *media_dir = print;
	
	if (print == NULL)
	{
		return ;
	}

	MINIDLNA_DEBUG("print media dirs:\n");
	while(media_dir)
	{
		MINIDLNA_DEBUG("media dirs:%s\n", media_dir->path);
		MINIDLNA_DEBUG("genable: %d, types: %d\n", media_dir->genable, media_dir->types);
		media_dir = media_dir->next;
	}
	MINIDLNA_DEBUG("print media dirs finish \n");
}

/* 
* fn	static int modifyStr(char *strName, 
							 int strNameSize, 
							 const char * before, 
							 const char * after)
 * brief	Modify strName, replace "before" with "after".
 * details	用after替代before。
 *
 * param[in]	strName			The string to modify.
 * param[in]	strNameSize		The size of strName.
 * param[in]	before			The string going to be replaced.
 * param[in]	after			The string to replace "before" string.
 * return		
 * retval		0	successful.
 * retval		-1	The size of strName is not enough.
 *
 * note			written by zengdongbiao, 07Apr15.
 */
static int modifyStr(char *strName, 
					 int strNameSize, 
					 const char * before, 
					 const char * after)
{	
	char *str = NULL;
	char *oldStrLoaction = NULL;
	int oldlen = 0, newlen = 0;
	int chgCnt = 0;

	oldlen = strlen(before);
	newlen = strlen(after);

	//DPRINTF(E_WARN, L_GENERAL, "Going to modify strName[%s] from %s to %s\n", 
								//strName, before, after);

	/* make sure the strNameSize is enough.  */
	if( newlen > oldlen )
	{
		str = strName;
		while( (oldStrLoaction = strstr(str, before)) )
		{
			chgCnt++;
			str = oldStrLoaction + oldlen;
		}

		if ( (strlen(strName) + ((newlen - oldlen) * chgCnt) + 1) > strNameSize)
		{
			DPRINTF(E_ERROR, L_GENERAL, "modifyStr failed: strNameSize[%d] is not enough\n",
															strNameSize);
			return -1;
		}
	}
	
	str = strName;
	while( str )
	{
		oldStrLoaction = strstr(str, before);
		if( !oldStrLoaction )
		{
			break;
		}
		memmove(oldStrLoaction + newlen, oldStrLoaction + oldlen, strlen(oldStrLoaction + oldlen) + 1);
		memcpy(oldStrLoaction, after, newlen);
		str = oldStrLoaction + newlen;
	}

	//DPRINTF(E_WARN, L_GENERAL, "After modifyStr, strName: %s\n", strName);

	return 0;
}


/* 
 * fn		static int insertMediaDirsList(struct media_dir_s *new_dir)
 * brief	Insert new_dir to media_dirs list.
 * details	If one share folder contains another, would only insert the bigger share folder.
 *
 * param[in]	new_dir		struct media_dir_s of new share folder.
 *
 * return	
 * retval	0
 *
 * note		written by zengdongbiao, 12May15.
 */
static int insertMediaDirsList(struct media_dir_s *new_dir)
{
	struct media_dir_s *old_dir = NULL;
	struct media_dir_s *dirPre = NULL;
	int newDirLen = strlen(new_dir->path);
	int oldDirLen = 0;
		
	if (media_dirs == NULL)
	{
		media_dirs = new_dir;
	}
	else	
	{
		for(old_dir = media_dirs; old_dir != NULL; old_dir = old_dir->next)
		{
			oldDirLen = strlen(old_dir->path);

			if (newDirLen <= oldDirLen)	/* Check if new_dir contains old_dir */
			{
				if (strncmp(new_dir->path, old_dir->path, newDirLen) == 0
					 && old_dir->path[newDirLen] == '/')
				{
					DPRINTF(E_WARN, L_GENERAL, "new share folder[%s] contains old share folder[%s], replace old share foler\n", 
												new_dir->path, old_dir->path);
					if (dirPre != NULL)
					{
						dirPre->next = new_dir;
					}
					else
					{
						media_dirs = new_dir;
					}
	
					new_dir->next = old_dir->next;
					
					free(old_dir->path);
					free(old_dir);

					return 0;
				}
			}
			else /* Check if old_dir contains new_dir */
			{
				if (strncmp(new_dir->path, old_dir->path, oldDirLen) == 0 
					&& new_dir->path[oldDirLen] == '/')
				{
					DPRINTF(E_WARN, L_GENERAL, "Old share folder[%s] contains new share folder[%s], donot add\n", 
												old_dir->path, new_dir->path);
					free(new_dir->path);
					free(new_dir);
					
					return 0;
				}
			}

			dirPre = old_dir;
		}

		dirPre->next = new_dir;
	}

	return 0;
}


/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

void free_media_dirs_realative()
{
	struct media_dir_s *media_path, *last_path;
	media_path = media_dirs_realative;
	while (media_path)
	{
		free(media_path->path);
		last_path = media_path;
		media_path = media_path->next;
		free(last_path);
	}	
}

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
BOOL minidlnaProcessInitInfo()
{
	int sRet = 0;
	fd_set usFds;
	BOOL iStep1_Done = FALSE;
	BOOL iStep2_Done = FALSE;
	unsigned int folderCnt = 0;

	while (!((iStep1_Done == TRUE) && (iStep2_Done == TRUE)))
	{	
		/*listen 娑堟伅*/
		memset(&g_usBuff, 0 , sizeof(CMSG_BUFF));

		FD_ZERO(&usFds);
   		FD_SET(g_usFd.fd, &usFds);

		MINIDLNA_DEBUG("waiting for message...\n");
	    sRet = select(g_usFd.fd + 1, &usFds, NULL, NULL, NULL); 
		if (sRet == 0)
		{	
			/*timeout.*/
			continue;
		}
		else if (sRet < 0)
		{
			if (sRet != EINTR && sRet != EAGAIN)
			{
				/*error and neither EINTR nor EAGAIN.*/
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
				MINIDLNA_DEBUG("received CMSG_DLNA_MEDIA_SERVER_INIT message.\n");
				if ((iStep1_Done == TRUE) || (iStep2_Done == TRUE))
				{
					/*order error in recving msg.*/	
					break;
				}
				if (TRUE != mdHandleInitInfoReq((DMS_INIT_INFO_MSG *)(g_usBuff.content)))
				{
					/*handle init info error,*/
					return FALSE;
				}
				
				iStep1_Done = TRUE;
				if (mdInitInfo.folderCnt == 0)
				{
					iStep2_Done = TRUE;
				}

				break;

			case CMSG_DLNA_MEDIA_SERVER_OP_FOLDER:
				MINIDLNA_DEBUG("received CMSG_DLNA_MEDIA_SERVER_OP_FOLDER message.\n");
				if (iStep1_Done != TRUE)
				{
					/*order error in recving msg.*/
					return FALSE;
				}
				
				if (folderCnt >= mdInitInfo.folderCnt)
				{
					return FALSE;	
				}
				
				if (TRUE != mdHandleInitFolderReq((DMS_OP_FOLDER_MSG *)(g_usBuff.content)))
				{
					return FALSE;
				}
				
				folderCnt++;
				
				if (folderCnt == mdInitInfo.folderCnt)
				{
					iStep2_Done = TRUE;
				}
				break;

			/* unknown msg, not supported yet */		
			default:
				break;
			}

		}

	}

	if ((iStep1_Done == TRUE) && (iStep2_Done == TRUE))
	{
		mdConvertMediaDirsToAbsolute();
		MINIDLNA_DEBUG("handle initInfo end, state:%d!\n", mdInitInfo.serverState);
		if (mdInitInfo.serverState > 0)
		{
			minidlnaSwitchOnOff(TRUE);
		}
		
		return TRUE;
	}

	return FALSE;
}

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
BOOL minidlnaProcessConfigureRequest()
{
	if (g_usFd.fd < 0)
	{
		DPRINTF(E_DEBUG, L_GENERAL, "configure socket colsed!\n");
		return FALSE;
	}
	
	msg_recv(&g_usFd, &g_usBuff);

	switch (g_usBuff.type)
	{
	case CMSG_DLNA_MEDIA_SERVER_RELOAD:
		MINIDLNA_DEBUG("received CMSG_DLNA_MEDIA_SERVER_RELOAD message.\n");
		if (TRUE != mdHandleReloadReq((DMS_RELOAD_MSG *)(g_usBuff.content)))
		{
			MINIDLNA_DEBUG("handle reload message error\n");
		}
		break;

	case CMSG_DLNA_MEDIA_SERVER_OP_FOLDER:
		MINIDLNA_DEBUG("received CMSG_DLNA_MEDIA_SERVER_OP_FOLDER message.\n");
		if (TRUE != mdHandleOperateFolderReq((DMS_OP_FOLDER_MSG *)(g_usBuff.content)))
		{					
			MINIDLNA_DEBUG("handle operatefolderreq message error\n");
		}
		break;
				
	case CMSG_DLNA_MEDIA_SERVER_MANUAL_SCAN:
		MINIDLNA_DEBUG("received CMSG_DLNA_MEDIA_SERVER_MANUAL_SCAN message.\n");
		if (TRUE != mdHandleManualScanReq())
		{
			MINIDLNA_DEBUG("handle manual scan message error\n");
		}
		break;
				
	case CMSG_DLNA_MEDIA_DEVICE_STATE_CHANGE:
		MINIDLNA_DEBUG("received CMSG_DLNA_MEDIA_DEVICE_STATE_CHANGE message.\n");
		if (TRUE != mdHandleDeviceStateChange())
		{
			MINIDLNA_DEBUG("handle manual scan message error\n");
		}
		break;
				
	/*unknown msg, not supported yet */
	default :
		MINIDLNA_DEBUG("^_^, not supported yet now!\n");
		break;
				
	}
			
	return TRUE;		
}

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
BOOL minidlnaSwitchOnOff(BOOL onOff)
{
	if (onOff == TRUE)
	{
		minidlnaStart();
	}
	else if (onOff == FALSE)
	{
		minidlnaShutdown();
	}

	return TRUE;
}


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
							  int friendlyNameSize)
{	
	int ret = 0;

	//DPRINTF(E_WARN, L_GENERAL, "Going to modify friendly_name[%s]\n", friendlyName);
	/* Limit the total length of friendly_name to ORIGINAL_FRIENDLYNAME_MAX_LEN(64), 4 indicates ": 1\0"*/
	if (strlen(friendlyName) > (ORIGINAL_FRIENDLYNAME_MAX_LEN - 4))
	{
		DPRINTF(E_ERROR, L_GENERAL, "friendly_name[%s] is too long\n", friendlyName);
		return -2;
	}

	/* If friendly_name contains '&', we would replace the '&' to "&amp;" */
	if (strchr(friendlyName, '&'))
	{
		ret = modifyStr(friendlyName, friendlyNameSize, "&", "&amp;");
	}

	/* If friendly_name contains '<', we would replace the '<' to "&lt;" */
	if ((ret == 0) && strchr(friendlyName, '<'))
	{
		ret = modifyStr(friendlyName, friendlyNameSize, "<", "&lt;");
	}

	/* If friendly_name contains '>', we would replace the '>' to "&gt;" */
	if ((ret == 0) && strchr(friendlyName, '>'))
	{
		ret = modifyStr(friendlyName, friendlyNameSize, ">", "&gt;");
	}
	
	//DPRINTF(E_WARN, L_GENERAL, "After minidlnaModifyFriendlyName, friendly_name: %s\n", friendlyName);

	return ret;
}


