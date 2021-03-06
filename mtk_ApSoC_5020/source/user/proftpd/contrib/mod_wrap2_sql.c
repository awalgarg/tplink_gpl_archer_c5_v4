/*
 * ProFTPD: mod_wrap2_sql -- a mod_wrap2 sub-module for supplying IP-based
 *                           access control data via SQL tables
 *
 * Copyright (c) 2002-2007 TJ Saunders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, TJ Saunders gives permission to link this program
 * with OpenSSL, and distribute the resulting executable, without including
 * the source code for OpenSSL in the source distribution.
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/proftpd/contrib/mod_wrap2_sql.c#1 $
 */

#include "mod_wrap2.h"
#include "mod_sql.h"

#define MOD_WRAP2_SQL_VERSION		"mod_wrap2_sql/1.0"

static cmd_rec *sql_cmd_create(pool *parent_pool, int argc, ...) {
  pool *cmd_pool = NULL;
  cmd_rec *cmd = NULL;
  register unsigned int i = 0;
  va_list argp;

  cmd_pool = make_sub_pool(parent_pool);
  cmd = (cmd_rec *) pcalloc(cmd_pool, sizeof(cmd_rec));
  cmd->pool = cmd_pool;

  cmd->argc = argc;
  cmd->argv = (char **) pcalloc(cmd->pool, argc * sizeof(char *));

  /* Hmmm... */
  cmd->tmp_pool = cmd->pool;

  va_start(argp, argc);
  for (i = 0; i < argc; i++)
    cmd->argv[i] = va_arg(argp, char *);
  va_end(argp);

  return cmd;
}

static int sqltab_close_cb(wrap2_table_t *sqltab) {
  return 0;
}

static array_header *sqltab_fetch_clients_cb(wrap2_table_t *sqltab,
    const char *name) {
  pool *tmp_pool = NULL;
  cmdtable *sql_cmdtab = NULL;
  cmd_rec *sql_cmd = NULL;
  modret_t *sql_res = NULL;
  array_header *sql_data = NULL;
  char *query = NULL, **vals = NULL;
  array_header *clients_list = NULL;

  /* Allocate a temporary pool for the duration of this read. */
  tmp_pool = make_sub_pool(sqltab->tab_pool);

  query = ((char **) sqltab->tab_data)[0];

  /* Find the cmdtable for the sql_lookup command. */
  sql_cmdtab = pr_stash_get_symbol(PR_SYM_HOOK, "sql_lookup", NULL, NULL);
  if (sql_cmdtab == NULL) {
    wrap2_log("error: unable to find SQL hook symbol 'sql_lookup'");
    destroy_pool(tmp_pool);
    return NULL;
  }

  /* Prepare the SELECT query. */
  sql_cmd = sql_cmd_create(tmp_pool, 3, "sql_lookup", query, name);

  /* Call the handler. */
  sql_res = call_module(sql_cmdtab->m, sql_cmdtab->handler, sql_cmd);

  /* Check the results. */
  if (!sql_res) {
    wrap2_log("NamedQuery '%s' returned no data", query);
    destroy_pool(tmp_pool);
    return NULL;
  }

  if (MODRET_ISERROR(sql_res)) {
    wrap2_log("error processing NamedQuery '%s'", query);
    destroy_pool(tmp_pool);
    return NULL;
  }

  /* Construct a single string, concatenating the returned client tokens
   * together.
   */
  sql_data = (array_header *) sql_res->data;
  vals = (char **) sql_data->elts;

  if (sql_data->nelts < 1) {
    wrap2_log("NamedQuery '%s' returned no data", query);
    destroy_pool(tmp_pool);
    return NULL;
  }

  clients_list = make_array(sqltab->tab_pool, sql_data->nelts, sizeof(char *));
  *((char **) push_array(clients_list)) = pstrdup(sqltab->tab_pool, vals[0]);

  if (sql_data->nelts > 1) {
    register unsigned int i = 0;

    for (i = 1; i < sql_data->nelts; i++) {
      *((char **) push_array(clients_list)) = pstrdup(sqltab->tab_pool,
        vals[i]);
    }
  }

  destroy_pool(tmp_pool);
  return clients_list;
}

static array_header *sqltab_fetch_daemons_cb(wrap2_table_t *sqltab,
    const char *name) {
  array_header *daemons_list = make_array(sqltab->tab_pool, 1, sizeof(char *));

  /* Simply return the service name we're given. */
  *((char **) push_array(daemons_list)) = pstrdup(sqltab->tab_pool, name);

  return daemons_list;
}

