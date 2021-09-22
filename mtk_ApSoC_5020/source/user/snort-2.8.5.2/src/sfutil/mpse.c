/*
*  $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/sfutil/mpse.c#1 $
*
*   mpse.c
*    
*   An abstracted interface to the Multi-Pattern Matching routines,
*   thats why we're passing 'void *' objects around.
*
*   Copyright (C) 2002-2009 Sourcefire, Inc.
*   Marc A Norton <mnorton@sourcefire.com>
*
*   Updates:
*   3/06 - Added AC_BNFA search
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bitop.h"
#include "bnfa_search.h"
#include "acsmx.h"
#include "acsmx2.h"
#include "sfksearch.h"
#include "mpse.h"  
#include "debug.h"  
#include "sf_types.h"
#include "util.h"


#include "profiler.h"
#ifdef PERF_PROFILING
#include "snort.h"
PreprocStats mpsePerfStats;
#endif

static uint64_t s_bcnt=0;

typedef struct _mpse_struct {

    int    method;
    void * obj;
    int    verbose;
    uint64_t bcnt;
    char   inc_global_counter;

} MPSE;

void * mpseNew( int method, int use_global_counter_flag,
                void (*userfree)(void *p),
                void (*optiontreefree)(void **p),
                void (*neg_list_free)(void **p))
{
    MPSE * p;

    p = (MPSE*)calloc( 1,sizeof(MPSE) );
    if( !p ) return NULL;

    p->method = method;
    p->verbose = 0;
    p->obj = NULL;
    p->bcnt = 0;
    p->inc_global_counter = use_global_counter_flag;

    switch( method )
    {
        case MPSE_AC_BNFA:
            p->obj=bnfaNew(userfree, optiontreefree, neg_list_free);
            if(p->obj)
               ((bnfa_struct_t*)(p->obj))->bnfaMethod = 1;
            break;
        case MPSE_AC_BNFA_Q:
            p->obj=bnfaNew(userfree, optiontreefree, neg_list_free);
            if(p->obj)
               ((bnfa_struct_t*)(p->obj))->bnfaMethod = 0;
            break;
        case MPSE_AC:
            p->obj = acsmNew(userfree, optiontreefree, neg_list_free);
            break;
        case MPSE_ACF:
            p->obj = acsmNew2(userfree, optiontreefree, neg_list_free);
            if(p->obj)acsmSelectFormat2((ACSM_STRUCT2*)p->obj,ACF_FULL  );
            break;
        case MPSE_ACF_Q:
            p->obj = acsmNew2(userfree, optiontreefree, neg_list_free);
            if(p->obj)acsmSelectFormat2((ACSM_STRUCT2*)p->obj,ACF_FULLQ  );
            break;
        case MPSE_ACS:
            p->obj = acsmNew2(userfree, optiontreefree, neg_list_free);
            if(p->obj)acsmSelectFormat2((ACSM_STRUCT2*)p->obj,ACF_SPARSE  );
            break;
        case MPSE_ACB:
            p->obj = acsmNew2(userfree, optiontreefree, neg_list_free);
            if(p->obj)acsmSelectFormat2((ACSM_STRUCT2*)p->obj,ACF_BANDED  );
            break;
        case MPSE_ACSB:
            p->obj = acsmNew2(userfree, optiontreefree, neg_list_free);
            if(p->obj)acsmSelectFormat2((ACSM_STRUCT2*)p->obj,ACF_SPARSEBANDS  );
            break;
        case MPSE_LOWMEM:
            p->obj = KTrieNew(0,userfree, optiontreefree, neg_list_free);
            break;
        case MPSE_LOWMEM_Q:
            p->obj = KTrieNew(1,userfree, optiontreefree, neg_list_free);
            break;

        default:
            /* p is free'd below if no case */
            break;
    }

    if( !p->obj )
    {
        free(p);
        p = NULL;
    }

    return (void *)p;
}

void   mpseVerbose( void * pvoid )
{
    MPSE * p = (MPSE*)pvoid;
    p->verbose = 1;
} 

void   mpseSetOpt( void * pvoid, int flag )
{
    MPSE * p = (MPSE*)pvoid;

    if (p == NULL)
        return;
    switch( p->method )
    {
        case MPSE_AC_BNFA_Q:
        case MPSE_AC_BNFA:
            if (p->obj)
                bnfaSetOpt((bnfa_struct_t*)p->obj,flag);
            break;
        default:
            break;
    }
}

