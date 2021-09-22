/****************************************************************************
 *
 * Copyright (C) 2004-2009 Sourcefire, Inc.
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
 
#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

#include "decode.h"
#include "sfutil/sfeventq.h"

#define SNORT_EVENTQ_PRIORITY    1
#define SNORT_EVENTQ_CONTENT_LEN 2

struct _OptTreeNode;

typedef struct _EventQueueConfig
{
    int max_events;
    int log_events;
    int order;
    int process_all_events;

} EventQueueConfig;

typedef struct s_SNORT_EVENTQ_USER
{
    char rule_alert;
    void *pkt;

} SNORT_EVENTQ_USER;

typedef struct _EventNode
{
    unsigned int gid;
    unsigned int sid;
    unsigned int rev;
    unsigned int classification;
    unsigned int priority;
    char        *msg;
    void        *rule_info;

} EventNode;

EventQueueConfig * EventQueueConfigNew(void);
void EventQueueConfigFree(EventQueueConfig *);
SF_EVENTQ * SnortEventqNew(EventQueueConfig *);
void SnortEventqFree(SF_EVENTQ *);
void SnortEventqReset(void);
int  SnortEventqLog(SF_EVENTQ *, Packet *);
int  SnortEventqAdd(unsigned int gid,unsigned int sid,unsigned int rev, 
                    unsigned int classification,unsigned int pri,char *msg,
                    void *rule_info);

#endif