static array_header *sqltab_fetch_options_cb(wrap2_table_t *sqltab,
    const char *name) {
  pool *tmp_pool = NULL;
  cmdtable *sql_cmdtab = NULL;
  cmd_rec *sql_cmd = NULL;
  modret_t *sql_res = NULL;
  array_header *sql_data = NULL;
  char *query = NULL, **vals = NULL;
  array_header *options_list = NULL;

  /* Allocate a temporary pool for the duration of this read. */
  tmp_pool = make_sub_pool(sqltab->tab_pool);

  query = ((char **) sqltab->tab_data)[1];

  /* The options-query is not necessary.  Skip if not present. */
  if (!query)
    return NULL;

  /* Find the cmdtable for the sql_lookup command. */
  sql_cmdtab = pr_stash_get_symbol(PR_SYM_HOOK, "sql_lookup", NULL, NULL);
  if (sql_cmdtab == NULL) {
    wrap2_log("error: unable to find SQL hook symbol 'sql_lookup'");
    destroy_pool(tmp_pool);
    return NULL;
  }

  /* Prepare the SELECT query. */
  sql_cmd = sql_cmd_create(tmp_pool, 3, "sql_lookup", query, name);

  /* Call the handler. */
  sql_res = call_module(sql_cmdtab->m, sql_cmdtab->handler, sql_cmd);

  /* Check the results. */
  if (!sql_res) {
    wrap2_log("NamedQuery '%s' returned no data", query);
    destroy_pool(tmp_pool);
    return NULL;
  }

  if (MODRET_ISERROR(sql_res)) {
    wrap2_log("error processing NamedQuery '%s'", query);
    destroy_pool(tmp_pool);
    return NULL;
  }

  /* Construct a single string, concatenating the returned client tokens
   * together.
   */
  sql_data = (array_header *) sql_res->data;
  vals = (char **) sql_data->elts;

  if (sql_data->nelts < 1) {
    wrap2_log("NamedQuery '%s' returned no data", query);
    destroy_pool(tmp_pool);
    return NULL;
  }

  options_list = make_array(sqltab->tab_pool, sql_data->nelts, sizeof(char *));
  *((char **) push_array(options_list)) = pstrdup(sqltab->tab_pool, vals[0]);

  if (sql_data->nelts > 1) {
    register unsigned int i = 0;

    for (i = 1; i < sql_data->nelts; i++) {
      *((char **) push_array(options_list)) = pstrdup(sqltab->tab_pool,
        vals[i]);
    }
  }

  destroy_pool(tmp_pool);
  return options_list;
}

static wrap2_table_t *sqltab_open_cb(pool *parent_pool, char *srcinfo) {
  wrap2_table_t *tab = NULL;
  pool *tab_pool = make_sub_pool(parent_pool),
    *tmp_pool = make_sub_pool(parent_pool);
  config_rec *c = NULL;
  char *start = NULL, *finish = NULL, *query = NULL, *clients_query = NULL,
    *options_query = NULL;

  tab = (wrap2_table_t *) pcalloc(tab_pool, sizeof(wrap2_table_t));
  tab->tab_pool = tab_pool;

  /* Parse the SELECT query name out of the srcinfo string.  Lookup and
   * store the query in the tab_data area, so that it need not be looked
   * up later.
   *
   * The srcinfo string for this case should look like:
   *  "/<clients-named-query>[/<options-named-query>]"
   */

  start = strchr(srcinfo, '/');
  if (start == NULL) {
    wrap2_log("error: badly formatted source info '%s'", srcinfo);
    destroy_pool(tab_pool);
    destroy_pool(tmp_pool);
    errno = EINVAL;
    return NULL;
  }

  /* Find the next slash. */
  finish = strchr(++start, '/');

  if (finish)
    *finish = '\0';

  clients_query = pstrdup(tab->tab_pool, start);

  /* Verify that the named query has indeed been defined.  This is
   * base on how mod_sql creates its config_rec names.
   */
  query = pstrcat(tmp_pool, "SQLNamedQuery_", clients_query, NULL);

  c = find_config(main_server->conf, CONF_PARAM, query, FALSE);
  if (c == NULL) {
    wrap2_log("error: unable to resolve SQLNamedQuery name '%s'",
      clients_query);
    destroy_pool(tab_pool);
    destroy_pool(tmp_pool);
    errno = EINVAL;
    return NULL;
  }

  /* Handle the options-query, if present. */
  if (finish) {
    options_query = pstrdup(tab->tab_pool, ++finish);

    query = pstrcat(tmp_pool, "SQLNamedQuery_", options_query, NULL);

    c = find_config(main_server->conf, CONF_PARAM, query, FALSE);
    if (c == NULL) {
      wrap2_log("error: unable to resolve SQLNamedQuery name '%s'",
        options_query);
      destroy_pool(tab_pool);
      destroy_pool(tmp_pool);
      errno = EINVAL;
      return NULL;
    }
  }

  tab->tab_name = pstrcat(tab->tab_pool, "SQL(", srcinfo, ")", NULL);

  tab->tab_data = pcalloc(tab->tab_pool, 2 * sizeof(char));
  ((char **) tab->tab_data)[0] = pstrdup(tab->tab_pool, clients_query);

  ((char **) tab->tab_data)[1] =
    (options_query ? pstrdup(tab->tab_pool, options_query) : NULL);

  /* Set the necessary callbacks. */
  tab->tab_close = sqltab_close_cb;
  tab->tab_fetch_clients = sqltab_fetch_clients_cb;
  tab->tab_fetch_daemons = sqltab_fetch_daemons_cb;
  tab->tab_fetch_options = sqltab_fetch_options_cb;

  destroy_pool(tmp_pool);
  return tab;
}

static int sqltab_init(void) {

  /* Initialize the wrap source objects for type "sql".  */
  wrap2_register("sql", sqltab_open_cb);

  return 0;
}

module wrap2_sql_module = {
  NULL, NULL,

  /* Module API version 2.0 */
  0x20,

  /* Module name */
  "wrap2_sql",

  /* Module configuration handler table */
  NULL,

  /* Module command handler table */
  NULL,

  /* Module authentication handler table */
  NULL,

  /* Module initialization function */
  sqltab_init,

  /* Session initialization function */
  NULL,

  /* Module version */
  MOD_WRAP2_SQL_VERSION
};
