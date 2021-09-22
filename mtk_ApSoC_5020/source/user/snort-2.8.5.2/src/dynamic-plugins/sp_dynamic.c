/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/dynamic-plugins/sp_dynamic.c#1 $ */
/*
 * sp_dynamic.c
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
 * Copyright (C) 2005-2009 Sourcefire, Inc.
 *
 * Author: Steven Sturges
 *
 * Purpose:
 *      Supports dynamically loaded detection plugin to check the packet.
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
 *   alert tcp any any -> any any (msg: "DynamicRuleCheck"; );
 *
 * Effect:
 *
 *      Returns 1 if the dynamic detection plugin matches, 0 if it doesn't.
 *
 * Comments:
 *
 *
 */
#ifdef DYNAMIC_PLUGIN

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

#include "rules.h"
#include "decode.h"
#include "bitop_funcs.h"
#include "plugbase.h"
#include "parser.h"
#include "debug.h"
#include "util.h"
#include "plugin_enum.h"
#include "sp_dynamic.h"
#include "sf_dynamic_engine.h"
#include "detection-plugins/sp_flowbits.h"
#include "detection-plugins/sp_asn1_detect.h"
#include "dynamic-plugins/sf_engine/sf_snort_plugin_api.h"
#include "sf_convert_dynamic.h"
#include "sfhashfcn.h"
#include "sp_preprocopt.h"

#include "snort.h"
#include "profiler.h"

#ifdef PERF_PROFILING
PreprocStats dynamicRuleEvalPerfStats;
extern PreprocStats ruleOTNEvalPerfStats;
#endif

extern const unsigned int giFlowbitSize;
extern SnortConfig *snort_conf_for_parsing;
extern SFGHASH *flowbits_hash;
extern SF_QUEUE *flowbits_bit_queue;
extern u_int32_t flowbits_count;
extern int flowbits_toggle;
extern volatile int snort_initializing;
extern DynamicRuleNode *dynamic_rules;


void DynamicInit(char *, OptTreeNode *, int);
void DynamicParse(char *, OptTreeNode *);
int DynamicCheck(void *option_data, Packet *p);

u_int32_t DynamicRuleHash(void *d)
{
    u_int32_t a,b,c;
    DynamicData *dynData = (DynamicData *)d;
#if (defined(__ia64) || defined(__amd64) || defined(_LP64))
    {
        /* Cleanup warning because of cast from 64bit ptr to 32bit int
         * warning on 64bit OSs */
        u_int64_t ptr; /* Addresses are 64bits */
        ptr = (u_int64_t)dynData->contextData;
        a = (ptr << 32) & 0XFFFFFFFF;
        b = (ptr & 0xFFFFFFFF);
        
        ptr = (u_int64_t)dynData->checkFunction;
        c = (ptr << 32) & 0XFFFFFFFF;
        
        mix (a,b,c);
        
        a += (ptr & 0xFFFFFFFF);

        ptr = (u_int64_t)dynData->hasOptionFunction;
        b += (ptr << 32) & 0XFFFFFFFF;
        c += (ptr & 0xFFFFFFFF);

        ptr = (u_int64_t)dynData->fastPatternContents;
        a += (ptr << 32) & 0XFFFFFFFF;
        b += (ptr & 0xFFFFFFFF);
        c += dynData->fpContentFlags;

        mix (a,b,c);
    
        a += RULE_OPTION_TYPE_DYNAMIC;
    }
#else
    {
        a = (u_int32_t)dynData->contextData;
        b = (u_int32_t)dynData->checkFunction;
        c = (u_int32_t)dynData->hasOptionFunction;
        mix(a,b,c);

        a += (u_int32_t)dynData->fastPatternContents;
        b += dynData->fpContentFlags;
        c += RULE_OPTION_TYPE_DYNAMIC;
    }
#endif

    final(a,b,c);

    return c;
}

