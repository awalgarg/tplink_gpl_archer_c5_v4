/* MiniDLNA media server
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

#include "scanner_sqlite.h"
#include "upnpglobalvars.h"
#include "metadata.h"
#include "playlist.h"
#include "utils.h"
#include "sql.h"
#include "scanner.h"
#include "albumart.h"
#include "containers.h"
#include "log.h"
#include "minidlna.h"

#ifdef USE_UPDATE
#include "updateDatabase.h"
#endif

#if SCANDIR_CONST
typedef const struct dirent scan_filter;
#else
typedef struct dirent scan_filter;
#endif
#ifndef AV_LOG_PANIC
#define AV_LOG_PANIC AV_LOG_FATAL
#endif

int valid_cache = 0;

/* 
 * brief	The total number of media files and directory.
 */
unsigned int g_scan_fileNumber = 0;

struct virtual_item
{
	int64_t objectID;
	char parentID[64];
	char name[256];
};

/*added by LY to make scanning thread safe*/
//pthread_mutex_t g_scanning_lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t g_allow_scanning_lock = PTHREAD_MUTEX_INITIALIZER;
/*end addd by LY*/




int64_t
get_next_available_id(const char *table, const char *parentID)
{
		char *ret, *base;
		int64_t objectID = 0;

		ret = sql_get_text_field(db, "SELECT OBJECT_ID from %s where ID = "
		                             "(SELECT max(ID) from %s where PARENT_ID = '%s')",
		                             table, table, parentID);
		if( ret )
		{
			base = strrchr(ret, '$');
			if( base )
				objectID = strtoll(base+1, NULL, 16) + 1;
			sqlite3_free(ret);
		}

		return objectID;
}

#ifndef LITE_MINIDLNA

int
insert_container(const char *item, const char *rootParent, const char *refID, const char *class,
                 const char *artist, const char *genre, const char *album_art, int64_t *objectID, int64_t *parentID)
{
	char *result;
	char *base;
	int ret = 0;

	result = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o "
	                                "left join DETAILS d on (o.DETAIL_ID = d.ID)"
	                                " where o.PARENT_ID = '%s'"
			                " and o.NAME like '%q'"
			                " and d.ARTIST %s %Q"
	                                " and o.CLASS = 'container.%s' limit 1",
	                                rootParent, item, artist?"like":"is", artist, class);
	if( result )
	{
		base = strrchr(result, '$');
		if( base )
			*parentID = strtoll(base+1, NULL, 16);
		else
			*parentID = 0;
		*objectID = get_next_available_id("OBJECTS", result);
	}
	else
	{
		int64_t detailID = 0;
		*objectID = 0;
		*parentID = get_next_available_id("OBJECTS", rootParent);
		if( refID )
		{
			result = sql_get_text_field(db, "SELECT DETAIL_ID from OBJECTS where OBJECT_ID = %Q", refID);
			if( result )
				detailID = strtoll(result, NULL, 10);
		}
		if( !detailID )
		{
			detailID = GetFolderMetadata(item, NULL, artist, genre, (album_art ? strtoll(album_art, NULL, 10) : 0));
		}
		ret = sql_exec(db, "INSERT into OBJECTS"
		                   " (OBJECT_ID, PARENT_ID, REF_ID, DETAIL_ID, CLASS, NAME) "
		                   "VALUES"
		                   " ('%s$%llX', '%s', %Q, %lld, 'container.%s', '%q')",
		                   rootParent, (long long)*parentID, rootParent,
		                   refID, (long long)detailID, class, item);
	}
	sqlite3_free(result);

	return ret;
}
#endif


static void
insert_containers(const char *name, const char *path, const char *refID, const char *class, int64_t detailID)
{
#ifndef LITE_MINIDLNA
	char sql[128];
	char **result;
	int ret;
	int cols, row;
	int64_t objectID, parentID;
#endif

	//DPRINTF(E_WARN, L_SCANNER, "insert_containers path: %s, name: %s\n", path, name);

	if( strstr(class, "imageItem") )
	{
#ifndef LITE_MINIDLNA
		char *date_taken = NULL, *camera = NULL;
		static struct virtual_item last_date;
		static struct virtual_item last_cam;
		static struct virtual_item last_camdate;
#endif
		static long long last_all_objectID = 0;

#ifndef LITE_MINIDLNA
		snprintf(sql, sizeof(sql), "SELECT DATE, CREATOR from DETAILS where ID = %lld", (long long)detailID);
		ret = sql_get_table(db, sql, &result, &row, &cols);
		if( ret == SQLITE_OK )
		{
			date_taken = result[2];
			camera = result[3];
		}
		if( date_taken )
			date_taken[10] = '\0';
		else
			date_taken = _("Unknown Date");
		if( !camera )
			camera = _("Unknown Camera");

		if( valid_cache && strcmp(last_date.name, date_taken) == 0 )
		{
			last_date.objectID++;
			//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Using last date item: %s/%s/%X\n", last_date.name, last_date.parentID, last_date.objectID);
		}
		else
		{
			insert_container(date_taken, IMAGE_DATE_ID, NULL, "album.photoAlbum", NULL, NULL, NULL, &objectID, &parentID);
			sprintf(last_date.parentID, IMAGE_DATE_ID"$%llX", (unsigned long long)parentID);
			last_date.objectID = objectID;
			strncpyt(last_date.name, date_taken, sizeof(last_date.name));
			//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Creating cached date item: %s/%s/%X\n", last_date.name, last_date.parentID, last_date.objectID);
		}
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
		             "VALUES"
		             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
		             last_date.parentID, (long long)last_date.objectID, last_date.parentID, refID, class, (long long)detailID, name);

		if( !valid_cache || strcmp(camera, last_cam.name) != 0 )
		{
			insert_container(camera, IMAGE_CAMERA_ID, NULL, "storageFolder", NULL, NULL, NULL, &objectID, &parentID);
			sprintf(last_cam.parentID, IMAGE_CAMERA_ID"$%llX", (long long)parentID);
			strncpyt(last_cam.name, camera, sizeof(last_cam.name));
			/* Invalidate last_camdate cache */
			last_camdate.name[0] = '\0';
		}
		if( valid_cache && strcmp(last_camdate.name, date_taken) == 0 )
		{
			last_camdate.objectID++;
			//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Using last camdate item: %s/%s/%s/%X\n", camera, last_camdate.name, last_camdate.parentID, last_camdate.objectID);
		}
		else
		{
			insert_container(date_taken, last_cam.parentID, NULL, "album.photoAlbum", NULL, NULL, NULL, &objectID, &parentID);
			sprintf(last_camdate.parentID, "%s$%llX", last_cam.parentID, (long long)parentID);
			last_camdate.objectID = objectID;
			strncpyt(last_camdate.name, date_taken, sizeof(last_camdate.name));
			//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Creating cached camdate item: %s/%s/%s/%X\n", camera, last_camdate.name, last_camdate.parentID, last_camdate.objectID);
		}
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
		             "VALUES"
		             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
		             last_camdate.parentID, last_camdate.objectID, last_camdate.parentID, refID, class, (long long)detailID, name);
