/*  
 * file		updateDatabase.c
 * brief	This file is created for updating sqlite database.
 * details	
 *
 * history 	\arg 1.0.0, 8Feb15, zengdongbiao, create the file.	
 */
 
#include "minidlna.h"
 
#ifdef USE_UPDATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <locale.h>
#include <libgen.h>
#include <inttypes.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

/* Use sqlite amalgamation, modify by zengdongbiao, 20May15. */
//#include <sqlite3.h>
#include "sqlite3.h"
/* End modify by zengdongbiao */

#include "libav.h"

#include "upnpglobalvars.h"
#include "metadata.h"
#include "playlist.h"
#include "utils.h"
#include "sql.h"
#include "scanner.h"
#include "albumart.h"
#include "log.h"
#include "upnphttp.h"
#include "scanner.h"

#include "updateDatabase.h"

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
 * fn		satic int isCaption(const char * file)
 * brief	Check a file is caption or not.
 * details	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int isCaption(const char * file);

/* 
 * fn		static int remvDisappearedFile(const char *filePath, int64_t detailID)
 * brief	
 * details	
 *
 * param[in]	filePath	The path of disappeared file.
 * param[in]	detailID	The detailID of disappeared file.
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedFile(const char *filePath, int64_t detailID);


/* 
 * fn		static int remvDisappearedDir(const char *dirPath)
 * brief	Remove directory.
 * details	
 *
 * param[in]	dirPath		The path of directory.
 *
 * return	Return 0 when remove success, or else return -1. 
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedDir(const char *dirPath);


/* 
 * fn		static int remvDisappearedContent(const char *parentPath, const char *parentID)
 * brief	Find disappeared content under parent directory and Remove them from sqlite database.
 * details	
 *
 * param[in]	parentPath	parent directory path of the disappeared content.
 * param[in]	parentID	parent directory ID of the disappeared content.
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedContent(const char *parentPath, const char *parentID);

/* 
 * fn		static void remvAlbumArtOfDir(int64_t detailID)
 * brief	Remove albumArt of empty directory.
 * details	
 *
 * param[in]	detailID	The detailID of empty directory.	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static void remvAlbumArtOfDir(int64_t detailID);

/* 
 * fn		static int insertParentDirs(const char *subDirPath)
 * brief	Insert directory and all necessary parents.
 * details	Find change in a empty directory, so need to insert it and all necessary parent dir.
 *
 * param[in]	
 * param[out]	
 *
 * return	The num of parent dir been inserted.
 * retval	
 *
 * note		Here do not limit file_number, because if file_number achieve the limit, then would 
 *			insert nothing when update_scanDirectory. so the directory would be treated as empty folder
 *			which would be removed by remvEmptyDirs. Written by zengdongbiao, 8Feb15.
 */
static int insertParentDirs(const char *subDirPath);

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/* 
 * brief	The total number of watch directorys which have been inserted into WATCHDIRS.
 */
unsigned int g_watchDirsNum = 0;

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/* 
 * fn		satic int isCaption(const char * file)
 * brief	Check a file is caption or not.
 * details	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int isCaption(const char * file)
{
	return (ends_with(file, ".srt"));
}


/* 
 * fn		static void remvAlbumArtOfDir(int64_t detailID)
 * brief	Remove albumArt of empty directory.
 * details	
 *
 * param[in]	detailID	The detailID of empty directory.	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static void remvAlbumArtOfDir(int64_t detailID)
{
	char art_cache[PATH_MAX];
	int64_t albumArtID = 0;
	char * dirPath = NULL;
	char *albumArtPath = NULL;
	char * sql = NULL;
	char **result = NULL;
	int rows = 0;

#ifdef USE_ALBUM_RESPONSE
	albumArtID = sql_get_int64_field(db, "SELECT ALBUM_ART from DETAILS where ID = %lld", detailID);
	if (albumArtID > 0)
	{
		sql_exec(db, "DELETE from ALBUM_ART where ID = %lld", albumArtID);
	}
	return;
#endif /* USE_ALBUM_RESPONSE */

	sql = sqlite3_mprintf("SELECT ALBUM_ART, PATH from DETAILS where ID = %lld",
										detailID);

	if ((SQLITE_OK == sql_get_table(db, sql, &result, &rows, NULL)) && rows )
	{
		albumArtID = strtoll(result[2], NULL, 10);
		dirPath = result[3];
		snprintf(art_cache, sizeof(art_cache), "%s/art_cache%s", db_path, dirPath);
		
		if (albumArtID > 0)
		{	
			albumArtPath = sql_get_text_field(db, "SELECT PATH from ALBUM_ART where ID = '%lld'",
															albumArtID);	
			sql_exec(db, "DELETE from ALBUM_ART where ID = %lld", albumArtID);
			
			if(albumArtPath)
			{
				//DPRINTF(E_WARN, L_SCANNER, "albumArtPath of dir: %s\n", albumArtPath);
				/* Only remove album_art created in art_cache. */
				if(strncmp(art_cache, albumArtPath, strlen(art_cache)) == 0)
				{
					remove(albumArtPath);
				}
			}
		}
		/* Delete the album_arl dir created in art_cache, if it is not empty, remove(art_cache) will failed.  */
		remove(art_cache);
	}
	
	sqlite3_free(albumArtPath);
	sqlite3_free(sql);
	sqlite3_free_table(result);
}


