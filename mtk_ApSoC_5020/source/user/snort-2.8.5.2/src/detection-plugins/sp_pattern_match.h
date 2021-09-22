/*
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
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

/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/detection-plugins/sp_pattern_match.h#1 $ */

#ifndef __SP_PATTERN_MATCH_H__
#define __SP_PATTERN_MATCH_H__

#include "snort.h"
#include "debug.h"
#include "rules.h" /* needed for OptTreeNode defintion */
#include "sf_engine/sf_snort_plugin_api.h"
#include <ctype.h>

#define CHECK_AND_PATTERN_MATCH 1
#define CHECK_URI_PATTERN_MATCH 2

#define HTTP_SEARCH_URI 0x01 
#define HTTP_SEARCH_HEADER 0x02
#define HTTP_SEARCH_CLIENT_BODY 0x04
#define HTTP_SEARCH_METHOD 0x08
#define HTTP_SEARCH_COOKIE 0x10

/* Flags */
//#define CONTENT_FAST_PATTERN 0x01

typedef struct _PatternMatchData
{
    uint8_t exception_flag; /* search for "not this pattern" */
    int offset;             /* pattern search start offset */
    int depth;              /* pattern search depth */

    int distance;           /* offset to start from based on last match */
    int within;             /* this pattern must be found 
                               within X bytes of last match*/
    int rawbytes;           /* Search the raw bytes rather than any decoded app
                               buffer */

    int nocase;             /* Toggle case insensitity */
    int use_doe;            /* Use the doe_ptr for relative pattern searching */
    int uri_buffer;         /* Index of the URI buffer */
    int buffer_func;        /* buffer function CheckAND or CheckUri */
    u_int pattern_size;     /* size of app layer pattern */
    u_int replace_size;     /* size of app layer replace pattern */
    char *replace_buf;      /* app layer pattern to replace with */
    char *pattern_buf;      /* app layer pattern to match on */
    int (*search)(const char *, int, struct _PatternMatchData *);  /* search function */
    int *skip_stride; /* B-M skip array */
    int *shift_stride; /* B-M shift array */
    u_int pattern_max_jump_size; /* Maximum distance we can jump to search for
                                  * this pattern again. */
    struct _PatternMatchData *next; /* ptr to next match struct */
    int flags;              /* flags */
    OptFpList *fpl;         /* Pointer to the OTN FPList for this pattern */
                            /* Needed to be able to set the isRelative flag */

    /* Set if fast pattern matcher found a content in the packet,
       but the rule option specifies a negated content. Only 
       applies to negative contents that are not relative */
    struct 
    {
        struct timeval ts;
        uint64_t packet_number;
        uint32_t rebuild_flag;

    } last_check;

    int replace_depth;      /* >=0 is offset to start of replace */

} PatternMatchData;

void SetupPatternMatch(void);
int SetUseDoePtr(OptTreeNode *otn);
void PatternMatchFree(void *d);
uint32_t PatternMatchHash(void *d);
int PatternMatchCompare(void *l, void *r);
void FinalizeContentUniqueness(OptTreeNode *otn);
void PatternMatchDuplicatePmd(void *src, PatternMatchData *pmd_dup);
int PatternMatchAdjustRelativeOffsets(PatternMatchData *pmd, const uint8_t *orig_doe_ptr, const uint8_t *start_doe_ptr, const uint8_t *dp);
int PatternMatchUriBuffer(void *p);

PatternMatchData * NewNode(OptTreeNode *, int);                                                                            
void ParsePattern(char *, OptTreeNode *, int);
int CheckANDPatternMatch(void *option_data, Packet *p);
int CheckUriPatternMatch(void *option_data, Packet *p);

#endif /* __SP_PATTERN_MATCH_H__ */
