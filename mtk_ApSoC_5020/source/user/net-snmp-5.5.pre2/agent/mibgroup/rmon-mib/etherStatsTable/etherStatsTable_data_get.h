/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 12088 $ of $
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/net-snmp-5.5.pre2/agent/mibgroup/rmon-mib/etherStatsTable/etherStatsTable_data_get.h#1 $
 *
 * @file etherStatsTable_data_get.h
 *
 * @addtogroup get
 *
 * Prototypes for get functions
 *
 * @{
 */
#ifndef ETHERSTATSTABLE_DATA_GET_H
#define ETHERSTATSTABLE_DATA_GET_H

#ifdef __cplusplus
extern          "C" {
#endif

    /*
     *********************************************************************
     * GET function declarations
     */

    /*
     *********************************************************************
     * GET Table declarations
     */
/**********************************************************************
 **********************************************************************
 ***
 *** Table etherStatsTable
 ***
 **********************************************************************
 **********************************************************************/
    /*
     * RMON-MIB::etherStatsTable is subid 1 of statistics.
     * Its status is Current.
     * OID: .1.3.6.1.2.1.16.1.1, length: 9
     */
    /*
     * indexes
     */

    int             etherStatsDataSource_get(etherStatsTable_rowreq_ctx *
                                             rowreq_ctx,
                                             oid **
                                             etherStatsDataSource_val_ptr_ptr,
                                             size_t
                                             *etherStatsDataSource_val_ptr_len_ptr);
    int             etherStatsDropEvents_get(etherStatsTable_rowreq_ctx *
                                             rowreq_ctx,
                                             u_long *
                                             etherStatsDropEvents_val_ptr);
    int             etherStatsOctets_get(etherStatsTable_rowreq_ctx *
                                         rowreq_ctx,
                                         u_long *
                                         etherStatsOctets_val_ptr);
    int             etherStatsPkts_get(etherStatsTable_rowreq_ctx *
                                       rowreq_ctx,
                                       u_long * etherStatsPkts_val_ptr);
    int             etherStatsBroadcastPkts_get(etherStatsTable_rowreq_ctx
                                                * rowreq_ctx,
                                                u_long *
                                                etherStatsBroadcastPkts_val_ptr);
    int             etherStatsMulticastPkts_get(etherStatsTable_rowreq_ctx
                                                * rowreq_ctx,
                                                u_long *
                                                etherStatsMulticastPkts_val_ptr);
    int             etherStatsCRCAlignErrors_get(etherStatsTable_rowreq_ctx
                                                 * rowreq_ctx,
                                                 u_long *
                                                 etherStatsCRCAlignErrors_val_ptr);
    int             etherStatsUndersizePkts_get(etherStatsTable_rowreq_ctx
                                                * rowreq_ctx,
                                                u_long *
                                                etherStatsUndersizePkts_val_ptr);
    int             etherStatsOversizePkts_get(etherStatsTable_rowreq_ctx *
                                               rowreq_ctx,
                                               u_long *
                                               etherStatsOversizePkts_val_ptr);
    int             etherStatsFragments_get(etherStatsTable_rowreq_ctx *
                                            rowreq_ctx,
                                            u_long *
                                            etherStatsFragments_val_ptr);
    int             etherStatsJabbers_get(etherStatsTable_rowreq_ctx *
                                          rowreq_ctx,
                                          u_long *
                                          etherStatsJabbers_val_ptr);
    int             etherStatsCollisions_get(etherStatsTable_rowreq_ctx *
                                             rowreq_ctx,
                                             u_long *
                                             etherStatsCollisions_val_ptr);
    int             etherStatsPkts64Octets_get(etherStatsTable_rowreq_ctx *
                                               rowreq_ctx,
                                               u_long *
                                               etherStatsPkts64Octets_val_ptr);
    int            
        etherStatsPkts65to127Octets_get(etherStatsTable_rowreq_ctx *
                                        rowreq_ctx,
                                        u_long *
                                        etherStatsPkts65to127Octets_val_ptr);
    int            
        etherStatsPkts128to255Octets_get(etherStatsTable_rowreq_ctx *
                                         rowreq_ctx,
                                         u_long *
                                         etherStatsPkts128to255Octets_val_ptr);
    int            
        etherStatsPkts256to511Octets_get(etherStatsTable_rowreq_ctx *
                                         rowreq_ctx,
                                         u_long *
                                         etherStatsPkts256to511Octets_val_ptr);
    int            
        etherStatsPkts512to1023Octets_get(etherStatsTable_rowreq_ctx *
                                          rowreq_ctx,
                                          u_long *
                                          etherStatsPkts512to1023Octets_val_ptr);
    int            
        etherStatsPkts1024to1518Octets_get(etherStatsTable_rowreq_ctx *
                                           rowreq_ctx,
                                           u_long *
                                           etherStatsPkts1024to1518Octets_val_ptr);
    int             etherStatsOwner_get(etherStatsTable_rowreq_ctx *
                                        rowreq_ctx,
                                        char **etherStatsOwner_val_ptr_ptr,
                                        size_t
                                        *etherStatsOwner_val_ptr_len_ptr);
    int             etherStatsStatus_get(etherStatsTable_rowreq_ctx *
                                         rowreq_ctx,
                                         u_long *
                                         etherStatsStatus_val_ptr);


    int            
        etherStatsTable_indexes_set_tbl_idx(etherStatsTable_mib_index *
                                            tbl_idx,
                                            long etherStatsIndex_val);
    int             etherStatsTable_indexes_set(etherStatsTable_rowreq_ctx
                                                * rowreq_ctx,
                                                long etherStatsIndex_val);




#ifdef __cplusplus
}
#endif
#endif                          /* ETHERSTATSTABLE_DATA_GET_H */
/** @} */