/* 
 * fn		static int remvDisappearedFile(const char *filePath, int64_t detailID)
 * brief	
 * details	
 *
 * param[in]	filePath	The path of disappeared file.
 * param[in]	detailID	The detailID of disappeared file.
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedFile(const char *filePath, int64_t detailID)
{
	char sql[128];	
	char **result = NULL;
	int rows = 0, i = 0;
	int children = 0;
	char *ptr = NULL;

	/* Invalidate the scanner cache so we don't insert files into non-existent containers */
	valid_cache = 0;
	
	/* Delete the parent containers if we are about to empty them. */
	snprintf(sql, sizeof(sql), "SELECT PARENT_ID from OBJECTS where DETAIL_ID = %lld"
	                           " and PARENT_ID not like '64$%%'",
	                           (long long int)detailID);
	if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) )
	{
		for( i=1; i <= rows; i++ )
		{
			/* If it's a playlist item, adjust the item count of the playlist */
			if( strncmp(result[i], MUSIC_PLIST_ID, strlen(MUSIC_PLIST_ID)) == 0 )
			{
				sql_exec(db, "UPDATE PLAYLISTS set FOUND = (FOUND-1) where ID = %d",
				         atoi(strrchr(result[i], '$') + 1));
			}

			/* A second base folder always contain only one '$', such as "1$4" "1$5".... */
			if ( strrchr(result[i], '$') ==  strchr(result[i], '$') )
			{
				continue;
			}
			
			children = sql_get_int_field(db, "SELECT count(*) from OBJECTS where PARENT_ID = '%s'", result[i]);
			if( children < 0 )
			{
				//DPRINTF(E_WARN, L_SCANNER, "[%s] had been removed before\n");
				continue;

			}
			if( children < 2 )
			{
				sql_exec(db, "DELETE from OBJECTS where OBJECT_ID = '%s'", result[i]);

				while ( (ptr = strrchr(result[i], '$')) != NULL ) 
				{
					*ptr = '\0';

					/* A base folder always contain only one '$', such as "1$4" "1$5".... */
					if ( strrchr(result[i], '$') ==  strchr(result[i], '$') )
					{
						break;
					}
					
					if( sql_get_int_field(db, "SELECT count(*) from OBJECTS where PARENT_ID = '%s'",
											result[i]) == 0 )
					{
						sql_exec(db, "DELETE from OBJECTS where OBJECT_ID = '%s'", result[i]);
					}
				}
			}
		}
		sqlite3_free_table(result);
	}
	
	/* Now delete the caption*/
	sql_exec(db, "DELETE from CAPTIONS where ID = %lld", detailID);

	/* Now delete the BOOKMARKS*/
	sql_exec(db, "DELETE from BOOKMARKS where ID = %lld", detailID);

	//DPRINTF(E_WARN, L_SCANNER, "going to remove albumArt of file: %s\n", filePath);
	/* Now delete the album_art */
	update_remvAlbumArtOfFile(detailID);

	/* Now delete the actual objects */
	sql_exec(db, "DELETE from DETAILS where ID = %lld", detailID);
	sql_exec(db, "DELETE from OBJECTS where DETAIL_ID = %lld", detailID);

	scan_decreaseFileNumber();

	return 0;

}


/* 
 * fn		static int remvDisappearedDir(const char *dirPath)
 * brief	Remove directory.
 * details	
 *
 * param[in]	dirPath		The path of directory.
 *
 * return	Return 0 when remove success, or else return -1. 
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedDir(const char *dirPath)
{
	char * sql;
	char **result;
	int64_t detailID = 0;
	int rows = 0;
	int children = 0;

	/* Invalidate the scanner cache so we don't insert files into non-existent containers */
	valid_cache = 0;

	rows = sql_get_int_field(db, "SELECT count(*) from WATCHDIRS where PATH = '%q'", dirPath);
	if (rows > 0)
	{
		sql_exec(db, "DELETE from WATCHDIRS where PATH = '%q'", dirPath);
		g_watchDirsNum--;
	}

	sql = sqlite3_mprintf("SELECT OBJECT_ID, DETAIL_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
										"where PATH = '%q' and REF_ID is NULL", dirPath);

	if ( (SQLITE_OK == sql_get_table(db, sql, &result, &rows, NULL)) && rows )
	{
		children = sql_get_int_field(db, "SELECT count(*) from OBJECTS where PARENT_ID = '%s'", 
										result[2]);
		if (children > 0)
		{
			DPRINTF(E_WARN, L_SCANNER, "This directory is not empty: %s [%d]\n", dirPath, children);
			//sqlite3_free_table(result);
			//sqlite3_free(sql);
			//return 0;
		}

		detailID = strtoll(result[3], NULL, 10);

		//DPRINTF(E_WARN, L_SCANNER, "going to remove albumArt of dir: %s\n", dirPath);
		/* Remove albumArt of empty directory. */
		remvAlbumArtOfDir(detailID);
		
		if (SQLITE_OK != sql_exec(db, "DELETE from DETAILS where ID = %lld; "
							"DELETE from OBJECTS where DETAIL_ID = %lld", detailID, detailID))
		{
			DPRINTF(E_ERROR, L_SCANNER, "DELETE from db error(detailID = %lld)\n", detailID);
		}
		scan_decreaseFileNumber();
		
		sqlite3_free_table(result);
	}
	else
	{
		DPRINTF(E_ERROR, L_SCANNER, "sql_get_table Error in remvEmptyDirs\n");
		sqlite3_free(sql);
		sqlite3_free_table(result);
		return -1;
	}

	sqlite3_free(sql);

	//DPRINTF(E_WARN, L_SCANNER, "parentID: %s\n", parentID);

	return 0;
}


