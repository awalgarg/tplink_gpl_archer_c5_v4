/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * access.c
 *
 * Routines to do very very simple access control based on filenames.
 */

#include "access.h"
#include "ls.h"
#include "tunables.h"
#include "str.h"
/* add by chz */
#include "defs.h"
#include "session.h"
#include "filestr.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
/* end add */


/* add by chz */
#define MAX_LEN_READ_ONLY_FILE  1280
/* end add */

/* Add by chz to load the read only list.
 * We can't do it later in the child process, or it can not open the
 * file with path "/var/vsftp/...", the root directory has changed.
 */
void vsf_read_only_load(struct vsf_session* p_sess)
{
	int ret;
	char path[128];

	sprintf(path, VSFTP_ETC_DIR"/%s_read_only_list", str_getbuf(&p_sess->user_str_real));

	ret = str_fileread(&p_sess->read_only_list, path, MAX_LEN_READ_ONLY_FILE);
	if (ret <= 0)
	{
		/* If fail, don't die and don't say anything, just return. 
		 * Not all the users have read only list. */
		return;
	}

	return;
}
/* end add */

/* add by chz for read only file list checking.*/
/* return 1 if it's read only, otherwise 0.*/
int
vsf_read_only_check(struct mystr* list_str, struct mystr* dir_name_str)
{
  struct mystr listEntry = INIT_MYSTR;
  char dir_name[128];
  char buf[128];
  int str_pos = 0;
  
  memset(buf, 0, sizeof(buf));
  memset(dir_name, 0, sizeof(dir_name));

  strcpy(dir_name, str_getbuf(dir_name_str));

  getcwd(buf, sizeof(buf));

  /* the home directory "/" is always read only */
  if (strlen(buf) == 1)
  {
  	return 1;
  }
  
  if (dir_name[0] == '/')
  {
  	sprintf(buf, "%s", dir_name);
  }

  while (str_getline(list_str, &listEntry, &str_pos))
  {
	if (strstr(buf, str_getbuf(&listEntry)))
	{
		str_free(&listEntry);
		return 1;
	}
	else
	{
		continue;
	}
  }

  str_free(&listEntry);
  return 0;
}
/* end add */

int
vsf_access_check_file(const struct mystr* p_filename_str)
{
  static struct mystr s_access_str;

  if (!tunable_deny_file)
  {
    return 1;
  }
  if (str_isempty(&s_access_str))
  {
    str_alloc_text(&s_access_str, tunable_deny_file);
  }
  if (vsf_filename_passes_filter(p_filename_str, &s_access_str))
  {
    return 0;
  }
  else
  {
    struct str_locate_result loc_res =
      str_locate_str(p_filename_str, &s_access_str);
    if (loc_res.found)
    {
      return 0;
    }
  }
  return 1;
}

int
vsf_access_check_file_visible(const struct mystr* p_filename_str)
{
  static struct mystr s_access_str;

  if (!tunable_hide_file)
  {
    return 1;
  }
  if (str_isempty(&s_access_str))
  {
    str_alloc_text(&s_access_str, tunable_hide_file);
  }
  if (vsf_filename_passes_filter(p_filename_str, &s_access_str))
  {
    return 0;
  }
  else
  {
    struct str_locate_result loc_res =
      str_locate_str(p_filename_str, &s_access_str);
    if (loc_res.found)
    {
      return 0;
    }
  }
  return 1;
}

