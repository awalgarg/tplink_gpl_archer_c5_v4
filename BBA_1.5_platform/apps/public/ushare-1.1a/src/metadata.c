/*
 * metadata.c : GeeXboX uShare CDS Metadata DB.
 * Originally developped for the GeeXboX project.
 * Copyright (C) 2005-2007 Benjamin Zores <ben@geexbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>

#include <upnp.h>
#include <upnptools.h>

#include "mime.h"
#include "metadata.h"
#include "util_iconv.h"
#include "content.h"
#include "gettext.h"
#include "trace.h"

//#define USE_FILE_MODE 1

/*
 * brief "4<>1"  = "dlna" ascii code - 0x30 
 */
#define DLNA_SIGNATURE	"4<>1"
#define DLNA_SIGN_OFFSET	4

#define DIR_FILE_LIMIT 100

#define TITLE_UNKNOWN "unknown"

#define FILE_MODEL_MAX_MEDIA_FILES_NUM	100000
#define MEM_MODEL_MAX_MEDIA_FILES_NUM	3000

#define DLNA_DIR		".dlna.rc.d"
#define DLNA_FILE		".dlnaServices.db"
#define DLNA_NEXT_PAGE	"!NextPage"

extern struct ushare_t *ut;

static record_t * record = NULL;
static int record_num = 0;
static FILE * record_db = NULL;
static char db_dir[128] = {0};
#ifdef USE_FILE_MODE
static char db_name[300] = {0};
static char record_cache[RECORD_BUFFER_SIZE] = {0};
static int record_cache_id = -1;
#endif
static pthread_mutex_t IO_mutex;


void record_index_log(char * title);


static char *
getExtension (const char *filename)
{
  char *str = NULL;

  str = strrchr (filename, '.');
  if (str)
    str++;

  return str;
}

static struct mime_type_t *
getMimeType (const char *extension)
{
  extern struct mime_type_t MIME_Type_List[];
  struct mime_type_t *list;

  if (!extension)
    return NULL;

  list = MIME_Type_List;
  while (list->extension)
  {
    if (!strcasecmp (list->extension, extension))
      return list;
    list++;
  }

  return NULL;
}

static bool
is_valid_extension (const char *extension)
{
  if (!extension)
    return false;

  if (getMimeType (extension))
    return true;

  return false;
}

/*port from netgear,(used when scandir), Added by LI CHENGLONG , 2011-Nov-13.*/

/* 
 * fn		int ends_with(const char * haystack, const char * needle)
 * brief	判断字符串是否是以needle指向的字符串结尾.		
 *
 * param[in]	haystack		源字符串
 * param[in]	needle			要查找的字符串
 *
 * return	int	
 * retval	0		字符串haystack 不是以needle结尾.	
 *
 * note	written by  13Nov11, LI CHENGLONG	
 */
int
ends_with(const char * haystack, const char * needle)
{
	const char * end;
	int nlen = strlen(needle);
	int hlen = strlen(haystack);

	if( nlen > hlen )
		return 0;
 	end = haystack + hlen - nlen;

	return (strcasecmp(end, needle) ? 0 : 1);
}


/* 
 * fn		int is_video(const char * file)
 * brief	判断一个文件是否是视频文件.
 * details	当增加视频格式时需要更新该函数.	
 *
 * param[in]	file	文件名
 *
 * return		int
 * retval		0		不是视频文件
 *				1		是视频文件
 *
 * note	written by  13Nov11, LI CHENGLONG	
 */
