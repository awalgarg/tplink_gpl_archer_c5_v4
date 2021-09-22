/*
**  $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/preprocessors/perf-event.c#1 $
**
**  perf-event.c
**
**  Copyright (C) 2002-2009 Sourcefire, Inc.
**  Marc Norton <mnorton@sourcefire.com>
**  Dan Roelker <droelker@sourcefire.com>
**
**  NOTES
**  5.28.02 - Initial Source Code. Norton/Roelker
**
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License Version 2 as
**  published by the Free Software Foundation.  You may not use, modify or
**  distribute this program under any other version of the GNU General
**  Public License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
*/

#include "snort.h"
#include "util.h"

extern SFPERF *perfmon_config;

int DisplayEventPerfStats(SFEVENT_STATS *sfEventStats);

int InitEventStats(SFEVENT *sfEvent)
{
    sfEvent->NQEvents = 0;
    sfEvent->QEvents  = 0;

    return 0;
}

int UpdateNQEvents(SFEVENT *sfEvent)
{
    if (sfEvent == NULL)
        return -1;

    if ((perfmon_config == NULL) ||
        !(perfmon_config->perf_flags & SFPERF_EVENT))
    {
        return 0;
    }

    sfEvent->NQEvents++;
    sfEvent->TotalEvents++;

    return 0;
}

int UpdateQEvents(SFEVENT *sfEvent)
{
    if (sfEvent == NULL)
        return -1;

    if ((perfmon_config == NULL) ||
        !(perfmon_config->perf_flags & SFPERF_EVENT))
    {
        return 0;
    }

    sfEvent->QEvents++;
    sfEvent->TotalEvents++;

    return 0;
}

int ProcessEventStats(SFEVENT *sfEvent)
{
    SFEVENT_STATS sfEventStats;

    sfEventStats.NQEvents = sfEvent->NQEvents;
    sfEventStats.QEvents = sfEvent->QEvents;
    sfEventStats.TotalEvents = sfEvent->TotalEvents;

    if(sfEvent->TotalEvents)
    {
        sfEventStats.NQPercent = 100.0 * (double)sfEvent->NQEvents / 
                                 (double)sfEvent->TotalEvents;
        sfEventStats.QPercent  = 100.0 * (double)sfEvent->QEvents / 
                                 (double)sfEvent->TotalEvents;
    }
    else
    {
        sfEventStats.NQPercent = 0;
        sfEventStats.QPercent = 0;
    }

    sfEvent->NQEvents    = 0;
    sfEvent->QEvents     = 0;
    sfEvent->TotalEvents = 0;

    DisplayEventPerfStats(&sfEventStats);

    return 0;
}

int DisplayEventPerfStats(SFEVENT_STATS *sfEventStats)
{
    LogMessage("\n");
    LogMessage("\n");
    LogMessage("Snort Setwise Event Stats\n");
    LogMessage("-------------------------\n");

    LogMessage( "Total Events:           " STDu64 "\n", sfEventStats->TotalEvents);
    LogMessage( "Qualified Events:       " STDu64 "\n", sfEventStats->QEvents);
    LogMessage( "Non-Qualified Events:   " STDu64 "\n", sfEventStats->NQEvents);

    LogMessage("%%Qualified Events:      %.4f%%\n", sfEventStats->QPercent);
    LogMessage("%%Non-Qualified Events:  %.4f%%\n", sfEventStats->NQPercent);

    return 0;
}
    