#if 0
/* 
 * fn		static int remvDisappearedDir(const char *dirPath)
 * brief	Remove disappeared directory from database.
 * details	
 *
 * param[in]	dirPath	 the disappeared directory path.
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedDirAndFiles(const char *dirPath)
{
	char * sql = NULL;
	char **result = NULL;
	char *classType = NULL; 
	char *fullPath = NULL;
	int64_t detailIDParent = 0;
	int64_t detailIDChild = 0;
	int rows = 0, i = 0;
	char objID[PATH_MAX];

	/* Invalidate the scanner cache so we don't insert files into non-existent containers */
	valid_cache = 0;

	sql = sqlite3_mprintf("SELECT OBJECT_ID, DETAIL_ID from OBJECTS o left join DETAILS d "
							"on (d.ID = o.DETAIL_ID) where PATH = '%q' and REF_ID is NULL", dirPath);
	if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) && rows )
	{
		strncpyt(objID, result[2], sizeof(objID));
		detailIDParent = strtoll(result[3], NULL, 10);
	}
	else
	{
		sqlite3_free(sql);
		sqlite3_free_table(result);
		DPRINTF(E_WARN, L_SCANNER, "Cannot find directory in database: %s\n", dirPath);
		sql_exec(db, "DELETE from WATCHDIRS where PATH = '%q'", dirPath);
		g_watchDirsNum--;
		return -1;
	}
	
	sqlite3_free(sql);
	sqlite3_free_table(result);
	
	sql = sqlite3_mprintf("SELECT o.CLASS, d.ID, d.PATH from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID) "
		                   "where PARENT_ID = '%q'", objID);

	if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) )
	{	
		for(i=1; i <= rows; i++)
		{
			classType = result[3*i];
			detailIDChild = strtoll(result[3*i + 1], NULL, 10);
			fullPath = result[3*i + 2];
			
			if (strcmp(classType, "container.storageFolder") == 0)/* Remove directory */
			{
				DPRINTF(E_DEBUG, L_SCANNER, "%s is a folder, remove later\n", fullPath);
				continue;
			}
			else/* Remove file.*/
			{	
				remvDisappearedFile(fullPath, detailIDChild);			
			}
		}
	}
	
	sqlite3_free(sql);
	sqlite3_free_table(result);

	//DPRINTF(E_WARN, L_SCANNER, "going to remove albumArt of dir: %s\n", dirPath);
	remvAlbumArtOfDir(detailIDParent);

	sql_exec(db, "DELETE from WATCHDIRS where PATH = '%q'", dirPath);
	g_watchDirsNum--;

	if (SQLITE_OK != sql_exec(db, "DELETE from DETAILS where ID = %lld; "
			"DELETE from OBJECTS where DETAIL_ID = %lld", detailIDParent, detailIDParent))
	{
		DPRINTF(E_ERROR, L_SCANNER, "DELETE from db error: %s[%lld]\n", dirPath, detailIDParent);
		return -1;
	}
	scan_decreaseFileNumber();
	
	return 0;
}

#endif

/* 
 * fn		static int remvDisappearedContent(const char *parentPath, const char *parentID)
 * brief	Find disappeared content under parent directory and Remove them from sqlite database.
 * details	
 *
 * param[in]	parentPath	parent directory path of the disappeared content.
 * param[in]	parentID	parent directory ID of the disappeared content.
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int remvDisappearedContent(const char *parentPath, const char *parentID)
{
	char **result = NULL;
	char *sql = NULL;
	char * fullPath = NULL;
	char * classType = NULL;
	char parentIDFull[PATH_MAX];
	int rows = 0, columns = 0;
	int64_t detailID = 0;
	int i = 0;

	if (!parentPath)
	{
		DPRINTF(E_ERROR, L_SCANNER, "Error in updateRemoveFile: input parameter is null!\n");
		return -1;
	}
	
	snprintf(parentIDFull, sizeof(parentIDFull), "%s%s", BROWSEDIR_ID, (parentID ? parentID:""));	
	
	sql = sqlite3_mprintf("SELECT d.PATH, o.CLASS, o.DETAIL_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
										"where PARENT_ID = '%q' and REF_ID is NULL", parentIDFull);
	sql_get_table(db, sql, &result, &rows, &columns);
	for (i = 1; i <= rows; i++)
	{
		fullPath = result[3*i];
		
		if (!access(fullPath, F_OK))
		{
			continue;
		}
		else
		{
			classType =  result[3*i + 1];
			
			if (strcmp(classType, "container.storageFolder") == 0)/* Remove directory */
			{
				//remvDisappearedDirAndFiles(fullPath);
				update_remvFolder(fullPath);
			}
			else/* Remove file.  */
			{	
				detailID = strtoll(result[3*i + 2], NULL, 10);
				remvDisappearedFile(fullPath, detailID);
				
			}
		}
	}

	sqlite3_free(sql);
	sqlite3_free_table(result);

	return 0;
}

/* 
 * fn		static int insertParentDirs(const char *subDirPath)
 * brief	Insert directory and all necessary parents.
 * details	Find change in a empty directory, so need to insert it and all necessary parent dir.
 *
 * param[in]	
 * param[out]	
 *
 * return	The num of parent dir been inserted.
 * retval	
 *
 * note		Here do not limit file_number, because if file_number achieve the limit, then would 
 *			insert nothing when update_scanDirectory. so the directory would be treated as empty folder
 *			which would be removed by remvEmptyDirs. Written by zengdongbiao, 8Feb15.
 */