void   mpseFree( void * pvoid )
{
    MPSE * p = (MPSE*)pvoid;

    if (p == NULL)
        return;

    switch( p->method )
    {
        case MPSE_AC_BNFA:
        case MPSE_AC_BNFA_Q:
            if (p->obj)
                bnfaFree((bnfa_struct_t*)p->obj);
            free(p);
            return;

        case MPSE_AC:
            if (p->obj)
                acsmFree((ACSM_STRUCT *)p->obj);
            free(p);
            return;

        case MPSE_ACF:
        case MPSE_ACF_Q:
        case MPSE_ACS:
        case MPSE_ACB:
        case MPSE_ACSB:
            if (p->obj)
                acsmFree2((ACSM_STRUCT2 *)p->obj);
            free(p);
            return;

        case MPSE_LOWMEM:
        case MPSE_LOWMEM_Q:
            if (p->obj)
                KTrieDelete((KTRIE_STRUCT *)p->obj);
            free(p);
            return;

        default:
            return;
    }
}

int  mpseAddPattern ( void * pvoid, void * P, int m, 
                      unsigned noCase, unsigned offset, unsigned depth,
                      unsigned negative, void* ID, int IID )
{
  MPSE * p = (MPSE*)pvoid;

  switch( p->method )
   {
     case MPSE_AC_BNFA:
     case MPSE_AC_BNFA_Q:
       return bnfaAddPattern( (bnfa_struct_t*)p->obj, (unsigned char *)P, m,
              noCase, negative, ID );

     case MPSE_AC:
       return acsmAddPattern( (ACSM_STRUCT*)p->obj, (unsigned char *)P, m,
              noCase, offset, depth, negative, ID, IID );

     case MPSE_ACF:
     case MPSE_ACF_Q:
     case MPSE_ACS:
     case MPSE_ACB:
     case MPSE_ACSB:
       return acsmAddPattern2( (ACSM_STRUCT2*)p->obj, (unsigned char *)P, m,
              noCase, offset, depth, negative, ID, IID );

     case MPSE_LOWMEM:
     case MPSE_LOWMEM_Q:
       return KTrieAddPattern( (KTRIE_STRUCT *)p->obj, (unsigned char *)P, m,
                                noCase, negative, ID );

     default:
       return -1;
   }
}

void mpseLargeShifts   ( void * pvoid, int flag )
{
  MPSE * p = (MPSE*)pvoid;
 
  switch( p->method )
   {
     default:
       return ;
   }
}

int  mpsePrepPatterns  ( void * pvoid,
                         int ( *build_tree )(void *id, void **existing_tree),
                         int ( *neg_list_func )(void *id, void **list) )
{
  int retv;
  MPSE * p = (MPSE*)pvoid;

  switch( p->method )
   {
     case MPSE_AC_BNFA:
     case MPSE_AC_BNFA_Q:
       retv = bnfaCompile( (bnfa_struct_t*) p->obj, build_tree, neg_list_func );
     break;
     
     case MPSE_AC:
       retv = acsmCompile( (ACSM_STRUCT*) p->obj, build_tree, neg_list_func );
     break;
     
     case MPSE_ACF:
     case MPSE_ACF_Q:
     case MPSE_ACS:
     case MPSE_ACB:
     case MPSE_ACSB:
       retv = acsmCompile2( (ACSM_STRUCT2*) p->obj, build_tree, neg_list_func );
     break;
     
     case MPSE_LOWMEM:
     case MPSE_LOWMEM_Q:
       return KTrieCompile( (KTRIE_STRUCT *)p->obj, build_tree, neg_list_func );

     default:
       retv = 1;
     break; 
   }
  
  return retv;
}

