/***********************************************************************
*
* common.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Common functions used by PPPoE client and server
*
* Copyright (C) 2000 by Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
***********************************************************************/

static char const RCSID[] =
"$Id: common.c,v 1.3 2008/06/09 08:34:23 paulus Exp $";

#define _GNU_SOURCE 1
#include "pppoe.h"
#include "pppd.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>	/* for LOG_DEBUG */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Added by chz from rp-pppoe 3.8. 2017-1-22 */
#include <sys/types.h>
#include <pwd.h>
#include "util.h"

/* Are we running SUID or SGID? */
int IsSetID = 0;

static uid_t saved_uid = -2;
static uid_t saved_gid = -2;
/* end add */

#ifdef INCLUDE_PADT_BEFORE_DIAL
/* 每次启动pppd进程，都尝试发PADT包终结上次的session. Added by Wang Jianfeng 2014-05-05*/
unsigned int oldSession;
unsigned char serverMAC[6];
#endif

/**********************************************************************
*%FUNCTION: parsePacket
*%ARGUMENTS:
* packet -- the PPPoE discovery packet to parse
* func -- function called for each tag in the packet
* extra -- an opaque data pointer supplied to parsing function
*%RETURNS:
* 0 if everything went well; -1 if there was an error
*%DESCRIPTION:
* Parses a PPPoE discovery packet, calling "func" for each tag in the packet.
* "func" is passed the additional argument "extra".
***********************************************************************/
int
parsePacket(PPPoEPacket *packet, ParseFunc *func, void *extra)
{
    UINT16_t len = ntohs(packet->length);
    unsigned char *curTag;
    UINT16_t tagType, tagLen;

    if (PPPOE_VER(packet->vertype) != 1) {
	error("Invalid PPPoE version (%d)", PPPOE_VER(packet->vertype));
	return -1;
    }
    if (PPPOE_TYPE(packet->vertype) != 1) {
	error("Invalid PPPoE type (%d)", PPPOE_TYPE(packet->vertype));
	return -1;
    }

    /* Do some sanity checks on packet */
    if (len > ETH_DATA_LEN - 6) { /* 6-byte overhead for PPPoE header */
	error("Invalid PPPoE packet length (%u)", len);
	return -1;
    }

    /* Step through the tags */
    curTag = packet->payload;
    while(curTag - packet->payload < len) {
	/* Alignment is not guaranteed, so do this by hand... */
	tagType = (curTag[0] << 8) + curTag[1];
	tagLen = (curTag[2] << 8) + curTag[3];
	if (tagType == TAG_END_OF_LIST) {
	    return 0;
	}
	if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len) {
	    error("Invalid PPPoE tag length (%u)", tagLen);
	    return -1;
	}
	func(tagType, tagLen, curTag+TAG_HDR_SIZE, extra);
	curTag = curTag + TAG_HDR_SIZE + tagLen;
    }
    return 0;
}

#ifdef INCLUDE_PADT_BEFORE_DIAL
/***********************************************************************
*%FUNCTION: sendTermination
*%ARGUMENTS:
* conn -- PPPoEConnection structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADT packet after reboot
***********************************************************************/
void
sendTermination(PPPoEConnection *conn, unsigned int session_id)
{
    PPPoEPacket packet;
    unsigned char *cursor = packet.payload;
    char mac[6];
    UINT16_t plen = 0;
 
    if (session_id == 0) return;
    /* Do nothing if no session established yet */
    //if (!conn->session) return;
 
    /* Do nothing if no discovery socket */
    if (conn->discoverySocket < 0) return;
    memcpy(packet.ethHdr.h_dest, serverMAC, ETH_ALEN);
    //memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);
 
    packet.ethHdr.h_proto = htons(Eth_PPPOE_Session);
    packet.vertype = 0x11;
    packet.code = CODE_SESS;
    packet.session = htons(session_id);//conn->session;
 
    /* if last session doesn't terminate ,terminate it now! */
    cursor[0] = 0xc0;
    cursor[1] = 0x21;
    cursor[2] = 0x05;     
    cursor[3] = 0x02;
    cursor[4] = 0x00;
    cursor[5] = 0x10;
    cursor[6] = 0x55;
    cursor[7] = 0x73;
    cursor[8] = 0x65;
    cursor[9] = 0x72;
    cursor[10] = 0x20;
    cursor[11] = 0x72;
    cursor[12] = 0x65;
    cursor[13] = 0x71;
    cursor[14] = 0x75;
    cursor[15] = 0x65;
    cursor[16] = 0x73;
    cursor[17] = 0x74;
 
    plen = 18;
    packet.length = htons(plen);
    sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE));