static int insertParentDirs(const char *subDirPath)
{
	char lastDir[PATH_MAX];
	char pathBuf[PATH_MAX];
	char baseName[PATH_MAX];
	char *baseCopy = NULL;
	char *parentBuf = NULL;
	char *id = NULL;
	int depth = 1;
	struct media_dir_s * media_path = media_dirs;
	char shareFolderPath[PATH_MAX];
	
	/* Find the parentID.  If it's not found, create all necessary parents. */
	baseCopy = baseName;

	/* Find the share folder which the directory belong to*/
	while(media_path)
	{
		if( strncmp(subDirPath, media_path->path, strlen(media_path->path)) == 0 )
		{
			strncpyt(shareFolderPath, media_path->path, sizeof(shareFolderPath));
			break;
		}
		media_path = media_path->next;
	}
	
	/* subDirPath = /sda1/empty1/empty2/empty3
	 * parentDir = /sda1/empty1/empty2
	 * because subDirPath not been insert, so start from /sda1/empty1/empty2
	 */
	while(depth)
	{
		depth = 0;
		strcpy(pathBuf, subDirPath);
		parentBuf = dirname(pathBuf);

		while (strcmp(parentBuf, "/") != 0)
		{
			/* Share folder have not been inserted to OBJECTS */
			if (strncmp(parentBuf, shareFolderPath, strlen(parentBuf)) == 0)
			{
				id = sqlite3_mprintf("%s", BROWSEDIR_ID);
			}
			else
			{
				id = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
				                            " where d.PATH = '%q' and REF_ID is NULL", parentBuf);
			}
			if ( id )
			{
				if( !depth )
					break;
				
				/* Insert newly-found directory */
				strcpy(baseName, lastDir);
				baseCopy = basename(baseName);
				insert_directory(baseCopy, lastDir, BROWSEDIR_ID, id+2, get_next_available_id("OBJECTS", id));
				scan_increaseFileNumber();
				sqlite3_free(id);
				break;
			}
			depth++;
			strcpy(lastDir, parentBuf);
			parentBuf = dirname(parentBuf);
		}

		/* 出现了意想不到的情况，将此目录放在BROWSEDIR_ID下  */
		if( strcmp(parentBuf, "/") == 0 )
		{
			id = sqlite3_mprintf("%s", BROWSEDIR_ID);
			depth = 0;
			DPRINTF(E_ERROR, L_SCANNER, "Some unexpect thing happend in updateInsertParentDir [%s]\n",
											parentBuf);
			break;
		}
	}

	if (depth == 0)
	{
		memset(pathBuf, 0, sizeof(pathBuf));
		strncpyt(pathBuf, subDirPath, sizeof(pathBuf));
		//DPRINTF(E_DEBUG, L_SCANNER, "Insert parent directory: %s\n", subDirPath);
		insert_directory(basename(pathBuf), subDirPath, BROWSEDIR_ID, id+2, get_next_available_id("OBJECTS", id));
		scan_increaseFileNumber();
		sqlite3_free(id);
	}

	return 0;
}


/* 
 * fn		static int checkWatchDirs(void)
 * brief	Check watch directorys, find if any directory diappeared or need to be update.
 * details		
 *
 * return	Return 0.
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int checkWatchDirs(void)
{
	char **result = NULL;
	char *dirPath = NULL;
	char *parentID = NULL;
	char *parentIDFull = NULL;
	long timestamp = 0;
	int rows = 0, columns = 0;
	int i = 0;

	struct stat fileStat;
	
	sql_get_table(db, "SELECT * from WATCHDIRS", &result, &rows, &columns);
	//for (i = 1; i <= rows; i++)
	for (i = rows; i >= 1; i--)	
	{	
		/*added by LY to control the scanning action to stop or start*/
		if (0 == get_allow_scanning_value())
		{
			break;
		}
		/*end added by LY*/
		
#if !USE_SCANNER_THREAD
		if( quitting )
		{
			break;
		}
#endif

		dirPath = result[2*i];
		timestamp = strtol(result[2*i+1], NULL, 10);
		
		if ( stat(dirPath, &fileStat) != 0 )
		{
			DPRINTF(E_DEBUG, L_METADATA, "%s removed\n", dirPath);
			/* Remove directory that do not exist  */
			//remvDisappearedDirAndFiles(dirPath);
			update_remvFolder(dirPath);
			continue;
		}
		else
		{
			//DPRINTF(E_WARN, L_SCANNER, "%s\n", dirPath);
			//DPRINTF(E_WARN, L_SCANNER, "mtime: %s", ctime(&(fileStat.st_mtime)));
			//DPRINTF(E_WARN, L_SCANNER, "ctime: %s", ctime(&(fileStat.st_ctime)));
			//DPRINTF(E_WARN, L_SCANNER, "atime: %s", ctime(&(fileStat.st_atime)));
			//DPRINTF(E_WARN, L_SCANNER, "timestamp: %s", ctime(&(timestamp)));
			if (timestamp != fileStat.st_mtime)
			{
				DPRINTF(E_WARN, L_SCANNER, "%s changed!\n", dirPath);
				//DPRINTF(E_WARN, L_SCANNER, "mtime: %s", ctime(&(fileStat.st_mtime)));
				//DPRINTF(E_WARN, L_SCANNER, "timestamp: %s", ctime(&(timestamp)));
				if (SQLITE_OK != sql_exec(db, "UPDATE WATCHDIRS set TIMESTAMP = %ld where PATH = %Q", 
							fileStat.st_mtime, dirPath))
				{
					DPRINTF(E_ERROR, L_METADATA, "UPDATE WATCHDIRS error: %s\n", dirPath);
					continue;
				}

				/* Find the parentID.  If it's not found, create all necessary parents.  */
				parentIDFull = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
	                           						 " where d.PATH = '%q' and REF_ID is NULL", dirPath);
				if( !parentIDFull )
				{
				    /*  Insert all necessary parent dirs. */
					insertParentDirs(dirPath);
					parentIDFull = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
	                           						 " where d.PATH = '%q' and REF_ID is NULL", dirPath);
					if (!parentIDFull)
					{
						DPRINTF(E_ERROR, L_SCANNER, "Create ParentDir error: %s\n", dirPath);
						continue;
					}
				}
				parentID = parentIDFull + 2;
				update_scanDirectory(dirPath, parentID, ALL_MEDIA);
				
				sqlite3_free(parentIDFull);
			}else
			{
				continue;
			}
		}
			
	}
	
	sqlite3_free_table(result);

	return 0;
}


/* 
 * fn		static int checkPlaylists(void)
 * brief	Check playlists and remove disappeared playlist from database.
 * details
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int checkPlaylists(void)
{
	char **result = NULL;
	char *playlistPath = NULL;
	int64_t playlistID = 0;
	int rows = 0, columns = 0;
	int i = 0;
	
	sql_get_table(db, "SELECT ID, PATH from PLAYLISTS", &result, &rows, &columns);
	for (i = 1; i <= rows; i++)	
	{	
		/*added by LY to control the scanning action to stop or start*/
		if (0 == get_allow_scanning_value())
		{
			break;
		}
		/*end added by LY*/
		
