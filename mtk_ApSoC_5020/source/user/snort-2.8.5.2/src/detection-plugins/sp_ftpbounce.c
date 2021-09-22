/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/detection-plugins/sp_ftpbounce.c#1 $ */
/*
 ** Copyright (C) 2005-2009 Sourcefire, Inc.
 ** Author: Steven Sturges
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

/* sp_ftpbounce 
 * 
 * Purpose:
 *      Checks the address listed (a,b,c,d format) in the packet
 *      against the source address.
 *
 *      does not update the doe_ptr
 *
 * Arguments:
 *      Required:
 *        None
 *      Optional:
 *        None
 *   
 *   sample rules:
 *   alert tcp any any -> any 21 (content: "PORT"; \
 *       ftpbounce;
 *       msg: "FTP Bounce attack";)
 *
 * Effect:
 *
 *      Returns 1 if the address matches, 0 if it doesn't.
 *
 * Comments:
 *
 * Any comments?
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <errno.h>

#include "bounds.h"
#include "rules.h"
#include "decode.h"
#include "plugbase.h"
#include "parser.h"
#include "debug.h"
#include "util.h"
#include "plugin_enum.h"
#include "mstring.h"

#include "snort.h"
#include "profiler.h"
#ifdef PERF_PROFILING
PreprocStats ftpBouncePerfStats;
extern PreprocStats ruleOTNEvalPerfStats;
#endif

#include "sfhashfcn.h"
#include "detection_options.h"

extern const uint8_t *doe_ptr;
extern uint8_t DecodeBuffer[DECODE_BLEN];

void FTPBounceInit(char *, OptTreeNode *, int);
void FTPBounceParse(char *, OptTreeNode *);
int FTPBounce(void *option_data, Packet *p);

uint32_t FTPBounceHash(void *d)
{
    uint32_t a,b,c;

    /* NO data stored for the option */

    a = RULE_OPTION_TYPE_FTPBOUNCE;
    b = 0;
    c = 0;

    final(a,b,c);

    return c;
}

int FTPBounceCompare(void *l, void *r)
{
    /* NO data stored for the option */
    return DETECTION_OPTION_EQUAL;
}

/****************************************************************************
 * 
 * Function: SetupFTPBounce()
 *
 * Purpose: Load 'er up
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
void SetupFTPBounce(void)
{
    /* map the keyword to an initialization/processing function */
    RegisterRuleOption("ftpbounce", FTPBounceInit, NULL, OPT_TYPE_DETECTION);

#ifdef PERF_PROFILING
    RegisterPreprocessorProfile("ftpbounce", &ftpBouncePerfStats, 3, &ruleOTNEvalPerfStats);
#endif

    DEBUG_WRAP(DebugMessage(DEBUG_PLUGIN,"Plugin: FTPBounce Setup\n"););
}


/****************************************************************************
 * 
 * Function: FTPBounceInit(char *, OptTreeNode *)
 *
 * Purpose: Generic rule configuration function.  Handles parsing the rule 
 *          information and attaching the associated detection function to
 *          the OTN.
 *
 * Arguments: data => rule arguments/data
 *            otn => pointer to the current rule option list node
 *            protocol => protocol the rule is on (we don't care in this case)
 *
 * Returns: void function
 *
 ****************************************************************************/
void FTPBounceInit(char *data, OptTreeNode *otn, int protocol)
{
    OptFpList *fpl;
    void *ds_ptr_dup;

    /* this is where the keyword arguments are processed and placed into the 
       rule option's data structure */
    FTPBounceParse(data, otn);

    fpl = AddOptFuncToList(FTPBounce, otn);
    fpl->type = RULE_OPTION_TYPE_FTPBOUNCE;

    /* attach it to the context node so that we can call each instance
     * individually
     */
    fpl->context = (void *) NULL;

    if (add_detection_option(RULE_OPTION_TYPE_FTPBOUNCE, (void *)NULL, &ds_ptr_dup) == DETECTION_OPTION_EQUAL)
    {
        //otn->ds_list[PLUGIN_FTPBOUNCE] = ds_ptr_dup;
    }
}



/****************************************************************************
 * 
 * Function: FTPBounceParse(char *, void *, OptTreeNode *)
 *
 * Purpose: This is the function that is used to process the option keyword's
 *          arguments and attach them to the rule's data structures.
 *
 * Arguments: data => argument data
 *            otn => pointer to the current rule's OTN
 *
 * Returns: void function
 *
 ****************************************************************************/
