/*
** $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/preprocessors/perf-base.h#1 $
**
** perf-base.h
**
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Dan Roelker (droelker@sourcefire.com)
** Marc Norton (mnorton@sourcefire.com)
** Chris Green (stream4 instrumentation)
**
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
** 9.1.04  : Added SFBASE iReset (MAN)
**	     This is set by perfmonitor 'accrure' and 'reset' commands
*/
#ifndef _PERFBASE_H
#define _PERFBASE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sfprocpidstats.h"
#include "debug.h"
#include "sf_types.h"

#include <time.h>
#include <stdio.h>

#define MAX_PERF_STATS 1

typedef struct _PKTSTATS
{
    uint64_t pkts_recv;
    uint64_t pkts_drop;

}  PKTSTATS;

typedef struct _SFBASE
{
    uint64_t   total_wire_packets;
    uint64_t   total_ipfragmented_packets;
    uint64_t   total_ipreassembled_packets;
    uint64_t   total_packets;  /* Really, total packets of
                              * unfragmented/stream rebuilt
                              */
    uint64_t   total_blocked_packets;
    
    uint64_t   total_rebuilt_packets;
    uint64_t   total_wire_bytes;
    uint64_t   total_ipfragmented_bytes;
    uint64_t   total_ipreassembled_bytes;
    uint64_t   total_bytes;    /* Total non-stream rebuilt (but
                              * includes IP reassembled) bytes
                              */
    uint64_t   total_rebuilt_bytes;
    uint64_t   total_blocked_bytes;

    PKTSTATS pkt_stats;

    double   usertime_sec;
    double   systemtime_sec;
    double   realtime_sec;

    time_t time;

    uint64_t   iAlerts;
    uint64_t   iSyns;      /* SYNS != Connections */
    uint64_t   iSynAcks;   /* better estimator  */
    uint64_t   iTotalSessions;
    uint64_t   iNewSessions;
    uint64_t   iDeletedSessions;
    uint64_t   iMaxSessions;

    uint64_t   iStreamFlushes;  /* # of fake packet is flushed */
    uint64_t   iStreamFaults;  /* # of times we run out of memory */
    uint64_t   iStreamTimeouts; /* # of timeouts we get in this quanta */
    
    uint64_t   iFragCreates;    /* # of times we call Frag3NewTracker() */
    uint64_t   iFragCompletes;  /* # of times we call FragIsComplete() */
    uint64_t   iFragInserts;    /* # of fraginserts */
    uint64_t   iFragDeletes;    /* # of times we call Frag3RemoveTracker() */
    uint64_t   iFragAutoFrees;  /* # of times we auto free a FragTracker */
    uint64_t   iFragFlushes;    /* # of times we call Frag3Rebuild() */
    uint64_t   iMaxFrags;
    uint64_t   iCurrentFrags;
    uint64_t   iFragTimeouts;   /* # of times we've reached timeout */
    uint64_t   iFragFaults;     /* # of times we've run out of memory */    

#ifdef LINUX_SMP
    SFPROCPIDSTATS sfProcPidStats;
#endif

    uint64_t   iTotalUDPSessions;
    uint64_t   iNewUDPSessions;
    uint64_t   iDeletedUDPSessions;
    uint64_t   iMaxUDPSessions;

    uint64_t   iMaxSessionsInterval;
    uint64_t   iMidStreamSessions;
    uint64_t   iClosedSessions;
    uint64_t   iPrunedSessions;
    uint64_t   iDroppedAsyncSessions;
    uint64_t   iSessionsInitializing;
    uint64_t   iSessionsEstablished;
    uint64_t   iSessionsClosing;

    uint64_t   iAttributeHosts;
    uint64_t   iAttributeReloads;

    uint64_t   total_mpls_packets;
    uint64_t   total_mpls_bytes;
    uint64_t   total_blocked_mpls_packets;
    uint64_t   total_blocked_mpls_bytes;

    /**TCP packets ignored due to port/service filtering.*/
    uint64_t   total_tcp_filtered_packets;
    /**UDP packets ignored due to port/service filtering.*/
    uint64_t   total_udp_filtered_packets;

}  SFBASE;

typedef struct _SYSTIMES {

    double usertime;
    double systemtime;
    double totaltime;
    double realtime;

}  SYSTIMES;