void mpseSetRuleMask ( void *pvoid, BITOP * rm )
{
  MPSE * p = (MPSE*)pvoid;

  switch( p->method )
   {
     default:
       return ;
   }


}
int mpsePrintInfo( void *pvoid )
{
  MPSE * p = (MPSE*)pvoid;

  fflush(stderr);
  fflush(stdout);
  switch( p->method )
   {
     case MPSE_AC_BNFA:
     case MPSE_AC_BNFA_Q:
      bnfaPrintInfo( (bnfa_struct_t*) p->obj );
     break;
     case MPSE_AC:
      return acsmPrintDetailInfo( (ACSM_STRUCT*) p->obj );
     case MPSE_ACF:
     case MPSE_ACF_Q:
     case MPSE_ACS:
     case MPSE_ACB:
     case MPSE_ACSB:
      return acsmPrintDetailInfo2( (ACSM_STRUCT2*) p->obj );
     
     default:
       return 1;
   }
   fflush(stderr);
   fflush(stdout);

 return 0;
}

int mpsePrintSummary(void )
{
   acsmPrintSummaryInfo();
   acsmPrintSummaryInfo2();
   bnfaPrintSummary();
  
   if( KTrieMemUsed() )
   {
     double x;
     x = (double) KTrieMemUsed();
     LogMessage("[ LowMem Search-Method Memory Used : %g %s ]\n",
     (x > 1.e+6) ?  x/1.e+6 : x/1.e+3,
     (x > 1.e+6) ? "MBytes" : "KBytes" );
     
   }

   return 0;
}

void mpseInitSummary(void)
{
    acsm_init_summary();
    bnfaInitSummary();
    KTrieInitMemUsed();
}

int mpseSearch( void *pvoid, const unsigned char * T, int n, 
                int ( *action )(void* id, void * tree, int index, void *data, void *neg_list), 
                void * data, int* current_state ) 
{
  MPSE * p = (MPSE*)pvoid;
  int ret;
  PROFILE_VARS;

  PREPROC_PROFILE_START(mpsePerfStats);

  p->bcnt += n;

  if(p->inc_global_counter)
    s_bcnt += n;
  
  switch( p->method )
   {
     case MPSE_AC_BNFA:
     case MPSE_AC_BNFA_Q:
      /* return is actually the state */
      ret = bnfaSearch((bnfa_struct_t*) p->obj, (unsigned char *)T, n,
                       action, data, 0 /* start-state */, current_state );
      PREPROC_PROFILE_END(mpsePerfStats);
      return ret;

     case MPSE_AC:
      ret = acsmSearch( (ACSM_STRUCT*) p->obj, (unsigned char *)T, n, action, data, current_state );
      PREPROC_PROFILE_END(mpsePerfStats);
      return ret;
     
     case MPSE_ACF:
     case MPSE_ACF_Q:
     case MPSE_ACS:
     case MPSE_ACB:
     case MPSE_ACSB:
      ret = acsmSearch2( (ACSM_STRUCT2*) p->obj, (unsigned char *)T, n, action, data, current_state );
      PREPROC_PROFILE_END(mpsePerfStats);
      return ret;

     case MPSE_LOWMEM:
     case MPSE_LOWMEM_Q:
        ret = KTrieSearch( (KTRIE_STRUCT *)p->obj, (unsigned char *)T, n, action, data);
        *current_state = 0;
        PREPROC_PROFILE_END(mpsePerfStats);
        return ret;

     default:
       PREPROC_PROFILE_END(mpsePerfStats);
       return 1;
   }

}

int mpseGetPatternCount(void *pvoid)
{
    MPSE * p = (MPSE*)pvoid;

    switch( p->method )
    {
        case MPSE_AC_BNFA:
        case MPSE_AC_BNFA_Q:
            return bnfaPatternCount((bnfa_struct_t *)p->obj);
            break;
        case MPSE_AC:
            return acsmPatternCount((ACSM_STRUCT*)p->obj);
            break;
        case MPSE_ACF:
        case MPSE_ACF_Q:
        case MPSE_ACS:
        case MPSE_ACB:
        case MPSE_ACSB:
            return acsmPatternCount2((ACSM_STRUCT2*)p->obj);
            break;
        case MPSE_LOWMEM:
        case MPSE_LOWMEM_Q:
            return KTriePatternCount((KTRIE_STRUCT*)p->obj);
            break;
    }
    return 0;
}

uint64_t mpseGetPatByteCount(void)
{
    return s_bcnt; 
}

void mpseResetByteCount(void)
{
    s_bcnt = 0;
}

void mpse_print_qinfo(void)
{
    sfksearch_print_qinfo();
    bnfa_print_qinfo();
    acsmx2_print_qinfo();
}