//info("Sent TERMINATION");
}
#endif

/* Added by chz from rp-pppoe 3.8. 2017-1-22 */
/**********************************************************************
*%FUNCTION: findTag
*%ARGUMENTS:
* packet -- the PPPoE discovery packet to parse
* type -- the type of the tag to look for
* tag -- will be filled in with tag contents
*%RETURNS:
* A pointer to the tag if one of the specified type is found; NULL
* otherwise.
*%DESCRIPTION:
* Looks for a specific tag type.
***********************************************************************/
unsigned char *
findTag(PPPoEPacket *packet, UINT16_t type, PPPoETag *tag)
{
    UINT16_t len = ntohs(packet->length);
    unsigned char *curTag;
    UINT16_t tagType, tagLen;
	
	/* Modified by chz to fit pppd-2.4.5. 2017-1-22 */
#if 0
    if (packet->ver != 1) {
	syslog(LOG_ERR, "Invalid PPPoE version (%d)", (int) packet->ver);
	return NULL;
    }
    if (packet->type != 1) {
	syslog(LOG_ERR, "Invalid PPPoE type (%d)", (int) packet->type);
	return NULL;
    }
#else
	if (PPPOE_VER(packet->vertype) != 1) {
	syslog(LOG_ERR, "Invalid PPPoE version (%d)", (int)PPPOE_VER(packet->vertype));
	return NULL;
    }
    if (PPPOE_TYPE(packet->vertype) != 1) {
	syslog(LOG_ERR, "Invalid PPPoE type (%d)", (int)PPPOE_TYPE(packet->vertype));
	return NULL;
    }
#endif
	/* end modify */

    /* Do some sanity checks on packet */
    if (len > ETH_DATA_LEN - 6) { /* 6-byte overhead for PPPoE header */
	syslog(LOG_ERR, "Invalid PPPoE packet length (%u)", len);
	return NULL;
    }

    /* Step through the tags */
    curTag = packet->payload;
    while(curTag - packet->payload < len) {
	/* Alignment is not guaranteed, so do this by hand... */
	tagType = (((UINT16_t) curTag[0]) << 8) +
	    (UINT16_t) curTag[1];
	tagLen = (((UINT16_t) curTag[2]) << 8) +
	    (UINT16_t) curTag[3];
	if (tagType == TAG_END_OF_LIST) {
	    return NULL;
	}
	if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len) {
	    syslog(LOG_ERR, "Invalid PPPoE tag length (%u)", tagLen);
	    return NULL;
	}
	if (tagType == type) {
	    memcpy(tag, curTag, tagLen + TAG_HDR_SIZE);
	    return curTag;
	}
	curTag = curTag + TAG_HDR_SIZE + tagLen;
    }
    return NULL;
}

/**********************************************************************
*%FUNCTION: switchToRealID
*%ARGUMENTS:
* None
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sets effective user-ID and group-ID to real ones.  Aborts on failure
***********************************************************************/
void
switchToRealID (void) {
    if (IsSetID) {
	if (saved_uid < 0) saved_uid = geteuid();
	if (saved_gid < 0) saved_gid = getegid();
	if (setegid(getgid()) < 0) {
	    printErr("setgid failed");
	    exit(EXIT_FAILURE);
	}
	if (seteuid(getuid()) < 0) {
	    printErr("seteuid failed");
	    exit(EXIT_FAILURE);
	}
    }
}

