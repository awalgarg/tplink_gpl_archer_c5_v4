/****************************************************************************
 *
 * Copyright (C) 2003-2009 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/**
**  @file       hi_mi.c
**
**  @author     Daniel Roelker <droelker@sourcefire.com>
**
**  @brief      This file contains functions that deal with the logic of
**              selecting the appropriate mode inspection (client, server,
**              or anomalous server detection).
**
**  Not too much more to say about this file, it's really just one function
**  that wraps which mode gets called.
**
**  NOTES:
**    - 3.2.03:  Initial development.  DJR
*/

#include "sys/types.h"

#include "hi_si.h"
#include "hi_client.h"
#include "hi_server.h"
#include "hi_ad.h"
#include "hi_return_codes.h"

/*
**  NAME
**    hi_mi_mode_inspection::
*/
/**
**  Wrap the logic that HttpInspect uses for which mode to inspect.
**
**  This function just uses logic to decide which type of inspection to
**  do depending on the inspection mode.  Not much to it.
**
**  @param Session      pointer to the session inspection structure
**  @param iInspectMode the type of inspection to perform
**  @param data         the packet payload
**  @param dsize        the size of the data
**
**  @return integer
**
**  @retval HI_SUCCESS      function successful
**  @retval HI_NONFATAL_ERR the inspection mode is unknown
**  @retval HI_INVALID_ARG  argument(s) was invalid or NULL
*/

int hi_mi_mode_inspection(HI_SESSION *Session, int iInspectMode, 
        const u_char *data, int dsize)
{
    int iRet;

    
    if(!Session || !data || dsize < 0)
    {
        return HI_INVALID_ARG;
    }

    /*
    **  Depending on the mode, we inspect the packet differently.
    **  
    **  HI_SI_NO_MODE:
    **    This means that the packet is neither an HTTP client or server,
    **    so we can do what we want with the packet, like look for rogue
    **    HTTP servers or HTTP tunneling.
    **
    **  HI_SI_CLIENT_MODE:
    **    Inspect for HTTP client communication.
    **
    **  HI_SI_SERVER_MODE:
    **    Inspect for HTTP server communication.
    */
    if(iInspectMode == HI_SI_NO_MODE)
    {
        /*
        **  Let's look for rogue HTTP servers and stuff
        */
        iRet = hi_server_anomaly_detection(Session, data, dsize);
        if (iRet)
        {
            return iRet;
        }
    }
    else if(iInspectMode == HI_SI_CLIENT_MODE)
    {
        iRet = hi_client_inspection((void *)Session, data, dsize);
        if (iRet)
        {
            return iRet;
        }
    }
    else if(iInspectMode == HI_SI_SERVER_MODE)
    {
        iRet = hi_server_inspection((void *)Session, data, dsize);
        if (iRet)
        {
            return iRet;
        }
    }
    else
    {
        /*
        **  We only get here if the inspection mode is different, then
        **  the defines, which we should never get here.  In case we do
        **  then we return non-fatal error.
        */
        return HI_NONFATAL_ERR;
    }

    return HI_SUCCESS;
}