#if !USE_SCANNER_THREAD
		if( quitting )
		{
			break;
		}
#endif

		playlistPath = result[2*i + 1];
		if (!access(playlistPath, F_OK))
		{
			continue;
		}
		playlistID = strtoll(result[2*i], NULL, 10);

		//DPRINTF(E_WARN, L_INOTIFY, "delete playlist from DB: %s\n", playlistPath);
		sql_exec(db, "DELETE from PLAYLISTS where ID = %lld", playlistID);
		sql_exec(db, "DELETE from DETAILS where ID ="
		             " (SELECT DETAIL_ID from OBJECTS where OBJECT_ID = '%s$%llX')",
		         MUSIC_PLIST_ID, playlistID);
		sql_exec(db, "DELETE from OBJECTS where OBJECT_ID = '%s$%llX' or PARENT_ID = '%s$%llX'",
		         MUSIC_PLIST_ID, playlistID, MUSIC_PLIST_ID, playlistID);

	}

	sqlite3_free_table(result);

	return 0;
}

/* 
 * fn		static int checkCaptions(void)
 * brief	Check captions and remove disappeared caption from database.
 * details	
 *
 * return	
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
static int checkCaptions(void)
{
	char **result = NULL;
	char *captionPath = NULL;
	char *captionID = NULL;
	int rows = 0, columns = 0;
	int i = 0;
	
	sql_get_table(db, "SELECT * from CAPTIONS", &result, &rows, &columns);
	for (i=1; i <= rows; i++)
	{
		/*added by LY to control the scanning action to stop or start*/
		if (0 == get_allow_scanning_value())
		{
			break;
		}
		/*end added by LY*/
		
#if !USE_SCANNER_THREAD
		if( quitting )
		{
			break;
		}
