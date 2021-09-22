/*
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
 */

/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/dynamic-plugins/sp_dynamic.h#1 $ */

#ifndef __SP_DYNAMIC_H_
#define __SP_DYNAMIC_H_

#include "sf_dynamic_define.h"
#include "sf_dynamic_engine.h"
#include "snort.h"
#include "sf_types.h"
#include "sf_engine/sf_snort_plugin_api.h"
#include "detection-plugins/sp_pattern_match.h"

typedef struct _DynamicData
{
    void *contextData;
    OTNCheckFunction checkFunction;
    OTNHasFunction hasOptionFunction;
    int fpContentFlags;
    GetFPContentFunction fastPatternContents;
    PatternMatchData *pmds;

} DynamicData;

void SetupDynamic(void);

int RegisterDynamicRule(
    u_int32_t gid,
    u_int32_t sid,
    void *info,
    OTNCheckFunction,
    OTNHasFunction,
    int fpContentFlags,
    GetFPContentFunction,
    RuleFreeFunc freeFunc
    );

typedef struct _DynamicRuleNode
{
    Rule *rule;
    OTNCheckFunction chkFunc;
    OTNHasFunction hasFunc;
    int fpContentFlags;
    GetFPContentFunction fpFunc;
    int converted;
    RuleFreeFunc freeFunc;
    struct _DynamicRuleNode *next;

} DynamicRuleNode;

void DynamicRuleListFree(DynamicRuleNode *);

#ifdef SNORT_RELOAD
int ReloadDynamicRules(SnortConfig *);
#endif

int DynamicPreprocRuleOptInit(void *);
u_int32_t DynamicFlowbitRegister(char *name, int op);
int DynamicFlowbitCheck(void *pkt, int op, u_int32_t id);
int DynamicAsn1Detect(void *pkt, void *ctxt, const u_int8_t *cursor);
int DynamicHasFlow(OptTreeNode *otn);
int DynamicHasFlowbit(OptTreeNode *otn);
int DynamicHasContent(OptTreeNode *otn);
int DynamicHasByteTest(OptTreeNode *otn);
int DynamicHasPCRE(OptTreeNode *otn);

u_int32_t DynamicRuleHash(void *d);
int DynamicRuleCompare(void *l, void *r);

#endif  /* __SP_DYNAMIC_H_ */

