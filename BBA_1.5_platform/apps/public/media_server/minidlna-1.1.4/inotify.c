/* MiniDLNA media server
 * Copyright (C) 2008-2010  Justin Maggard
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
#include "config.h"

#ifdef HAVE_INOTIFY
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <poll.h>
#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#else
#include "linux/inotify.h"
#include "linux/inotify-syscalls.h"
#endif
#include "libav.h"

#include "upnpglobalvars.h"
#include "inotify.h"
#include "utils.h"
#include "sql.h"
#include "scanner.h"
#include "metadata.h"
#include "albumart.h"
#include "playlist.h"
#include "log.h"

#ifdef USE_UPDATE	
#include "updateDatabase.h"
#endif

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define DESIRED_WATCH_LIMIT 65536

#define PATH_BUF_SIZE PATH_MAX

struct watch
{
	int wd;		/* watch descriptor */
	char *path;	/* watched path */
	struct watch *next;
};

static struct watch *watches = NULL;
static struct watch *lastwatch = NULL;
static time_t next_pl_fill = 0;

char *get_path_from_wd(int wd)
{
	struct watch *w = watches;

	while( w != NULL )
	{
		if( w->wd == wd )
			return w->path;
		w = w->next;
	}

	return NULL;
}

int
add_watch(int fd, const char * path)
{
	struct watch *nw;
	int wd;

	wd = inotify_add_watch(fd, path, IN_CREATE|IN_CLOSE_WRITE|IN_DELETE|IN_MOVE);
	if( wd < 0 )
	{
		DPRINTF(E_ERROR, L_INOTIFY, "inotify_add_watch(%s) [%s]\n", path, strerror(errno));
		return -1;
	}

	nw = malloc(sizeof(struct watch));
	if( nw == NULL )
	{
		DPRINTF(E_ERROR, L_INOTIFY, "malloc() error\n");
		return -1;
	}
	nw->wd = wd;
	nw->next = NULL;
	nw->path = strdup(path);

	if( watches == NULL )
	{
		watches = nw;
	}

	if( lastwatch != NULL )
	{
		lastwatch->next = nw;
	}
	lastwatch = nw;

	return wd;
}

int
remove_watch(int fd, const char * path)
{
	struct watch *w;

	for( w = watches; w; w = w->next )
	{
		if( strcmp(path, w->path) == 0 )
			return(inotify_rm_watch(fd, w->wd));
	}

	return 1;
}

unsigned int
next_highest(unsigned int num)
{
	num |= num >> 1;
	num |= num >> 2;
	num |= num >> 4;
	num |= num >> 8;
	num |= num >> 16;
	return(++num);
}

int
inotify_create_watches(int fd)
{
	FILE * max_watches;
	unsigned int num_watches = 0, watch_limit;
	char **result;
	int i, rows = 0;
	struct media_dir_s * media_path;

	/* At begin of create watches, set /proc/sys/fs/inotify/max_user_watches to MAX_FILE_NUMBER,
	 *	Moved from the end( of inotify_create_watches) and modify by zengdongbiao, 27Apr15.
	 */
	max_watches = fopen("/proc/sys/fs/inotify/max_user_watches", "r");
	if( max_watches )
	{
		if( fscanf(max_watches, "%10u", &watch_limit) < 1 )
			watch_limit = 8192;
		fclose(max_watches);
		if( watch_limit < MAX_FILE_NUMBER )
		{
			max_watches = fopen("/proc/sys/fs/inotify/max_user_watches", "w");
			if( max_watches )
			{
				fprintf(max_watches, "%u", MAX_FILE_NUMBER);
				fclose(max_watches);
			}
			else
			{
				DPRINTF(E_WARN, L_INOTIFY, "WARNING: Inotify max_user_watches [%u] is low or close to the number of used watches [%u] "
				                        "and I do not have permission to increase this limit.  Please do so manually by "
				                        "writing a higher value into /proc/sys/fs/inotify/max_user_watches.\n", watch_limit, MAX_FILE_NUMBER);
			}
		}
	}
	else
	{
		DPRINTF(E_WARN, L_INOTIFY, "WARNING: Could not read inotify max_user_watches!  "
		                        "Hopefully it is enough to cover %u current directories plus any new ones added.\n", MAX_FILE_NUMBER);
	}
	/* End modify by zengdongbiao. */

	for( media_path = media_dirs; media_path != NULL; media_path = media_path->next )
	{
		DPRINTF(E_DEBUG, L_INOTIFY, "Add watch to %s\n", media_path->path);
		add_watch(fd, media_path->path);
		num_watches++;
	}
	sql_get_table(db, "SELECT PATH from DETAILS where MIME is NULL and PATH is not NULL", &result, &rows, NULL);
	for( i=1; i <= rows; i++ )
	{
		DPRINTF(E_DEBUG, L_INOTIFY, "Add watch to %s\n", result[i]);
		add_watch(fd, result[i]);
		num_watches++;
	}
	sqlite3_free_table(result);
		
	return rows;
}

