/*
 * defaultrouter data access header
 *
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/net-snmp-5.5.pre2/agent/mibgroup/ip-mib/data_access/defaultrouter.h#1 $
 */
/**---------------------------------------------------------------------*/
/*
 * configure required files
 *
 * Notes:
 *
 * 1) prefer functionality over platform, where possible. If a method
 *    is available for multiple platforms, test that first. That way
 *    when a new platform is ported, it won't need a new test here.
 *
 * 2) don't do detail requirements here. If, for example,
 *    HPUX11 had different reuirements than other HPUX, that should
 *    be handled in the *_hpux.h header file.
 */
config_require(ip-mib/data_access/defaultrouter_common)
#if defined( linux )
config_require(ip-mib/data_access/defaultrouter_linux)
#else
/*
 * couldn't determine the correct file!
 */
config_error(the defaultrouter data access library is not available in this environment.)
#endif
