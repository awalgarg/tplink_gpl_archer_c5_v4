/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/detection-plugins/sp_byte_jump.c#1 $ */
/*
 ** Copyright (C) 2002-2009 Sourcefire, Inc.
 ** Author: Martin Roesch
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

/* sp_byte_jump 
 * 
 * Purpose:
 *      Grab some number of bytes, convert them to their numeric 
 *      representation, jump the doe_ptr up that many bytes (for
 *      further pattern matching/byte_testing).
 *
 *
 * Arguments:
 *      Required:
 *      <bytes_to_grab>: number of bytes to pick up from the packet
 *      <offset>: number of bytes into the payload to grab the bytes
 *      Optional:
 *      ["relative"]: offset relative to last pattern match
 *      ["big"]: process data as big endian (default)
 *      ["little"]: process data as little endian
 *      ["string"]: converted bytes represented as a string needing conversion
 *      ["hex"]: converted string data is represented in hexidecimal
 *      ["dec"]: converted string data is represented in decimal
 *      ["oct"]: converted string data is represented in octal
 *      ["align"]: round the number of converted bytes up to the next 
 *                 32-bit boundry
 *      ["post_offset"]: number of bytes to adjust after applying 
 *   
 *   sample rules:
 *   alert udp any any -> any 32770:34000 (content: "|00 01 86 B8|"; \
 *       content: "|00 00 00 01|"; distance: 4; within: 4; \
 *       byte_jump: 4, 12, relative, align; \
 *       byte_test: 4, >, 900, 20, relative; \
 *       msg: "statd format string buffer overflow";)
 *
 * Effect:
 *
 *      Reads in the indicated bytes, converts them to an numeric 
 *      representation and then jumps the doe_ptr up
 *      that number of bytes.  Returns 1 if the jump is in range (within the
 *      packet) and 0 if it's not.
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
#include "byte_extract.h"
#include "sp_byte_jump.h"
#include "sfhashfcn.h"

#include "snort.h"
#include "profiler.h"
#ifdef PERF_PROFILING
PreprocStats byteJumpPerfStats;
extern PreprocStats ruleOTNEvalPerfStats;
#endif

#include "sfhashfcn.h"
#include "detection_options.h"

extern const uint8_t *doe_ptr;
extern uint8_t DecodeBuffer[DECODE_BLEN];

typedef struct _ByteJumpOverrideData
{
    char *keyword;
    char *option;
    RuleOptOverrideFunc func;
    struct _ByteJumpOverrideData *next;
} ByteJumpOverrideData;

ByteJumpOverrideData *byteJumpOverrideFuncs = NULL;

static void ByteJumpOverride(char *keyword, char *option, RuleOptOverrideFunc func);
static void ByteJumpOverrideFuncsFree(void);
static void ByteJumpInit(char *, OptTreeNode *, int);
static ByteJumpOverrideData * ByteJumpParse(char *, ByteJumpData *, OptTreeNode *);
static void ByteJumpOverrideCleanup(int, void *);


uint32_t ByteJumpHash(void *d)
{
    uint32_t a,b,c;
    ByteJumpData *data = (ByteJumpData *)d;

    a = data->bytes_to_grab;
    b = data->offset;
    c = data->base;

    mix(a,b,c); 
                                     
    a += (data->relative_flag << 24 |
          data->data_string_convert_flag << 16 |
          data->from_beginning_flag << 8 |
          data->align_flag); 
    b += data->endianess;
    c += data->multiplier;

    mix(a,b,c);
                                                         
    a += RULE_OPTION_TYPE_BYTE_JUMP;
    b += data->post_offset;

    final(a,b,c);
                                                                     
    return c;
}   
    
int ByteJumpCompare(void *l, void *r)
{
    ByteJumpData *left = (ByteJumpData *)l;
    ByteJumpData *right = (ByteJumpData *)r;

    if (!left || !right)
        return DETECTION_OPTION_NOT_EQUAL;

    if (( left->bytes_to_grab == right->bytes_to_grab) &&
        ( left->offset == right->offset) &&
        ( left->relative_flag == right->relative_flag) &&
        ( left->data_string_convert_flag == right->data_string_convert_flag) &&
        ( left->from_beginning_flag == right->from_beginning_flag) &&
        ( left->align_flag == right->align_flag) &&
        ( left->endianess == right->endianess) &&
        ( left->base == right->base) &&
        ( left->multiplier == right->multiplier) &&
        ( left->post_offset == right->post_offset))
    {
        return DETECTION_OPTION_EQUAL;
    }

    return DETECTION_OPTION_NOT_EQUAL;
}

static void ByteJumpOverride(char *keyword, char *option, RuleOptOverrideFunc func)
{
    ByteJumpOverrideData *new = SnortAlloc(sizeof(ByteJumpOverrideData));

    new->keyword = strdup(keyword);
    new->option = strdup(option);
    new->func = func;
    
    new->next = byteJumpOverrideFuncs;
    byteJumpOverrideFuncs = new;
}

static void ByteJumpOverrideFuncsFree(void)
{
    ByteJumpOverrideData *node = byteJumpOverrideFuncs;

    while (node != NULL)
    {
        ByteJumpOverrideData *tmp = node;

        node = node->next;

        if (tmp->keyword != NULL)
            free(tmp->keyword);

        if (tmp->option != NULL)
            free(tmp->option);

        free(tmp);
    }

    byteJumpOverrideFuncs = NULL;
}

/****************************************************************************
 * 
 * Function: SetupByteJump()
 *
 * Purpose: Load 'er up
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
void SetupByteJump(void)
{
    /* This list is only used during parsing */
    if (byteJumpOverrideFuncs != NULL)
        ByteJumpOverrideFuncsFree();

    /* map the keyword to an initialization/processing function */
    RegisterRuleOption("byte_jump", ByteJumpInit, ByteJumpOverride, OPT_TYPE_DETECTION);
    AddFuncToCleanExitList(ByteJumpOverrideCleanup, NULL);
    AddFuncToRuleOptParseCleanupList(ByteJumpOverrideFuncsFree);