int 
inotify_remove_watches(int fd)
{
	struct watch *w = watches;
	struct watch *last_w;
	int rm_watches = 0;

	while( w )
	{
		last_w = w;
		inotify_rm_watch(fd, w->wd);
		free(w->path);
		rm_watches++;
		w = w->next;
		free(last_w);
	}

	/* Fix the bug, add by zengdongbiao, 27Mar15  */
	watches = NULL; /* Set the head of chain to NULL.  */
	lastwatch = NULL; /* Set the tail of chain to NULL.  */
	/* End add by zengdongbiao  */

	return rm_watches;
}

int add_dir_watch(int fd, char * path, char * filename)
{
	DIR *ds;
	struct dirent *e;
	char *dir;
	char buf[PATH_MAX];
	int wd;
	int i = 0;

	if( filename )
	{
		snprintf(buf, sizeof(buf), "%s/%s", path, filename);
		dir = buf;
	}
	else
		dir = path;

	wd = add_watch(fd, dir);
	if( wd == -1 )
	{
		DPRINTF(E_ERROR, L_INOTIFY, "add_watch() [%s]\n", strerror(errno));
	}
	else
	{
		DPRINTF(E_INFO, L_INOTIFY, "Added watch to %s [%d]\n", dir, wd);
	}

	ds = opendir(dir);
	if( ds != NULL )
	{
		while( (e = readdir(ds)) )
		{
			if( strcmp(e->d_name, ".") == 0 ||
			    strcmp(e->d_name, "..") == 0 )
				continue;
			if( (e->d_type == DT_DIR) ||
			    (e->d_type == DT_UNKNOWN && resolve_unknown_type(dir, NO_MEDIA) == TYPE_DIR) )
				i += add_dir_watch(fd, dir, e->d_name);
		}
	}
	else
	{
		DPRINTF(E_ERROR, L_INOTIFY, "Opendir error! [%s]\n", strerror(errno));
	}
	closedir(ds);
	i++;

	return(i);
}

