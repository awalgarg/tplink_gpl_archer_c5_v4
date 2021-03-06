/*
 * swrun data access header
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/net-snmp-5.5.pre2/agent/mibgroup/host/data_access/swrun.h#1 $
 */
/*
 * Copyright (C) 2007 Apple, Inc. All rights reserved.
 * Use is subject to license terms specified in the COPYING file
 * distributed with the Net-SNMP package.
 */
#ifndef NETSNMP_ACCESS_SWRUN_CONFIG_H
#define NETSNMP_ACCESS_SWRUN_CONFIG_H

/*
 * all platforms use this generic code
 */
config_require(host/data_access/swrun)
config_exclude(host/hr_swrun)

/*
 * select the appropriate architecture-specific interface code
 */
#if   defined( darwin )
    config_require(host/data_access/swrun_darwin)
#elif defined( HAVE_SYS_PSTAT_H )
    config_require(host/data_access/swrun_pstat)
#elif defined( dynix )
    config_require(host/data_access/swrun_prpsinfo)
#elif defined( solaris2 )
# if _SLASH_PROC_METHOD_
    config_require(host/data_access/swrun_procfs_psinfo)
# else
    config_require(host/data_access/swrun_kvm_proc)
# endif
#elif defined( aix4 ) || defined( aix5 ) || defined( aix6 )
    config_require(host/data_access/swrun_procinfo)
/* Should really be #elif HAVE_KVM_GETPROCS */
#elif defined(freebsd) || defined(openbsd) || defined(netbsd)
    config_require(host/data_access/swrun_kinfo)
#elif defined( linux )
    config_require(host/data_access/swrun_procfs_status)
#elif defined( cygwin )
    config_require(host/data_access/swrun_cygwin)
#else
    config_warning(This platform does not yet support hrSWRunTable rewrites)
    config_require(host/data_access/swrun_null)
#endif

netsnmp_cache     *netsnmp_swrun_cache(void);
netsnmp_container *netsnmp_swrun_container(void);

#endif /* NETSNMP_ACCESS_SWRUN_CONFIG_H */