int
is_video(const char * file)
{
	return (ends_with(file, ".mpg") ||
		 	ends_with(file, ".mpeg")||
			ends_with(file, ".avi") ||
			ends_with(file, ".divx")||
			ends_with(file, ".asf") ||
			ends_with(file, ".wmv") ||
			ends_with(file, ".m4v") ||
			ends_with(file, ".mkv") ||
			ends_with(file, ".vob") ||
			ends_with(file, ".ts")  ||
			ends_with(file, ".mov") ||
			ends_with(file, ".dv")  ||
			ends_with(file, ".mjpg")||
			ends_with(file, ".mjpeg")  ||
			ends_with(file, ".mpe")  ||
			ends_with(file, ".mp2p")  ||
			ends_with(file, ".m1v")  ||
			ends_with(file, ".m2v")  ||
			ends_with(file, ".m4p")  ||
			ends_with(file, ".mp4") ||
			ends_with(file, ".mp4ps")  ||
			ends_with(file, ".ogm")  ||
			ends_with(file, ".rmvb")  ||
			ends_with(file, ".mov")  ||
			ends_with(file, ".qt")  ||
			ends_with(file, ".hdmov"));
	
}

/* 
 * fn		int is_audio(const char * file)
 * brief	判断一个文件是否是音频文件.	
 * details	当增加音频格式时需要更新该函数.	
 *
 * param[in]	file	文件名
 *
 * return	int	
 * retval	0			不是音频文件
 *			1			是音频文件
 *
 * note	written by  13Nov11, LI CHENGLONG	
 */
int
is_audio(const char * file)
{
	return (ends_with(file, ".mp3") ||
			ends_with(file, ".flac")||
			ends_with(file, ".wma") || 
			ends_with(file, ".aac") ||
			ends_with(file, ".wav") || 
			ends_with(file, ".ogg") ||
			ends_with(file, ".pcm") || 
			ends_with(file, ".ac3") ||
			ends_with(file, ".at3p")||
			ends_with(file, ".lpcm")||
			ends_with(file, ".aif") ||
			ends_with(file, ".aiff") ||
			ends_with(file, ".au") ||
			ends_with(file, ".m4a") ||
			ends_with(file, ".amr") ||
			ends_with(file, ".acm") ||
			ends_with(file, ".snd") ||
			ends_with(file, ".dts") ||
			ends_with(file, ".rmi") ||
			ends_with(file, ".aif") ||
			ends_with(file, ".mp1") ||
			ends_with(file, ".mp2") ||
			ends_with(file, ".mpa") ||
			ends_with(file, ".l16") ||
			ends_with(file, ".mka") ||
			ends_with(file, ".ra") ||
			ends_with(file, ".rm") ||
			ends_with(file, ".ram") ||
			ends_with(file, ".mid"));
}

/* 
 * fn		int is_image(const char * file)
 * brief	判断一个文件是否是图像文件.	
 * details	当增加音频格式时需要更新该函数.	
 *
 * param[in]	file	文件名
 *
 * return	int	
 * retval	0			不是图像文件
 *			1			是图像文件
 *
 * note	written by  13Nov11, LI CHENGLONG	
 */
int
is_image(const char * file)
{
	return (ends_with(file, ".jpg") || 
			ends_with(file, ".jpeg")||
			ends_with(file, ".jpe")||
			ends_with(file, ".png")||
			ends_with(file, ".bmp")||
			ends_with(file, ".ico")||
			ends_with(file, ".gif")||
			ends_with(file, ".pcd")||
			ends_with(file, ".pnm")||
			ends_with(file, ".ppm")||
			ends_with(file, ".qti")||
			ends_with(file, ".qtf")||
			ends_with(file, ".qtif")||
			ends_with(file, ".tif")||
			ends_with(file, ".tiff"));
}

/* 
 * fn		int is_playlist(const char * file)
 * brief	判断一个文件是否是playlist文件.	
 * details	当增加playlist格式时需要更新该函数.	
 *
 * param[in]	file	文件名
 *
 * return	int	
 * retval	0			不是playlist文件
 *			1			是playlist文件
 *
 * note	written by  13Nov11, LI CHENGLONG	
 */
 int
is_playlist(const char * file)
{
	return (ends_with(file, ".m3u") || ends_with(file, ".pls"));
}