int
inotify_insert_file(char * name, const char * path)
{
	int len;
	char * last_dir;
	char * path_buf;
	char * base_name;
	char * base_copy;
	char * parent_buf = NULL;
	char * id = NULL;
	int depth = 1;
	int ts;
	media_types types = ALL_MEDIA;
	struct media_dir_s * media_path = media_dirs;
	struct stat st;

	/* Is it cover art for another file? */
	if( is_image(path) )
	{
#ifndef LITE_MINIDLNA
		update_if_album_art(path);
#endif
	}
	else if( is_caption(path) )
		check_for_captions(path, 0);


	/* Check if we're supposed to be scanning for this file type in this directory */
	while( media_path )
	{
		if( strncmp(path, media_path->path, strlen(media_path->path)) == 0 )
		{
			types = media_path->types;
			break;
		}
		media_path = media_path->next;
	}
	switch( types )
	{
		case ALL_MEDIA:
			if( !is_image(path) &&
			    !is_audio(path) &&
			    !is_video(path) &&
			    !is_playlist(path) )
				return -1;
			break;
		case TYPE_AUDIO:
			if( !is_audio(path) &&
			    !is_playlist(path) )
				return -1;
			break;
		case TYPE_AUDIO|TYPE_VIDEO:
			if( !is_audio(path) &&
			    !is_video(path) &&
			    !is_playlist(path) )
			break;
		case TYPE_AUDIO|TYPE_IMAGES:
			if( !is_image(path) &&
			    !is_audio(path) &&
			    !is_playlist(path) )
				return -1;
			break;
		case TYPE_VIDEO:
			if( !is_video(path) )
				return -1;
			break;
		case TYPE_VIDEO|TYPE_IMAGES:
			if( !is_image(path) &&
			    !is_video(path) )
				return -1;
			break;
		case TYPE_IMAGES:
			if( !is_image(path) )
				return -1;
			break;
                default:
			return -1;
			break;
	}
	
	/* If it's already in the database and hasn't been modified, skip it. */
	if( stat(path, &st) != 0 )
		return -1;

	ts = sql_get_int_field(db, "SELECT TIMESTAMP from DETAILS where PATH = '%q'", path);
	if( !ts && is_playlist(path) && (sql_get_int_field(db, "SELECT ID from PLAYLISTS where PATH = '%q'", path) > 0) )
	{
		DPRINTF(E_DEBUG, L_INOTIFY, "Re-reading modified playlist (%s).\n", path);
		inotify_remove_file(path);
		next_pl_fill = 1;
	}
	/* Modify by zengdongbiao, 02Apr15.  */	
	//else if( ts < st.st_mtime )
	else if( ts != st.st_mtime )
	{
		if( ts > 0 )
		{
			DPRINTF(E_DEBUG, L_INOTIFY, "%s is newer than the last db entry.\n", path);
			inotify_remove_file(path);
		}
	}
	/* End modify by zengdongbiao.  */

	/* If files number has been reach MAX_FILE_NUMBER, do not insert files. Add by zengdongbiao, 27Apr15.  */
	if ( scan_getFileNumber() >= MAX_FILE_NUMBER )
	{
		DPRINTF(E_WARN, L_INOTIFY, "files number has been reach max[%d], can not insert file[%s]\n",
									MAX_FILE_NUMBER, path);
		return -1;
	}

	/* Find the parentID.  If it's not found, create all necessary parents. */
	len = strlen(path)+1;
	if( !(path_buf = malloc(len)) ||
	    !(last_dir = malloc(len)) ||
	    !(base_name = malloc(len)) )
		return -1;
	base_copy = base_name;
	while( depth )
	{
		depth = 0;
		strcpy(path_buf, path);
		parent_buf = dirname(path_buf);

		do
		{
			//DEBUG DPRINTF(E_DEBUG, L_INOTIFY, "Checking %s\n", parent_buf);
			id = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
			                            " where d.PATH = '%q' and REF_ID is NULL", parent_buf);
			if( id )
			{
				if( !depth )
					break;
				DPRINTF(E_DEBUG, L_INOTIFY, "Found first known parentID: %s [%s]\n", id, parent_buf);
				/* Insert newly-found directory */
				strcpy(base_name, last_dir);
				base_copy = basename(base_name);
				insert_directory(base_copy, last_dir, BROWSEDIR_ID, id+2, get_next_available_id("OBJECTS", id));
				/* Increase the files number. Add by zengdongbiao, 27Apr15. */
				scan_increaseFileNumber();
				sqlite3_free(id);
				break;
			}
			depth++;
			strcpy(last_dir, parent_buf);
			parent_buf = dirname(parent_buf);
		}
		while( strcmp(parent_buf, "/") != 0 );

		if( strcmp(parent_buf, "/") == 0 )
		{
			id = sqlite3_mprintf("%s", BROWSEDIR_ID);
			depth = 0;
			break;
		}
		strcpy(path_buf, path);
	}
	free(last_dir);
	free(path_buf);
	free(base_name);

	if( !depth )
	{
		//DEBUG DPRINTF(E_DEBUG, L_INOTIFY, "Inserting %s\n", name);
		if (insert_file(name, path, id+2, get_next_available_id("OBJECTS", id), types) == 0)
		{
			/* Increase the files number. Add by zengdongbiao, 27Apr15. */
			scan_increaseFileNumber();
		}
		sqlite3_free(id);
		if( (is_audio(path) || is_playlist(path)) && next_pl_fill != 1 )
		{
			next_pl_fill = time(NULL) + 120; // Schedule a playlist scan for 2 minutes from now.
			//DEBUG DPRINTF(E_WARN, L_INOTIFY,  "Playlist scan scheduled for %s", ctime(&next_pl_fill));
		}
	}
	return depth;
}

