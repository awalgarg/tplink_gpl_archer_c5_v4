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
**  @file       hi_util.h
**  
**  @author     Daniel Roelker <droelker@sourcefire.com>
**
**  @brief      HttpInspect utility functions.
**  
**  Contains function prototype and inline utility functions.
**
**  NOTES:
**      - Initial development.  DJR
*/

#ifndef __HI_UTIL_H__
#define __HI_UTIL_H__

#include "hi_include.h"

/*
**  NAME
**    hi_util_in_bounds::
*/
/**
**  This function checks for in bounds condition on buffers.  
**  
**  This is very important for much of what we do here, since inspecting
**  data buffers is mainly what we do.  So we always make sure that we are
**  within the buffer.
**  
**  This checks a half-open interval with the end pointer being one char
**  after the end of the buffer.
**  
**  @param start the start of the buffer.
**  @param end   the end of the buffer.
**  @param p     the pointer within the buffer
**  
**  @return integer
**  
**  @retval 1 within bounds
**  @retval 0 not within bounds
*/
static INLINE int hi_util_in_bounds(const u_char *start, const u_char *end, const u_char *p)
{
    if(p >= start && p < end)
    {
        return 1;
    }

    return 0;
}

#endif

