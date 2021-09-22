/****************************************************************************
 *
 * Copyright (C) 2003-2009 Sourcefire, Inc.
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
 
/**
**  @file       preproc_setup.c
**  
**  @author     Daniel Roelker <droelker@sourcefire.com>
**
**  @brief      This file initializes HttpInspect as a Snort 
**              preprocessor.
**
**  This file registers the HttpInspect initialization function,
**  adds the HttpInspect function into the preprocessor list, reads
**  the user configuration in the snort.conf file, and prints out
**  the configuration that is read.
**
**  In general, this file is a wrapper to HttpInspect functionality,
**  by interfacing with the Snort preprocessor functions.  The rest
**  of HttpInspect should be separate from the preprocessor hooks.
**
**  NOTES
**
**  - 2.10.03:  Initial Development.  DJR
*/

#include <string.h>
#include <sys/types.h>

#include "decode.h"
#include "plugbase.h"
#include "debug.h"
#include "util.h"
#include "parser.h"

#include "hi_ui_config.h"
#include "hi_ui_server_lookup.h"
#include "hi_client.h"
#include "hi_norm.h"
#include "snort_httpinspect.h"
#include "hi_util_kmap.h"
#include "hi_util_xmalloc.h"

#include "snort.h"
#include "profiler.h"

#ifdef TARGET_BASED
#include "stream_api.h"
#include "sftarget_protocol_reference.h"
#endif
#include "snort_stream5_session.h"
#include "sfPolicy.h"

/*
**  Defines for preprocessor initialization
*/
/**
**  snort.conf preprocessor keyword
*/
#define GLOBAL_KEYWORD   "http_inspect"
#define SERVER_KEYWORD   "http_inspect_server"

/**
**  The length of the error string buffer.
*/
#define ERRSTRLEN 1000

/*
**  External Global Variables
**  Variables that we need from Snort to log errors correctly and such.
*/
extern char *file_name;
extern int file_line;
extern HttpUri UriBufs[URI_COUNT];

/*
**  Global Variables
**  This is the only way to work with Snort preprocessors because
**  the user configuration must be kept between the Init function
**  the actual preprocessor.  There is no interaction between the
**  two except through global variable usage.
*/
tSfPolicyUserContextId hi_config = NULL;

#ifdef TARGET_BASED
/* Store the protocol id received from the stream reassembler */
int16_t hi_app_protocol_id;
#endif

#ifdef PERF_PROFILING
PreprocStats hiPerfStats;
PreprocStats hiDetectPerfStats;
int hiDetectCalled = 0;
#endif

static tSfPolicyId httpCurrentPolicy = 0;

/*
** Prototypes
*/
static void HttpInspectDropStats(int);
static void HttpInspect(Packet *, void *);
static void HttpInspectCleanExit(int, void *);
static void HttpInspectReset(int, void *);
static void HttpInspectResetStats(int, void *);
static void HttpInspectInit(char *);
static void addServerConfPortsToStream5(void *);
static void HttpInspectFreeConfigs(tSfPolicyUserContextId);
static void HttpInspectFreeConfig(HTTPINSPECT_GLOBAL_CONF *);
static void HttpInspectCheckConfig(void);
static void HttpInspectAddPortsOfInterest(HTTPINSPECT_GLOBAL_CONF *, tSfPolicyId);
#ifdef TARGET_BASED
static void HttpInspectAddServicesOfInterest(tSfPolicyId);
#endif

#ifdef SNORT_RELOAD
tSfPolicyUserContextId hi_swap_config = NULL;
static void HttpInspectReload(char *);
static int HttpInspectReloadVerify(void);
static void * HttpInspectReloadSwap(void);
static void HttpInspectReloadSwapFree(void *);
#endif