int DynamicRuleCompare(void *l, void *r)
{
    DynamicData *left = (DynamicData *)l;
    DynamicData *right = (DynamicData *)r;

    if (!left || !right)
        return DETECTION_OPTION_NOT_EQUAL;

    if ((left->contextData == right->contextData) &&
        (left->checkFunction == right->checkFunction) &&
        (left->hasOptionFunction == right->hasOptionFunction) &&
        (left->fastPatternContents == right->fastPatternContents) &&
        (left->fpContentFlags == right->fpContentFlags))
    {
        return DETECTION_OPTION_EQUAL;
    }

    return DETECTION_OPTION_NOT_EQUAL;
}

/****************************************************************************
 * 
 * Function: SetupDynamic()
 *
 * Purpose: Load it up
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
void SetupDynamic(void)
{
    /* map the keyword to an initialization/processing function */
    RegisterRuleOption("dynamic", DynamicInit, NULL, OPT_TYPE_DETECTION);

#ifdef PERF_PROFILING
    RegisterPreprocessorProfile("dynamic_rule", &dynamicRuleEvalPerfStats, 3, &ruleOTNEvalPerfStats);
#endif
    DEBUG_WRAP(DebugMessage(DEBUG_PLUGIN,"Plugin: Dynamic Setup\n"););
}


/****************************************************************************
 * 
 * Function: DynamicInit(char *, OptTreeNode *)
 *
 * Purpose: Configuration function.  Handles parsing the rule 
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
void DynamicInit(char *data, OptTreeNode *otn, int protocol)
{
    OptFpList *fpl;
    DynamicData *dynData;
    void *option_dup;

    dynData = (DynamicData *)otn->ds_list[PLUGIN_DYNAMIC];

    fpl = AddOptFuncToList(DynamicCheck, otn);

    /* attach it to the context node so that we can call each instance
     * individually
     */
    fpl->context = (void *) dynData;

    if (add_detection_option(RULE_OPTION_TYPE_DYNAMIC, (void *)dynData, &option_dup) == DETECTION_OPTION_EQUAL)
    {
        free(dynData);
        fpl->context = dynData = option_dup;
    }
    fpl->type = RULE_OPTION_TYPE_DYNAMIC;
}

/****************************************************************************
 * 
 * Function: DynamicCheck(char *, OptTreeNode *, OptFpList *)
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
int DynamicCheck(void *option_data, Packet *p)
{
    DynamicData *dynData = (DynamicData *)option_data;
    int result = 0;
    PROFILE_VARS;

    PREPROC_PROFILE_START(dynamicRuleEvalPerfStats);

    if (!dynData)
    {
        LogMessage("Dynamic Rule with no context data available");
        PREPROC_PROFILE_END(dynamicRuleEvalPerfStats);
        return DETECTION_OPTION_NO_MATCH;
    }

    result = dynData->checkFunction((void *)p, dynData->contextData);
    if (result)
    {
        PREPROC_PROFILE_END(dynamicRuleEvalPerfStats);
        return DETECTION_OPTION_MATCH;
    }

    /* Detection failed */
    PREPROC_PROFILE_END(dynamicRuleEvalPerfStats);
    return DETECTION_OPTION_NO_MATCH;
}

void DynamicRuleListFree(DynamicRuleNode *head)
{
    while (head != NULL)
    {
        DynamicRuleNode *tmp = head->next;

        if (head->freeFunc)
            head->freeFunc((void *)head->rule);

        free(head);
        head = tmp;
    }
}

/****************************************************************************
 * 
 * Function: RegisterDynamicRule(u_int32_t, u_int32_t, char *, void *,
 *                               OTNCheckFunction, int, GetFPContentFunction)
 *
 * Purpose: A dynamically loaded detection engine library can use this
 *          function to register a dynamically loaded rule/preprocessor.  It
 *          provides a pointer to context specific data for the
 *          rule/preprocessor and a reference to the function used to
 *          check the rule.
 *
 * Arguments: sid => Signature ID
 *            gid => Generator ID  
 *            info => context specific data
 *            chkFunc => Function to call to check if the rule matches
 *            has*Funcs => Functions used to categorize this rule
 *            fpContentFlags => Flags indicating which Fast Pattern Contents
 *                              are available
 *            fpFunc => Function to call to get list of fast pattern contents
 *
 * Returns: 0 on success
 *
 ****************************************************************************/