#endif
		/* All Images */
		if( !last_all_objectID )
		{
			last_all_objectID = get_next_available_id("OBJECTS", IMAGE_ALL_ID);
		}
		
#ifdef LITE_MINIDLNA
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID) "
		             "VALUES"
		             " ('"IMAGE_ALL_ID"$%llX', '"IMAGE_ALL_ID"', '%s', '%s', %lld)",
		             last_all_objectID++, refID, class, (long long)detailID);
#else
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
		             "VALUES"
		             " ('"IMAGE_ALL_ID"$%llX', '"IMAGE_ALL_ID"', '%s', '%s', %lld, %Q)",
		             last_all_objectID++, refID, class, (long long)detailID, name);
#endif
	}
	else if( strstr(class, "audioItem") )
	{
#ifndef LITE_MINIDLNA
		snprintf(sql, sizeof(sql), "SELECT ALBUM, ARTIST, GENRE, ALBUM_ART from DETAILS where ID = %lld", (long long)detailID);
		ret = sql_get_table(db, sql, &result, &row, &cols);
		if( ret != SQLITE_OK )
			return;
		if( !row )
		{
			sqlite3_free_table(result);
			return;
		}
		char *album = result[4], *artist = result[5], *genre = result[6];
		char *album_art = result[7];
		static struct virtual_item last_album;
		static struct virtual_item last_artist;
		static struct virtual_item last_artistAlbum;
		static struct virtual_item last_artistAlbumAll;
		static struct virtual_item last_genre;
		static struct virtual_item last_genreArtist;
		static struct virtual_item last_genreArtistAll;
#endif
		static long long last_all_objectID = 0;

#ifndef LITE_MINIDLNA
		if( album )
		{
			if( valid_cache && strcmp(album, last_album.name) == 0 )
			{
				last_album.objectID++;
				//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Using last album item: %s/%s/%X\n", last_album.name, last_album.parentID, last_album.objectID);
			}
			else
			{
				strncpyt(last_album.name, album, sizeof(last_album.name));
				insert_container(album, MUSIC_ALBUM_ID, NULL, "album.musicAlbum", artist, genre, album_art, &objectID, &parentID);
				sprintf(last_album.parentID, MUSIC_ALBUM_ID"$%llX", (long long)parentID);
				last_album.objectID = objectID;
				//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Creating cached album item: %s/%s/%X\n", last_album.name, last_album.parentID, last_album.objectID);
			}
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
			             "VALUES"
			             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
			             last_album.parentID, last_album.objectID, last_album.parentID, refID, class, (long long)detailID, name);
		}
		if( artist )
		{
			if( !valid_cache || strcmp(artist, last_artist.name) != 0 )
			{
				insert_container(artist, MUSIC_ARTIST_ID, NULL, "person.musicArtist", NULL, genre, NULL, &objectID, &parentID);
				sprintf(last_artist.parentID, MUSIC_ARTIST_ID"$%llX", (long long)parentID);
				strncpyt(last_artist.name, artist, sizeof(last_artist.name));
				last_artistAlbum.name[0] = '\0';
				/* Add this file to the "- All Albums -" container as well */
				insert_container(_("- All Albums -"), last_artist.parentID, NULL, "album", artist, genre, NULL, &objectID, &parentID);
				sprintf(last_artistAlbumAll.parentID, "%s$%llX", last_artist.parentID, (long long)parentID);
				last_artistAlbumAll.objectID = objectID;
			}
			else
			{
				last_artistAlbumAll.objectID++;
			}
			if( valid_cache && strcmp(album?album:_("Unknown Album"), last_artistAlbum.name) == 0 )
			{
				last_artistAlbum.objectID++;
				//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Using last artist/album item: %s/%s/%X\n", last_artist.name, last_artist.parentID, last_artist.objectID);
			}
			else
			{
				insert_container(album?album:_("Unknown Album"), last_artist.parentID, album?last_album.parentID:NULL,
				                 "album.musicAlbum", artist, genre, album_art, &objectID, &parentID);
				sprintf(last_artistAlbum.parentID, "%s$%llX", last_artist.parentID, (long long)parentID);
				last_artistAlbum.objectID = objectID;
				strncpyt(last_artistAlbum.name, album ? album : _("Unknown Album"), sizeof(last_artistAlbum.name));
				//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Creating cached artist/album item: %s/%s/%X\n", last_artist.name, last_artist.parentID, last_artist.objectID);
			}
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
			             "VALUES"
			             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
			             last_artistAlbum.parentID, last_artistAlbum.objectID, last_artistAlbum.parentID, refID, class, (long long)detailID, name);
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
			             "VALUES"
			             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
			             last_artistAlbumAll.parentID, last_artistAlbumAll.objectID, last_artistAlbumAll.parentID, refID, class, (long long)detailID, name);
		}
		if( genre )
		{
			if( !valid_cache || strcmp(genre, last_genre.name) != 0 )
			{
				insert_container(genre, MUSIC_GENRE_ID, NULL, "genre.musicGenre", NULL, NULL, NULL, &objectID, &parentID);
				sprintf(last_genre.parentID, MUSIC_GENRE_ID"$%llX", (long long)parentID);
				strncpyt(last_genre.name, genre, sizeof(last_genre.name));
				/* Add this file to the "- All Artists -" container as well */
				insert_container(_("- All Artists -"), last_genre.parentID, NULL, "person", NULL, genre, NULL, &objectID, &parentID);
				sprintf(last_genreArtistAll.parentID, "%s$%llX", last_genre.parentID, (long long)parentID);
				last_genreArtistAll.objectID = objectID;
			}
			else
			{
				last_genreArtistAll.objectID++;
			}
			if( valid_cache && strcmp(artist?artist:_("Unknown Artist"), last_genreArtist.name) == 0 )
			{
				last_genreArtist.objectID++;
			}
			else
			{
				insert_container(artist?artist:_("Unknown Artist"), last_genre.parentID, artist?last_artist.parentID:NULL,
				                 "person.musicArtist", NULL, genre, NULL, &objectID, &parentID);
				sprintf(last_genreArtist.parentID, "%s$%llX", last_genre.parentID, (long long)parentID);
				last_genreArtist.objectID = objectID;
				strncpyt(last_genreArtist.name, artist ? artist : _("Unknown Artist"), sizeof(last_genreArtist.name));
				//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Creating cached genre/artist item: %s/%s/%X\n", last_genreArtist.name, last_genreArtist.parentID, last_genreArtist.objectID);
			}
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
			             "VALUES"
			             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
			             last_genreArtist.parentID, last_genreArtist.objectID, last_genreArtist.parentID, refID, class, (long long)detailID, name);
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
			             "VALUES"
			             " ('%s$%llX', '%s', '%s', '%s', %lld, %Q)",
			             last_genreArtistAll.parentID, last_genreArtistAll.objectID, last_genreArtistAll.parentID, refID, class, (long long)detailID, name);
		}
