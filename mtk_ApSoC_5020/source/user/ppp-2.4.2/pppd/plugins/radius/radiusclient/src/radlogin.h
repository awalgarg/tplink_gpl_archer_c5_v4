/*
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/ppp-2.4.2/pppd/plugins/radius/radiusclient/src/radlogin.h#1 $
 *
 * Copyright (C) 1996 Lars Fenneberg
 *
 * See the file COPYRIGHT for the respective terms and conditions. 
 * If the file is missing contact me at lf@elemental.net 
 * and I'll send you a copy.
 *
 */

#ifndef RADLOGIN_H
#define RADLOGIN_H

#undef __P
#if defined (__STDC__) || defined (_AIX) || (defined (__mips) && defined (_SYSTYPE_SVR4)) || defined(WIN32) || defined(__cplusplus)
# define __P(protos) protos
#else
# define __P(protos) ()
#endif

typedef void (*LFUNC)(char *);

/* radius.c */
LFUNC auth_radius(UINT4, char *, char *);
void radius_login(char *);

/* local.c */
LFUNC auth_local __P((char *, char *));
void local_login __P((char *));

#endif /* RADLOGIN_H */
