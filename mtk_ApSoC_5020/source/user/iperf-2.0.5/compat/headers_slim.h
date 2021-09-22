#ifndef HEADERS_SLIM_H
#define HEADERS_SLIM_H

/* -------------------------------------------------------------------
 * headers_slim.h
 * by Mark Gates <mgates@nlanr.net>
 * Copyright 1999, Board of Trustees of the University of Illinois.
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/iperf-2.0.5/compat/headers_slim.h#1 $
 * -------------------------------------------------------------------
 * headers
 *
 * Most system headers required by iperf.
 * This is a subset of lib/headers.h, to be used only while running
 * configure. It excludes things conditional on having _already_
 * run configure, and Win32 stuff.
 * ------------------------------------------------------------------- */

/* standard C headers */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* unix headers */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>   /* netinet/in.h must be before this on SunOS */

#endif /* HEADERS_SLIM_H */