int RegisterDynamicRule(
    u_int32_t sid,
    u_int32_t gid,
    void *info,
    OTNCheckFunction chkFunc,
    OTNHasFunction hasFunc,
    int fpContentFlags,
    GetFPContentFunction fpFunc,
    RuleFreeFunc freeFunc
    )
{
    DynamicData *dynData;
    struct _OptTreeNode *otn = NULL;
    OptFpList *idx;     /* index pointer */
    OptFpList *prev = NULL;
    OptFpList *fpl;
    char done_once = 0;
    void *option_dup;
    SnortConfig *sc = snort_conf_for_parsing;
    DynamicRuleNode *node = NULL;

    if (sc == NULL)
    {
        FatalError("%s(%d) Snort config is NULL.\n",
                   __FILE__, __LINE__);
    }

    if (snort_initializing)
    {
        node = (DynamicRuleNode *)SnortAlloc(sizeof(DynamicRuleNode));

        if (dynamic_rules == NULL)
        {
            dynamic_rules = node;
        }
        else
        {
            DynamicRuleNode *tmp = dynamic_rules;

            while (tmp->next != NULL)
                tmp = tmp->next;

            tmp->next = node;
        }

        node->rule = (Rule *)info;
        node->chkFunc = chkFunc;
        node->hasFunc = hasFunc;
        node->fpContentFlags = fpContentFlags;
        node->fpFunc = fpFunc;
        node->freeFunc = freeFunc;
    }

    /* Get OTN/RTN from SID */
    otn = SoRuleOtnLookup(sc->so_rule_otn_map, gid, sid);
    if (!otn)
    {
        if (ScConfErrorOut())
        {
            FatalError("DynamicPlugin: Rule [%u:%u] not enabled in configuration.\n", gid, sid);
        }
        else
        {
#ifndef SOURCEFIRE 
            LogMessage("DynamicPlugin: Rule [%u:%u] not enabled in "
                       "configuration, rule will not be used.\n", gid, sid);
#endif
        }

        return -1;
    }

    /* If this dynamic rule can be expressed as a regular rule, break it down
     * and convert it to use the rule option tree. */
    if (ConvertDynamicRule((Rule *)info, otn) > 0)
    {
        if (node != NULL)
            node->converted = 1;

        return 0;
    }

    /* allocate the data structure and attach it to the
     * rule's data struct list */
    dynData = (DynamicData *) SnortAlloc(sizeof(DynamicData));

    if(dynData == NULL)
    {
        FatalError("DynamicPlugin: Unable to allocate Dynamic data node for rule [%u:%u]\n",
                    gid, sid);
    }

    dynData->contextData = info;
    dynData->checkFunction = chkFunc;
    dynData->hasOptionFunction = hasFunc;
    dynData->fastPatternContents = fpFunc;
    dynData->fpContentFlags = fpContentFlags;

    while (otn)
    {
        otn->ds_list[PLUGIN_DYNAMIC] = (void *)dynData;

        /* And add this function into the tail of the list */
        fpl = AddOptFuncToList(DynamicCheck, otn);
        fpl->context = dynData;
        if (done_once == 0)
        {
            if (add_detection_option(RULE_OPTION_TYPE_DYNAMIC,
                                     (void *)dynData, &option_dup) == DETECTION_OPTION_EQUAL)
            {
                free(dynData);
                fpl->context = dynData = option_dup;
            }
            fpl->type = RULE_OPTION_TYPE_DYNAMIC;
            done_once = 1;
        }

        /* Arrgh.  Because we read this rule in earlier, there is
         * already an OptListEnd node there.  Need to move this new
         * one to just before it.
         */
        DEBUG_WRAP(DebugMessage(DEBUG_CONFIGRULES,"Adding new rule to list\n"););

        /* set the index pointer to the start of this OTN's function list */
        idx = otn->opt_func;

        /* if there are no nodes on the function list... */
        while(idx != NULL)
        {
            if (idx->next == fpl) /* The last one in the list before us */
            {
                if (prev)
                {
                    prev->next = fpl;
                    fpl->next = idx;
                    idx->next = NULL;
                }
                else /* idx is the head of the list */
                {
                    otn->opt_func = fpl;
                    fpl->next = idx;
                    idx->next = NULL;
                }
            }
            prev = idx;
            idx = idx->next;
        }

        otn = SoRuleOtnLookupNext(gid, sid);
    }

    return 0;
}

