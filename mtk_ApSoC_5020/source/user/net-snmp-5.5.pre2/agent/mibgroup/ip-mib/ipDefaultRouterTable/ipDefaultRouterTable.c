/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 14170 $ of $ 
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/net-snmp-5.5.pre2/agent/mibgroup/ip-mib/ipDefaultRouterTable/ipDefaultRouterTable.c#1 $
 */
/** \page MFD helper for ipDefaultRouterTable
 *
 * \section intro Introduction
 * Introductory text.
 *
 */
/*
 * standard Net-SNMP includes 
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/*
 * include our parent header 
 */
#include "ipDefaultRouterTable.h"

#include <net-snmp/agent/mib_modules.h>

#include "ipDefaultRouterTable_interface.h"

const oid       ipDefaultRouterTable_oid[] = { IPDEFAULTROUTERTABLE_OID };
const int       ipDefaultRouterTable_oid_size =
OID_LENGTH(ipDefaultRouterTable_oid);

ipDefaultRouterTable_registration ipDefaultRouterTable_user_context;

void            initialize_table_ipDefaultRouterTable(void);
void            shutdown_table_ipDefaultRouterTable(void);


/**
 * Initializes the ipDefaultRouterTable module
 */
void
init_ipDefaultRouterTable(void)
{
    DEBUGMSGTL(("verbose:ipDefaultRouterTable:init_ipDefaultRouterTable",
                "called\n"));

    /*
     * TODO:300:o: Perform ipDefaultRouterTable one-time module initialization.
     */

    /*
     * here we initialize all the tables we're planning on supporting
     */
    if (should_init("ipDefaultRouterTable"))
        initialize_table_ipDefaultRouterTable();

}                               /* init_ipDefaultRouterTable */

/**
 * Shut-down the ipDefaultRouterTable module (agent is exiting)
 */
void
shutdown_ipDefaultRouterTable(void)
{
    if (should_init("ipDefaultRouterTable"))
        shutdown_table_ipDefaultRouterTable();

}

/**
 * Initialize the table ipDefaultRouterTable 
 *    (Define its contents and how it's structured)
 */
void
initialize_table_ipDefaultRouterTable(void)
{
    ipDefaultRouterTable_registration *user_context;
    u_long          flags;

    DEBUGMSGTL(("verbose:ipDefaultRouterTable:initialize_table_ipDefaultRouterTable", "called\n"));

    /*
     * TODO:301:o: Perform ipDefaultRouterTable one-time table initialization.
     */

    /*
     * TODO:302:o: |->Initialize ipDefaultRouterTable user context
     * if you'd like to pass in a pointer to some data for this
     * table, allocate or set it up here.
     */
    /*
     * a netsnmp_data_list is a simple way to store void pointers. A simple
     * string token is used to add, find or remove pointers.
     */
    user_context =
        netsnmp_create_data_list("ipDefaultRouterTable", NULL, NULL);

    /*
     * No support for any flags yet, but in the future you would
     * set any flags here.
     */
    flags = 0;

    /*
     * call interface initialization code
     */
    _ipDefaultRouterTable_initialize_interface(user_context, flags);
}                               /* initialize_table_ipDefaultRouterTable */

/**
 * Shutdown the table ipDefaultRouterTable 
 */
void
shutdown_table_ipDefaultRouterTable(void)
{
    /*
     * call interface shutdown code
     */
    _ipDefaultRouterTable_shutdown_interface
        (&ipDefaultRouterTable_user_context);
}

/**
 * extra context initialization (eg default values)
 *
 * @param rowreq_ctx    : row request context
 * @param user_init_ctx : void pointer for user (parameter to rowreq_ctx_allocate)
 *
 * @retval MFD_SUCCESS  : no errors
 * @retval MFD_ERROR    : error (context allocate will fail)
 */
int
ipDefaultRouterTable_rowreq_ctx_init(ipDefaultRouterTable_rowreq_ctx *
                                     rowreq_ctx, void *user_init_ctx)
{
    DEBUGMSGTL(("verbose:ipDefaultRouterTable:ipDefaultRouterTable_rowreq_ctx_init", "called\n"));

    netsnmp_assert(NULL != rowreq_ctx);

    /*
     * TODO:210:o: |-> Perform extra ipDefaultRouterTable rowreq initialization. (eg DEFVALS)
     */

    return MFD_SUCCESS;
}                               /* ipDefaultRouterTable_rowreq_ctx_init */

/**
 * extra context cleanup
 *
 */
void
ipDefaultRouterTable_rowreq_ctx_cleanup(ipDefaultRouterTable_rowreq_ctx *
                                        rowreq_ctx)
{
    DEBUGMSGTL(("verbose:ipDefaultRouterTable:ipDefaultRouterTable_rowreq_ctx_cleanup", "called\n"));

    netsnmp_assert(NULL != rowreq_ctx);

    /*
     * TODO:211:o: |-> Perform extra ipDefaultRouterTable rowreq cleanup.
     */
}                               /* ipDefaultRouterTable_rowreq_ctx_cleanup */

/**
 * pre-request callback
 *
 *
 * @retval MFD_SUCCESS              : success.
 * @retval MFD_ERROR                : other error
 */
int
ipDefaultRouterTable_pre_request(ipDefaultRouterTable_registration *
                                 user_context)
{
    DEBUGMSGTL(("verbose:ipDefaultRouterTable:ipDefaultRouterTable_pre_request", "called\n"));

    /*
     * TODO:510:o: Perform ipDefaultRouterTable pre-request actions.
     */

    return MFD_SUCCESS;
}                               /* ipDefaultRouterTable_pre_request */

/**
 * post-request callback
 *
 * Note:
 *   New rows have been inserted into the container, and
 *   deleted rows have been removed from the container and
 *   released.
 *
 * @param user_context
 * @param rc : MFD_SUCCESS if all requests succeeded
 *
 * @retval MFD_SUCCESS : success.
 * @retval MFD_ERROR   : other error (ignored)
 */
int
ipDefaultRouterTable_post_request(ipDefaultRouterTable_registration *
                                  user_context, int rc)
{
    DEBUGMSGTL(("verbose:ipDefaultRouterTable:ipDefaultRouterTable_post_request", "called\n"));

    /*
     * TODO:511:o: Perform ipDefaultRouterTable post-request actions.
     */

    return MFD_SUCCESS;
}                               /* ipDefaultRouterTable_post_request */


/** @{ */