/* 
 * fn		int filter_media(const struct dirent *d)
 * brief	扫描目录时的过滤器,只对感兴趣的媒体文件和目录进行收集.	
 *
 * param[in]	dirent		dir entry 目录项
 *
 * return		int
 * retval		0			不符合需求,被过滤掉
 *				1			符合需求,收集起来
 *
 * note	written by  13Nov11, LI CHENGLONG	
 */
int
filter_media(const struct dirent *d)
{
	return ( (*d->d_name != '.') &&
	         ((d->d_type == DT_DIR) ||
	          (d->d_type == DT_LNK) ||
	          (d->d_type == DT_UNKNOWN) ||
	          ((d->d_type == DT_REG) &&
	           (is_image(d->d_name) ||
	            is_audio(d->d_name) ||
	            is_video(d->d_name)
	           )
	       ) ));
}
/* Ended by LI CHENGLONG , 2011-Nov-13.*/

static xml_convert_t xml_convert[] = {
  {'"' , "&quot;"},
  {'&' , "&amp;"},
  {'\'', "&apos;"},
  {'<' , "&lt;"},
  {'>' , "&gt;"},
  {'\n', "&#xA;"},
  {'\r', "&#xD;"},
  {'\t', "&#x9;"},
  {0, NULL},
};

static char *
get_xmlconvert (int c)
{
  int j;
  for (j = 0; xml_convert[j].xml; j++)
  {
    if (c == xml_convert[j].charac)
      return xml_convert[j].xml;
  }
  return NULL;
}

static char *
convert_xml (char *title)
{
  char *s, *t, *xml;
  char newtitle[MAX_TITLE_SIZE];
  int nbconvert = 0;

  /* calculate extra size needed */
  for (t = (char*) title; *t; t++)
  {
    xml = get_xmlconvert (*t);
    if (xml)
      nbconvert += strlen (xml) - 1;
  }
  if (!nbconvert)
    return NULL;

  s = newtitle;

  for (t = (char*) title; *t; t++)
  {
  	nbconvert = s - newtitle;
	if (nbconvert >= MAX_TITLE_SIZE)
	{
		break;
	}
    xml = get_xmlconvert (*t);
    if (xml)
    {
      strncpy (s, xml, MAX_TITLE_SIZE - nbconvert);
      s += strlen (xml);
    }
    else
      *s++ = *t;
  }
  newtitle[MAX_TITLE_SIZE - 1] = '\0';

  strcpy(title, newtitle);

  return title;
}

static struct mime_type_t Container_MIME_Type =
  { NULL, "object.container.storageFolder", NULL};


int getChildCount(int id)
{
	//USHARE_DEBUG("get count id: %d\n", id);
	int i = 0;
	int count = 0;
	
	for (i = 0; i < record_num; i++)
	{
		if (record[i].parent == id)
		{
			count++;
		}
	}

	return (count ? count : -1);
}

int getChild(int id, int location, int index)
{
	int i = 0;
	int j = 0;
	//USHARE_DEBUG("get child id: %d location:%d\n", id, location);
	if (location < 0)
	{
		USHARE_ERROR("location error: %d\n", location);
		return -1;
	}
	
	if (location == 0)
	{
		location = ut->starting_id;
	}
	else
	{
		location++;
	}
	
	for (i = location - ut->starting_id; i < record_num && j < index; i++)
	{
		if (record[i].parent == id)
		{
			j++;
		}
	}

	//USHARE_DEBUG("id: %d index: %d location: %d child id is %d\n", id, index, i, record[i].id);
	if (j < index)
	{
		USHARE_ERROR("index too large : %d\n", index);
		return -1;
	}
	return ut->starting_id + i - 1;
}

struct mime_type_t *getRecordMimeType(int dir, const char * name)
{
	struct mime_type_t *mime = NULL;
   if (!dir) /* item */
    {
      mime = getMimeType (getExtension (name));
      if (!mime)
      {
        log_error ("Invalid Mime type for %s, entry ignored", name);
        return NULL;
      }
      
    }
  else /* container */
    {
      mime = &Container_MIME_Type;
    }

  return mime;
}

