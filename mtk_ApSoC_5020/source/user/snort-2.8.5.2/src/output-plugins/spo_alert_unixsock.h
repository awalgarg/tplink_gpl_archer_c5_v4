/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/snort-2.8.5.2/src/output-plugins/spo_alert_unixsock.h#1 $ */
/*
** Copyright (C) 2002-2009 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
** Copyright (C) 2000,2001 Andrew R. Baker <andrewb@uab.edu>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* This file gets included in plugbase.h when it is integrated into the rest 
 * of the program.  Sometime in The Future, I'll whip up a bad ass Perl script
 * to handle automatically loading all the required info into the plugbase.*
 * files.
 */

#ifndef __SPO_ALERT_UNIXSOCK_H__
#define __SPO_ALERT_UNIXSOCK_H__

#include <sys/types.h>
#include <pcap.h>
#include "event.h"

/* this struct is for the alert socket code.... */
typedef struct _Alertpkt
{
    uint8_t alertmsg[ALERTMSG_LENGTH]; /* variable.. */
    struct pcap_pkthdr pkth;
    uint32_t dlthdr;       /* datalink header offset. (ethernet, etc.. ) */
    uint32_t nethdr;       /* network header offset. (ip etc...) */
    uint32_t transhdr;     /* transport header offset (tcp/udp/icmp ..) */
    uint32_t data;
    uint32_t val;  /* which fields are valid. (NULL could be
                    * valids also) */
    /* Packet struct --> was null */
#define NOPACKET_STRUCT 0x1
    /* no transport headers in packet */
#define NO_TRANSHDR    0x2
    uint8_t pkt[SNAPLEN];
    Event event;
} Alertpkt;

void AlertUnixSockSetup(void);

#endif  /* __SPO_ALERT_UNIXSOCK_H__ */