#endif
		/* All Music */
		if( !last_all_objectID )
		{
			last_all_objectID = get_next_available_id("OBJECTS", MUSIC_ALL_ID);
		}
#ifdef LITE_MINIDLNA
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID) "
		             "VALUES"
		             " ('"MUSIC_ALL_ID"$%llX', '"MUSIC_ALL_ID"', '%s', '%s', %lld)",
		             last_all_objectID++, refID, class, (long long)detailID);
#else
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
		             "VALUES"
		             " ('"MUSIC_ALL_ID"$%llX', '"MUSIC_ALL_ID"', '%s', '%s', %lld, %Q)",
		             last_all_objectID++, refID, class, (long long)detailID, name);		
#endif
	}
	else if( strstr(class, "videoItem") )
	{
		static long long last_all_objectID = 0;

		/* All Videos */
		if( !last_all_objectID )
		{
			last_all_objectID = get_next_available_id("OBJECTS", VIDEO_ALL_ID);
		}

#ifdef LITE_MINIDLNA
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID) "
		             "VALUES"
		             " ('"VIDEO_ALL_ID"$%llX', '"VIDEO_ALL_ID"', '%s', '%s', %lld)",
		             last_all_objectID++, refID, class, (long long)detailID);
#else
		sql_exec(db, "INSERT into OBJECTS"
		             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
		             "VALUES"
		             " ('"VIDEO_ALL_ID"$%llX', '"VIDEO_ALL_ID"', '%s', '%s', %lld, %Q)",
		             last_all_objectID++, refID, class, (long long)detailID, name);	
#endif
		return;
	}
	else
	{
		return;
	}
#ifndef LITE_MINIDLNA
	sqlite3_free_table(result);
#endif
	valid_cache = 1;
}