dlna_profile_t * getDlna_profile(const char *fullpath, int dir)
{
#ifdef HAVE_DLNA
  dlna_profile_t *p = NULL;

  if (ut->dlna_enabled && fullpath && !dir)
  {

	p = dlna_guess_media_profile2 (ut->dlna, fullpath);
	
    if (!p)
    {
      return NULL;
    }
   return p;
  }
#endif /* HAVE_DLNA */

  return NULL;
}

char * getURL(int id, const char * name, char * url, int dir)
{
	if (!dir)
	{
      if (snprintf (url, MAX_URL_SIZE, "%d.%s",
                    id, getExtension (name)) >= MAX_URL_SIZE)
        log_error ("URL string too long for id %d, url:%s, truncated!!", id, url);

	  return url;
    }
 	else /* container */
    {
     return NULL;
    }

	log_error("can not be here!\n");
	return NULL;
}

char * makeTitle(struct ushare_t *ut, int id, const char *name, char * title, int dir)
{
  char *x = NULL;
  char *title_or_name = NULL;

  if (!name)
    return NULL;
    
  /* Try Iconv'ing the name but if it fails the end device
     may still be able to handle it */
  if (iconv_convert (name, title))
  {
    title_or_name = title;
  }
  else
  {
    if (ut->override_iconv_err)
    {
      strcpy(title, name);
	  title_or_name = title;
      log_error ("Entry invalid name id=%d [%s]\n", id, name);
    }
    else
    {
      title[0] = '\0';
      log_error ("Freeing entry invalid name id=%d [%s]\n", id, name);
      return NULL;
    }
  }

  if (!dir)
  {
    x = strrchr (title_or_name, '.');
    if (x)  /* avoid displaying file extension */
      *x = '\0';
  }
  x = convert_xml (title_or_name);

  if (!strcmp (title_or_name, "")) /* DIDL dc:title can't be empty */
  {
	 strcpy(title, TITLE_UNKNOWN);
  }
	
  return title;
}

char * getRecord(int id, char * dst)
{
	int index = 0;
	if (id == 0)
	{
		memset(dst, 0, RECORD_BUFFER_SIZE);
		char * title = dst + OFFSET_TITLE;
		makeTitle(ut, id, "root", title, 1);
		*(dst + OFFSET_DIR) = 1;
		return dst;
	}
	pthread_mutex_lock (&IO_mutex);

	index = id - ut->starting_id;
	
	if (index >= record_num || index < 0)
	{
		USHARE_ERROR("id:%d unreachable\n", id);
		pthread_mutex_unlock (&IO_mutex);
		return NULL;
	}
#ifdef USE_FILE_MODE
	if (id == record_cache_id)
	{
		memcpy(dst, record_cache, RECORD_BUFFER_SIZE);
		pthread_mutex_unlock (&IO_mutex);
		return dst;
	}

	if(fseek(record_db, RECORD_BUFFER_SIZE * index + DLNA_SIGN_OFFSET, SEEK_SET) < 0)
	{	
		USHARE_ERROR("data file has problem. Now rebuild list.\n");
		pthread_mutex_unlock (&IO_mutex);
		return NULL;
	}

	if(fread(dst, RECORD_BUFFER_SIZE, 1, record_db) < 1)
	{
		USHARE_ERROR("read file error.\n");
		pthread_mutex_unlock (&IO_mutex);
		return NULL;
	}

	record_cache_id = id;
	memcpy(record_cache, dst, RECORD_BUFFER_SIZE);
#else
	memcpy(dst, ut->record_buf + index * RECORD_BUFFER_SIZE, RECORD_BUFFER_SIZE);
#endif
	pthread_mutex_unlock (&IO_mutex);
	return dst;
}


void free_metadata_data()
{
	free_metadata_list(ut, REBUILD_CHANGE);

	if (record)
	{
		free(record);
		record = NULL;
	}

  
	if (ut->record_buf)
	{
		free(ut->record_buf);
		ut->record_buf = NULL;
	}
	
}

