/* $Id: main.c 3553 2011-05-05 06:14:19Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <pjsua-lib/pjsua.h>

#define THIS_FILE	"main.c"


/*
 * These are defined in pjsua_app.c.
 */
extern pj_bool_t app_restart;
pj_status_t app_init(int argc, char *argv[]);
pj_status_t app_main(void);
pj_status_t app_destroy(void);

#include <signal.h>

static void setup_signal_handler(void)
{
}

static void setup_socket_signal()
{
    signal(SIGPIPE, SIG_IGN);
}


int main(int argc, char *argv[])
{
	pj_status_t status;
    setup_socket_signal();

    do
	 {
			app_restart = PJ_FALSE;

			if (app_init(argc, argv) != PJ_SUCCESS)
			    return 1;

			setup_signal_handler();

			status = app_main();
			if (PJ_EEXITNOUNREG == status)
			{
				app_exit_without_unregister = PJ_TRUE;
			}

			app_destroy();
			/* This is on purpose */
			app_destroy();

			/*ycw-pjsip*/
			if (CMSIP_UNKNOWN == g_cmsip_cliSockfd || cmsip_sockClose(&g_cmsip_cliSockfd) == 0 )
			{
				CMSIP_PRINT("close cmsip client socket success!");
				g_cmsip_cliSockfd = CMSIP_UNKNOWN;
			}
			else
			{
				CMSIP_PRINT("close cmsip client socket error!");		
			}
    } while (app_restart);
    return 0;
}