int64_t
insert_directory(const char *name, const char *path, const char *base, const char *parentID, int objectID)
{
	int64_t detailID = 0;
	char class[] = "container.storageFolder";
	char *result, *p;
	static char last_found[256] = "-1";

	if( strcmp(base, BROWSEDIR_ID) != 0 )
	{
		int found = 0;
		char id_buf[64], parent_buf[64], refID[64];
		char *dir_buf, *dir;

 		dir_buf = strdup(path);
		dir = dirname(dir_buf);
		snprintf(refID, sizeof(refID), "%s%s$%X", BROWSEDIR_ID, parentID, objectID);
		snprintf(id_buf, sizeof(id_buf), "%s%s$%X", base, parentID, objectID);
		snprintf(parent_buf, sizeof(parent_buf), "%s%s", base, parentID);
		while( !found )
		{
			if( valid_cache && strcmp(id_buf, last_found) == 0 )
				break;
			if( sql_get_int_field(db, "SELECT count(*) from OBJECTS where OBJECT_ID = '%s'", id_buf) > 0 )
			{
				strcpy(last_found, id_buf);
				break;
			}
			/* Does not exist.  Need to create, and may need to create parents also */
			result = sql_get_text_field(db, "SELECT DETAIL_ID from OBJECTS where OBJECT_ID = '%s'", refID);
			if( result )
			{
				detailID = strtoll(result, NULL, 10);
				sqlite3_free(result);
			}
#ifndef LITE_MINIDLNA	
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, DETAIL_ID, CLASS, NAME) "
			             "VALUES"
			             " ('%s', '%s', %Q, %lld, '%s', '%q')",
			             id_buf, parent_buf, refID, detailID, class, strrchr(dir, '/')+1);
#else
			sql_exec(db, "INSERT into OBJECTS"
			             " (OBJECT_ID, PARENT_ID, REF_ID, DETAIL_ID, CLASS) "
			             "VALUES"
			             " ('%s', '%s', %Q, %lld, '%s')",
			             id_buf, parent_buf, refID, detailID, class);
#endif
			if( (p = strrchr(id_buf, '$')) )
				*p = '\0';
			if( (p = strrchr(parent_buf, '$')) )
				*p = '\0';
			if( (p = strrchr(refID, '$')) )
				*p = '\0';
			dir = dirname(dir);
		}
		free(dir_buf);
		return 0;
	}
	
#ifndef LITE_MINIDLNA
	detailID = GetFolderMetadata(name, path, NULL, NULL, find_album_art(path, NULL, 0));
	sql_exec(db, "INSERT into OBJECTS"
					 " (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS, NAME) "
					 "VALUES"
					 " ('%s%s$%X', '%s%s', %lld, '%s', '%q')",
					 base, parentID, objectID, base, parentID, detailID, class, name);
#else
	detailID = GetFolderMetadata(name, path, NULL, NULL, 0);
	sql_exec(db, "INSERT into OBJECTS"
					 " (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS) "
					 "VALUES"
					 " ('%s%s$%X', '%s%s', %lld, '%s')",
					 base, parentID, objectID, base, parentID, detailID, class);
#endif

	return detailID;
}

int
insert_file(char *name, const char *path, const char *parentID, int object, media_types types)
{
	char class[32];
	char objectID[64];
	int64_t detailID = 0;
	char base[8];
	char *typedir_parentID;
	char *baseid;
	char *orig_name = NULL;

	//DPRINTF(E_WARN, L_SCANNER, "file: %s, name: %s\n", path, name);

	if( (types & TYPE_IMAGES) && is_image(name) )
	{
		if( is_album_art(name) )
			return -1;
		strcpy(base, IMAGE_DIR_ID);
		strcpy(class, "item.imageItem.photo");
		detailID = GetImageMetadata(path, name);
	}
	else if( (types & TYPE_VIDEO) && is_video(name) )
	{
 		orig_name = strdup(name);
		strcpy(base, VIDEO_DIR_ID);
		strcpy(class, "item.videoItem");
		detailID = GetVideoMetadata(path, name);
		if( !detailID )
			strcpy(name, orig_name);
	}
	else if( is_playlist(name) )
	{
		if( insert_playlist(path, name) == 0 )
			return 1;
	}
	if( !detailID && (types & TYPE_AUDIO) && is_audio(name) )
	{
		strcpy(base, MUSIC_DIR_ID);
		strcpy(class, "item.audioItem.musicTrack");
		detailID = GetAudioMetadata(path, name);
	}
	free(orig_name);
	if( !detailID )
	{
		DPRINTF(E_WARN, L_SCANNER, "Unsuccessful getting details for %s!\n", path);
		return -1;
	}

	sprintf(objectID, "%s%s$%X", BROWSEDIR_ID, parentID, object);

#ifdef LITE_MINIDLNA
	sql_exec(db, "INSERT into OBJECTS"
				 " (OBJECT_ID, PARENT_ID, CLASS, DETAIL_ID) "
				 "VALUES"
				 " ('%s', '%s%s', '%s', %lld)",
				 objectID, BROWSEDIR_ID, parentID, class, detailID);

#else
	sql_exec(db, "INSERT into OBJECTS"
	             " (OBJECT_ID, PARENT_ID, CLASS, DETAIL_ID, NAME) "
	             "VALUES"
	             " ('%s', '%s%s', '%s', %lld, '%q')",
	             objectID, BROWSEDIR_ID, parentID, class, detailID, name);
#endif

	if( *parentID )
	{
		int typedir_objectID = 0;
		typedir_parentID = strdup(parentID);
		baseid = strrchr(typedir_parentID, '$');
		if( baseid )
		{
			typedir_objectID = strtol(baseid+1, NULL, 16);
			*baseid = '\0';
		}
		insert_directory(name, path, base, typedir_parentID, typedir_objectID);
		free(typedir_parentID);
	}

#ifdef LITE_MINIDLNA
	sql_exec(db, "INSERT into OBJECTS"
	             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID) "
	             "VALUES"
	             " ('%s%s$%X', '%s%s', '%s', '%s', %lld)",
	             base, parentID, object, base, parentID, objectID, class, detailID);
#else
	sql_exec(db, "INSERT into OBJECTS"
	             " (OBJECT_ID, PARENT_ID, REF_ID, CLASS, DETAIL_ID, NAME) "
	             "VALUES"
	             " ('%s%s$%X', '%s%s', '%s', '%s', %lld, '%q')",
	             base, parentID, object, base, parentID, objectID, class, detailID, name);
#endif

	insert_containers(name, path, objectID, class, detailID);

	return 0;
}

