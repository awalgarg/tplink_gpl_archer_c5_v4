/*
** $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/preprocessors/sfprocpidstats.h#1 $
**
** sfprocpidstats.h
**
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Dan Roelker <droelker@sourcefire.com>
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
**
**
**  DESCRIPTION
**    This file gets the correct CPU usage for SMP linux machines.
**
*/
#ifndef __SFPROCPIDSTATS__
#define __SFPROCPIDSTATS__

#include "config.h"

#ifdef LINUX_SMP

typedef struct _CPUSTAT {
    
    double user;
    double sys;
    double total;
    double idle;

} CPUSTAT;

typedef struct _SFPROCPIDSTATS {
    
    CPUSTAT *SysCPUs;

    int iCPUs;
    
} SFPROCPIDSTATS;

int sfInitProcPidStats(SFPROCPIDSTATS *sfProcPidStats);
int sfProcessProcPidStats(SFPROCPIDSTATS *sfProcPidStats);

#endif

#endif
