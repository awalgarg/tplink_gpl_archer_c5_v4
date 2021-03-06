/*
 * Note: this file originally auto-generated by mib2c using
 *  : generic-table-oids.m2c 12855 2005-09-27 15:56:08Z rstory $
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/net-snmp-5.5.pre2/agent/mibgroup/etherlike-mib/dot3StatsTable/dot3StatsTable_oids.h#1 $
 */
#ifndef DOT3STATSTABLE_OIDS_H
#define DOT3STATSTABLE_OIDS_H

#ifdef __cplusplus
extern          "C" {
#endif


    /*
     * column number definitions for table dot3StatsTable 
     */
#define DOT3STATSTABLE_OID              1,3,6,1,2,1,10,7,2

#define COLUMN_DOT3STATSINDEX         1
#define COLUMN_DOT3STATSINDEX_FLAG    (0x1 << 0)

#define COLUMN_DOT3STATSALIGNMENTERRORS         2
#define COLUMN_DOT3STATSALIGNMENTERRORS_FLAG    (0x1 << 1)

#define COLUMN_DOT3STATSFCSERRORS         3
#define COLUMN_DOT3STATSFCSERRORS_FLAG    (0x1 << 2)

#define COLUMN_DOT3STATSSINGLECOLLISIONFRAMES         4
#define COLUMN_DOT3STATSSINGLECOLLISIONFRAMES_FLAG    (0x1 << 3)

#define COLUMN_DOT3STATSMULTIPLECOLLISIONFRAMES         5
#define COLUMN_DOT3STATSMULTIPLECOLLISIONFRAMES_FLAG    (0x1 << 4)

#define COLUMN_DOT3STATSSQETESTERRORS         6
#define COLUMN_DOT3STATSSQETESTERRORS_FLAG    (0x1 << 5)

#define COLUMN_DOT3STATSDEFERREDTRANSMISSIONS         7
#define COLUMN_DOT3STATSDEFERREDTRANSMISSIONS_FLAG    (0x1 << 6)

#define COLUMN_DOT3STATSLATECOLLISIONS         8
#define COLUMN_DOT3STATSLATECOLLISIONS_FLAG    (0x1 << 7)

#define COLUMN_DOT3STATSEXCESSIVECOLLISIONS         9
#define COLUMN_DOT3STATSEXCESSIVECOLLISIONS_FLAG    (0x1 << 8)

#define COLUMN_DOT3STATSINTERNALMACTRANSMITERRORS         10
#define COLUMN_DOT3STATSINTERNALMACTRANSMITERRORS_FLAG    (0x1 << 9)

#define COLUMN_DOT3STATSCARRIERSENSEERRORS         11
#define COLUMN_DOT3STATSCARRIERSENSEERRORS_FLAG    (0x1 << 10)

#define COLUMN_DOT3STATSFRAMETOOLONGS         13
#define COLUMN_DOT3STATSFRAMETOOLONGS_FLAG    (0x1 << 12)

#define COLUMN_DOT3STATSINTERNALMACRECEIVEERRORS         16
#define COLUMN_DOT3STATSINTERNALMACRECEIVEERRORS_FLAG    (0x1 << 15)

#define COLUMN_DOT3STATSETHERCHIPSET         17
#define COLUMN_DOT3STATSETHERCHIPSET_FLAG    (0x1 << 16)

#define COLUMN_DOT3STATSSYMBOLERRORS         18
#define COLUMN_DOT3STATSSYMBOLERRORS_FLAG    (0x1 << 17)

#define COLUMN_DOT3STATSDUPLEXSTATUS         19
#define COLUMN_DOT3STATSDUPLEXSTATUS_FLAG    (0x1 << 18)

#define COLUMN_DOT3STATSRATECONTROLABILITY         20
#define COLUMN_DOT3STATSRATECONTROLABILITY_FLAG    (0x1 << 19)

#define COLUMN_DOT3STATSRATECONTROLSTATUS         21
#define COLUMN_DOT3STATSRATECONTROLSTATUS_FLAG    (0x1 << 20)


#define DOT3STATSTABLE_MIN_COL   COLUMN_DOT3STATSINDEX
#define DOT3STATSTABLE_MAX_COL   COLUMN_DOT3STATSRATECONTROLSTATUS

	
#ifdef __cplusplus
}
#endif
#endif                          /* DOT3STATSTABLE_OIDS_H */