int
inotify_insert_directory(int fd, char *name, const char * path)
{
	DIR * ds;
	struct dirent * e;
	char *id, *parent_buf, *esc_name;
	char path_buf[PATH_MAX];
	int wd;
	enum file_types type = TYPE_UNKNOWN;
	media_types dir_types = ALL_MEDIA;
	struct media_dir_s* media_path;
	struct stat st;

	if( access(path, R_OK|X_OK) != 0 )
	{
		DPRINTF(E_WARN, L_INOTIFY, "Could not access %s [%s]\n", path, strerror(errno));
		return -1;
	}

	/* 跳过链接文件，否则，如果存在一个指向上层目录的链接，会一直循环, add by zengdongbiao. */
	if (scan_isLink(path) != 0)
	{
		DPRINTF(E_WARN, L_SCANNER, "skip link file (%s)\n", path);
		return 0;
	}

	/* Excluding RECYCLE diretory, add by zengdongbiao, 27Apr15.  */
	if (scan_dirIsRecycle(name))
	{
		DPRINTF(E_WARN, L_INOTIFY, "do not insert directory: %s\n", path);
		return 0;
	}
	/* End add by zengdongbiao.  */
			
	if( sql_get_int_field(db, "SELECT ID from DETAILS where PATH = '%q'", path) > 0 )
	{
		DPRINTF(E_DEBUG, L_INOTIFY, "%s already exists\n", path);
		return 0;
	}

	/* If files number has been reach MAX_FILE_NUMBER, do not insert files. Add by zengdongbiao, 27Apr15.  */
	if ( scan_getFileNumber() >= MAX_FILE_NUMBER )
	{
		DPRINTF(E_WARN, L_INOTIFY, "files number has been reach max[%d], can not insert file[%s]\n",
									MAX_FILE_NUMBER, path);
		return -1;
	}

 	parent_buf = strdup(path);
	id = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
	                            " where d.PATH = '%q' and REF_ID is NULL", dirname(parent_buf));
	if( !id )
		id = sqlite3_mprintf("%s", BROWSEDIR_ID);
	insert_directory(name, path, BROWSEDIR_ID, id+2, get_next_available_id("OBJECTS", id));
	sqlite3_free(id);
	free(parent_buf);

	/* Increase the files number. Add by zengdongbiao, 27Apr15. */
	scan_increaseFileNumber();
	
#ifdef USE_UPDATE		
	/*  Add watch directory for DB updating. Add by zengdongbiao, 05May15. */
	update_addWatchDir(path);
