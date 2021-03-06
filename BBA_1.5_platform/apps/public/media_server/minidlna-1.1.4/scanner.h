/* Media file scanner
 *
 * Project : minidlna
 * Website : http://sourceforge.net/projects/minidlna/
 * Author  : Justin Maggard
 *
 * MiniDLNA media server
 * Copyright (C) 2008-2009  Justin Maggard
 *
 * This file is part of MiniDLNA.
 *
 * MiniDLNA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * MiniDLNA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MiniDLNA. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __SCANNER_H__
#define __SCANNER_H__


#include <dirent.h>


/* Try to be generally PlaysForSure compatible by using similar IDs */
#define BROWSEDIR_ID		"64"

#define MUSIC_ID		"1"
#define MUSIC_ALL_ID		"1$4"
#define MUSIC_GENRE_ID		"1$5"
#define MUSIC_ARTIST_ID		"1$6"
#define MUSIC_ALBUM_ID		"1$7"
#define MUSIC_PLIST_ID		"1$F"
#define MUSIC_DIR_ID		"1$14"
#define MUSIC_CONTRIB_ARTIST_ID	"1$100"
#define MUSIC_ALBUM_ARTIST_ID	"1$107"
#define MUSIC_COMPOSER_ID	"1$108"
#define MUSIC_RATING_ID		"1$101"

#define VIDEO_ID		"2"
#define VIDEO_ALL_ID		"2$8"
#define VIDEO_GENRE_ID		"2$9"
#define VIDEO_ACTOR_ID		"2$A"
#define VIDEO_SERIES_ID		"2$E"
#define VIDEO_PLIST_ID		"2$10"
#define VIDEO_DIR_ID		"2$15"
#define VIDEO_RATING_ID		"2$200"

#define IMAGE_ID		"3"
#define IMAGE_ALL_ID		"3$B"
#define IMAGE_DATE_ID		"3$C"
#define IMAGE_ALBUM_ID		"3$D"
#define IMAGE_CAMERA_ID		"3$D2" // PlaysForSure == Keyword
#define IMAGE_PLIST_ID		"3$11"
#define IMAGE_DIR_ID		"3$16"
#define IMAGE_RATING_ID		"3$300"

/*added by LY to restrict the number of total entries, both file and directory count*/
#ifdef LITE_MINIDLNA
#define MAX_FILE_NUMBER	5000
#else
#define MAX_FILE_NUMBER	10000
#endif
/*end added by LY*/

/*added by LY in 2014.06.19*/
#define FLAG_IS_FILE_OR_DIR		1
#define FLAG_NO_FILE_OR_DIR	0
/*end added by LY*/

extern int valid_cache;

int
is_video(const char *file);

int
is_audio(const char *file);

int
is_image(const char *file);

int64_t
get_next_available_id(const char *table, const char *parentID);

int64_t
insert_directory(const char *name, const char *path, const char *base, const char *parentID, int objectID);

int
insert_file(char *name, const char *path, const char *parentID, int object, media_types dir_types);

int
CreateDatabase(void);

void
ScanDirectory(const char *dir, const char *parent, media_types dir_types);

void *
start_scanner(void *arg);

/*added by LY*/
void set_scanning_value(short int arg);
short int get_scanning_value();

void set_allow_scanning_value(short int arg);
short int get_allow_scanning_value();
/*end added by LY*/

/* 
 * fn		unsigned int scan_getFileNumber()
 * brief	Get value of files number.
 *
 * return	g_scan_fileNumber
 * retval	
 *
 * note		
 */
unsigned int scan_getFileNumber();
/* 
 * fn		void scan_setFileNumber(unsigned int number)
 * brief	Set value of files number.
 * details	g_scan_fileNumber = number;
 *
 * param[in]		
 *
 * note		
 */
void scan_setFileNumber(unsigned int number);

/* 
 * fn		void scan_increaseFileNumber()
 * brief	Increase file numbers.
 * details	g_scan_fileNumber++;
 *
 * return	void
 *
 * note		
 */
void scan_increaseFileNumber();

/* 
 * fn		void scan_decreaseFileNumber()
 * brief	Decrease file numbers.
 * details	g_scan_fileNumber--;	
 *
 * return	void
 *
 * note		
 */
void scan_decreaseFileNumber();

/* 
 * fn		int scan_dirIsRecycle(const char *name)
 * brief	Check if the dir is $RECYCLE.BIN.
 * details	We would not insert the information of $RECYCLE.BIN.
 *
 * param[in]	name	the name(not full path) of directory.
 *
 * return	
 * retval	1	dir is RECYCLE.BIN
 *			0	dir is not RECYCLE.BIN
 * note		Written by zengdongbiao, 27Apr15.
 */
int scan_dirIsRecycle(const char *name);

/* To indentify link file, add by zengdongbiao.  */
int scan_isLink(const char *filePath);

#ifdef USE_UPDATE
/* 
 * fn		int scan_mediaTypesIsCorrect(media_types dirTypes, struct dirent *entryItem)
 * brief	Check media types of file/dir is correct (match with the dirTypes) or not.
 * details	
 *
 * param[in]	dirTypes	The media types of parent directory.
 * param[out]	entryItem	The dirent struct of file/dir.
 *
 * return	Return 1 when media_types is correct, otherwise return 0.
 * retval	
 *
 * note		Written by zengdongbiao, 8Feb15.
 */
int scan_mediaTypesIsCorrect(media_types dirTypes, struct dirent *entryItem);
#endif

#endif