typedef struct _SFBASE_STATS {

    uint64_t   total_packets;
    uint64_t   total_sessions;
    uint64_t   max_sessions;
    SYSTIMES kpackets_per_sec;
    SYSTIMES kpackets_wire_per_sec;
    SYSTIMES kpackets_ipfrag_per_sec;
    SYSTIMES kpackets_ipreass_per_sec;
    SYSTIMES kpackets_rebuilt_per_sec;
    SYSTIMES usecs_per_packet;
    SYSTIMES wire_mbits_per_sec;
    SYSTIMES ipfrag_mbits_per_sec;
    SYSTIMES ipreass_mbits_per_sec;
    SYSTIMES rebuilt_mbits_per_sec;
    SYSTIMES mbits_per_sec;
    int      avg_bytes_per_wire_packet;
    int      avg_bytes_per_ipfrag_packet;
    int      avg_bytes_per_ipreass_packet;
    int      avg_bytes_per_packet;
    int      avg_bytes_per_rebuilt_packet;
    double   idle_cpu_time;
    double   user_cpu_time;
    double   system_cpu_time;
    PKTSTATS pkt_stats;
    double   pkt_drop_percent; 
    double   alerts_per_second;
    double   syns_per_second;
    double   synacks_per_second;
    double   deleted_sessions_per_second;
    double   new_sessions_per_second;

    double stream_flushes_per_second;
    uint64_t stream_faults;
    uint64_t stream_timeouts;

    double frag_creates_per_second;
    double frag_completes_per_second;
    double frag_inserts_per_second;
    double frag_deletes_per_second;
    double frag_autofrees_per_second;
    double frag_flushes_per_second;
    uint64_t frag_timeouts;
    uint64_t frag_faults;
    uint64_t current_frags;
    uint64_t max_frags;
    
    double   patmatch_percent;
    time_t   time;

#ifdef LINUX_SMP
    SFPROCPIDSTATS *sfProcPidStats;
#endif

    uint64_t   total_blocked_packets;
    uint64_t   total_blocked_bytes;

    uint64_t   total_udp_sessions;
    uint64_t   max_udp_sessions;
    double   deleted_udp_sessions_per_second;
    double   new_udp_sessions_per_second;

    uint64_t   max_tcp_sessions_interval;
    uint64_t   curr_tcp_sessions_initializing;
    uint64_t   curr_tcp_sessions_established;
    uint64_t   curr_tcp_sessions_closing;
    double   tcp_sessions_midstream_per_second;
    double   tcp_sessions_closed_per_second;
    double   tcp_sessions_timedout_per_second;
    double   tcp_sessions_pruned_per_second;
    double   tcp_sessions_dropped_async_per_second;

    uint64_t   current_attribute_hosts;
    uint64_t   attribute_table_reloads;
    uint64_t   total_mpls_packets;
    uint64_t   total_mpls_bytes;
    uint64_t   total_blocked_mpls_packets;
    uint64_t   total_blocked_mpls_bytes;
    SYSTIMES kpackets_per_sec_mpls;
    SYSTIMES mpls_mbits_per_sec;
    int      avg_bytes_per_mpls_packet;

    /**TCP packets ignored due to port/service filtering.*/
    uint64_t   total_tcp_filtered_packets;
    /**UDP packets ignored due to port/service filtering.*/
    uint64_t   total_udp_filtered_packets;
}  SFBASE_STATS;


int InitBaseStats(SFBASE *sfBase);
int UpdateBaseStats(SFBASE *sfBase, int len, int iRebuiltPkt);
int ProcessBaseStats(SFBASE *sfBase,int console, int file, FILE * fh);
int AddStreamSession(SFBASE *sfBase, uint32_t flags);
#define SESSION_CLOSED_NORMALLY 0x01
#define SESSION_CLOSED_TIMEDOUT 0x02
#define SESSION_CLOSED_PRUNED   0x04
#define SESSION_CLOSED_ASYNC    0x08
int CloseStreamSession(SFBASE *sfBase, char flags);
int RemoveStreamSession(SFBASE *sfBase);
int AddUDPSession(SFBASE *sfBase);
int RemoveUDPSession(SFBASE *sfBase);

void UpdateWireStats(SFBASE *sfBase, int len);  
void UpdateMPLSStats(SFBASE *sfBase, int len);
void UpdateIPFragStats(SFBASE *sfBase, int len);
void UpdateIPReassStats(SFBASE *sfBase, int len);
void UpdateFilteredPacketStats(SFBASE *sfBase, unsigned int proto);

#endif