#endif

	wd = add_watch(fd, path);
	if( wd == -1 )
	{
		DPRINTF(E_ERROR, L_INOTIFY, "add_watch() failed\n");
	}
	else
	{
		DPRINTF(E_INFO, L_INOTIFY, "Added watch to %s [%d]\n", path, wd);
	}

	media_path = media_dirs;
	while( media_path )
	{
		if( strncmp(path, media_path->path, strlen(media_path->path)) == 0 )
		{
			dir_types = media_path->types;
			break;
		}
		media_path = media_path->next;
	}

	ds = opendir(path);
	if( !ds )
	{
		DPRINTF(E_ERROR, L_INOTIFY, "opendir failed! [%s]\n", strerror(errno));
		return -1;
	}
	while( (e = readdir(ds)) )
	{
		if( e->d_name[0] == '.' )
			continue;
		esc_name = escape_tag(e->d_name, 1);
		snprintf(path_buf, sizeof(path_buf), "%s/%s", path, e->d_name);
		switch( e->d_type )
		{
			case DT_DIR:
			case DT_REG:
			case DT_LNK:
			case DT_UNKNOWN:
				type = resolve_unknown_type(path_buf, dir_types);
			default:
				break;
		}

		/* If files number has been reach MAX_FILE_NUMBER, do not insert files. Add by zengdongbiao, 27Apr15.  */
		if ( scan_getFileNumber() >= MAX_FILE_NUMBER )
		{
			DPRINTF(E_WARN, L_INOTIFY, "files number has been reach max[%d], can not insert file[%s]\n",
										MAX_FILE_NUMBER, path);
			break;
		}
		
		if( type == TYPE_DIR )
		{
			inotify_insert_directory(fd, esc_name, path_buf);
		}
		else if( type == TYPE_FILE )
		{
			if( (stat(path_buf, &st) == 0) && (st.st_blocks<<9 >= st.st_size) )
			{
				inotify_insert_file(esc_name, path_buf);
			}
		}
		free(esc_name);
	}
	closedir(ds);

	return 0;
}

int
inotify_remove_file(const char * path)
{
	char sql[128];
	char art_cache[PATH_MAX];
	char *id;
	char *ptr;
	char **result;
	int64_t detailID;
	int rows, playlist;
		
	if( is_caption(path) )
	{
		return sql_exec(db, "DELETE from CAPTIONS where PATH = '%q'", path);
	}

	/* Invalidate the scanner cache so we don't insert files into non-existent containers */
	valid_cache = 0;
	playlist = is_playlist(path);
	id = sql_get_text_field(db, "SELECT ID from %s where PATH = '%q'", playlist?"PLAYLISTS":"DETAILS", path);
	if( !id )
		return 1;
	detailID = strtoll(id, NULL, 10);
	sqlite3_free(id);
	if( playlist )
	{
		DPRINTF(E_WARN, L_INOTIFY, "delete playlist from DB: %s\n", path);
		sql_exec(db, "DELETE from PLAYLISTS where ID = %lld", detailID);
		sql_exec(db, "DELETE from DETAILS where ID ="
		             " (SELECT DETAIL_ID from OBJECTS where OBJECT_ID = '%s$%llX')",
		         MUSIC_PLIST_ID, detailID);
		sql_exec(db, "DELETE from OBJECTS where OBJECT_ID = '%s$%llX' or PARENT_ID = '%s$%llX'",
		         MUSIC_PLIST_ID, detailID, MUSIC_PLIST_ID, detailID);
	}
	else
	{
		/* Delete the parent containers if we are about to empty them. */
		snprintf(sql, sizeof(sql), "SELECT PARENT_ID from OBJECTS where DETAIL_ID = %lld"
		                           " and PARENT_ID not like '64$%%'",
		                           (long long int)detailID);
		if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) )
		{
			int i, children;
			for( i=1; i <= rows; i++ )
			{
				/* If it's a playlist item, adjust the item count of the playlist */
				if( strncmp(result[i], MUSIC_PLIST_ID, strlen(MUSIC_PLIST_ID)) == 0 )
				{
					sql_exec(db, "UPDATE PLAYLISTS set FOUND = (FOUND-1) where ID = %d",
					         atoi(strrchr(result[i], '$') + 1));
				}

				/* A second base folder always contain only one '$', such as "1$4" "1$5"...
				 * Add by zengdongbiao, 26Mar15.
				 */
				if ( strrchr(result[i], '$') ==  strchr(result[i], '$') )
				{
					/* Do not delete the second base folder.  */
					continue;
				}
				/* End add by zengdongbiao.  */

				children = sql_get_int_field(db, "SELECT count(*) from OBJECTS where PARENT_ID = '%s'", result[i]);
				if( children < 0 )
					continue;
				if( children < 2 )
				{
					sql_exec(db, "DELETE from OBJECTS where OBJECT_ID = '%s'", result[i]);

					/* Modify by zengdongbiao, 26Mar15.  */
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
					/* End modify by zengdongbiao.  */
				}
			}
			sqlite3_free_table(result);
		}
		
		/* Now delete the caption*/
		sql_exec(db, "DELETE from CAPTIONS where ID = %lld", detailID);		
		/* Now delete the BOOKMARKS*/
		sql_exec(db, "DELETE from BOOKMARKS where ID = %lld", detailID);

