/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/sfutil/getopt.h#1 $ */
/*
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Copyright (C) 2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _SNORT_GETOPT_H_
#define _SNORT_GETOPT_H_

#ifdef SNORT_GETOPT
#define _next_char(string)  (char)(*(string+1))

extern char * optarg; 
extern int    optind; 

int getopt(int, char**, char*);

#else
#include <getopt1.h>
#endif

#endif /* _SNORT_GETOPT_H_ */