/*
**  NAME
**    HttpInspect::
*/
/**
**  This function wraps the functionality in the generic HttpInspect
**  processing.  We get a Packet structure and pass this into the
**  HttpInspect module where the first stage in HttpInspect is the
**  Session Inspection stage where most of the other Snortisms are
**  taken care of.  After that, the modules should be fairly generic,
**  and that's what we're trying to do here.
**
**  @param p a Packet structure that contains Snort info about the
**  packet.
**
**  @return void
*/
static void HttpInspect(Packet *p, void *context)
{
    tSfPolicyId policy_id = getRuntimePolicy();
    HTTPINSPECT_GLOBAL_CONF *pPolicyConfig = NULL ;
    PROFILE_VARS;
    sfPolicyUserPolicySet (hi_config, policy_id);
    pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)sfPolicyUserDataGetCurrent(hi_config);

    if ( pPolicyConfig == NULL)
        return;

    /*
    **  IMPORTANT:
    **  This is where we initialize any variables that can impact other
    **  aspects of detection/processing.
    **
    **  First thing that we do is reset the p->uri_count to zero, so there
    **  is no way that we would inspect a buffer that was completely bogus.
    */

    /*
    **  Check for valid packet
    **  if neither header or data is good, then we just abort.
    */
    if (!p->dsize || !IsTCP(p) || !p->data)
        return;

    PREPROC_PROFILE_START(hiPerfStats);

    /*
    **  Pass in the configuration and the packet.
    */
    SnortHttpInspect(pPolicyConfig, p);

    p->uri_count = 0;
    UriBufs[0].decode_flags = 0;

    /* XXX:
     * NOTE: this includes the HTTPInspect directly
     * calling the detection engine - 
     * to get the true HTTPInspect only stats, have another
     * var inside SnortHttpInspect that tracks the time
     * spent in Detect().
     * Subtract the ticks from this if iCallDetect == 0
     */
    PREPROC_PROFILE_END(hiPerfStats);
#ifdef PERF_PROFILING
    if (hiDetectCalled)
    {
        hiPerfStats.ticks -= hiDetectPerfStats.ticks;
        /* And Reset ticks to 0 */
        hiDetectPerfStats.ticks = 0;
        hiDetectCalled = 0;
    }
#endif

    return;
}

static void HttpInspectDropStats(int exiting) 
{
    if(!hi_stats.total)
        return;

    LogMessage("HTTP Inspect - encodings (Note: stream-reassembled "
               "packets included):\n");

#ifdef WIN32
    LogMessage("    POST methods:                   %-10I64u\n", hi_stats.post);
    LogMessage("    GET methods:                    %-10I64u\n", hi_stats.get);
    LogMessage("    Headers extracted:              %-10I64u\n", hi_stats.headers);
#ifdef DEBUG
    if (hi_stats.headers == 0)
    LogMessage("    Avg Header length:              %-10s\n", "n/a");
    else
    LogMessage("    Avg Header length:              %-10.2f\n", (double)hi_stats.header_len / (double)hi_stats.headers);
#endif
    LogMessage("    Header Cookies extracted:       %-10I64u\n", hi_stats.cookies);
#ifdef DEBUG
    if (hi_stats.cookies == 0)
    LogMessage("    Avg Cookie length:              %-10s\n", "n/a");
    else
    LogMessage("    Avg Cookie length:              %-10.2f\n", (double)hi_stats.cookie_len / (double)hi_stats.cookies);
#endif
    LogMessage("    Post parameters extracted:      %-10I64u\n", hi_stats.post_params);
    LogMessage("    Unicode:                        %-10I64u\n", hi_stats.unicode);
    LogMessage("    Double unicode:                 %-10I64u\n", hi_stats.double_unicode);
    LogMessage("    Non-ASCII representable:        %-10I64u\n", hi_stats.non_ascii);
    LogMessage("    Base 36:                        %-10I64u\n", hi_stats.base36);
    LogMessage("    Directory traversals:           %-10I64u\n", hi_stats.dir_trav);
    LogMessage("    Extra slashes (\"//\"):           %-10I64u\n", hi_stats.slashes);
    LogMessage("    Self-referencing paths (\"./\"):  %-10I64u\n", hi_stats.self_ref);
    LogMessage("    Total packets processed:        %-10I64u\n", hi_stats.total);
#else
    LogMessage("    POST methods:                   %-10llu\n", hi_stats.post);
    LogMessage("    GET methods:                    %-10llu\n", hi_stats.get);
    LogMessage("    Headers extracted:              %-10llu\n", hi_stats.headers);
#ifdef DEBUG
    if (hi_stats.headers == 0)
    LogMessage("    Avg Header length:              %-10s\n", "n/a");
    else
    LogMessage("    Avg Header length:              %-10.2f\n", (double)hi_stats.header_len / (double)hi_stats.headers);
#endif
    LogMessage("    Header Cookies extracted:       %-10llu\n", hi_stats.cookies);
#ifdef DEBUG
    if (hi_stats.cookies == 0)
    LogMessage("    Avg Cookie length:              %-10s\n", "n/a");
    else
    LogMessage("    Avg Cookie length:              %-10.2f\n", (double)hi_stats.cookie_len / (double)hi_stats.cookies);
#endif
    LogMessage("    Post parameters extracted:      %-10llu\n", hi_stats.post_params);
    LogMessage("    Unicode:                        %-10llu\n", hi_stats.unicode);
    LogMessage("    Double unicode:                 %-10llu\n", hi_stats.double_unicode);
    LogMessage("    Non-ASCII representable:        %-10llu\n", hi_stats.non_ascii);
    LogMessage("    Base 36:                        %-10llu\n", hi_stats.base36);
    LogMessage("    Directory traversals:           %-10llu\n", hi_stats.dir_trav);
    LogMessage("    Extra slashes (\"//\"):           %-10llu\n", hi_stats.slashes);
    LogMessage("    Self-referencing paths (\"./\"):  %-10llu\n", hi_stats.self_ref);
    LogMessage("    Total packets processed:        %-10llu\n", hi_stats.total);
#endif
}

