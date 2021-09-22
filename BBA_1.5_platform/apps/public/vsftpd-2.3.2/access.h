#ifndef VSF_ACCESS_H
#define VSF_ACCESS_H

struct mystr;

/* Add by chz to load the read only list.
 * We can't do it later in the child process, or it can not open the
 * file with path "/var/vsftp/...", the root directory has changed.
 */
void vsf_read_only_load();

/* add by chz for read only file list checking.*/
/* vsf_read_only_check()
 * return 1 if it's read only, otherwise 0.
 */
int
vsf_read_only_check(struct mystr* list_str, struct mystr* dir_name_str);
/* end add */


/* vsf_access_check_file()
 * PURPOSE
 * Check whether the current session has permission to access the given
 * filename.
 * PARAMETERS
 * p_filename_str  - the filename to check access for
 * RETURNS
 * Returns 1 if access is granted, otherwise 0.
 */
int vsf_access_check_file(const struct mystr* p_filename_str);

/* vsf_access_check_file_visible()
 * PURPOSE
 * Check whether the current session has permission to view the given
 * filename in directory listings.
 * PARAMETERS
 * p_filename_str  - the filename to check visibility for
 * RETURNS
 * Returns 1 if the file should be visible, otherwise 0.
 */
int vsf_access_check_file_visible(const struct mystr* p_filename_str);

#endif /* VSF_ACCESS_H */

