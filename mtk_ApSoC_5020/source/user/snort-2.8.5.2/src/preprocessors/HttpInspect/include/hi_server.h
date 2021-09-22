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
**  @file       hi_server.h
**  
**  @author     Daniel Roelker <droelker@sourcefire.com>
**  
**  @brief      Header file for HttpInspect Server Module
**  
**  This file defines the server structure and functions to access server
**  inspection.
**  
**  NOTE:
**      - Initial development.  DJR
*/
#ifndef __HI_SERVER_H__
#define __HI_SERVER_H__

#include "hi_include.h"

typedef struct s_HI_SERVER
{
    const unsigned char *header;
    int           header_size;

} HI_SERVER;

int hi_server_inspection(void *S, const unsigned char *data, int dsize);

#endif