static void HttpInspectCleanExit(int signal, void *data)
{
    HttpInspectFreeConfigs(hi_config);
}

static void HttpInspectReset(int signal, void *data)
{
    return;
}

static void HttpInspectResetStats(int signal, void *data)
{
    memset(&hi_stats, 0, sizeof(hi_stats));
}

/*
**  NAME
**    HttpInspectInit::
*/
/**
**  This function initializes HttpInspect with a user configuration.
**
**  The function is called when HttpInspect is configured in 
**  snort.conf.  It gets passed a string of arguments, which gets
**  parsed into configuration constructs that HttpInspect understands.
**
**  This function gets called for every HttpInspect configure line.  We
**  use this characteristic to split up the configuration, so each line
**  is a configuration construct.  We need to keep track of what part
**  of the configuration has been configured, so we don't configure one
**  part, then configure it again.
**
**  Any upfront memory is allocated here (if necessary).
**
**  @param args a string to the preprocessor arguments.
**
**  @return void
*/
static void HttpInspectInit(char *args)
{
    char ErrorString[ERRSTRLEN];
    int  iErrStrLen = ERRSTRLEN;
    int  iRet;
    char *pcToken;
    HTTPINSPECT_GLOBAL_CONF *pPolicyConfig = NULL;
    tSfPolicyId policy_id = getParserPolicy();

    ErrorString[0] = '\0';

    if ((args == NULL) || (strlen(args) == 0))
        ParseError("No arguments to HttpInspect configuration.");

    /* Find out what is getting configured */
    pcToken = strtok(args, CONF_SEPARATORS);
    if (pcToken == NULL)
    {
        FatalError("%s(%d)strtok returned NULL when it should not.",
                   __FILE__, __LINE__);
    }

    if (hi_config == NULL)
    {
        hi_config = sfPolicyConfigCreate();
        memset(&hi_stats, 0, sizeof(HIStats)); 

        /*
         **  Remember to add any cleanup functions into the appropriate
         **  lists.
         */
        AddFuncToPreprocCleanExitList(HttpInspectCleanExit, NULL, PRIORITY_APPLICATION, PP_HTTPINSPECT);
        AddFuncToPreprocRestartList(HttpInspectCleanExit, NULL, PRIORITY_APPLICATION, PP_HTTPINSPECT);
        AddFuncToPreprocResetList(HttpInspectReset, NULL, PRIORITY_APPLICATION, PP_HTTPINSPECT);
        AddFuncToPreprocResetStatsList(HttpInspectResetStats, NULL, PRIORITY_APPLICATION, PP_HTTPINSPECT);
        AddFuncToConfigCheckList(HttpInspectCheckConfig);

        RegisterPreprocStats("http_inspect", HttpInspectDropStats);

#ifdef PERF_PROFILING
        RegisterPreprocessorProfile("httpinspect", &hiPerfStats, 0, &totalPerfStats);
#endif

#ifdef TARGET_BASED
        /* Find and cache protocol ID for packet comparison */
        hi_app_protocol_id = AddProtocolReference("http");
#endif
    }

    /*
    **  Global Configuration Processing
    **  We only process the global configuration once, but always check for
    **  user mistakes, like configuring more than once.  That's why we
    **  still check for the global token even if it's been checked.
    **  Force the first configuration to be the global one.
    */
    sfPolicyUserPolicySet (hi_config, policy_id);
    pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)sfPolicyUserDataGetCurrent(hi_config);
    if (pPolicyConfig == NULL)
    {
        if (strcasecmp(pcToken, GLOBAL) != 0) 
        {
            ParseError("Must configure the http inspect global "
                       "configuration first.");
        }
        pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)SnortAlloc(sizeof(HTTPINSPECT_GLOBAL_CONF)); 
        if (!pPolicyConfig)
        {
             ParseError("HTTP INSPECT preprocessor: memory allocate failed.\n");
        }
        sfPolicyUserDataSetCurrent(hi_config, pPolicyConfig);

        iRet = HttpInspectInitializeGlobalConfig(pPolicyConfig,
                                                 ErrorString, iErrStrLen);
        if (iRet == 0)
        {
            iRet = ProcessGlobalConf(pPolicyConfig,
                                     ErrorString, iErrStrLen);

            if (iRet == 0)
            {
                PrintGlobalConf(pPolicyConfig);

                /* Add HttpInspect into the preprocessor list */
                AddFuncToPreprocList(HttpInspect, PRIORITY_APPLICATION, PP_HTTPINSPECT, PROTO_BIT__TCP);
            }
        }
    }
    else
    {
        if (strcasecmp(pcToken, SERVER) != 0)
        {
            if (strcasecmp(pcToken, GLOBAL) != 0) 
                ParseError("Must configure the http inspect global configuration first.");
            else
                ParseError("Invalid http inspect token: %s.", pcToken);
        }

        iRet = ProcessUniqueServerConf(pPolicyConfig,
                                       ErrorString, iErrStrLen);
    }
    
    if (iRet)
    {
        if(iRet > 0)
        {
            /*
            **  Non-fatal Error
            */
            if(*ErrorString)
            {
                ErrorMessage("%s(%d) => %s\n", 
                        file_name, file_line, ErrorString);
            }
        }
        else
        {
            /*
            **  Fatal Error, log error and exit.
            */
            if(*ErrorString)
            {
                FatalError("%s(%d) => %s\n", 
                        file_name, file_line, ErrorString);
            }
            else
            {
                /*
                **  Check if ErrorString is undefined.
                */
                if(iRet == -2)
                {
                    FatalError("%s(%d) => ErrorString is undefined.\n", 
                            file_name, file_line);
                }
                else
                {
                    FatalError("%s(%d) => Undefined Error.\n", 
                            file_name, file_line);
                }
            }
        }
    }
}