/**********************************************************************
*%FUNCTION: switchToEffectiveID
*%ARGUMENTS:
* None
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sets effective user-ID and group-ID back to saved gid/uid
***********************************************************************/
void
switchToEffectiveID (void) {
    if (IsSetID) {
	if (setegid(saved_gid) < 0) {
	    printErr("setgid failed");
	    exit(EXIT_FAILURE);
	}
	if (seteuid(saved_uid) < 0) {
	    printErr("seteuid failed");
	    exit(EXIT_FAILURE);
	}
    }
}

/**********************************************************************
*%FUNCTION: dropPrivs
*%ARGUMENTS:
* None
*%RETURNS:
* Nothing
*%DESCRIPTION:
* If effective ID is root, try to become "nobody".  If that fails and
* we're SUID, switch to real user-ID
***********************************************************************/
void
dropPrivs(void)
{
    struct passwd *pw = NULL;
    int ok = 0;
    if (geteuid() == 0) {
	pw = getpwnam("nobody");
	if (pw) {
	    if (setgid(pw->pw_gid) < 0) ok++;
	    if (setuid(pw->pw_uid) < 0) ok++;
	}
    }
    if (ok < 2 && IsSetID) {
	setegid(getgid());
	seteuid(getuid());
    }
}

/**********************************************************************
*%FUNCTION: printErr
*%ARGUMENTS:
* str -- error message
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Prints a message to stderr and syslog.
***********************************************************************/
void
printErr(char const *str)
{
    fprintf(stderr, "pppoe: %s\n", str);
    syslog(LOG_ERR, "%s", str);
}


/**********************************************************************
*%FUNCTION: strDup
*%ARGUMENTS:
* str -- string to copy
*%RETURNS:
* A malloc'd copy of str.  Exits if malloc fails.
***********************************************************************/
char *
strDup(char const *str)
{
    char *copy = malloc(strlen(str)+1);
    if (!copy) {
	/* Modified by chz fo fit our ppp-2.4.5. 2017-1-22 */
#if 0
	rp_fatal("strdup failed");
#else
	fatal("strdup failed");
#endif
	/* end modify */
    }
    strcpy(copy, str);
    return copy;
}

/**********************************************************************
*%FUNCTION: computeTCPChecksum
*%ARGUMENTS:
* ipHdr -- pointer to IP header
* tcpHdr -- pointer to TCP header
*%RETURNS:
* The computed TCP checksum
***********************************************************************/
UINT16_t
computeTCPChecksum(unsigned char *ipHdr, unsigned char *tcpHdr)
{
    UINT32_t sum = 0;
    UINT16_t count = ipHdr[2] * 256 + ipHdr[3];
    UINT16_t tmp;

    unsigned char *addr = tcpHdr;
    unsigned char pseudoHeader[12];

    /* Count number of bytes in TCP header and data */
    count -= (ipHdr[0] & 0x0F) * 4;

    memcpy(pseudoHeader, ipHdr+12, 8);
    pseudoHeader[8] = 0;
    pseudoHeader[9] = ipHdr[9];
    pseudoHeader[10] = (count >> 8) & 0xFF;
    pseudoHeader[11] = (count & 0xFF);

    /* Checksum the pseudo-header */
    sum += * (UINT16_t *) pseudoHeader;
    sum += * ((UINT16_t *) (pseudoHeader+2));
    sum += * ((UINT16_t *) (pseudoHeader+4));
    sum += * ((UINT16_t *) (pseudoHeader+6));
    sum += * ((UINT16_t *) (pseudoHeader+8));
    sum += * ((UINT16_t *) (pseudoHeader+10));

    /* Checksum the TCP header and data */
    while (count > 1) {
	memcpy(&tmp, addr, sizeof(tmp));
	sum += (UINT32_t) tmp;
	addr += sizeof(tmp);
	count -= sizeof(tmp);
    }
    if (count > 0) {
	sum += (unsigned char) *addr;
    }

    while(sum >> 16) {
	sum = (sum & 0xffff) + (sum >> 16);
    }
    return (UINT16_t) ((~sum) & 0xFFFF);
}