#ifdef PERF_PROFILING
    RegisterPreprocessorProfile("byte_jump", &byteJumpPerfStats, 3, &ruleOTNEvalPerfStats);
#endif

    DEBUG_WRAP(DebugMessage(DEBUG_PLUGIN,"Plugin: ByteJump Setup\n"););
}


/****************************************************************************
 * 
 * Function: ByteJumpInit(char *, OptTreeNode *)
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
static void ByteJumpInit(char *data, OptTreeNode *otn, int protocol)
{
    ByteJumpData *idx;
    OptFpList *fpl;
    ByteJumpOverrideData *override;
    void *idx_dup;

    /* allocate the data structure and attach it to the
       rule's data struct list */
    idx = (ByteJumpData *) calloc(sizeof(ByteJumpData), sizeof(char));

    if(idx == NULL)
    {
        FatalError("%s(%d): Unable to allocate byte_jump data node\n", 
                   file_name, file_line);
    }

    /* this is where the keyword arguments are processed and placed into the 
       rule option's data structure */
    override = ByteJumpParse(data, idx, otn);
    if (override != NULL)
    {
        /* There is an override function */
        free(idx);
        override->func(override->keyword, override->option, data, otn, protocol);
        return;
    }

    fpl = AddOptFuncToList(ByteJump, otn);
    fpl->type = RULE_OPTION_TYPE_BYTE_JUMP;

    if (add_detection_option(RULE_OPTION_TYPE_BYTE_JUMP, (void *)idx, &idx_dup) == DETECTION_OPTION_EQUAL)
    {
#ifdef DEBUG_RULE_OPTION_TREE
        LogMessage("Duplicate ByteJump:\n%d %d %c %c %c %c %c %d %d\n"
            "%d %d %c %c %c %c %c %d %d %d\n\n",
            idx->bytes_to_grab,
            idx->offset,
            idx->relative_flag,
            idx->data_string_convert_flag,
            idx->from_beginning_flag,
            idx->align_flag,
            idx->endianess,
            idx->base, idx->multiplier,
            ((ByteJumpData *)idx_dup)->bytes_to_grab,
            ((ByteJumpData *)idx_dup)->offset,
            ((ByteJumpData *)idx_dup)->relative_flag,
            ((ByteJumpData *)idx_dup)->data_string_convert_flag,
            ((ByteJumpData *)idx_dup)->from_beginning_flag,
            ((ByteJumpData *)idx_dup)->align_flag,
            ((ByteJumpData *)idx_dup)->endianess,
            ((ByteJumpData *)idx_dup)->base,
            ((ByteJumpData *)idx_dup)->multiplier,
            ((ByteJumpData *)idx_dup)->post_offset);
#endif
        free(idx);
        idx = idx_dup;
    }


    /* attach it to the context node so that we can call each instance
     * individually
     */
    fpl->context = (void *) idx;

    if (idx->relative_flag == 1)
        fpl->isRelative = 1;
}

