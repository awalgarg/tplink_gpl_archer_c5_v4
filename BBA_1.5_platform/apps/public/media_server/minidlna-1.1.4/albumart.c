/* MiniDLNA media server
 * Copyright (C) 2008  Justin Maggard
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
#ifndef LITE_MINIDLNA
 
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <limits.h>
#include <libgen.h>
#include <setjmp.h>
#include <errno.h>

#include <jpeglib.h>

#include "upnpglobalvars.h"
#include "albumart.h"
#include "sql.h"
#include "utils.h"
#include "image_utils.h"
#include "log.h"

#ifndef USE_ALBUM_RESPONSE

static int
art_cache_exists(const char *orig_path, char **cache_file)
{
	if( xasprintf(cache_file, "%s/art_cache%s", db_path, orig_path) < 0 )
		return 0;

	strcpy(strchr(*cache_file, '\0')-4, ".jpg");

	return (!access(*cache_file, F_OK));
}

static char *
save_resized_album_art(image_s *imsrc, const char *path)
{
	int dstw, dsth;
	image_s *imdst;
	char *cache_file;
	char cache_dir[MAXPATHLEN];

	if( !imsrc )
		return NULL;

	if( art_cache_exists(path, &cache_file) )
		return cache_file;

	strncpyt(cache_dir, cache_file, sizeof(cache_dir));
	make_dir(dirname(cache_dir), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

	if( imsrc->width > imsrc->height )
	{
		dstw = 160;
		dsth = (imsrc->height<<8) / ((imsrc->width<<8)/160);
	}
	else
	{
		dstw = (imsrc->width<<8) / ((imsrc->height<<8)/160);
		dsth = 160;
	}
	imdst = image_resize(imsrc, dstw, dsth);
	if( !imdst )
	{
		free(cache_file);
		return NULL;
	}

	cache_file = image_save_to_jpeg_file(imdst, cache_file);
	image_free(imdst);
	
	return cache_file;
}
#endif /* USE_ALBUM_RESPONSE */


/* And our main album art functions */
void
update_if_album_art(const char *path)
{
	char *dir;
	char *match;
	char file[MAXPATHLEN];
	char fpath[MAXPATHLEN];
	char dpath[MAXPATHLEN];
	int ncmp = 0;
	int album_art;
	DIR *dh;
	struct dirent *dp;
	enum file_types type = TYPE_UNKNOWN;
	int64_t art_id = 0;
	int ret;

	strncpyt(fpath, path, sizeof(fpath));
	match = basename(fpath);
	/* Check if this file name matches a specific audio or video file */
	if( ends_with(match, ".cover.jpg") )
	{
		ncmp = strlen(match)-10;
	}
	else
	{
		ncmp = strrchr(match, '.') - match;
	}
	/* Check if this file name matches one of the default album art names */
	album_art = is_album_art(match);

	strncpyt(dpath, path, sizeof(dpath));
	dir = dirname(dpath);
	dh = opendir(dir);
	if( !dh )
		return;
	while ((dp = readdir(dh)) != NULL)
	{
		if (is_reg(dp) == 1)
		{
			type = TYPE_FILE;
		}
		else if (is_dir(dp) == 1)
		{
			type = TYPE_DIR;
		}
		else
		{
			snprintf(file, sizeof(file), "%s/%s", dir, dp->d_name);
			type = resolve_unknown_type(file, ALL_MEDIA);
		}
		if( type != TYPE_FILE )
			continue;
		if( (dp->d_name[0] != '.') &&
		    (is_video(dp->d_name) || is_audio(dp->d_name)) &&
		    (album_art || strncmp(dp->d_name, match, ncmp) == 0) )
		{
			DPRINTF(E_DEBUG, L_METADATA, "New file %s looks like cover art for %s\n", path, dp->d_name);
			snprintf(file, sizeof(file), "%s/%s", dir, dp->d_name);
			art_id = find_album_art(file, NULL, 0);
			ret = sql_exec(db, "UPDATE DETAILS set ALBUM_ART = %lld where PATH = '%q'", (long long)art_id, file);
			if( ret != SQLITE_OK )
				DPRINTF(E_WARN, L_METADATA, "Error setting %s as cover art for %s\n", match, dp->d_name);
		}
	}
	closedir(dh);
}

