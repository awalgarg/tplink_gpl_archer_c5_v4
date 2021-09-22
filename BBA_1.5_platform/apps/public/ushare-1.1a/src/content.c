/*
 * content.c : GeeXboX uShare content list
 * Originally developped for the GeeXboX project.
 * Copyright (C) 2005-2007 Alexis Saettler <asbin@asbin.org>
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
#include <stdio.h>
#include <string.h>

#include "content.h"
#include "ushare_tplink.h"

content_list *
content_add(content_list *list, const char *item, const char *name)
{
	
  if (!list)
  {
   	USHARE_DEBUG("list null\n");
    list = (content_list*) malloc (sizeof(content_list));
    list->content = NULL;
	list->displayName = NULL;
    list->count = 0;
  }
  if (item)
  {
  	USHARE_DEBUG("add entry to content, item=%s, name=%s\n", item, name);
	list->count++;
	
    list->content = (char**) realloc (list->content, list->count * sizeof(char*));
    if (!list->content)
    {
      perror ("error realloc");
      exit (2);
    }
    list->content[list->count-1] = strdup (item);
	list->displayName = (char**) realloc (list->displayName,  list->count * sizeof(char*));
	if (!list->displayName)
	{
		perror ("error realloc");		
		exit (2);
	}
	list->displayName[list->count-1] = strdup (name);

	USHARE_DEBUG("list->count=%d\n", list->count);
	
  }
  return list;
}

/*
 * Remove the n'th content (start from 0)
 */
content_list *
content_del(content_list *list, int n)
{
	  int i;
	
	  if (!list || n >= list->count)
		  return NULL;
	
	  if (n >= list->count)
		  return list;
	
	  if (list->content[n] || list->displayName[n])
	  {
	  if (list->content[n])
		  free (list->content[n]);
		  if (list->displayName[n])
			  free (list->displayName[n]);
		  
		  for (i = n ; i < list->count - 1 ; i++)
		  {
			  list->content[i] = list->content[i+1];
			  list->displayName[i] = list->displayName[i + 1];
		  }
			  
		  list->count--;
		  list->content[list->count] = NULL;
		  list->displayName[list->count] = NULL;
	  }
	return list;

}

void
content_free(content_list *list_content)
{
	int i;

	if (!list_content)
	{
	  return;
	}
	USHARE_DEBUG("free all content...\n");	
	if (list_content->content)
	{
	  for (i=0 ; i < list_content->count ; i++)
	  {
		if (list_content->content[i])
		  free (list_content->content[i]);
	  }
	  free (list_content->content);
	}
	
	/* free name. by HouXB, 21Jan11 */	  
	if (list_content->displayName)
	{
	  for (i=0 ; i < list_content->count; i++)
	  {
		if (list_content->displayName[i])
		  free (list_content->displayName[i]);
	  }
	  free (list_content->displayName);
	}
	free (list_content);
}
