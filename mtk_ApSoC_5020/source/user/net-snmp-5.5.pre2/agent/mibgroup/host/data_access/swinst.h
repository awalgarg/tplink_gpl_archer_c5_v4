/*
 * swinst data access header
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/net-snmp-5.5.pre2/agent/mibgroup/host/data_access/swinst.h#1 $
 */
/*
 * Copyright (C) 2007 Apple, Inc. All rights reserved.
 * Use is subject to license terms specified in the COPYING file
 * distributed with the Net-SNMP package.
 */
#ifndef NETSNMP_ACCESS_SWINST_CONFIG_H
#define NETSNMP_ACCESS_SWINST_CONFIG_H

/*
 * all platforms use this generic code
 */
config_require(host/data_access/swinst)
config_exclude(host/hr_swinst)

/*
 * select the appropriate architecture-specific interface code
 */
#if   defined( darwin )
    config_require(host/data_access/swinst_darwin)
#elif defined( HAVE_LIBRPM ) || defined( linux )
    config_require(host/data_access/swinst_rpm)
#elif defined( HAVE_PKGLOCS_H ) || defined( hpux9 ) || defined( hpux10 ) || defined( hpux11 ) || defined( freebsd2 )
    config_require(host/data_access/swinst_pkginfo)
#else
    config_warning(This platform does not yet support hrSWInstalledTable rewrites)
    config_require(host/data_access/swinst_null)
#endif

#endif /* NETSNMP_ACCESS_SWINST_CONFIG_H */
