/*
** $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/fpdetect.h#1 $
**
** fpfuncs.h
**
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Dan Roelker <droelker@sourcefire.com>
** Marc Norton <mnorton@sourcefire.com>
**
** NOTES
** 5.15.02 - Initial Source Code. Norton/Roelker
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
*/

#ifndef __FPDETECT_H__
#define __FPDETECT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fpcreate.h"
#include "debug.h"
#include "decode.h"
#include "sflsq.h"
#include "event_queue.h"

#define REBUILD_FLAGS (PKT_REBUILT_FRAG | PKT_REBUILT_STREAM)

/*
**  This is the only function that is needed to do an
**  inspection on a packet.
*/
int fpEvalPacket(Packet *p);

int fpLogEvent(RuleTreeNode *rtn, OptTreeNode *otn, Packet *p);
int fpEvalRTN(RuleTreeNode *rtn, Packet *p, int check_ports);

/*
**  This define is for the number of unique events
**  to match before choosing which event to log.
**  (Since we can only log one.) This define is the limit.
*/
#define MAX_EVENT_MATCH 100 

/*              
**  MATCH_INFO
**  The events that are matched get held in this structure,
**  and iMatchIndex gets set to the event that holds the
**  highest priority.
*/
typedef struct {

 OptTreeNode *MatchArray[MAX_EVENT_MATCH];
 int  iMatchCount;
 int  iMatchIndex;
 int  iMatchMaxLen;
 
}MATCH_INFO;

/*
**  OTNX_MATCH_DATA
**  This structure holds information that is
**  referenced during setwise pattern matches.
**  It also contains information regarding the
**  number of matches that have occurred and
**  the event to log based on the event comparison
**  function.
*/
typedef struct 
{
    PORT_GROUP * pg;
    Packet * p;
    int check_ports;

    MATCH_INFO *matchInfo;
    int iMatchInfoArraySize;
} OTNX_MATCH_DATA;

OTNX_MATCH_DATA * OtnXMatchDataNew(int);
void OtnxMatchDataFree(OTNX_MATCH_DATA *);

int fpAddMatch( OTNX_MATCH_DATA *omd_local, OTNX *otnx, int pLen,
                OptTreeNode *otn);
void fpEvalIpProtoOnlyRules(SF_LIST **, Packet *);

#define TO_SERVER 1
#define TO_CLIENT 0

#endif