#endif

		captionPath = result[2*i + 1];
		if (!access(captionPath, F_OK))
		{
			continue;
		}

		captionID = result[2*i];
		//DPRINTF(E_WARN, L_SCANNER, "caption: [%s]%s dissappeared\n", captionID, captionPath);		
		
		if (SQLITE_OK != sql_exec(db, "DELETE from CAPTIONS where ID = '%q'", captionID))
		{
			DPRINTF(E_ERROR, L_SCANNER, "DELETE from CAPTIONS error: [%s]%s\n",
										captionID, captionPath);		
		}
	}
	sqlite3_free_table(result);

	return 0;
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
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
int update_checkFilesytem(const char *path)
{
	struct statfs diskInfo;
	
	if (path == NULL)
	{
		return -1;
	}

	statfs(path, &diskInfo);

	if (diskInfo.f_type == NTFS_FILESYSTEM_TYPE)
	{
		DPRINTF(E_WARN, L_GENERAL, "filesystem: NTFS(0X%X), %s\n", diskInfo.f_type, path);
		return 0;
	}

	DPRINTF(E_WARN, L_GENERAL, "filesystem: not NTFS(0X%X), %s\n", diskInfo.f_type, path);
	
	return -1;
}


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
int update_remvFolder(const char *dirPath)
{
	char * sql = NULL;
	char **result;
	int64_t detailID = 0;
	int64_t playlistID = 0;
	int rows, i, ret = 1;
	char *classType = NULL; 
	char *fullPath = NULL;

	/* Invalidate the scanner cache so we don't insert files into non-existent containers */
	valid_cache = 0;

	//DPRINTF(E_WARN, L_SCANNER, "going to remove directory[%s] from DB\n", dirPath);

	sql = sqlite3_mprintf("SELECT ID from DETAILS where PATH = '%q' or (PATH > '%q/' and PATH <= '%q/%c')", 
							dirPath, dirPath, dirPath, 0xFF);
	if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) )
	{
		if( rows )
		{
			for( i = rows; i >= 1; i--)
			//for( i=1; i <= rows; i++ )
			{
				detailID = strtoll(result[i], NULL, 10);

				classType = sql_get_text_field(db, "SELECT CLASS from OBJECTS where "
												"DETAIL_ID = %lld and REF_ID is NULL", detailID);
				if (classType != NULL)
				{
					fullPath = sql_get_text_field(db, "SELECT PATH from DETAILS where ID = %lld", detailID);
					if (fullPath !=	NULL)
					{
						if (strcmp(classType, "container.storageFolder") == 0)/* Remove directory */
						{
							//DPRINTF(E_WARN, L_SCANNER, "going to remove subdir[%s] from DB\n", fullPath);
							remvDisappearedDir(fullPath);
						}
						else/* Remove file.*/
						{	
							//DPRINTF(E_WARN, L_SCANNER, "going to remove subFile[%s] from DB\n", fullPath);
							remvDisappearedFile(fullPath, detailID);			
						}
					}
					sqlite3_free(fullPath);
					sqlite3_free(classType);
				}			
						
				sql_exec(db, "DELETE from DETAILS where ID = %lld", detailID);
				sql_exec(db, "DELETE from OBJECTS where DETAIL_ID = %lld", detailID);

			}
			ret = 0;
		}
		sqlite3_free_table(result);
	}
	sqlite3_free(sql);

	/* Delete playlist under folder.  */
	sql = sqlite3_mprintf("SELECT ID from PLAYLISTS where (PATH > '%q/' and PATH <= '%q/%c')", 
							dirPath, dirPath, 0xFF);
	if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) )
	{
		if( rows )
		{
			for( i=1; i <= rows; i++ )
			{
				playlistID = strtoll(result[i], NULL, 10);

				//DPRINTF(E_WARN, L_INOTIFY, "delete playlist from DB: %s\n", playlistPath);
				sql_exec(db, "DELETE from PLAYLISTS where ID = %lld", playlistID);
				sql_exec(db, "DELETE from DETAILS where ID ="
				             " (SELECT DETAIL_ID from OBJECTS where OBJECT_ID = '%s$%llX')",
				         MUSIC_PLIST_ID, playlistID);
				sql_exec(db, "DELETE from OBJECTS where OBJECT_ID = '%s$%llX' or PARENT_ID = '%s$%llX'",
				         MUSIC_PLIST_ID, playlistID, MUSIC_PLIST_ID, playlistID);
			}
		}
		sqlite3_free_table(result);
	}
	sqlite3_free(sql);
	
	/* Clean up any album art entries in the deleted directory */
	sql_exec(db, "DELETE from ALBUM_ART where (PATH > '%q/' and PATH <= '%q/%c')", dirPath, dirPath, 0xFF);

	return ret;
}

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
int update_remvShareFolder(const char *folderPath)
{	
	update_selectFileAndWatchDirNum();

	/* update_remvFolder删除共享条目时，g_scan_fileNumber减1，但共享条目未纳入计数，因此需要加1. */
	update_remvFolder(folderPath);
	scan_increaseFileNumber();
	
	if (SQLITE_OK != sql_exec(db, "DELETE from SETTINGS where KEY = 'media_dir' and VALUE = %Q", folderPath))
	{
		DPRINTF(E_ERROR, L_GENERAL, "Delete %s from SETTINGS error: %s\n", folderPath, strerror(errno));
	}
	
	update_storeFileAndWatchDirNum(0);
}



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
void update_scanDirectory(const char *dir, const char *parent, media_types dir_types)
{
	int startID=0;
	char parent_id[PATH_MAX];
	char parentIDFull[PATH_MAX];/* whole parent ID  */
	char full_path[PATH_MAX];
	char *name = NULL;
	enum file_types type;

	DIR *pDir = NULL;
	struct dirent *pResult = NULL;
	struct dirent entryItem;
	int ret = -1;
	/* to deal with empty dir, add by zengdongbiao, 25Dec2014*/
	
	/*use readdir_r instead of scandir, because you cannot predict the number of files in one directory, 
	 *and use scandir may casue memory problems */
	DPRINTF(E_WARN, L_SCANNER, _("Updating %s\n"), dir);

	/* Remove disappeared file from database */
	remvDisappearedContent(dir, parent);

	/*to restrict total file number, stop scanner if file number reaches the limit*/
	if (scan_getFileNumber() >= MAX_FILE_NUMBER)
	{
		return;
	}
	/*end added by LY*/

	pDir = opendir(dir);
	if (NULL == pDir)
	{
		DPRINTF(E_ERROR, L_SCANNER, _("Scanning dir NULL\n"));
		return;
	}

	while (!readdir_r(pDir, &entryItem, &pResult) && (NULL != pResult))
	{
		/*added by LY to control the scanning action to stop or start*/
		if (0 == get_allow_scanning_value())
		{
			break;
		}
		/*end added by LY*/

#if !USE_SCANNER_THREAD
		if( quitting )
			break;
#endif
		/*continue if the current entry not matched */
		if (scan_mediaTypesIsCorrect(dir_types, &entryItem) == 0)
		{
			/* Check if any caption added in the directory.  */
			if (isCaption(entryItem.d_name))
			{
				sprintf(full_path, "%s/%s", dir, entryItem.d_name);
				
				if( sql_get_int_field(db, "SELECT ID from CAPTIONS where PATH = '%q'", full_path) > 0 )
				{
					DPRINTF(E_DEBUG, L_METADATA, "Caption %s already exist\n", full_path);
				}
				else 
				{		
					check_for_captions(full_path, 0);
				}
			}
			continue;
		}

		/*deal with each matched entry in the current directory, including both files and dirs*/
		type = TYPE_UNKNOWN;
		sprintf(full_path, "%s/%s", dir, entryItem.d_name);
		name = escape_tag(entryItem.d_name, 1);

		if( sql_get_int_field(db, "SELECT ID from DETAILS where PATH = '%q'", full_path) > 0 )
		{
			//DPRINTF(E_DEBUG, L_METADATA, "%s already exist\n", full_path);
			continue;
		}
		
		/* Avoid repeat inserting same playlist.*/
		if (is_playlist(entryItem.d_name))
		{
			if( sql_get_int_field(db, "SELECT ID from PLAYLISTS where PATH = '%q'", full_path) > 0 )
			{
				//DPRINTF(E_DEBUG, L_METADATA, "playlist %s already exist\n", full_path);
				continue;
			}
		}
		
		if( !parent )
		{
			startID = get_next_available_id("OBJECTS", BROWSEDIR_ID);
		}else
		{
			snprintf(parentIDFull, sizeof(parentIDFull), "%s%s", BROWSEDIR_ID, (parent ? parent:""));
			startID = get_next_available_id("OBJECTS", parentIDFull);
		}
		
		/* 跳过链接文件，否则，如果存在一个指向上层目录的链接，会一直循环, add by zengdongbiao. */
		if (scan_isLink(full_path) != 0)
		{
			DPRINTF(E_WARN, L_SCANNER, "skip link file (%s)\n", full_path);
			continue;
		}
		
		if ( is_dir(&entryItem) == 1 )
		{
			/* Excluding RECYCLE diretory, add by zengdongbiao, 27Apr15.  */
			if (scan_dirIsRecycle(name))
			{
				//DPRINTF(E_WARN, L_SCANNER, "do not insert directory: %s\n", full_path);
				continue;
			}
			/* End add by zengdongbiao.  */
			
			type = TYPE_DIR;

		}
		else if( is_reg(&entryItem) == 1 )	
		{
			type = TYPE_FILE;
		}
		else
		{
			type = resolve_unknown_type(full_path, dir_types);
		}
		
		if( (type == TYPE_DIR) && (access(full_path, R_OK|X_OK) == 0) )
		{
			/* Empty folders have been inserted to WATCHDIRS, so avoid to insert again. */
			//if (sql_get_int_field(db, "SELECT count(*) from WATCHDIRS where PATH = '%q'",
			//						full_path) <= 0)
			//{
				/*  Add watch directory for DB updating. */
				if ( update_addWatchDir(full_path) < 0 )
				{
					continue;
				}
				
			//}
			//else
			//{
				/* If the directory have been in WATCHDIRS, we has updated the directory when
				 * check watch directorys.
				 */
			//	continue;
			//}
				
			insert_directory(name, full_path, BROWSEDIR_ID, (parent ? parent:""), startID);

			scan_increaseFileNumber();
			
			sprintf(parent_id, "%s$%X", (parent ? parent:""), startID);
			
			ScanDirectory(full_path, parent_id, dir_types);
		}
		else if( type == TYPE_FILE && (access(full_path, R_OK) == 0) )
		{
			if( insert_file(name, full_path, (parent ? parent:""), startID, dir_types) == 0 )
			{
				scan_increaseFileNumber();
			}
		}
	
		if (scan_getFileNumber() >= MAX_FILE_NUMBER)
		{
			/*stop scanner*/
			break;
		}
	}
	
	
	/* close dir, add by liweijie, 2014-06-27 */
	if (NULL != pDir)
	{
		ret = closedir(pDir);
		if (-1 == ret)
		{
			DPRINTF(E_ERROR, L_SCANNER, _("Close Dir fail, error: %d!\n"), errno);
		}
	}
	DPRINTF(E_INFO, L_SCANNER, _("Close Dir success !\n"));
	
}


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
unsigned int update_setWatchDirsNum(unsigned int watchDirsNum)
{
	g_watchDirsNum = watchDirsNum;
	
	return g_watchDirsNum;
}