/****************************************************************************
 * 
 * Function: ByteJumpParse(char *, ByteJumpData *, OptTreeNode *)
 *
 * Purpose: This is the function that is used to process the option keyword's
 *          arguments and attach them to the rule's data structures.
 *
 * Arguments: data => argument data
 *            idx => pointer to the processed argument storage
 *            otn => pointer to the current rule's OTN
 *
 * Returns: void function
 *
 ****************************************************************************/
static ByteJumpOverrideData * ByteJumpParse(char *data, ByteJumpData *idx, OptTreeNode *otn)
{
    char **toks;
    char *endp;
    int num_toks;
    char *cptr;
    int i =0;

    idx->multiplier = 1;

    toks = mSplit(data, ",", 12, &num_toks, 0);

    if(num_toks < 2)
        FatalError("%s (%d): Bad arguments to byte_jump: %s\n", file_name,
                file_line, data);

    /* set how many bytes to process from the packet */
    idx->bytes_to_grab = strtoul(toks[0], &endp, 10);

    if(endp==toks[0])
    {
        FatalError("%s(%d): Unable to parse as byte value %s\n",
                   file_name, file_line, toks[0]);
    }

    if(idx->bytes_to_grab > PARSELEN || idx->bytes_to_grab == 0)
    {
        FatalError("%s(%d): byte_jump can't process more than "
                "%d bytes!\n", file_name, file_line, PARSELEN);
    }

    /* set offset */
    idx->offset = strtol(toks[1], &endp, 10);

    if(endp==toks[1])
    {
        FatalError("%s(%d): Unable to parse as offset %s\n",
                   file_name, file_line, toks[1]);
    }

    i = 2;

    /* is it a relative offset? */
    if(num_toks > 2)
    {
        while(i < num_toks)
        {
            cptr = toks[i];

            while(isspace((int)*cptr)) {cptr++;}

            if(!strcasecmp(cptr, "relative"))
            {
                /* the offset is relative to the last pattern match */
                idx->relative_flag = 1;
            }
            else if(!strcasecmp(cptr, "from_beginning"))
            {
                idx->from_beginning_flag = 1;
            }
            else if(!strcasecmp(cptr, "string"))
            {
                /* the data will be represented as a string that needs 
                 * to be converted to an int, binary is assumed otherwise
                 */
                idx->data_string_convert_flag = 1;
            }
            else if(!strcasecmp(cptr, "little"))
            {
                idx->endianess = LITTLE;
            }
            else if(!strcasecmp(cptr, "big"))
            {
                /* this is the default */
                idx->endianess = BIG;
            }
            else if(!strcasecmp(cptr, "hex"))
            {
                idx->base = 16;
            }
            else if(!strcasecmp(cptr, "dec"))
            {
                idx->base = 10;
            }
            else if(!strcasecmp(cptr, "oct"))
            {
                idx->base = 8;
            }
            else if(!strcasecmp(cptr, "align"))
            {
                idx->align_flag = 1;
            }
            else if(!strncasecmp(cptr, "multiplier ", 11))
            {
                /* Format of this option is multiplier xx.
                 * xx is a positive base 10 number.
                 */
                char *mval = &cptr[11];
                long factor = 0;
                int multiplier_len = strlen(cptr);
                if (multiplier_len > 11)
                {
                    factor = strtol(mval, &endp, 10);
                }
                if ((factor <= 0) || (endp != cptr + multiplier_len))
                {
                    FatalError("%s(%d): invalid length multiplier \"%s\"\n", 
                            file_name, file_line, cptr);
                }
                idx->multiplier = factor;
            }
            else if(!strncasecmp(cptr, "post_offset ", 12))
            {
                /* Format of this option is post_offset xx.
                 * xx is a positive or negative base 10 integer.
                 */
                char *mval = &cptr[12];
                int32_t factor = 0;
                int postoffset_len = strlen(cptr);
                if (postoffset_len > 12)
                {
                    factor = strtol(mval, &endp, 10);
                }
                if (endp != cptr + postoffset_len)
                {
                    FatalError("%s(%d): invalid post_offset \"%s\"\n", 
                            file_name, file_line, cptr);
                }
                idx->post_offset = factor;
            }
            else
            {
                ByteJumpOverrideData *override = byteJumpOverrideFuncs;

                while (override != NULL)
                {
                    if (strcasecmp(cptr, override->option) == 0)
                    {
                        mSplitFree(&toks, num_toks);
                        return override;
                    }

                    override = override->next;
                }

                FatalError("%s(%d): unknown modifier \"%s\"\n", 
                           file_name, file_line, cptr);
            }

            i++;
        }
    }

    /* idx->base is only set if the parameter is specified */
    if(!idx->data_string_convert_flag && idx->base)
    {
        FatalError("%s(%d): hex, dec and oct modifiers must be used in conjunction \n"
                   "        with the 'string' modifier\n", file_name, file_line);
    }

    mSplitFree(&toks, num_toks);
    return NULL;
}


