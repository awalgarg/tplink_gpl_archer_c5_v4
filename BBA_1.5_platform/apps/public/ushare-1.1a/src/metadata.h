/*
 * metadata.h : GeeXboX uShare CDS Metadata DB header.
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

#ifndef _METADATA_H_
#define _METADATA_H_

#include <upnp.h>
#include <upnptools.h>
#include <stdbool.h>
#include <sys/types.h>

#include "ushare.h"
#include "http.h"
#include "content.h"

#define MAX_FULLPATH_SIZE 	260
#define MAX_TITLE_SIZE		239
#define MAX_URL_SIZE 		32

#define RECORD_BUFFER_SIZE	512
#define RECORD_LENGTH		512

#define OFFSET_TITLE		260
#define OFFSET_SIZE			499
#define OFFSET_DIR			507
#define OFFSET_PARENT_ID	508

#define REBUILD_CHANGE		0
#define REBUILD_NOT_CHANGE	1

#define NOT_MEDIA			0
#define FILE_OP_ERROR		-1
#define MEM_OP_ERROR		-2
#define NO_SUBCHILD			-3
#define NO_INDEX_SPACE		-4


typedef struct xml_convert_s {
  char charac;
  char *xml;
} xml_convert_t;

typedef struct record_s {
  int parent;
}record_t;

void free_metadata_data();
void free_metadata_list (struct ushare_t *ut, int flag);
void build_metadata_list (struct ushare_t *ut,int flag);
char * getRecord(int id, char * dst);
int getChildCount(int id);
int getChild(int id, int location, int index);
char * getURL(int id, const char * name, char * url, int dir);
struct mime_type_t *getRecordMimeType(int dir, const char * name);
dlna_profile_t * getDlna_profile(const char *fullpath, int dir);
#endif /* _METADATA_H_ */