#ifdef USE_UPDATE	
		//DPRINTF(E_WARN, L_SCANNER, "going to remove albumArt of file: %s\n", path);
		/* Now delete the album_art */
		update_remvAlbumArtOfFile(detailID);
#endif	
		/* Now delete the actual objects */
		sql_exec(db, "DELETE from DETAILS where ID = %lld", detailID);
		sql_exec(db, "DELETE from OBJECTS where DETAIL_ID = %lld", detailID);
		
		/* Decrease the files number. Add by zengdongbiao, 27Apr15. */
		scan_decreaseFileNumber();
	}
	//snprintf(art_cache, sizeof(art_cache), "%s/art_cache%s", db_path, path);
	//remove(art_cache);

	return 0;
}


int
inotify_remove_directory(int fd, const char * path)
{
	char * sql;
	char **result;
	int64_t detailID = 0;
	int rows, i, ret = 1;

	/* Invalidate the scanner cache so we don't insert files into non-existent containers */
	valid_cache = 0;
	remove_watch(fd, path);

#ifdef USE_UPDATE
	update_remvFolder(path);

	return 0;
#else
	sql = sqlite3_mprintf("SELECT ID from DETAILS where (PATH > '%q/' and PATH <= '%q/%c')"
	                      " or PATH = '%q'", path, path, 0xFF, path);
	if( (sql_get_table(db, sql, &result, &rows, NULL) == SQLITE_OK) )
	{
		if( rows )
		{
			for( i=1; i <= rows; i++ )
			{
				detailID = strtoll(result[i], NULL, 10);
				sql_exec(db, "DELETE from DETAILS where ID = %lld", detailID);
				sql_exec(db, "DELETE from OBJECTS where DETAIL_ID = %lld", detailID);

				/* Decrease the files number. Add by zengdongbiao, 27Apr15. */
				scan_decreaseFileNumber();
			}
			ret = 0;
		}
		sqlite3_free_table(result);
	}
	sqlite3_free(sql);

	#ifndef LITE_MINIDLNA
	/* Clean up any album art entries in the deleted directory */
	sql_exec(db, "DELETE from ALBUM_ART where (PATH > '%q/' and PATH <= '%q/%c')", path, path, 0xFF);
	#endif

	return ret;
#endif
}


