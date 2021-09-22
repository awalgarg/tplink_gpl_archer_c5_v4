/*
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/ppp-2.4.2/pppd/plugins/radius/radiusclient/acconfig.h#1 $
 *
 * Copyright (C) 1996,1997 Lars Fenneberg
 *
 * See the file COPYRIGHT for the respective terms and conditions. 
 * If the file is missing contact me at lf@elemental.net 
 * and I'll send you a copy.
 *
 */

@TOP@

/* does /dev/urandom exist ? */
#undef HAVE_DEV_URANDOM

/* shadow password support */
#undef HAVE_SHADOW_PASSWORDS

/* struct utsname has domainname field */
#undef HAVE_STRUCT_UTSNAME_DOMAINNAME

/* do you need the sig* prototypes ? */
#undef NEED_SIG_PROTOTYPES

/* package name */
#undef PACKAGE

/* include code to kludge aroung Livingston's RADIUS server 1.16 */
#undef RADIUS_116

/* SCP support */
#undef SCP

/* package version */
#undef VERSION