int
CreateDatabase(void)
{
	int ret, i;
	const char *containers[] = { "0","-1",   "root",
	                        MUSIC_ID, "0", _("Music"),
	                    MUSIC_ALL_ID, MUSIC_ID, _("All Music"),
#ifndef LITE_MINIDLNA
	                  MUSIC_GENRE_ID, MUSIC_ID, _("Genre"),
	                 MUSIC_ARTIST_ID, MUSIC_ID, _("Artist"),
	                  MUSIC_ALBUM_ID, MUSIC_ID, _("Album"),
#endif
	                    MUSIC_DIR_ID, MUSIC_ID, _("Folders"),
	                  MUSIC_PLIST_ID, MUSIC_ID, _("Playlists"),

	                        VIDEO_ID, "0", _("Video"),
	                    VIDEO_ALL_ID, VIDEO_ID, _("All Video"),
	                    VIDEO_DIR_ID, VIDEO_ID, _("Folders"),

	                        IMAGE_ID, "0", _("Pictures"),
	                    IMAGE_ALL_ID, IMAGE_ID, _("All Pictures"),
#ifndef LITE_MINIDLNA	                    
	                   IMAGE_DATE_ID, IMAGE_ID, _("Date Taken"),
	                 IMAGE_CAMERA_ID, IMAGE_ID, _("Camera"),
#endif
	                    IMAGE_DIR_ID, IMAGE_ID, _("Folders"),
	                    BROWSEDIR_ID, "0", _("Browse Folders"),
			0 };

	ret = sql_exec(db, create_objectTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_detailTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	
#ifndef LITE_MINIDLNA
	ret = sql_exec(db, create_albumArtTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
#endif

	ret = sql_exec(db, create_captionTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_bookmarkTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_playlistTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, create_settingsTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	ret = sql_exec(db, "INSERT into SETTINGS values ('UPDATE_ID', '0')");
	if( ret != SQLITE_OK )
		goto sql_failed;
	
#ifdef USE_UPDATE	
	/* For database update, add by zengdongbiao, 05May15.  */
	ret = sql_exec(db, create_watchdirsTable_sqlite);
	if( ret != SQLITE_OK )
		goto sql_failed;
	/* End add by zengdongbiao.  */
#endif

	for( i=0; containers[i]; i=i+3 )
	{
#ifdef LITE_MINIDLNA
		/* For lite_minidlna, OBJECTS table has no NAME.  */
		ret = sql_exec(db, "INSERT into OBJECTS (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS)"
		                   " values "
		                   "('%s', '%s', %lld, 'container.storageFolder')",
		                   containers[i], containers[i+1], GetFolderMetadata(containers[i+2], NULL, NULL, NULL, 0));
#else
		ret = sql_exec(db, "INSERT into OBJECTS (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS, NAME)"
		                   " values "
		                   "('%s', '%s', %lld, 'container.storageFolder', '%q')",
		                   containers[i], containers[i+1], GetFolderMetadata(containers[i+2], NULL, NULL, NULL, 0), containers[i+2]);
#endif
		if( ret != SQLITE_OK )
			goto sql_failed;
	}
	
#ifndef DISABLE_MAGIC_CONTAINER
	for( i=0; magic_containers[i].objectid_match; i++ )
	{
		struct magic_container_s *magic = &magic_containers[i];
		if (!magic->name)
			continue;
		if( sql_get_int_field(db, "SELECT 1 from OBJECTS where OBJECT_ID = '%s'", magic->objectid_match) == 0 )
		{
			char *parent = strdup(magic->objectid_match);
			if (strrchr(parent, '$'))
				*strrchr(parent, '$') = '\0';
		#ifndef LITE_MINIDLNA
			ret = sql_exec(db, "INSERT into OBJECTS (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS, NAME)"
			                   " values "
					   "('%s', '%s', %lld, 'container.storageFolder', '%q')",
					   magic->objectid_match, parent, GetFolderMetadata(magic->name, NULL, NULL, NULL, 0), magic->name);
		#else
			ret = sql_exec(db, "INSERT into OBJECTS (OBJECT_ID, PARENT_ID, DETAIL_ID, CLASS)"
			                   " values "
					   "('%s', '%s', %lld, 'container.storageFolder')",
					   magic->objectid_match, parent, GetFolderMetadata(magic->name, NULL, NULL, NULL, 0));
		#endif
			free(parent);
			if( ret != SQLITE_OK )
				goto sql_failed;
		}
	}
#endif
	sql_exec(db, "create INDEX IDX_OBJECTS_OBJECT_ID ON OBJECTS(OBJECT_ID);");
	sql_exec(db, "create INDEX IDX_OBJECTS_PARENT_ID ON OBJECTS(PARENT_ID);");
	sql_exec(db, "create INDEX IDX_OBJECTS_DETAIL_ID ON OBJECTS(DETAIL_ID);");
	sql_exec(db, "create INDEX IDX_OBJECTS_CLASS ON OBJECTS(CLASS);");
	sql_exec(db, "create INDEX IDX_DETAILS_PATH ON DETAILS(PATH);");
	sql_exec(db, "create INDEX IDX_DETAILS_ID ON DETAILS(ID);");
#ifndef LITE_MINIDLNA
	sql_exec(db, "create INDEX IDX_ALBUM_ART ON ALBUM_ART(ID);");
	sql_exec(db, "create INDEX IDX_SCANNER_OPT ON OBJECTS(PARENT_ID, NAME, OBJECT_ID);");
#else
	sql_exec(db, "create INDEX IDX_SCANNER_OPT ON OBJECTS(PARENT_ID, OBJECT_ID);");
#endif

#ifdef USE_UPDATE	
	/* For database update, add by zengdongbiao, 05May15.  */
	sql_exec(db, "create INDEX IDX_WATCHDIRS_PATH ON WATCHDIRS(PATH);");
	/* End add by zengdongbiao.  */
#endif

sql_failed:
	if( ret != SQLITE_OK )
		DPRINTF(E_ERROR, L_DB_SQL, "Error creating SQLite3 database!\n");
	return (ret != SQLITE_OK);
}

static inline int
filter_hidden(const struct dirent *d)
{
	return (d->d_name[0] != '.');
}

static int
filter_type(const struct dirent *d)
{
#if HAVE_STRUCT_DIRENT_D_TYPE
	return ( (d->d_type == DT_DIR) ||
	         (d->d_type == DT_LNK) ||
	         (d->d_type == DT_UNKNOWN)
	       );
#else
	return 1;
#endif
}

static int
filter_a(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
		   (is_audio(d->d_name) ||
	            is_playlist(d->d_name))))
	       );
}