/*
**  NAME
**    SetupHttpInspect::
*/
/**
**  This function initializes HttpInspect as a Snort preprocessor.
**
**  It registers the preprocessor keyword for use in the snort.conf
**  and sets up the initialization module for the preprocessor, in
**  case it is configured.
**
**  This function must be called in InitPreprocessors() in plugbase.c
**  in order to be recognized by Snort.
**
**  @param none
**
**  @return void
*/
void SetupHttpInspect(void)
{
#ifndef SNORT_RELOAD
    RegisterPreprocessor(GLOBAL_KEYWORD, HttpInspectInit);
    RegisterPreprocessor(SERVER_KEYWORD, HttpInspectInit);
#else
    RegisterPreprocessor(GLOBAL_KEYWORD, HttpInspectInit, HttpInspectReload,
                         HttpInspectReloadSwap, HttpInspectReloadSwapFree);
    RegisterPreprocessor(SERVER_KEYWORD, HttpInspectInit,
                         HttpInspectReload, NULL, NULL);
#endif

    DEBUG_WRAP(DebugMessage(DEBUG_HTTPINSPECT, "Preprocessor: HttpInspect is "
                "setup . . .\n"););
}


static int HttpInspectVerifyPolicy(tSfPolicyUserContextId config,tSfPolicyId policyId, void* pData )
{
    HTTPINSPECT_GLOBAL_CONF *pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)pData;
    if (pPolicyConfig->global_server == NULL)
    {
       FatalError("HttpInspectConfigCheck() default server configuration "
                "not specified\n");
    }