/* 
 * fn		unsigned int update_getWatchDirsNum(void)
 * brief	Get value of g_watchDirsNum.
 * details		
 *
 * return	
 * retval	The value of g_watchDirsNum.
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
unsigned int update_getWatchDirsNum(void)
{
	return g_watchDirsNum;
}


/* 
 * fn		void update_increaseWatchDirsNum(void)
 * brief	Increase the value of g_watchDirsNum to (g_watchDirsNum + 1).
 * details	
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
void update_increaseWatchDirsNum(void)
{
	g_watchDirsNum++;
}


/* 
 * fn		void update_decreaseWatchDirsNum(void)
 * brief	Decrease the value of g_watchDirsNum to (g_watchDirsNum - 1).
 * details		
 *
 * note		Written by zengdongbiao, 9Feb15.
 */
void update_decreaseWatchDirsNum(void)
{
	g_watchDirsNum--;
}


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
int update_addWatchDir(const char *full_path)
{
	struct stat fileStat;
	int ret = 0;
	
	/* If number of watch dirs has been reach max limit, continue to next.	 */
	if (update_getWatchDirsNum() >= MAX_WATCH_DIR_MUNBER)
	{
		DPRINTF(E_WARN, L_SCANNER, "Number of watch dirs has been reach max(%d) [%s]\n",
									update_getWatchDirsNum(), full_path);
		ret = -1;
	}

	if ( stat(full_path, &fileStat) != 0 )
	{
		DPRINTF(E_ERROR, L_METADATA, "stat error: %s\n", full_path);
		ret = -1;
	}
	else
	{
	
		sql_exec(db, "INSERT into WATCHDIRS values (%Q, %ld)", full_path, fileStat.st_mtime);
		
		update_increaseWatchDirsNum();
	}

	return 0;
}


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
void update_selectFileAndWatchDirNum()
{
	int fileNum = 0;
	int watchDirsNum = 0;
	
	watchDirsNum = sql_get_int_field(db, "SELECT VALUE from SETTINGS where KEY = 'watch_dir_numbers'");
	if(watchDirsNum != -1)
	{	
		//DPRINTF(E_WARN, L_SCANNER, "File number at start is %u\n", strtol(fileNumStr, NULL, 10));
		update_setWatchDirsNum(watchDirsNum);
	}

	/* Delete watch_dir_numbers from SETTINGS and insert it at the end of update, so we know 
	 * if the last update had been completed. 
	 */
	if (SQLITE_OK != sql_exec(db, "DELETE from SETTINGS where KEY = 'watch_dir_numbers'"))
	{
		DPRINTF(E_ERROR, L_SCANNER, "Error DELETE from SETTINGS where KEY = watch_dir_numbers\n");
	}	
	
	fileNum  = sql_get_int_field(db, "SELECT VALUE from SETTINGS where KEY = 'media_file_numbers'");
	if(fileNum != -1)
	{	
		//DPRINTF(E_WARN, L_SCANNER, "File number at start is %u\n", fileNum);
		scan_setFileNumber(fileNum);
	}
	

	//DPRINTF(E_WARN, L_SCANNER, "before update, file number is %u, watch dir number is %u\n", 
							//scan_getFileNumber(), update_getWatchDirsNum());

}


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
void update_storeFileAndWatchDirNum(int rescanFlag)
{
	if (rescanFlag == 0)
	{	
		/* Store the media_file_numbers.*/
		sql_exec(db, "UPDATE SETTINGS set VALUE = %u where KEY = 'media_file_numbers'", 
					scan_getFileNumber());
	}
	else
	{
		/* Store the media_file_numbers.*/
		sql_exec(db, "INSERT into SETTINGS values (%Q, %u)", "media_file_numbers", 
				scan_getFileNumber());
	}

	/* If the scanning was stoped by shutdown minidlna, do not store the number of watch directorys.
	 * If cannot get the number of watch directorys at next check_db, will rescan. 
	 */
	sql_exec(db, "INSERT into SETTINGS values (%Q, %u)", "watch_dir_numbers", update_getWatchDirsNum());

	//DPRINTF(E_WARN, L_SCANNER, "after update, file number is %u, watch dir number is %u\n", 
							//scan_getFileNumber(), update_getWatchDirsNum());
}


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
void update_init(int rescan)
{
	if (rescan == 0)/* updating database  */
	{
		/* Check if any share folder disappeared or is not NTFS filesystem. */
		update_CheckShareFolder(0);
		
		update_selectFileAndWatchDirNum();
		DPRINTF(E_WARN, L_SCANNER, "At start, file number is %u, watch dir number is %u\n", 
								scan_getFileNumber(), update_getWatchDirsNum());

		/* Check if any directory diappeared or need to be update. */
		checkWatchDirs();

		/*  Check if any caption diappeared. */
		checkCaptions();

		/*  Check if any playlists diappeared. */
		checkPlaylists();
	}
	else /* rescanning media files.  */
	{
		scan_setFileNumber(0);
		
		update_setWatchDirsNum(0);	

		DPRINTF(E_WARN, L_SCANNER, "At start, file number is %u, watch dir number is %u\n", 
								scan_getFileNumber(), update_getWatchDirsNum());
	}
	
}