static int
filter_av(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
		   (is_audio(d->d_name) ||
		    is_video(d->d_name) ||
	            is_playlist(d->d_name))))
	       );
}

static int
filter_ap(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
		   (is_audio(d->d_name) ||
		    is_image(d->d_name) ||
	            is_playlist(d->d_name))))
	       );
}

static int
filter_v(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
	           is_video(d->d_name)))
	       );
}

static int
filter_vp(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
		   (is_video(d->d_name) ||
	            is_image(d->d_name))))
	       );
}

static int
filter_p(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
		   is_image(d->d_name)))
	       );
}

static int
filter_avp(const struct dirent *d)
{
	return ( filter_hidden(d) &&
	         (filter_type(d) ||
		  (is_reg(d) &&
		   (is_audio(d->d_name) ||
		    is_image(d->d_name) ||
		    is_video(d->d_name) ||
	            is_playlist(d->d_name))))
	       );
}

void
ScanDirectory(const char *dir, const char *parent, media_types dir_types)
{
	int i = 0;
	int startID=0;
	char parent_id[PATH_MAX];
	char full_path[PATH_MAX];
	char *name = NULL;
	enum file_types type;

	DIR *pDir = NULL;
	struct dirent *pResult = NULL;
	struct dirent entryItem;
	int flag = FLAG_NO_FILE_OR_DIR;
	int ret = -1;
	
	/*added by LY to restrict total file number, stop scanner if file number reaches the limit*/
	if (g_scan_fileNumber >= MAX_FILE_NUMBER)
	{
		return;
	}
	/*end added by LY*/

	DPRINTF(parent ? E_INFO : E_WARN, L_SCANNER, _("Scanning %s\n"), dir);

	/* Get startID only onece for (under) a directory, moved to here by zengdongbiao, 20Apr15.  */
	if( !parent )
	{
		startID = get_next_available_id("OBJECTS", BROWSEDIR_ID);
	}
	/* end modify by zengdongbiao */

	/*use readdir_r instead of scandir, because you cannot predict the number of files in one directory, 
	 *and use scandir may casue memory problems */
	pDir = opendir(dir);
	if (NULL == pDir)
	{
		DPRINTF(E_ERROR, L_SCANNER, _("Scanning dir NULL\n"));
		return;
	}
	while (!readdir_r(pDir, &entryItem, &pResult) && (NULL != pResult))
	{
		//DPRINTF(E_WARN, L_SCANNER, _("file name(%s)\n"), entryItem.d_name);

		//DPRINTF(E_DEBUG, L_SCANNER, _("scanning value(%d)\n"), get_allow_scanning_value());

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
		switch( dir_types )
		{
		case ALL_MEDIA:
			flag = filter_avp(&entryItem);
			break;
			
		case TYPE_AUDIO:
			flag = filter_a(&entryItem);
			break;
			
		case TYPE_AUDIO|TYPE_VIDEO:
			flag = filter_av(&entryItem);
			break;
			
		case TYPE_AUDIO|TYPE_IMAGES:
			flag = filter_ap(&entryItem);
			break;
			
		case TYPE_VIDEO:
			flag = filter_v(&entryItem);
			break;
			
		case TYPE_VIDEO|TYPE_IMAGES:
			flag = filter_vp(&entryItem);
			break;
			
		case TYPE_IMAGES:
			flag = filter_p(&entryItem);
			break;
			
		default:
			flag = FLAG_NO_FILE_OR_DIR;
			break;
		}

		/*continue if the current entry not  matched */
		if (flag == FLAG_NO_FILE_OR_DIR)
		{
			continue;
		}

		/*deal with each matched entry in the current directory, including both files and dirs*/
		type = TYPE_UNKNOWN;
		sprintf(full_path, "%s/%s", dir, entryItem.d_name);
		name = escape_tag(entryItem.d_name, 1);
		
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
#ifdef USE_UPDATE	
			/*  Add watch directory for DB updating. */
			if ( update_addWatchDir(full_path) < 0 )
			{
				continue;
			}
#endif						
			insert_directory(name, full_path, BROWSEDIR_ID, THISORNUL(parent), i+startID);
			/* account directory number into file_number */
			g_scan_fileNumber++;
			sprintf(parent_id, "%s$%X", THISORNUL(parent), (i++) + startID);
			ScanDirectory(full_path, parent_id, dir_types);
		}
		else if( type == TYPE_FILE && (access(full_path, R_OK) == 0) )
		{
			if( insert_file(name, full_path, THISORNUL(parent), (i++) + startID, dir_types) == 0 )
			{
				g_scan_fileNumber++;
			}
		}	

		if (g_scan_fileNumber >= MAX_FILE_NUMBER)
		{
			/*stop scanner*/
			break;
		}
	}

	if( !parent )
	{
		DPRINTF(E_INFO, L_SCANNER, _("Scanning %s finished (%u files)!\n"), dir, g_scan_fileNumber);
	}

	/* close dir, add by liweijie, 2014-06-27 */
	if (NULL != pDir)
	{
		ret = closedir(pDir);
		if (-1 == ret)
		{
			DPRINTF(E_ERROR, L_SCANNER, _("Close Dir fail, error: %s!\n"), strerror(errno));
		}
		else
		{
			DPRINTF(E_INFO, L_SCANNER, _("Close Dir success !\n"));
		}
	}
	
	
}