#ifdef SNORT_RELOAD
int ReloadDynamicRules(SnortConfig *sc)
{
    DynamicRuleNode *node = dynamic_rules;

    snort_conf_for_parsing = sc;

    for (; node != NULL; node = node->next)
    {
        int i;

        if (node->rule == NULL)
            continue;

        for (i = 0; node->rule->options[i] != NULL; i++)
        {
            RuleOption *option = node->rule->options[i];

            switch (option->optionType)
            {
                case OPTION_TYPE_FLOWBIT:
                    {
                        FlowBitsInfo *flowbits = option->option_u.flowBit;
                        flowbits->id = DynamicFlowbitRegister(flowbits->flowBitsName, flowbits->operation);
                    }

                    break;

                case OPTION_TYPE_PREPROCESSOR:
                    {
                        PreprocessorOption *preprocOpt = option->option_u.preprocOpt;
                        if (DynamicPreprocRuleOptInit(preprocOpt) == -1)
                            continue;
                    }

                    break;

                default:
                    break;
            }
        }

        RegisterDynamicRule(node->rule->info.sigID, node->rule->info.genID,
                            (void *)node->rule, node->chkFunc, node->hasFunc,
                            node->fpContentFlags, node->fpFunc, node->freeFunc);
    }

    snort_conf_for_parsing = NULL;

    return 0;
}
#endif

int DynamicPreprocRuleOptInit(void *opt)
{
    PreprocessorOption *preprocOpt = (PreprocessorOption *)opt;
    PreprocOptionInit optionInit;
    char *option_name = NULL;
    char *option_params = NULL;
    char *tmp;
    int result;

    if (preprocOpt == NULL)
        return -1;

    if (preprocOpt->optionName == NULL)
        return -1;

    result = GetPreprocessorRuleOptionFuncs((char *)preprocOpt->optionName,
                                     &preprocOpt->optionInit,
                                     &preprocOpt->optionEval);
    if (!result)
        return -1;

    optionInit = (PreprocOptionInit)preprocOpt->optionInit;

    option_name = SnortStrdup(preprocOpt->optionName);

    /* XXX Hack right now for override options where the rule
     * option is stored as <option> <override>, e.g.
     * "byte_test dce"
     * Since name is passed in to init function, the function
     * is expecting the first word in the option name and not
     * the whole string */
    tmp = option_name;
    while ((*tmp != '\0') && !isspace((int)*tmp)) tmp++;
    *tmp = '\0';

    if (preprocOpt->optionParameters != NULL)
        option_params = SnortStrdup(preprocOpt->optionParameters);

    result = optionInit(option_name, option_params, &preprocOpt->dataPtr);

    if (option_name != NULL) free(option_name);
    if (option_params != NULL) free(option_params);

    if (!result)
        return -1;

    return 0;
}