void free_metadata_list (struct ushare_t *ut, int flag)
{

	USHARE_DEBUG("free all metatata list...\n");
	
#ifdef USE_FILE_MODE
  	if (record_db)
  	{
  		fclose(record_db);
		record_db = NULL;

		remove(db_name);
		remove(db_dir);	
 	}
#endif

  	record_num = 0;
  	if (ut->init != flag)
  	{
     	ut->init = flag;
  	}
  	ut->nr_entries = 0;

  	ut->mediaFilesNum = 0;
}

int
upnp_record_new (struct ushare_t *ut, const char *fullpath, const char * name,
						off_t size, int pDirID, int dir)
{
	int id = 0; 
	char * title = NULL;
#ifdef USE_FILE_MODE
	char buff[RECORD_BUFFER_SIZE] = {0};
#else
	char * buff = (char *)(ut->record_buf + record_num * RECORD_BUFFER_SIZE);
#endif

	if (!fullpath)
	{
		return 0;
	}

#ifdef USE_FILE_MODE
	if (record_num >= FILE_MODEL_MAX_MEDIA_FILES_NUM)
#else
	if (record_num >= MEM_MODEL_MAX_MEDIA_FILES_NUM)
#endif
	{
		return NO_INDEX_SPACE;
	}


    id = ut->starting_id + ut->nr_entries;

	record[record_num].parent = pDirID;
	  
	ut->nr_entries++;

	strncpy(buff, fullpath, MAX_FULLPATH_SIZE);
	  
	title = buff + OFFSET_TITLE; 
	makeTitle(ut, id, name, title, dir);
	  
	memcpy(buff + OFFSET_SIZE, &size, sizeof(size));

	*(buff + OFFSET_DIR) = (unsigned char)dir;

	memcpy(buff + OFFSET_PARENT_ID, &pDirID, sizeof(pDirID));

#ifdef USE_FILE_MODE
  	if(fwrite(buff, RECORD_BUFFER_SIZE, 1, record_db) < 1)
  	{
  		perror("fwrite error form upnp_record_new");
		return FILE_OP_ERROR;
  	}
#endif

	record_num++;

	  //USHARE_DEBUG("get id: %d, name: %s\n", id, name);
	  //record_index_log(fullpath);
	return id;
}

void
upnp_record_new_undo ()
{
    ut->nr_entries--;
	record_num--;
	record[record_num].parent = -1;

#ifdef USE_FILE_MODE
	if(fseek(record_db, -RECORD_BUFFER_SIZE, SEEK_CUR) < 0)
	{
		perror("fseek error in build_container");
		return;
	}
#endif
}


static int
metadata_add_record (struct ushare_t *ut,
                   const char *file, const char *name, struct stat *st_ptr, int pDirID)
{
  if (!file || !name)
    return NOT_MEDIA;

#ifdef HAVE_DLNA
  if (ut->dlna_enabled && is_valid_extension (getExtension (file)))
#else
  if (is_valid_extension (getExtension (file)))
#endif
  {
    int child;
	
    child = upnp_record_new (ut, file, name, st_ptr->st_size, pDirID, FALSE);

	if (child < 0)
	{
		return child;
	}
	
	ut->mediaFilesNum++;
	return child;
  }

return NOT_MEDIA;
}