static void
_notify_start(void)
{
#ifdef READYNAS
	FILE *flag = fopen("/ramfs/.upnp-av_scan", "w");
	if( flag )
		fclose(flag);
#endif
}

static void
_notify_stop(void)
{
#ifdef READYNAS
	if( access("/ramfs/.rescan_done", F_OK) == 0 )
		system("/bin/sh /ramfs/.rescan_done");
	unlink("/ramfs/.upnp-av_scan");
#endif
}


void *start_scanner(void *arg)
{
	struct media_dir_s *media_path = media_dirs;
	char path[MAXPATHLEN];
    int now;
#ifdef USE_UPDATE
	int rescan = *((int *)arg);
#endif
	/* to test sqlite memory used, add by zengdongbiao, 25Dec2014*/
	//int curMemUsed = 0;
	//int highMemUsed = 0;
	/* end add by zengdongbiao */

    now = time(NULL);
	DPRINTF(E_WARN, L_SCANNER,"scanner thread start: pid=%u, tid=%u\n", 
								(unsigned int)getpid(), (unsigned int)pthread_self());

	//system("echo 2 > /proc/sys/vm/overcommit_memory");
	
	if (setpriority(PRIO_PROCESS, 0, 15) == -1)
		DPRINTF(E_WARN, L_INOTIFY,  "Failed to reduce scanner thread priority\n");
	_notify_start();

	setlocale(LC_COLLATE, "");

#ifndef LITE_MINIDLNA
	av_register_all();
	av_log_set_level(AV_LOG_PANIC);
#endif

#ifdef USE_UPDATE
	/* Prepare for updating database or rescanning media files.  */
	update_init(rescan);
#else
	/* set file number to zero, add by liweijie,2014-08-26 */
	scan_setFileNumber(0);
#endif	
	for( media_path = media_dirs; media_path != NULL; media_path = media_path->next )
	{
		int64_t id;
		char *bname, *parent = NULL;
		char buf[8];
		
		strncpyt(path, media_path->path, sizeof(path));
		bname = basename(path);

#ifdef USE_UPDATE
		if (rescan
			|| (sql_get_int64_field(db, "SELECT count(*) from SETTINGS where KEY = 'media_dir' and VALUE = %Q", media_path->path) <= 0) )
		{
#endif
			/* If there are multiple media locations, add a level to the ContentDirectory */
			//if( !GETFLAG(MERGE_MEDIA_DIRS_MASK) && media_dirs->next )
			/* minidlna behave same no matter how many media locations, modify by zengdongbiao, 22May15. */
			if( !GETFLAG(MERGE_MEDIA_DIRS_MASK))
			{
				int startID = get_next_available_id("OBJECTS", BROWSEDIR_ID);
				id = insert_directory(bname, path, BROWSEDIR_ID, "", startID);
				sprintf(buf, "$%X", startID);
				parent = buf;
			}
			else
			{
				id = GetFolderMetadata(bname, media_path->path, NULL, NULL, 0);
			}
			
			/* Use TIMESTAMP to store the media type */
			sql_exec(db, "UPDATE DETAILS set TIMESTAMP = %d where ID = %lld", media_path->types, (long long)id);
			//DPRINTF(E_DEBUG, L_SCANNER, "Scanning %s, parent: %s\n", media_path->path, parent ? parent : "null");
			ScanDirectory(media_path->path, parent, media_path->types);
			sql_exec(db, "INSERT into SETTINGS values (%Q, %Q)", "media_dir", media_path->path);
#ifdef USE_UPDATE
		}
		else
		{
			parent = sql_get_text_field(db, "SELECT OBJECT_ID from OBJECTS o left join DETAILS d on (d.ID = o.DETAIL_ID)"
                       						 " where d.PATH = '%q' and REF_ID is NULL", media_path->path);
			if(parent != NULL)
			{
				strncpyt(buf, parent + 2, sizeof(buf));
				sqlite3_free(parent);
				parent = buf;
			}
			//DPRINTF(E_DEBUG, L_SCANNER, "updating %s, parent: %s\n", media_path->path, parent ? parent : "null");
			update_scanDirectory(media_path->path, parent, media_path->types);
		}
#endif
	}
	_notify_stop();
	
#ifdef USE_UPDATE
	if (rescan == 1)
	{	
#endif
		/* Create this index after scanning, so it doesn't slow down the scanning process.
		 * This index is very useful for large libraries used with an XBox360 (or any
		 * client that uses UPnPSearch on large containers). */
		sql_exec(db, "create INDEX IDX_SEARCH_OPT ON OBJECTS(OBJECT_ID, CLASS, DETAIL_ID);");
#ifdef USE_UPDATE
	}
#endif


	if( GETFLAG(NO_PLAYLIST_MASK) )
	{
		DPRINTF(E_WARN, L_SCANNER, "Playlist creation disabled\n");	  
	}
	else
	{
		fill_playlists();
	}

    now = time(NULL) - now;
	//DPRINTF(E_INFO, L_SCANNER, "Initial file scan completed(%d),times(%d)\n", DB_VERSION, now);
	printf("Minidlna scanner complete! files and subdirectory:%u, time: %d\n", g_scan_fileNumber, now);
	//JM: Set up a db version number, so we know if we need to rebuild due to a new structure.
	/* completed scanner, we set user_version to DB_VERSION, add by liweijie, 2014-07-02 */
	if (get_allow_scanning_value())
	{
		sql_exec(db, "pragma user_version = %d;", DB_VERSION);
		
#ifdef USE_UPDATE
		update_storeFileAndWatchDirNum(rescan);
#endif
	}
	//system("echo 0 > /proc/sys/vm/overcommit_memory");
	

	//curMemUsed = sqlite3_memory_used();
	//highMemUsed = sqlite3_memory_highwater(0);
	//DPRINTF(E_WARN, L_SCANNER, "sqlite3_memory_used: curMemUsed: %d, highMemUsed: %d\n", curMemUsed, highMemUsed);
	//sqlite3_status(SQLITE_STATUS_MEMORY_USED, &curMemUsed, &highMemUsed, 0);
	//DPRINTF(E_WARN, L_SCANNER, "sqlite3_status: curMemUsed: %d, highMemUsed: %d\n", curMemUsed, highMemUsed);

	/* Set the scanning value to zero when scanning completed, add by zengdongbiao, 02Apr15. */
	set_scanning_value(0);
	/* End add by zengdongbiao */

	return ((void *)0);
}


