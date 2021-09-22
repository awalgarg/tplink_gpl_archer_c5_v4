/*
 * netlink/version.h	Compile Time Versioning Information
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2011 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_VERSION_H_
#define NETLINK_VERSION_H_

#define LIBNL_STRING "libnl 3.2.11"
#define LIBNL_VERSION "3.2.11"

#define LIBNL_VER_MAJ		3
#define LIBNL_VER_MIN		2
#define LIBNL_VER_MIC		11
#define LIBNL_VER(maj,min)	((maj) << 8 | (min))
#define LIBNL_VER_NUM		LIBNL_VER(LIBNL_VER_MAJ, LIBNL_VER_MIN)

#define LIBNL_CURRENT		209
#define LIBNL_REVISION		0
#define LIBNL_AGE		9

#endif
