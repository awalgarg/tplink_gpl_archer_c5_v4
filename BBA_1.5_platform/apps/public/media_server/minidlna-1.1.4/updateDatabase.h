/*  
 * file		updateDatabase.h
 * brief	This file is created for updating sqlite database.	
 * details	
 *
 * history 	\arg 1.0.0, 8Feb15, zengdongbiao, create the file.		
 */
 
#ifndef __UPDATEDATABASE_TPLINK_H__
#define __UPDATEDATABASE_TPLINK_H__

#ifdef USE_UPDATE

#include "scanner.h"

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* 
 * brief	The limit of max number of watch diretorys.
 */
#define MAX_WATCH_DIR_MUNBER	(MAX_FILE_NUMBER)


/* 
 * brief	NTFS filesystem type.
 */
#define NTFS_FILESYSTEM_TYPE	(0x5346544e)


/* 
 * brief	FAT32 filesystem type.
 */
#define FAT32_FILESYSTEM_TYPE	(0x4D44)


#define RESCAN_RATE	(0.6)


/* USE_UPDATE cannot be used for lite_minidlna. */
#ifdef LITE_MINIDLNA
#ifdef USE_UPDATE
#undef USE_UPDATE
#endif
#endif

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn		int update_checkFilesytem(const char *path)
 * brief	Check if filesystem is NTFS or ....
 * details	
 *
 * param[in]	path	path of folder.	
 *
 * return	
 * retval	0	filesystem is NTFS.
 *			-1	filesystem not NTFS.
 * note		Written by zengdongbiao, 8Feb15.
 */
int update_checkFilesytem(const char *path);


/* 
 * fn		int update_remvFolder(const char *dirPath)
 * brief	Remove directory and subcontent from database.
 * details	
 *
 * param[in]	dirPath	 the path of directory.
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
int update_remvFolder(const char *dirPath);

/* 
 * fn		int update_remvShareFolder(const char *dirPath)
 * brief	Remove share folder contents from database.
 * details	
 *
 * param[in]	dirPath	 the path of share folder.
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
int update_remvShareFolder(const char *folderPath);


/* 
 * fn		void update_removeEmptyDir(const char *base, const char *ID)
 * brief	Delete empty directory from sqlite database.
 * details	
 *
 * param[in]	base	First part of objID of empty directory, for example, BROWSEDIR_ID.
 * param[in]	ID		Second part of objID of empty directory.
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
void update_removeEmptyDir(const char *base, const char *ID);


/* 
 * fn		void update_scanDirectory(const char *dir, const char *parent, media_types dir_types)
 * brief	Scan directory which need to update.
 * details	This function would detect disappeared content(sub file/dir) at first, 
 *			then insert new content. 
 *
 * param[in]	dir		The directory path.
 * param[in]	parent	The parent ID of directory.
 * param[in]	dir_types	The types(TYPE_AUDIO/TYPE_VIDEO/TYPE_IMAGES) of directory.
 * param[out]	curDirIsEmpty		Return 1 when the directory is empty, else return 0.	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
void update_scanDirectory(const char *dir, const char *parent, media_types dir_types);


/* 
 * fn		unsigned int update_setWatchDirsNum(void)
 * brief	set value of g_watchDirsNum.
 * details		
 *
 * return	
 * retval	The value of g_watchDirsNum.
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
unsigned int update_setWatchDirsNum(unsigned int watchDirsNum);

/* 
 * fn		unsigned int update_getWatchDirsNum(void)
 * brief	Get value of g_watchDirsNum.
 * details		
 *
 * return	
 * retval	The value of file_number.
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
unsigned int update_getWatchDirsNum(void);

/* 
 * fn		void update_increaseWatchDirsNum(void)
 * brief	Increase the value of g_watchDirsNum to (g_watchDirsNum + 1).
 * details	
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
void update_increaseWatchDirsNum(void);

/* 
 * fn		void update_decreaseWatchDirsNum(void)
 * brief	Decrease the value of g_watchDirsNum to (g_watchDirsNum - 1).
 * details		
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
void update_decreaseWatchDirsNum(void);

/* 
 * fn		int update_addWatchDir(const char *full_path) 
 * brief	Add watch directory.
 * details	insert diretory to sqlite table WATCHDIRS.
 *
 * param[in]	full_path	path of directory.
 *
 * return	
 * retval	0	successful.
 *			-1	error.
 * note		Written by zengdongbiao, 05May15.
 */
int update_addWatchDir(const char *full_path);


/* 
 * fn		void update_selectFileAndWatchDirNum()
 * brief	Get file number and watch dir number from DB.
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 06May15.
 */
void update_selectFileAndWatchDirNum();


/* 
 * fn		void update_storeFileAndWatchDirNum(int rescanFlag)
 * brief	Store file number and watch dir number to DB.
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 06May15.
 */
void update_storeFileAndWatchDirNum(int rescanFlag);


/* 
 * fn		void update_init(int rescan)
 * brief	Prepare for updating database or rescanning media files.
 * details	
 *
 * param[in]	rescan	1: rescan media files; 0: update database.
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 06May15.
 */
void update_init(int rescan);


/* 
 * fn		void update_remvAlbumArtOfFile(int64_t detailID)
 * brief	Remove album_art of file.
 * details	
 *
 * param[in]	detailID	The detail ID of file.	
 *
 * note		Written by zengdongbiao, 05May15.
 */
void update_remvAlbumArtOfFile(int64_t detailID);


/* 
 * fn		int update_CheckShareFolder(int CheckOnly)
 * brief	Check if any share folder disappeared or is not NTFS filesystem, and delete them from DB.
 * details	if ChackOnly > 0, then check if need rescan or update based on the files number.
 *
 * param[in]	CheckOnly	Only check if need rescan.
 * param[out]	
 *
 * return	
 * retval	0	No need to rescan.
 *			-1	Need to rescan.
 * note		
 */
int update_CheckShareFolder(int CheckOnly);


#endif

#endif	/* __UPDATEDATABASE_TPLINK_H__ */