/****************************************************************************
 * 
 * Function: ByteJump(char *, OptTreeNode *, OptFpList *)
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
int ByteJump(void *option_data, Packet *p)
{
    ByteJumpData *bjd = (ByteJumpData *)option_data;
    int rval = DETECTION_OPTION_NO_MATCH;
    uint32_t value = 0;
    uint32_t jump_value = 0;
    uint32_t payload_bytes_grabbed = 0;
    int32_t tmp = 0;
    int dsize;
    int use_alt_buffer = p->packet_flags & PKT_ALT_DECODE;
    const uint8_t *base_ptr, *end_ptr, *start_ptr;
    PROFILE_VARS;

    PREPROC_PROFILE_START(byteJumpPerfStats);

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
            DebugMessage(DEBUG_PATTERN_MATCH,"[*] byte jump firing...\n");
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
                                    "[*] byte jump bounds check failed..\n"););

            PREPROC_PROFILE_END(byteJumpPerfStats);
            return rval;
        }
    }

    if(bjd->relative_flag && doe_ptr)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                "Checking relative offset!\n"););
        base_ptr = doe_ptr + bjd->offset;
    }
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                "checking absolute offset %d\n", bjd->offset););
        base_ptr = start_ptr + bjd->offset;
    }

    /* Both of the extraction functions contain checks to insure the data
     * is always inbounds */
    
    if(!bjd->data_string_convert_flag)
    {
        if(byte_extract(bjd->endianess, bjd->bytes_to_grab,
                        base_ptr, start_ptr, end_ptr, &value))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                    "Byte Extraction Failed\n"););

            PREPROC_PROFILE_END(byteJumpPerfStats);
            return rval;
        }

        payload_bytes_grabbed = bjd->bytes_to_grab;
    }
    else
    {
        payload_bytes_grabbed = tmp = string_extract(bjd->bytes_to_grab, bjd->base,
                                               base_ptr, start_ptr, end_ptr, &value);
        if (tmp < 0)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                    "Byte Extraction Failed\n"););

            PREPROC_PROFILE_END(byteJumpPerfStats);
            return rval;
        }

    }

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                            "grabbed %d of %d bytes, value = %08X\n", 
                            payload_bytes_grabbed, bjd->bytes_to_grab, value););

    /* Adjust the jump_value (# bytes to jump forward) with
     * the multiplier.
     */
    jump_value = value * bjd->multiplier;

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                            "grabbed %d of %d bytes, after multiplier value = %08X\n", 
                            payload_bytes_grabbed, bjd->bytes_to_grab, jump_value););


    /* if we need to align on 32-bit boundries, round up to the next
     * 32-bit value
     */
    if(bjd->align_flag)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                    "offset currently at %d\n", jump_value););
        if ((jump_value % 4) != 0)
        {
            jump_value += (4 - (jump_value % 4));
        }
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                    "offset aligned to %d\n", jump_value););
    }

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                            "Grabbed %d bytes at offset %d, value = 0x%08X\n",
                            payload_bytes_grabbed, bjd->offset, jump_value););

    if(bjd->from_beginning_flag)
    {
        /* Reset base_ptr if from_beginning */
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                "jumping from beginning %d bytes\n", jump_value););
        base_ptr = start_ptr;

        /* from base, push doe_ptr ahead "value" number of bytes */
        doe_ptr = base_ptr + jump_value;

    }
    else
    {
        doe_ptr = base_ptr + payload_bytes_grabbed + jump_value;

    }
   
    /* now adjust using post_offset -- before bounds checking */
    doe_ptr += bjd->post_offset;

    if(!inBounds(start_ptr, end_ptr, doe_ptr))
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                                "tmp ptr is not in bounds %p\n", doe_ptr););
        PREPROC_PROFILE_END(byteJumpPerfStats);
        return rval;
    }
    else
    {        
        rval = DETECTION_OPTION_MATCH;
    }

    PREPROC_PROFILE_END(byteJumpPerfStats);
    return rval;
}

static void ByteJumpOverrideCleanup(int signal, void *data)
{
    if (byteJumpOverrideFuncs != NULL)
        ByteJumpOverrideFuncsFree();
}