void *
start_inotify()
{
	struct pollfd pollfds[1];
	int timeout = 1000;
	char buffer[BUF_LEN];
	char path_buf[PATH_MAX];
	int length, i = 0;
	char * esc_name = NULL;
	struct stat st;
		
	pollfds[0].fd = inotify_init();
	pollfds[0].events = POLLIN;

	DPRINTF(E_WARN, L_SCANNER,"inotify thread start: pid=%u, tid=%u\n", 
								(unsigned int)getpid(), (unsigned int)pthread_self());

	if ( pollfds[0].fd < 0 )
		DPRINTF(E_ERROR, L_INOTIFY, "inotify_init() failed!\n");

	while( 1 == get_scanning_value())
	{
		if(inotify_quitting)
			goto quitting;
		sleep(1);
	}
	inotify_create_watches(pollfds[0].fd);
	if (setpriority(PRIO_PROCESS, 0, 19) == -1)
		DPRINTF(E_WARN, L_INOTIFY,  "Failed to reduce inotify thread priority\n");
	sqlite3_release_memory(1<<31);
	
#ifndef LITE_MINIDLNA
	av_register_all();
#endif
        
	while(!inotify_quitting)
	{
        length = poll(pollfds, 1, timeout);
		if( !length )
		{
			if( next_pl_fill && (time(NULL) >= next_pl_fill) )
			{
				fill_playlists();
				next_pl_fill = 0;
			}
			continue;
		}
		else if( length < 0 )
		{
            if( (errno == EINTR) || (errno == EAGAIN) )
                    continue;
            else
			DPRINTF(E_ERROR, L_INOTIFY, "read failed!\n");
		}
		else
		{
			length = read(pollfds[0].fd, buffer, BUF_LEN);
			buffer[BUF_LEN-1] = '\0';
		}
		
		i = 0;
		while( i < length )
		{
			struct inotify_event * event = (struct inotify_event *) &buffer[i];
			if( event->len )
			{
				if( *(event->name) == '.' )
				{
					i += EVENT_SIZE + event->len;
					continue;
				}
				esc_name = modifyString(strdup(event->name), "&", "&amp;amp;", 0);
				snprintf(path_buf, sizeof(path_buf), "%s/%s", get_path_from_wd(event->wd), event->name);
				if ( event->mask & IN_ISDIR && (event->mask & (IN_CREATE|IN_MOVED_TO)) )
				{
					DPRINTF(E_DEBUG, L_INOTIFY,  "The directory %s was %s.\n",
						path_buf, (event->mask & IN_MOVED_TO ? "moved here" : "created"));
#ifdef USE_UPDATE					
					update_selectFileAndWatchDirNum();
#endif
					inotify_insert_directory(pollfds[0].fd, esc_name, path_buf);
#ifdef USE_UPDATE	
					update_storeFileAndWatchDirNum(0);
#endif
				}
				else if ( (event->mask & (IN_CLOSE_WRITE|IN_MOVED_TO|IN_CREATE)) &&
				          (lstat(path_buf, &st) == 0) )
				{
					if( S_ISLNK(st.st_mode) )
					{
						DPRINTF(E_DEBUG, L_INOTIFY, "The symbolic link %s was %s.\n",
							path_buf, (event->mask & IN_MOVED_TO ? "moved here" : "created"));
#ifdef USE_UPDATE	
						update_selectFileAndWatchDirNum();
#endif
						if( stat(path_buf, &st) == 0 && S_ISDIR(st.st_mode) )
							inotify_insert_directory(pollfds[0].fd, esc_name, path_buf);
						else
							inotify_insert_file(esc_name, path_buf);
#ifdef USE_UPDATE	
						update_storeFileAndWatchDirNum(0);
#endif
					}
					else if( event->mask & (IN_CLOSE_WRITE|IN_MOVED_TO) && st.st_size > 0 )
					{
						if( (event->mask & IN_MOVED_TO) ||
						    (sql_get_int_field(db, "SELECT TIMESTAMP from DETAILS where PATH = '%q'", path_buf) != st.st_mtime) )
						{
							DPRINTF(E_DEBUG, L_INOTIFY, "The file %s was %s.\n",
								path_buf, (event->mask & IN_MOVED_TO ? "moved here" : "changed"));
#ifdef USE_UPDATE								
							update_selectFileAndWatchDirNum();
#endif
							inotify_insert_file(esc_name, path_buf);
#ifdef USE_UPDATE	
							update_storeFileAndWatchDirNum(0);
#endif
						}
					}
				}
				else if ( event->mask & (IN_DELETE|IN_MOVED_FROM) )
				{
					DPRINTF(E_DEBUG, L_INOTIFY, "The %s %s was %s.\n",
						(event->mask & IN_ISDIR ? "directory" : "file"),
						path_buf, (event->mask & IN_MOVED_FROM ? "moved away" : "deleted"));
#ifdef USE_UPDATE	
					update_selectFileAndWatchDirNum();
#endif
					if ( event->mask & IN_ISDIR )
					{
						inotify_remove_directory(pollfds[0].fd, path_buf);
					}
					else
					{
						inotify_remove_file(path_buf);
					}
#ifdef USE_UPDATE	
					update_storeFileAndWatchDirNum(0);
#endif
				}
				free(esc_name);
			}
			i += EVENT_SIZE + event->len;
		}
		
	}
	inotify_remove_watches(pollfds[0].fd);
quitting:
	close(pollfds[0].fd);

	return 0;
}
#endif