#ifdef TARGET_BASED
        HttpInspectAddServicesOfInterest(policyId);
#endif
        HttpInspectAddPortsOfInterest(pPolicyConfig, policyId);

    return 0;
}


/** Add ports configured for http preprocessor to stream5 port filtering so that if 
 * any_any rules are being ignored them the the packet still reaches http-inspect.
 *
 * For ports in global_server configuration, server_lookup,
 * add the port to stream5 port filter list.
 */
static void HttpInspectAddPortsOfInterest(HTTPINSPECT_GLOBAL_CONF *config, tSfPolicyId policy_id)
{
    if (config == NULL)
        return;

    httpCurrentPolicy = policy_id;

    addServerConfPortsToStream5((void *)config->global_server);
    hi_ui_server_iterate(config->server_lookup, addServerConfPortsToStream5);
}

/**Add server ports from http_inspect preprocessor from snort.comf file to pass through 
 * port filtering.
 */
void addServerConfPortsToStream5(void *pData)
{
    unsigned int i;

    HTTPINSPECT_CONF *pConf = (HTTPINSPECT_CONF *)pData;
    if (pConf)
    {
        for (i = 0; i < MAXPORTS; i++)
        {
            if (pConf->ports[i/8] & (1 << (i % 8) ))
            {
                //Add port the port
                stream_api->set_port_filter_status
                    (IPPROTO_TCP, (uint16_t)i, PORT_MONITOR_SESSION, httpCurrentPolicy, 1);
            }
        }
    }
}

#ifdef TARGET_BASED
/**
 * @param service ordinal number of service.
 */
void HttpInspectAddServicesOfInterest(tSfPolicyId policy_id)
{
    /* Add ordinal number for the service into stream5 */
    if (hi_app_protocol_id != SFTARGET_UNKNOWN_PROTOCOL)
    {
        stream_api->set_service_filter_status(hi_app_protocol_id, PORT_MONITOR_SESSION, policy_id, 1);
    }
}
#endif

/*
**  NAME
**    HttpInspectCheckConfig::
*/
/**
**  This function verifies the HttpInspect configuration is complete
**
**  @return none
*/
static void HttpInspectCheckConfig(void)
{
    if (hi_config == NULL)
        return;

    sfPolicyUserDataIterate (hi_config, HttpInspectVerifyPolicy);

    if (!stream_api || (stream_api->version < STREAM_API_VERSION5))
    {
        FatalError("HttpInspectConfigCheck() Streaming & reassembly "
                   "must be enabled\n");
    }
}
static int HttpInspectFreeConfigPolicy(tSfPolicyUserContextId config,tSfPolicyId policyId, void* pData )
{
    HTTPINSPECT_GLOBAL_CONF *pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)pData;
    HttpInspectFreeConfig(pPolicyConfig);
    sfPolicyUserDataClear (config, policyId);
    return 0;
}
static void HttpInspectFreeConfigs(tSfPolicyUserContextId config)
{

    if (config == NULL)
        return;
    sfPolicyUserDataIterate (config, HttpInspectFreeConfigPolicy);
    sfPolicyConfigDelete(config);

}

static void HttpInspectFreeConfig(HTTPINSPECT_GLOBAL_CONF *config)
{
    if (config == NULL)
        return;

    hi_ui_server_lookup_destroy(config->server_lookup);

    xfree(config->iis_unicode_map_filename);
    xfree(config->iis_unicode_map);

    if (config->global_server != NULL)
        free(config->global_server);

    free(config);
}