/**********************************************************************
*%FUNCTION: clampMSS
*%ARGUMENTS:
* packet -- PPPoE session packet
* dir -- either "incoming" or "outgoing"
* clampMss -- clamp value
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Clamps MSS option if TCP SYN flag is set.
***********************************************************************/
void
clampMSS(PPPoEPacket *packet, char const *dir, int clampMss)
{
    unsigned char *tcpHdr;
    unsigned char *ipHdr;
    unsigned char *opt;
    unsigned char *endHdr;
    unsigned char *mssopt = NULL;
    UINT16_t csum;

    int len, minlen;

    /* check PPP protocol type */
    if (packet->payload[0] & 0x01) {
        /* 8 bit protocol type */

        /* Is it IPv4? */
        if (packet->payload[0] != 0x21) {
            /* Nope, ignore it */
            return;
        }

        ipHdr = packet->payload + 1;
	minlen = 41;
    } else {
        /* 16 bit protocol type */

        /* Is it IPv4? */
        if (packet->payload[0] != 0x00 ||
            packet->payload[1] != 0x21) {
            /* Nope, ignore it */
            return;
        }

        ipHdr = packet->payload + 2;
	minlen = 42;
    }

    /* Is it too short? */
    len = (int) ntohs(packet->length);
    if (len < minlen) {
	/* 20 byte IP header; 20 byte TCP header; at least 1 or 2 byte PPP protocol */
	return;
    }

    /* Verify once more that it's IPv4 */
    if ((ipHdr[0] & 0xF0) != 0x40) {
	return;
    }

    /* Is it a fragment that's not at the beginning of the packet? */
    if ((ipHdr[6] & 0x1F) || ipHdr[7]) {
	/* Yup, don't touch! */
	return;
    }
    /* Is it TCP? */
    if (ipHdr[9] != 0x06) {
	return;
    }

    /* Get start of TCP header */
    tcpHdr = ipHdr + (ipHdr[0] & 0x0F) * 4;

    /* Is SYN set? */
    if (!(tcpHdr[13] & 0x02)) {
	return;
    }

    /* Compute and verify TCP checksum -- do not touch a packet with a bad
       checksum */
    csum = computeTCPChecksum(ipHdr, tcpHdr);
    if (csum) {
	syslog(LOG_ERR, "Bad TCP checksum %x", (unsigned int) csum);

	/* Upper layers will drop it */
	return;
    }

    /* Look for existing MSS option */
    endHdr = tcpHdr + ((tcpHdr[12] & 0xF0) >> 2);
    opt = tcpHdr + 20;
    while (opt < endHdr) {
	if (!*opt) break;	/* End of options */
	switch(*opt) {
	case 1:
	    opt++;
	    break;

	case 2:
	    if (opt[1] != 4) {
		/* Something fishy about MSS option length. */
		syslog(LOG_ERR,
		       "Bogus length for MSS option (%u) from %u.%u.%u.%u",
		       (unsigned int) opt[1],
		       (unsigned int) ipHdr[12],
		       (unsigned int) ipHdr[13],
		       (unsigned int) ipHdr[14],
		       (unsigned int) ipHdr[15]);
		return;
	    }
	    mssopt = opt;
	    break;
	default:
	    if (opt[1] < 2) {
		/* Someone's trying to attack us? */
		syslog(LOG_ERR,
		       "Bogus TCP option length (%u) from %u.%u.%u.%u",
		       (unsigned int) opt[1],
		       (unsigned int) ipHdr[12],
		       (unsigned int) ipHdr[13],
		       (unsigned int) ipHdr[14],
		       (unsigned int) ipHdr[15]);
		return;
	    }
	    opt += (opt[1]);
	    break;
	}
	/* Found existing MSS option? */
	if (mssopt) break;
    }

    /* If MSS exists and it's low enough, do nothing */
    if (mssopt) {
	unsigned mss = mssopt[2] * 256 + mssopt[3];
	if (mss <= clampMss) {
	    return;
	}

	mssopt[2] = (((unsigned) clampMss) >> 8) & 0xFF;
	mssopt[3] = ((unsigned) clampMss) & 0xFF;
    } else {
	/* No MSS option.  Don't add one; we'll have to use 536. */
	return;
    }

    /* Recompute TCP checksum */
    tcpHdr[16] = 0;
    tcpHdr[17] = 0;
    csum = computeTCPChecksum(ipHdr, tcpHdr);
    (* (UINT16_t *) (tcpHdr+16)) = csum;
}
/* end add */