/*added by LY*/
void set_scanning_value(short int arg)
{
	//pthread_mutex_lock(&g_scanning_lock);
	scanning = arg;
	//pthread_mutex_unlock(&g_scanning_lock);
}

short int get_scanning_value()
{
	short int retValue;
	
	//pthread_mutex_lock(&g_scanning_lock);
	retValue = scanning;
	//pthread_mutex_unlock(&g_scanning_lock);

	return retValue;
}

void set_allow_scanning_value(short int arg)
{
	//pthread_mutex_lock(&g_allow_scanning_lock);
	allow_scanning = arg;
	//pthread_mutex_unlock(&g_allow_scanning_lock);
}

short int get_allow_scanning_value()
{
	short int retValue;
	
	//pthread_mutex_lock(&g_allow_scanning_lock);
	retValue = allow_scanning;
	//pthread_mutex_unlock(&g_allow_scanning_lock);

	return retValue;
}

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
unsigned int scan_getFileNumber()
{
	return g_scan_fileNumber;
}

/* 
 * fn		void scan_setFileNumber(unsigned int number)
 * brief	Set value of files number.
 * details	g_scan_fileNumber = number;
 *
 * param[in]		
 *
 * note		
 */
void scan_setFileNumber(unsigned int number)
{
	g_scan_fileNumber = number;
}

/* 
 * fn		void scan_increaseFileNumber(unsigned int number)
 * brief	Increase file numbers.
 * details	g_scan_fileNumber++;
 *
 * return	void
 *
 * note		
 */
void scan_increaseFileNumber()
{
	g_scan_fileNumber++;
}

/* 
 * fn		void scan_decreaseFileNumber(unsigned int number)
 * brief	Decrease file numbers.
 * details	g_scan_fileNumber--;	
 *
 * return	void
 *
 * note		
 */
void scan_decreaseFileNumber()
{
	g_scan_fileNumber--;
}

/* 
 * fn		int scan_dirIsRecycle(const char *name)
 * brief	Check if the dir is $RECYCLE.BIN or RECYCLER.
 * details	We would not insert the information of $RECYCLE.BIN.
 *
 * param[in]	name	the name(not full path) of directory.
 *
 * return	
 * retval	1	dir is RECYCLE.BIN or RECYCLER.
 *			0	dir is not RECYCLE.BIN or RECYCLER.
 * note		Written by zengdongbiao, 27Apr15.
 */
int scan_dirIsRecycle(const char *name)
{
	int ret = 0;
	
	ret = strcmp(name, "$RECYCLE.BIN"); 

	if (ret != 0)
	{
		ret = strcmp(name, "RECYCLER");
	}

	return (ret == 0) ? 1 : 0;
}

/* To indentify link file, add by zengdongbiao.  */
int scan_isLink(const char *filePath)
{
	struct stat file;

	if (lstat(filePath, &file) != 0)
	{
		return -1;
	}

	if ( ((file.st_mode) & S_IFMT) == S_IFLNK )
	{
		DPRINTF(E_WARN, L_SCANNER, "%s is a link file\n", filePath);
		return 1;
	}
	
	return 0;
}


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
int scan_mediaTypesIsCorrect(media_types dirTypes, struct dirent *entryItem)
{
	int flag = 0;

	switch( dirTypes )
	{
	case ALL_MEDIA:
		flag = filter_avp(entryItem);
		break;
		
	case TYPE_AUDIO:
		flag = filter_a(entryItem);
		break;
		
	case TYPE_AUDIO|TYPE_VIDEO:
		flag = filter_av(entryItem);
		break;
		
	case TYPE_AUDIO|TYPE_IMAGES:
		flag = filter_ap(entryItem);
		break;
		
	case TYPE_VIDEO:
		flag = filter_v(entryItem);
		break;
		
	case TYPE_VIDEO|TYPE_IMAGES:
		flag = filter_vp(entryItem);
		break;
		
	case TYPE_IMAGES:
		flag = filter_p(entryItem);
		break;
		
	default:
		flag = 0;
		break;
	}
		
	return flag;
}

#endif