#ifdef SNORT_RELOAD
static void HttpInspectReload(char *args)
{
    char ErrorString[ERRSTRLEN];
    int  iErrStrLen = ERRSTRLEN;
    int  iRet;
    HTTPINSPECT_GLOBAL_CONF *pPolicyConfig = NULL;	
    char *pcToken;
    tSfPolicyId policy_id = getParserPolicy();

    ErrorString[0] = '\0';

    if ((args == NULL) || (strlen(args) == 0))
        ParseError("No arguments to HttpInspect configuration.");

    /* Find out what is getting configured */
    pcToken = strtok(args, CONF_SEPARATORS);
    if (pcToken == NULL)
    {
        FatalError("%s(%d)strtok returned NULL when it should not.",
                   __FILE__, __LINE__);
    }

    if (hi_swap_config == NULL)
    {
        hi_swap_config = sfPolicyConfigCreate();
    }

    /*
    **  Global Configuration Processing
    **  We only process the global configuration once, but always check for
    **  user mistakes, like configuring more than once.  That's why we
    **  still check for the global token even if it's been checked.
    **  Force the first configuration to be the global one.
    */
    sfPolicyUserPolicySet (hi_swap_config, policy_id);
    pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)sfPolicyUserDataGetCurrent(hi_swap_config);
    if (pPolicyConfig == NULL)
    {
        if (strcasecmp(pcToken, GLOBAL) != 0) 
            ParseError("Must configure the http inspect global configuration first.");

        pPolicyConfig = (HTTPINSPECT_GLOBAL_CONF *)SnortAlloc(sizeof(HTTPINSPECT_GLOBAL_CONF));
        if (!pPolicyConfig)
        {
             ParseError("HTTP INSPECT preprocessor: memory allocate failed.\n");
        }
        sfPolicyUserDataSetCurrent(hi_swap_config, pPolicyConfig);
        iRet = HttpInspectInitializeGlobalConfig(pPolicyConfig,
                                                 ErrorString, iErrStrLen);
        if (iRet == 0)
        {
            iRet = ProcessGlobalConf(pPolicyConfig,
                                     ErrorString, iErrStrLen);

            if (iRet == 0)
            {
                PrintGlobalConf(pPolicyConfig);

                /* Add HttpInspect into the preprocessor list */
                AddFuncToPreprocList(HttpInspect, PRIORITY_APPLICATION, PP_HTTPINSPECT, PROTO_BIT__TCP);
                AddFuncToPreprocReloadVerifyList(HttpInspectReloadVerify);
            }
        }
    }
    else
    {
        if (strcasecmp(pcToken, SERVER) != 0)
        {
            if (strcasecmp(pcToken, GLOBAL) != 0) 
                ParseError("Must configure the http inspect global configuration first.");
            else
                ParseError("Invalid http inspect token: %s.", pcToken);
        }

        iRet = ProcessUniqueServerConf(pPolicyConfig,
                                       ErrorString, iErrStrLen);
    }
    
    if (iRet)
    {
        if(iRet > 0)
        {
            /*
            **  Non-fatal Error
            */
            if(*ErrorString)
            {
                ErrorMessage("%s(%d) => %s\n", 
                        file_name, file_line, ErrorString);
            }
        }
        else
        {
            /*
            **  Fatal Error, log error and exit.
            */
            if(*ErrorString)
            {
                FatalError("%s(%d) => %s\n", 
                        file_name, file_line, ErrorString);
            }
            else
            {
                /*
                **  Check if ErrorString is undefined.
                */
                if(iRet == -2)
                {
                    FatalError("%s(%d) => ErrorString is undefined.\n", 
                            file_name, file_line);
                }
                else
                {
                    FatalError("%s(%d) => Undefined Error.\n", 
                            file_name, file_line);
                }
            }
        }
    }
}

static int HttpInspectReloadVerify(void)
{
    if (hi_swap_config == NULL)
        return 0;

    sfPolicyUserDataIterate (hi_swap_config, HttpInspectVerifyPolicy);

    if (!stream_api || (stream_api->version < STREAM_API_VERSION5))
    {
        FatalError("HttpInspectConfigCheck() Streaming & reassembly "
                   "must be enabled\n");
    }

    return 0;
}

static void * HttpInspectReloadSwap(void)
{
    tSfPolicyUserContextId old_config = hi_config;

    if (hi_swap_config == NULL)
        return NULL;

    hi_config = hi_swap_config;
    hi_swap_config = NULL;

    return (void *)old_config;
}

static void HttpInspectReloadSwapFree(void *data)
{
    if (data == NULL)
        return;

    HttpInspectFreeConfigs((tSfPolicyUserContextId)data);
}
#endif