/***********************************************************************
*%FUNCTION: sendPADT
*%ARGUMENTS:
* conn -- PPPoE connection
* msg -- if non-NULL, extra error message to include in PADT packet.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADT packet
***********************************************************************/
void
sendPADT(PPPoEConnection *conn, char const *msg)
{
    PPPoEPacket packet;
    unsigned char *cursor = packet.payload;

    UINT16_t plen = 0;

    /* Do nothing if no session established yet */
#ifdef INCLUDE_PADT_BEFORE_DIAL
    if ((!conn->session) && (oldSession == 0xffff )) return;
#else
    if (!conn->session) return;
#endif

    /* Do nothing if no discovery socket */
    if (conn->discoverySocket < 0) return;

#ifdef INCLUDE_PADT_BEFORE_DIAL
	/* 每次启动pppd进程，都尝试发PADT包终结上次的session. Modified by Wang Jianfeng 2014-05-05*/
	if (!conn->session)
	{
		memcpy(packet.ethHdr.h_dest, serverMAC, ETH_ALEN);
	}
	else
	{

		memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);

	}
#else
    memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
#endif

    memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

    packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
    packet.vertype = PPPOE_VER_TYPE(1, 1);
    packet.code = CODE_PADT;

#ifdef INCLUDE_PADT_BEFORE_DIAL
	/* 每次启动pppd进程，都尝试发PADT包终结上次的session. Modified by Wang Jianfeng 2014-05-05*/
	if (!conn->session)
	{
		packet.session = (UINT16_t)oldSession;
	}
	else
	{
		packet.session = conn->session;
	}
#else
    packet.session = conn->session;
#endif

    /* Reset Session to zero so there is no possibility of
       recursive calls to this function by any signal handler */
    conn->session = 0;

    /* If we're using Host-Uniq, copy it over */
#ifdef INCLUDE_PADT_BEFORE_DIAL
    if (conn->useHostUniq && (oldSession == 0xffff )) {
#else
    if (conn->useHostUniq) {
#endif
	PPPoETag hostUniq;
	pid_t pid = getpid();
	hostUniq.type = htons(TAG_HOST_UNIQ);
	hostUniq.length = htons(sizeof(pid));
	memcpy(hostUniq.payload, &pid, sizeof(pid));
	memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
	cursor += sizeof(pid) + TAG_HDR_SIZE;
	plen += sizeof(pid) + TAG_HDR_SIZE;
    }

    /* Copy error message */
    if (msg) {
	PPPoETag err;
	size_t elen = strlen(msg);
	err.type = htons(TAG_GENERIC_ERROR);
	err.length = htons(elen);
	strcpy(err.payload, msg);
	memcpy(cursor, &err, elen + TAG_HDR_SIZE);
	cursor += elen + TAG_HDR_SIZE;
	plen += elen + TAG_HDR_SIZE;
    }

    /* Copy cookie and relay-ID if needed */
    if (conn->cookie.type) {
	CHECK_ROOM(cursor, packet.payload,
		   ntohs(conn->cookie.length) + TAG_HDR_SIZE);
	memcpy(cursor, &conn->cookie, ntohs(conn->cookie.length) + TAG_HDR_SIZE);
	cursor += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
	plen += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
    }

    if (conn->relayId.type) {
	CHECK_ROOM(cursor, packet.payload,
		   ntohs(conn->relayId.length) + TAG_HDR_SIZE);
	memcpy(cursor, &conn->relayId, ntohs(conn->relayId.length) + TAG_HDR_SIZE);
	cursor += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
	plen += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
    }

    packet.length = htons(plen);
    sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE));
    //info("Sent PADT"); //Deleted by chz for pppoe-relay. 2017-1-22
}

#define EH(x)	(x)[0], (x)[1], (x)[2], (x)[3], (x)[4], (x)[5]