#ifndef USE_ALBUM_RESPONSE

char *
check_embedded_art(const char *path, uint8_t *image_data, int image_size)
{
	int width = 0, height = 0;
	char *art_path = NULL;
	char *cache_dir;
	FILE *dstfile;
	image_s *imsrc = NULL;
	static char last_path[PATH_MAX];
	static unsigned int last_hash = 0;
	static int last_success = 0;
	unsigned int hash;

	if( !image_data || !image_size || !path )
	{
		return NULL;
	}

	/* If the embedded image matches the embedded image from the last file we
	 * checked, just make a hard link.  Better than storing it on the disk twice. */
	hash = DJBHash(image_data, image_size);
	if( hash == last_hash )
	{
		if( !last_success )
			return NULL;

		/* Use the last album_art image and do not make a hard link, if user pull out the usb 
		 * while making hard link, the hard link will unable to remove. 
		 * Modify by zengdongbiao, 04May15.
		 */
		art_path = strdup(last_path);
		return (art_path);
		//art_cache_exists(path, &art_path);
		//DPRINTF(E_WARN, L_METADATA, "going to link %s to %s\n", art_path, last_path);
		//if( link(last_path, art_path) == 0 )
		//{
		//	return(art_path);
		//}
		//else
		//{
		//	if( errno == ENOENT )
		//	{
		//		cache_dir = strdup(art_path);
		//		make_dir(dirname(cache_dir), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		//		free(cache_dir);
		//		if( link(last_path, art_path) == 0 )
		//			return(art_path);
		//	}
		//	DPRINTF(E_WARN, L_METADATA, "Linking %s to %s failed [%s]\n", art_path, last_path, strerror(errno));
		//	free(art_path);
		//	art_path = NULL;
		//}
		/* End modify by zengdongbiao, 04May15.  */
	}
	last_hash = hash;

	/* Firtly get image size without decompress, modify by zengdongbiao, 25May15. */
#ifdef MEMORY_OPTIMIZE_FOR_ALBUMART
	if (image_getJpegSize(NULL, 0, image_data, image_size, &width, &height) < 0)
	{
		last_success = 0;
		return NULL;
	}
#else
	imsrc = image_new_from_jpeg(NULL, 0, image_data, image_size, 1, ROTATE_NONE);
	if( !imsrc )
	{
		last_success = 0;
		return NULL;
	}
	width = imsrc->width;
	height = imsrc->height;
#endif
	/* End modify by zengdongbiao  */
	
	if( width > 160 || height > 160 )
	{
		/* Secondly decompress image with appropriate scale, add by zengdongbiao, 25May15. */
#ifdef MEMORY_OPTIMIZE_FOR_ALBUMART
		int scale = 1;
		int valueTmp = (width >= height)? width: height;
		
		/* Find appropriate scale for decompress, valueTmp = [160,320)  */
		while (valueTmp >= 320)
		{
			valueTmp /= 2;
			scale *= 2;
			/* max scale: 1/8  */
			if (scale == 8)
			{
				break;
			}
		}
		
		imsrc = image_new_from_jpeg(NULL, 0, image_data, image_size, scale, ROTATE_NONE);
		if( !imsrc )
		{
			last_success = 0;
			return NULL;
		}
#endif /* MEMORY_OPTIMIZE_FOR_ALBUMART */
		/* End aodify by zengdongbiao  */

		art_path = save_resized_album_art(imsrc, path);
	}
	else if( width > 0 && height > 0 )
	{
		size_t nwritten;
		if( art_cache_exists(path, &art_path) )
			goto end_art;
		cache_dir = strdup(art_path);
		make_dir(dirname(cache_dir), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		free(cache_dir);
		dstfile = fopen(art_path, "w");
		if( !dstfile )
		{
			free(art_path);
			art_path = NULL;
			goto end_art;
		}
		nwritten = fwrite((void *)image_data, 1, image_size, dstfile);
		fclose(dstfile);
		if( nwritten != image_size )
		{
			DPRINTF(E_WARN, L_METADATA, "Embedded art error: wrote %lu/%d bytes\n",
				(unsigned long)nwritten, image_size);
			remove(art_path);
			free(art_path);
			art_path = NULL;
			goto end_art;
		}
	}
end_art:
	if (imsrc != NULL)
	{
		image_free(imsrc);
	}
	if( !art_path )
	{
		DPRINTF(E_WARN, L_METADATA, "Invalid embedded album art in %s\n", basename((char *)path));
		last_success = 0;
		return NULL;
	}
	DPRINTF(E_DEBUG, L_METADATA, "Found new embedded album art in %s\n", basename((char *)path));
	last_success = 1;
	strcpy(last_path, art_path);

	return(art_path);
}

static char *
check_for_album_file(const char *path)
{
	char file[MAXPATHLEN];
	char mypath[MAXPATHLEN];
	struct album_art_name_s *album_art_name;
	image_s *imsrc = NULL;
	int width=0, height=0;
	char *art_file, *p;
	const char *dir;
	struct stat st;
	int ret;

	if( stat(path, &st) != 0 )
		return NULL;

	if( S_ISDIR(st.st_mode) )
	{
		dir = path;
		goto check_dir;
	}
	strncpyt(mypath, path, sizeof(mypath));
	dir = dirname(mypath);

	/* First look for file-specific cover art */
	snprintf(file, sizeof(file), "%s.cover.jpg", path);
	ret = access(file, R_OK);
	if( ret != 0 )
	{
		strncpyt(file, path, sizeof(file));
		p = strrchr(file, '.');
		if( p )
		{
			strcpy(p, ".jpg");
			ret = access(file, R_OK);
		}
		if( ret != 0 )
		{
			p = strrchr(file, '/');
			if( p )
			{
				memmove(p+2, p+1, file+MAXPATHLEN-p-2);
				p[1] = '.';
				ret = access(file, R_OK);
			}
		}
	}
	if( ret == 0 )
	{
		if( art_cache_exists(file, &art_file) )
			goto existing_file;
		free(art_file);

		/* Firtly get image size without decompress, modify by zengdongbiao, 25May15. */
	#ifdef MEMORY_OPTIMIZE_FOR_ALBUMART
		if (image_getJpegSize(file, 1, NULL, 0, &width, &height) == 0)
		{
			goto found_file;
		}
	#else
		imsrc = image_new_from_jpeg(file, 1, NULL, 0, 1, ROTATE_NONE);
		if( imsrc )
			goto found_file;
	#endif
		/* End modify by zengdongbiao */
	}
check_dir:
	/* Then fall back to possible generic cover art file names */
	for( album_art_name = album_art_names; album_art_name; album_art_name = album_art_name->next )
	{
		snprintf(file, sizeof(file), "%s/%s", dir, album_art_name->name);
		if( access(file, R_OK) == 0 )
		{
			if( art_cache_exists(file, &art_file) )
			{
existing_file:
				return art_file;
			}
			free(art_file);
			
			/* Firtly get image size without decompress, modify by zengdongbiao, 25May15. */	
		#ifdef MEMORY_OPTIMIZE_FOR_ALBUMART
			if (image_getJpegSize(file, 1, NULL, 0, &width, &height) < 0)
			{
				continue;
			}
		#else
			imsrc = image_new_from_jpeg(file, 1, NULL, 0, 1, ROTATE_NONE);
			if( !imsrc )
				continue;
		#endif
			/* End modify by zengdongbiao */
found_file:
		#ifndef MEMORY_OPTIMIZE_FOR_ALBUMART
			width = imsrc->width;
			height = imsrc->height;
		#endif
			if( width > 160 || height > 160 )
			{
				/* Secondly decompress image with appropriate scale, add by zengdongbiao, 25May15. */
			#ifdef MEMORY_OPTIMIZE_FOR_ALBUMART
				int scale = 1;
				int valueTmp = (width >= height)? width: height;
				
				/* Find appropriate scale for decompress, valueTmp = [160,320)	*/
				while (valueTmp >= 320)
				{
					valueTmp /= 2;
					scale *= 2;
					/* max scale: 1/8  */
					if (scale == 8)
					{
						break;
					}
				}
				
				imsrc = image_new_from_jpeg(file, 1, NULL, 0, scale, ROTATE_NONE);
				if( !imsrc )
				{
					return NULL;
				}	
			#endif
				/* End add by zengdongbiao. */
			
				art_file = save_resized_album_art(imsrc, file);
			}
			else
				art_file = strdup(file);
			if( imsrc != NULL)
			{
				image_free(imsrc);
			}
			return(art_file);
		}
	}
	return NULL;
}

#else /* USE_ALBUM_RESPONSE */

char *
check_embedded_art(const char *path, uint8_t *image_data, int image_size, char *pResolution)
{
	int width = 0, height = 0;
	char *art_path = NULL;
	
	if( !image_data || !image_size || !path || !pResolution)
	{
		return NULL;
	}

	if (image_getJpegSize(NULL, 0, image_data, image_size, &width, &height) < 0)
	{
		return NULL;
	}

	/* Album art image is too big. */
	if (width >= 1200 || height >= 1200)
	{
		return NULL;
	}

	snprintf(pResolution, 20, "%dx%d", width, height);

	art_path = strdup(path);
	
	return art_path;
}

static char *
check_for_album_file(const char *path, char *pResolution)
{
	char file[MAXPATHLEN];
	char mypath[MAXPATHLEN];
	struct album_art_name_s *album_art_name;
	char *art_file = NULL, *p = NULL;
	const char *dir;
	struct stat st;
	int ret;
	int width = 0, height = 0;

	if( stat(path, &st) != 0 )
		return NULL;

	if( S_ISDIR(st.st_mode) )
	{
		dir = path;
		goto check_dir;
	}
	strncpyt(mypath, path, sizeof(mypath));
	dir = dirname(mypath);

	/* First look for file-specific cover art */
	snprintf(file, sizeof(file), "%s.cover.jpg", path);
	ret = access(file, R_OK);
	if( ret != 0 )
	{
		strncpyt(file, path, sizeof(file));
		p = strrchr(file, '.');
		if( p )
		{
			strcpy(p, ".jpg");
			ret = access(file, R_OK);
		}
		if( ret != 0 )
		{
			p = strrchr(file, '/');
			if( p )
			{
				memmove(p+2, p+1, file+MAXPATHLEN-p-2);
				p[1] = '.';
				ret = access(file, R_OK);
			}
		}
	}
	if( ret == 0 )
	{
		if (image_getJpegSize(file, 1, NULL, 0, &width, &height) == 0)
		{
			/* Album art image is too big. */
			if (width >= 1200 || height >= 1200)
			{
				return NULL;
			}
			
			snprintf(pResolution, 20, "%dx%d", width, height);
			art_file = strdup(file);
			return art_file;
		}
	}
check_dir:
	/* Then fall back to possible generic cover art file names */
	for( album_art_name = album_art_names; album_art_name; album_art_name = album_art_name->next )
	{
		snprintf(file, sizeof(file), "%s/%s", dir, album_art_name->name);
		if( access(file, R_OK) == 0 )
		{
			if (image_getJpegSize(file, 1, NULL, 0, &width, &height) == 0)
			{
				/* Album art image is too big. */
				if (width >= 1200 || height >= 1200)
				{
					return NULL;
				}
				
				snprintf(pResolution, 20, "%dx%d", width, height);
				art_file = strdup(file);
				return art_file;
			}
		}
	}
	
	return NULL;
}


char *
get_embedded_art(uint8_t *image_data, int width, int height, int *pBufSize)
{
	char *album_buf = NULL;
	image_s *imsrc = NULL;
	int scale = 1;
	int valueTmp = (width >= height)? width: height;

	if( !image_data || !pBufSize || (*pBufSize == 0))
	{
		return NULL;
	}
	
	if( width > 160 || height > 160 )
	{	
		while (valueTmp > 160)
		{
			valueTmp /= 2;
			scale *= 2;
			/* max scale: 1/8  */
			if (scale == 8)
			{
				break;
			}
		}
		
		imsrc = image_new_from_jpeg(NULL, 0, image_data, *pBufSize, scale, ROTATE_NONE);

		if( !imsrc )
		{
			return NULL;
		}

		album_buf = (char *)image_save_to_jpeg_buf(imsrc, pBufSize);

		image_free(imsrc);
	}
	else if( width > 0 && height > 0 )
	{
		album_buf = malloc(*pBufSize);
		if (album_buf)
		{
			memcpy(album_buf, image_data, *pBufSize);
		}
	}

	return album_buf;
}

char *
get_album_file(const char *path, int width, int height, int *pBufSize, int *pAlbumType)
{
	image_s *imsrc = NULL;
	char *album_buf = NULL;
	int scale = 1;
	int valueTmp = (width >= height)? width: height;

	if ((path == NULL) || (pBufSize == NULL) || (pAlbumType == NULL))
	{
		return NULL;
	}

	if (0 != access(path, R_OK))
	{
		return NULL;
	}

	if( width > 160 || height > 160 )
	{		
		while (valueTmp > 160)
		{
			valueTmp /= 2;
			scale *= 2;
			/* max scale: 1/8  */
			if (scale == 8)
			{
				break;
			}
		}
		
		imsrc = image_new_from_jpeg(path, 1, NULL, 0, scale, ROTATE_NONE);

		if( !imsrc )
		{
			return NULL;
		}	

		album_buf = (char *)image_save_to_jpeg_buf(imsrc, pBufSize);

		image_free(imsrc);
	}
	else
	{
		*pAlbumType = ALBUM_FILE;
		album_buf = strdup(path);
	}
	
	return(album_buf);
}

#endif /* USE_ALBUM_RESPONSE */

int64_t
find_album_art(const char *path, uint8_t *image_data, int image_size)
{
	char *album_art = NULL;
#ifdef USE_ALBUM_RESPONSE
	char resolution[20] = {0};
#endif /* USE_ALBUM_RESPONSE */
	int64_t ret = 0;

#ifndef USE_ALBUM_RESPONSE
	if( (image_size && (album_art = check_embedded_art(path, image_data, image_size))) ||
	    (album_art = check_for_album_file(path)) )
	{
		ret = sql_get_int_field(db, "SELECT ID from ALBUM_ART where PATH = '%q'", album_art);
		if( !ret )
		{
			if( sql_exec(db, "INSERT into ALBUM_ART (PATH) VALUES ('%q')", album_art) == SQLITE_OK )
				ret = sqlite3_last_insert_rowid(db);
		}
		else/* the album_art has been inserted into TABLE ALBUM_ART.  */
		{
			if( SQLITE_OK !=
				sql_exec(db, "UPDATE ALBUM_ART set COUNT = (COUNT + 1) where ID = %lld", ret) )
			{
				ret = 0;/* set ret = 0 if update failed.  */
			}
		}
	}
#else /* USE_ALBUM_RESPONSE */
	if( (image_size && (album_art = check_embedded_art(path, image_data, image_size, resolution))) ||
	    (album_art = check_for_album_file(path, resolution)) )
	{
		if( sql_exec(db, "INSERT into ALBUM_ART (RESOLUTION, PATH) VALUES (%Q, '%q')", 
						resolution, album_art) == SQLITE_OK )
		{
				ret = sqlite3_last_insert_rowid(db);
		}
	}
#endif /* USE_ALBUM_RESPONSE */

	if (album_art != NULL)
	{
		free(album_art);
	}

	return ret;
}

#endif /* LITE_MINIDLNA */