/* 
 * fn		void update_remvAlbumArtOfFile(int64_t detailID)
 * brief	Remove album_art of file.
 * details	
 *
 * param[in]	detailID	The detail ID of file.	
 *
 * note		Written by zengdongbiao, 05May15.
 */
void update_remvAlbumArtOfFile(int64_t detailID)
{
	char art_cache[PATH_MAX] = {0};
	int64_t albumArtID = 0;
	int albumArtCnt = 0;
	char *albumArtPath = NULL;

#ifdef USE_ALBUM_RESPONSE
	albumArtID = sql_get_int64_field(db, "SELECT ALBUM_ART from DETAILS where ID = %lld", detailID);
	if (albumArtID > 0)
	{
		sql_exec(db, "DELETE from ALBUM_ART where ID = %lld", albumArtID);
	}
	return;
#endif /* USE_ALBUM_RESPONSE */
	
	albumArtID = sql_get_int64_field(db, "SELECT ALBUM_ART from DETAILS where ID = %lld", detailID);
	if (albumArtID > 0)
	{	
		albumArtCnt = sql_get_int_field(db, "SELECT COUNT from ALBUM_ART where ID = '%lld'", albumArtID);
		//DPRINTF(E_WARN, L_SCANNER, "albumArtCnt: %d\n", albumArtCnt);
		if (albumArtCnt > 1)/* More than one file point to hte AlbumArt, COUNT--  */
		{
			sql_exec(db, "UPDATE ALBUM_ART set COUNT = %d where ID = %lld", albumArtCnt - 1, albumArtID);
		}
		else/* The last file point to the AlbumArt, so delete the AlbumArt */
		{
			albumArtPath = sql_get_text_field(db, "SELECT PATH from ALBUM_ART where ID = '%lld'",
														albumArtID);
			if(albumArtPath != NULL)
			{
				snprintf(art_cache, sizeof(art_cache), "%s/art_cache", db_path);
				//DPRINTF(E_WARN, L_SCANNER, "art_cache: %s, albumArtPath: %s\n", art_cache, albumArtPath);
				/* Only remove album_art created in art_cache. */
				if(strncmp(art_cache, albumArtPath, strlen(art_cache)) == 0)
				{
					remove(albumArtPath);
				}

				sqlite3_free(albumArtPath);
			}
			
			sql_exec(db, "DELETE from ALBUM_ART where ID = %lld", albumArtID);
		}
			
	}
	
}



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
int update_CheckShareFolder(int CheckOnly)
{
	struct media_dir_s *media_path = NULL;
	char **result;
	int i = 0, rows = 0;
	int fileNums = 0;
	int fileNumsTotal =  0;
	
	/* Check if any share folder disappeared or is not NTFS filesystem. */
	sql_get_table(db, "SELECT VALUE from SETTINGS where KEY = 'media_dir'", &result, &rows, NULL);
	for (i=1; i <= rows; i++)
	{
		/* if the filesystem is not NTFS, remove the share media directory from DB. */
		if ( update_checkFilesytem(result[i]) < 0 )
		{
			DPRINTF(E_WARN, L_GENERAL, "filesystem is not ntfs: %s\n", result[i]);

			if (CheckOnly > 0)
			{
				fileNums += sql_get_int_field(db, "SELECT count(*) from DETAILS where PATH = '%q' "
													"or (PATH > '%q/' and PATH <= '%q/%c')", 
													result[i], result[i], result[i], 0xFF);
				/* 共享条目本身未纳入计数 */
				fileNums--;	
			}
			else
			{
				update_remvShareFolder(result[i]);
			}
			
			continue;
		}

		media_path = media_dirs;
		while (media_path)
		{
			if (strcmp(result[i], media_path->path) == 0)
			{
				break;
			}
			media_path = media_path->next;
		}
		
		if (!media_path)
		{
			DPRINTF(E_WARN, L_GENERAL, "share media dirs disappeared: %s\n", result[i]);

			if (CheckOnly > 0)
			{
				fileNums += sql_get_int_field(db, "SELECT count(*) from DETAILS where PATH = '%q' "
													"or (PATH > '%q/' and PATH <= '%q/%c')", 
													result[i], result[i], result[i], 0xFF);			
				/* 共享条目本身未纳入计数 */
				fileNums--;	
			}
			else
			{
				update_remvShareFolder(result[i]);
			}
		}
	}
	sqlite3_free_table(result);

	if (CheckOnly)
	{
		fileNumsTotal = sql_get_int_field(db, "SELECT VALUE from SETTINGS where KEY = 'media_file_numbers'");
		
		if (fileNums < 0 || fileNumsTotal < 0)
		{
			DPRINTF(E_ERROR, L_GENERAL, "fileNums: %d, fileNumsTotal: %d\n", 
										fileNums, fileNumsTotal);
			return -1;
		}

		if (fileNums >= (RESCAN_RATE * fileNumsTotal))
		{
			//DPRINTF(E_WARN, L_GENERAL, "fileNums: %d, fileNumsTotal: %d, rescan is faster than remove.\n",
			//							fileNums, fileNumsTotal);
			return -1;
		}
	}

	return 0;
}



#endif