/* Print out a PPPOE packet for debugging */
void pppoe_printpkt(PPPoEPacket *packet,
		    void (*printer)(void *, char *, ...), void *arg)
{
    int len = ntohs(packet->length);
    int i, tag, tlen, text;

    switch (ntohs(packet->ethHdr.h_proto)) {
    case ETH_PPPOE_DISCOVERY:
	printer(arg, "PPPOE Discovery V%dT%d ", PPPOE_VER(packet->vertype),
		PPPOE_TYPE(packet->vertype));
	switch (packet->code) {
	case CODE_PADI:
	    printer(arg, "PADI");
	    break;
	case CODE_PADO:
	    printer(arg, "PADO");
	    break;
	case CODE_PADR:
	    printer(arg, "PADR");
	    break;
	case CODE_PADS:
	    printer(arg, "PADS");
	    break;
	case CODE_PADT:
	    printer(arg, "PADT");
	    break;
	default:
	    printer(arg, "unknown code %x", packet->code);
	}
	printer(arg, " session 0x%x length %d\n", ntohs(packet->session), len);
	break;
    case ETH_PPPOE_SESSION:
	printer(arg, "PPPOE Session V%dT%d", PPPOE_VER(packet->vertype),
		PPPOE_TYPE(packet->vertype));
	printer(arg, " code 0x%x session 0x%x length %d\n", packet->code,
		ntohs(packet->session), len);
	break;
    default:
	printer(arg, "Unknown ethernet frame with proto = 0x%x\n",
		ntohs(packet->ethHdr.h_proto));
    }

    printer(arg, " dst %x:%x:%x:%x:%x:%x ", EH(packet->ethHdr.h_dest));
    printer(arg, " src %x:%x:%x:%x:%x:%x\n", EH(packet->ethHdr.h_source));
    if (ntohs(packet->ethHdr.h_proto) != ETH_PPPOE_DISCOVERY)
	return;

    for (i = 0; i + TAG_HDR_SIZE <= len; i += tlen) {
	tag = (packet->payload[i] << 8) + packet->payload[i+1];
	tlen = (packet->payload[i+2] << 8) + packet->payload[i+3];
	if (i + tlen + TAG_HDR_SIZE > len)
	    break;
	text = 0;
	i += TAG_HDR_SIZE;
	printer(arg, " [");
	switch (tag) {
	case TAG_END_OF_LIST:
	    printer(arg, "end-of-list");
	    break;
	case TAG_SERVICE_NAME:
	    printer(arg, "service-name");
	    text = 1;
	    break;
	case TAG_AC_NAME:
	    printer(arg, "AC-name");
	    text = 1;
	    break;
	case TAG_HOST_UNIQ:
	    printer(arg, "host-uniq");
	    break;
	case TAG_AC_COOKIE:
	    printer(arg, "AC-cookie");
	    break;
	case TAG_VENDOR_SPECIFIC:
	    printer(arg, "vendor-specific");
	    break;
	case TAG_RELAY_SESSION_ID:
	    printer(arg, "relay-session-id");
	    break;
	case TAG_SERVICE_NAME_ERROR:
	    printer(arg, "service-name-error");
	    text = 1;
	    break;
	case TAG_AC_SYSTEM_ERROR:
	    printer(arg, "AC-system-error");
	    text = 1;
	    break;
	case TAG_GENERIC_ERROR:
	    printer(arg, "generic-error");
	    text = 1;
	    break;
	default:
	    printer(arg, "unknown tag 0x%x", tag);
	}
	if (tlen) {
	    if (text)
		printer(arg, " %.*v", tlen, &packet->payload[i]);
	    else if (tlen <= 32)
		printer(arg, " %.*B", tlen, &packet->payload[i]);
	    else
		printer(arg, " %.32B... (length %d)",
			&packet->payload[i], tlen);
	}
	printer(arg, "]");
    }
    printer(arg, "\n");
}

void pppoe_log_packet(const char *prefix, PPPoEPacket *packet)
{
	/* Deleted by chz for pppoe-relay. 2017-1-22 */
#if 0
    init_pr_log(prefix, LOG_DEBUG);
    pppoe_printpkt(packet, pr_log, NULL);
    end_pr_log();
#endif
	/* end delete */
}