u_int32_t DynamicFlowbitRegister(char *name, int op)
{
    u_int32_t retFlowId; /* ID */
    int hashRet;
    FLOWBITS_OBJECT *flowbits_item;

    /* Auto init hash table and queue */
    if (flowbits_hash == NULL)
        FlowBitsHashInit();

    flowbits_item = sfghash_find(flowbits_hash, name);

    if (flowbits_item != NULL) 
    {
        retFlowId = flowbits_item->id;
    }
    else
    {
        flowbits_item = 
            (FLOWBITS_OBJECT *)SnortAlloc(sizeof(FLOWBITS_OBJECT));

        if (sfqueue_count(flowbits_bit_queue) > 0)
        {
            retFlowId = (u_int32_t)(uintptr_t)sfqueue_remove(flowbits_bit_queue);
            flowbits_item->id = retFlowId;
        }
        else
        {
            retFlowId = flowbits_count;
            flowbits_item->id = flowbits_count;

            hashRet = sfghash_add(flowbits_hash, name, flowbits_item);
            if (hashRet != SFGHASH_OK) 
            {
                FatalError("Could not add flowbits key (%s) to hash.\n", name);
            }

            flowbits_count++;

            if(flowbits_count > (giFlowbitSize<<3) )
            {
                FatalError("FLOWBITS: The number of flowbit IDs in the "
                           "current ruleset (%d) exceed the maximum number of IDs "
                           "that are allowed (%d).\n", flowbits_count,giFlowbitSize<<3);
            }
        }
    }

    flowbits_item->toggle = flowbits_toggle;
    flowbits_item->types |= op;

    return retFlowId;
}

int DynamicFlowbitCheck(void *pkt, int op, u_int32_t id)
{
    StreamFlowData *flowdata;
    Packet *p = (Packet *)pkt;
    int result = 0;

    flowdata = GetFlowbitsData(p);
    if (!flowdata)
    {
        return 0;
    }

    switch(op)
    {
        case FLOWBITS_SET:
            boSetBit(&(flowdata->boFlowbits), id);
            result = 1;
            break;

        case FLOWBITS_UNSET:
            boClearBit(&(flowdata->boFlowbits), id);
            result = 1;
            break;

        case FLOWBITS_RESET:
            boResetBITOP(&(flowdata->boFlowbits));
            result = 1;

        case FLOWBITS_ISSET:
            if (boIsBitSet(&(flowdata->boFlowbits), id))
                result = 1;
            break;

        case FLOWBITS_ISNOTSET:
            if (boIsBitSet(&(flowdata->boFlowbits), id))
                result = 0;
            else
                result = 1;
            break;

        case FLOWBITS_TOGGLE:
            if (boIsBitSet(&(flowdata->boFlowbits), id))
            {
                boClearBit(&(flowdata->boFlowbits), id);
            }
            else
            {
                boSetBit(&(flowdata->boFlowbits), id);
            }
            result = 1;
            break;
        case FLOWBITS_NOALERT:
            /* Shouldn't see this case here... But, just for
             * safety sake, return 0.
             */
            result = 0;
            break;

        default:
            /* Shouldn't see this case here... But, just for
             * safety sake, return 0.
             */
            result = 0;
            break;
    }

    return result;
}


int DynamicAsn1Detect(void *pkt, void *ctxt, const u_int8_t *cursor)
{
    Packet *p    = (Packet *) pkt;
    ASN1_CTXT *c = (ASN1_CTXT *) ctxt;   
    
    /* Call same detection function that snort calls */
    return Asn1DoDetect(p->data, p->dsize, c, cursor);
}

static INLINE int DynamicHasOption(
    OptTreeNode *otn, DynamicOptionType optionType, int flowFlag
) {
    DynamicData *dynData;
    
    dynData = (DynamicData *)otn->ds_list[PLUGIN_DYNAMIC];
    if (!dynData)
    {
        return 0;
    }

    return dynData->hasOptionFunction(dynData->contextData, optionType, flowFlag);
}

int DynamicHasFlow(OptTreeNode *otn)
{
    return DynamicHasOption(otn, OPTION_TYPE_FLOWFLAGS, 0);
}

int DynamicHasFlowbit(OptTreeNode *otn)
{
    return DynamicHasOption(otn, OPTION_TYPE_FLOWBIT, 0);
}

int DynamicHasContent(OptTreeNode *otn)
{
    return DynamicHasOption(otn, OPTION_TYPE_CONTENT, 0);
}

int DynamicHasByteTest(OptTreeNode *otn)
{
    return DynamicHasOption(otn, OPTION_TYPE_BYTE_TEST, 0);
}

int DynamicHasPCRE(OptTreeNode *otn)
{
    return DynamicHasOption(otn, OPTION_TYPE_PCRE, 0);
}

#endif /* DYNAMIC_PLUGIN */