int metadata_add_record_container (struct ushare_t *ut, int pDirID, const char *container)
{
 // struct dirent **namelist;
	struct dirent namelist;
	struct dirent *namelistPtr = &namelist;
	DIR * dir;
	char fullpath[512] = {0};
	int dir_num = 0;
	int isEmpty = 1;
	int isNextPageEmpty = -1;
	int nextPage_count = 0;
	int flag = 0;
	int id = 0;

	if (!container)
  	{
  		USHARE_DEBUG("parent or container is null\n");
 		return 0;
	}
  	memset(&namelist, 0, sizeof(namelist));

  	dir = opendir(container);
  	if (!dir)
  	{
		perror("opendir");
		return 0;
  	}
  
  	do
  	{
   		struct stat st;

#ifdef USE_FILE_MODE
		if (ut->nr_entries >= FILE_MODEL_MAX_MEDIA_FILES_NUM)
#else
		if (ut->nr_entries >= MEM_MODEL_MAX_MEDIA_FILES_NUM)
#endif
		{
			flag = NO_INDEX_SPACE;
	  		break;
		}

		if (readdir_r(dir, &namelist, &namelistPtr))
		{
			if (errno == EINTR)
			{
				continue;
			}
		
			perror("readdir_r");
			printf("err path = %s\n",container);
			closedir(dir);
			return 0;
		}

		if (!namelistPtr)
		{
			break;
		}
		
		if (!filter_media(&namelist))
		{
			continue;
		}

	    if (namelist.d_name[0] == '.')
	    {
	      	continue;
	    }

	    sprintf (fullpath, "%s/%s", container, namelist.d_name);

	    log_verbose ("%s\n", fullpath);

	    if (lstat (fullpath, &st) < 0)
	    {
	      	continue;
	    }
    
	    if (namelist.d_type == DT_DIR) 
	    {
	      	if ((flag = upnp_record_new(ut, fullpath, namelist.d_name, 0, pDirID, true)) < 0)
	      	{
	      		closedir(dir);
	      		return flag;
	      	}

			id = flag;
	      	if((flag = metadata_add_record_container (ut, id, fullpath)) < 0)
	      	{
	          	if (flag == NO_SUBCHILD)
	      	  	{
	      	  		//USHARE_DEBUG("remove new record.\n");
	      			upnp_record_new_undo();
					continue;
	      		}
				else if (flag == NO_INDEX_SPACE)
				{
					break;
				}
				else
				{
					closedir(dir);
	      			return flag;
				}
	     	}
			dir_num++;
			isEmpty = 0;
			isNextPageEmpty = 0;
	    }
	    else
	    {
		    flag = metadata_add_record(ut, fullpath, namelist.d_name, &st, pDirID);
		    if(flag)
		    {
		      	if (flag < 0)
		      	{
		      		if (flag == NO_INDEX_SPACE)
		      		{
		      			break;
		      		}
		      		closedir(dir);
		      		return flag;
		      	}
		      	isEmpty = 0;		// make sure that dir is not empty.
		      	isNextPageEmpty = 0;
		      	dir_num++;
		    }
	    }

		if (dir_num >= DIR_FILE_LIMIT)
		{
			sprintf (fullpath, "%s/%s", container, DLNA_NEXT_PAGE);
			if((flag = upnp_record_new(ut, fullpath, DLNA_NEXT_PAGE, 0, pDirID, true)) < 0)
			{
	      		if (flag == NO_INDEX_SPACE)
	      		{
	      			break;
	      		}
	      		closedir(dir);
				return flag;
			}

			pDirID = flag;
			
			nextPage_count++;
			dir_num = 0;
			isNextPageEmpty = 1;
		}
  	}while (namelistPtr);

	if (dir_num == 0 && isNextPageEmpty == 1)
	{
		upnp_record_new_undo();
		nextPage_count--;
	}

	if (nextPage_count)
	{
		dir_num = DIR_FILE_LIMIT + 1;
	}
	
    closedir(dir);
	return (flag == NO_INDEX_SPACE ? NO_INDEX_SPACE : (isEmpty ? NO_SUBCHILD : dir_num));
}

#if 1
void record_index_log(char * title)
{
	FILE * log = NULL;
    int i = 0;
	char name[512] = {0};

	sprintf(name, "%s/%s", db_dir, "log.log");

	if (!(log = fopen(name, "w+")))
	{
		perror("fopen from log");
		return;
	}

	fprintf(log, "%s\r\n", title);

	fprintf(log, "\r\n%s", "record:\r\n");

	i = 0;
	while(i < record_num)	
	{
		fprintf(log, "【%d】(%d, %d) ", i, i + ut->starting_id, record[i].parent);
		i++;
	}

	fprintf(log, "\r\n");
	fclose(log);
}
#endif