void FTPBounceParse(char *data, OptTreeNode *otn)
{
    if (data != NULL)
    {
        FatalError("%s (%d): Bad arguments to ftpbounce: %s\n", file_name,
                   file_line, data);
    }
}


/****************************************************************************
 * 
 * Function: FTPBounce(char *, OptTreeNode *, OptFpList *)
 *
 * Purpose: Use this function to perform the particular detection routine
 *          that this rule keyword is supposed to encompass.
 *
 * Arguments: p => pointer to the decoded packet
 *            otn => pointer to the current rule's OTN
 *            fp_list => pointer to the function pointer list
 *
 * Returns: If the detection test fails, this function *must* return a zero!
 *          On success, it calls the next function in the detection list 
 *
 ****************************************************************************/
int FTPBounce(void *option_data, Packet *p)
{
    uint32_t ip = 0;
    int octet=0;
    const uint8_t *this_param = doe_ptr;

    int dsize;
    int use_alt_buffer = p->packet_flags & PKT_ALT_DECODE;
    const uint8_t *base_ptr, *end_ptr, *start_ptr;
    PROFILE_VARS;

    if (!doe_ptr)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                    "[*] ftpbounce no doe_ptr set..\n"););
        return 0;
    }

    PREPROC_PROFILE_START(ftpBouncePerfStats);

    if(use_alt_buffer)
    {
        dsize = p->alt_dsize;
        start_ptr = DecodeBuffer;        
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                    "Using Alternative Decode buffer!\n"););

    }
    else
    {
        start_ptr = p->data;
        dsize = p->dsize;
    }

    DEBUG_WRAP(
            DebugMessage(DEBUG_PATTERN_MATCH,"[*] ftpbounce firing...\n");
            DebugMessage(DEBUG_PATTERN_MATCH,"payload starts at %p\n", start_ptr);
            );  /* END DEBUG_WRAP */

    /* save off whatever our ending pointer is */
    end_ptr = start_ptr + dsize;
    base_ptr = start_ptr;

    if(doe_ptr)
    {
        /* @todo: possibly degrade to use the other buffer, seems non-intuitive*/        
        if(!inBounds(start_ptr, end_ptr, doe_ptr))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                    "[*] ftpbounce bounds check failed..\n"););
            PREPROC_PROFILE_END(ftpBouncePerfStats);
            return DETECTION_OPTION_NO_MATCH;
        }
    }

    while (isspace((int)*this_param) && (this_param < end_ptr)) this_param++;
    
    do
    {
        int value = 0;
        do
        {
            if (!isdigit((int)*this_param))
            {
                DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                    "[*] ftpbounce non digit char failed..\n"););
                PREPROC_PROFILE_END(ftpBouncePerfStats);
                return DETECTION_OPTION_NO_MATCH;
            }
            value = value * 10 + (*this_param - '0');
            this_param++;
        } while ((this_param < end_ptr) &&
                 (*this_param != ',') &&
                  (!(isspace((int)*this_param))));
        if (value > 0xFF)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                "[*] ftpbounce value > 256 ..\n"););
            PREPROC_PROFILE_END(ftpBouncePerfStats);
            return DETECTION_OPTION_NO_MATCH;
        }
        if (octet  < 4)
        {
            ip = (ip << 8) + value;
        }

        if (!isspace((int)*this_param))
            this_param++;
        octet++;
    } while ((this_param < end_ptr) && !isspace((int)*this_param) && (octet < 4));

    if (octet < 4)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
            "[*] ftpbounce insufficient data ..\n"););
        PREPROC_PROFILE_END(ftpBouncePerfStats);
        return DETECTION_OPTION_NO_MATCH;
    }

    if (ip != ntohl(p->iph->ip_src.s_addr))
    {
        PREPROC_PROFILE_END(ftpBouncePerfStats);
        return DETECTION_OPTION_MATCH;
    }
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
            "PORT command not being used in bounce\n"););
        PREPROC_PROFILE_END(ftpBouncePerfStats);
        return DETECTION_OPTION_NO_MATCH;
    }
    
    /* Never reached */
    PREPROC_PROFILE_END(ftpBouncePerfStats);
    return DETECTION_OPTION_NO_MATCH;
}