void
build_metadata_list (struct ushare_t *ut, int flag)
{
  	int i;
	static int rebuildIndex = 0;
	int id = 0;
	char signature[10] = DLNA_SIGNATURE;
    char *title = NULL;
#ifdef USE_FILE_MODE
	char *pTmp = NULL;
#endif
    int size = 0;
	int err_flag = 0;

	/* remove by lwj, for bug 101193
	if (!ut->contentlist->content)
	{
		return;
	}
	*/

	pthread_mutex_init (&IO_mutex, NULL);
	pthread_mutex_lock (&IO_mutex);

#ifdef USE_FILE_MODE
	if ((pTmp = strstr(ut->contentlist->content[0], "/sd")) == NULL)
	{
		if ((pTmp = strstr(ut->contentlist->content[0], "/hd")) == NULL)
		{
			USHARE_ERROR("unknow usb device:%s\n", ut->contentlist->content[0]);
			pthread_mutex_unlock (&IO_mutex);
			return;
		}
	}

	pTmp++;
	if ((pTmp = strchr(pTmp, '/')) == NULL)
	{
		USHARE_ERROR("illegal share folder:%s\n", pTmp);
		pthread_mutex_unlock (&IO_mutex);
		return;
	}

	pTmp++;
	strncpy(db_dir, ut->contentlist->content[0], pTmp - ut->contentlist->content[0]);
	db_dir[pTmp - ut->contentlist->content[0]] = '\0';
	strcat(db_dir, DLNA_DIR);

	if (mkdir(db_dir, S_IRWXU | S_IRWXG) < 0 && errno != EEXIST)
	{		
		perror("mkdir error");
		pthread_mutex_unlock (&IO_mutex);
		return;
	}
	
	if (chmod(db_dir, 0777) < 0)
	{
		USHARE_ERROR("Fail to chmod(). ERR:%d, %s\n", errno, strerror(errno));
	}
	
	sprintf(db_name, "%s/%s", db_dir, DLNA_FILE);
	USHARE_DEBUG("db_name: %s\n", db_name);

	if (0 == access(db_name, R_OK))
	{
		if(!(record_db = fopen(db_name, "rb")))
		{
			perror("fopen error");
			pthread_mutex_unlock (&IO_mutex);
			return;
		}
		
		if(fread(signature, 4, 1, record_db) < 1)
		{
			USHARE_ERROR("read file error.\n");
			fclose(record_db);
			record_db = NULL;
			pthread_mutex_unlock (&IO_mutex);
			return;
		}

		if (strncmp(signature, DLNA_SIGNATURE, 4))
		{
			USHARE_ERROR("user file at same name.\n");
			fclose(record_db);
			record_db = NULL;
			pthread_mutex_unlock (&IO_mutex);
			return;		
		}

		fclose(record_db);
		record_db = NULL;
	}
	
	if(!(record_db = fopen(db_name, "wb+")))
	{
		perror("fopen error from check_model");
		pthread_mutex_unlock (&IO_mutex);
		return;
	}

  	if(fwrite(DLNA_SIGNATURE, DLNA_SIGN_OFFSET, 1, record_db) < 1)
  	{
  		perror("fwrite error form upnp_record_new");
		pthread_mutex_unlock (&IO_mutex);
		return;
  	}
  	fflush(record_db);

	if (record == NULL)
	{
		if (!(record = realloc(record, FILE_MODEL_MAX_MEDIA_FILES_NUM * sizeof(record_t))))
		{
			perror("malloc error from record\n");
			pthread_mutex_unlock (&IO_mutex);
			return;
		}
	}
	USHARE_DEBUG("malloc record success!\n");

	memset(record, 0, FILE_MODEL_MAX_MEDIA_FILES_NUM * sizeof(record_t));
#else /* #ifdef USE_FILE_MODE */
	if (record == NULL)
	{
		if (!(record = realloc(record, MEM_MODEL_MAX_MEDIA_FILES_NUM * sizeof(record_t))))
		{
			perror("malloc error from record\n");
			pthread_mutex_unlock (&IO_mutex);
			return;
		}
	}
	USHARE_DEBUG("malloc record success!\n");

	if (ut->record_buf == NULL)
	{
		if (!(ut->record_buf = realloc(ut->record_buf, MEM_MODEL_MAX_MEDIA_FILES_NUM * RECORD_BUFFER_SIZE)))
		{
			perror("malloc error from record buf\n");
			pthread_mutex_unlock (&IO_mutex);
			return;
		}
	}
	USHARE_DEBUG("malloc record buf success!\n");
	
	memset(record, 0, MEM_MODEL_MAX_MEDIA_FILES_NUM * sizeof(record_t));
	memset(ut->record_buf, 0, MEM_MODEL_MAX_MEDIA_FILES_NUM * RECORD_BUFFER_SIZE);
#endif /* #ifdef USE_FILE_MODE */
	
	rebuildIndex++;
	if (rebuildIndex > STARTING_ENTRY_ID_CYCLE)
	{
		rebuildIndex = 0;
	}

    /*添加 flag 为了防止自动扫描时，改变starting_id*/
	if (!flag)
	{
		if (ut->xbox360)
		{
  			ut->starting_id = STARTING_ENTRY_ID_XBOX360 + rebuildIndex * STARTING_ENTRY_ID_STEP;
		}
		else
		{
			ut->starting_id = STARTING_ENTRY_ID_DEFAULT + rebuildIndex * STARTING_ENTRY_ID_STEP;
		}
	}
	
  	USHARE_DEBUG("media list starting id: %d\n", ut->starting_id);
	if (!ut->contentlist)
	{	
		USHARE_DEBUG("ut->conententlist=null\n");
		ut->contentlist = (content_list*) malloc (sizeof(content_list));
		ut->contentlist->content = NULL;
		ut->contentlist->displayName = NULL;
		ut->contentlist->count = 0;
	}
  
  /* add files from content directory */
  	for (i=0 ; i < ut->contentlist->count ; i++)
	{
	    log_info (_("Looking for files in content directory : %s\n"),
	              ut->contentlist->content[i]);

	    size = strlen (ut->contentlist->content[i]);
	    if (ut->contentlist->content[i][size - 1] == '/')
	      ut->contentlist->content[i][size - 1] = '\0';


		if (strlen(ut->contentlist->displayName[i]) > 0)
		{
			title = ut->contentlist->displayName[i];
		}	
		else
		{
		    title = strrchr (ut->contentlist->content[i], '/');
		    if (title)
		      title++;
		    else
		    {
		      title = ut->contentlist->content[i];
		    }
		}
		
	    if((err_flag = upnp_record_new(ut, ut->contentlist->content[i], title, 0, 0, true)) < 0)
	    {
			if (err_flag == NO_INDEX_SPACE)
			{
				USHARE_DEBUG("reach NO_INDEX_SPACE.\n");
				break;
			}
		
			log_error("build metadata list fail \n");
			free_metadata_list(ut, REBUILD_CHANGE);
			pthread_mutex_unlock (&IO_mutex);
			return;
	    }

		id = err_flag;
	    if ((err_flag = metadata_add_record_container (ut, id, ut->contentlist->content[i])) < 0)
	    {
			if (err_flag == NO_SUBCHILD)
			{
				upnp_record_new_undo();
				continue;
			}
			else if (err_flag == NO_INDEX_SPACE)
			{
				USHARE_DEBUG("reach NO_INDEX_SPACE.\n");
				break;
			}
			else
			{
				log_error("build metadata list fail \n");
				free_metadata_list(ut, REBUILD_CHANGE);
				pthread_mutex_unlock (&IO_mutex);
				return;
			}
	    }
	}

  	fflush(record_db);
  	log_info (_("Found %d files and subdirectories.\n"), ut->nr_entries);

 	ut->init = 1;
  	pthread_mutex_unlock (&IO_mutex);
  //	record_index_log("NULL");
}

