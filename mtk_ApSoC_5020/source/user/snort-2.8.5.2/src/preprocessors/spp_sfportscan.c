/****************************************************************************
 *
 * Copyright (C) 2004-2009 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/*
**  @file       spp_sfportscan.c
**
**  @author     Daniel Roelker <droelker@sourcefire.com>
**
**  @brief      Portscan detection
**
**  NOTES
**    - User Configuration:  The following is a list of parameters that can
**      be configured through the user interface:
**
**      proto  { tcp udp icmp ip all }
**      scan_type { portscan portsweep decoy_portscan distributed_portscan all }
**      sense_level { high }    # high, medium, low
**      watch_ip { }            # list of IPs, CIDR blocks
**      ignore_scanners { }     # list of IPs, CIDR blocks
**      ignore_scanned { }      # list of IPs, CIDR blocks
**      memcap { 10000000 }     # number of max bytes to allocate
**      logfile { /tmp/ps.log } # file to log detailed portscan info
*/

#include <sys/types.h>
#include <errno.h>

#ifndef WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif /* !WIN32 */

#include "decode.h"
#include "plugbase.h"
#include "generators.h"
#include "event_wrapper.h"
#include "util.h"
#include "ipobj.h"
#include "checksum.h"
#include "packet_time.h"
#include "snort.h"
#include "sfthreshold.h"
#include "sfsnprintfappend.h"
#include "sf_iph.h"

#include "portscan.h"

#include "profiler.h"

#define DELIMITERS " \t\n"
#define TOKEN_ARG_BEGIN "{"
#define TOKEN_ARG_END   "}"

#define PROTO_BUFFER_SIZE 256

extern char *file_name;
extern int   file_line;
extern SnortConfig *snort_conf_for_parsing;

tSfPolicyUserContextId portscan_config = NULL;
PortscanConfig *portscan_eval_config = NULL;
static Packet *g_tmp_pkt = NULL;
static FILE   *g_logfile = NULL;

#ifdef PERF_PROFILING
PreprocStats sfpsPerfStats;
#endif

static void PortscanResetFunction(int signal, void *foo);
static void PortscanResetStatsFunction(int signal, void *foo);
static void ParsePortscan(PortscanConfig *, char *);
static void PortscanFreeConfigs(tSfPolicyUserContextId );
static void PortscanFreeConfig(PortscanConfig *);
static void PortscanOpenLogFile(void *);
static int PortscanGetProtoBits(int);

#ifdef SNORT_RELOAD
tSfPolicyUserContextId portscan_swap_config = NULL;
static void PortscanReload(char *);
static int PortscanReloadVerify(void);
static void * PortscanReloadSwap(void);
static void PortscanReloadSwapFree(void *);
#endif

/*
**  NAME
**    PortscanPacketInit::
*/
/**
**  Initialize the Packet structure buffer so we can generate our
**  alert packets for portscan.  We initialize the various fields in
**  the Packet structure and set the hardware layer for easy identification
**  by user interfaces.
**
**  @return int
**
**  @retval !0 initialization failed
**  @retval  0 success
*/
static int PortscanPacketInit(void)
{
    const char mac_addr[] = "MACDAD";
    Packet *p;

    p = (Packet *)SnortAlloc(sizeof(Packet));
    p->pkth = (struct pcap_pkthdr *)SnortAlloc(sizeof(struct pcap_pkthdr) +
                    ETHERNET_HEADER_LEN + VLAN_HEADER_LEN + SPARC_TWIDDLE + IP_MAXPACKET);
    
    /* This is a space holder in case we get a packet with a vlan header.  It
     * is not used in the portscan packet created, but in unified2 logging */
    p->vh   = (VlanTagHdr *)((u_char *)p->pkth + sizeof(struct pcap_pkthdr));

    /* Add 2 to align iph struct members on 4 byte boundaries - for sparc, etc */
    p->pkt  = (u_char *)p->vh + VLAN_HEADER_LEN + SPARC_TWIDDLE;
    p->eh   = (EtherHdr *)((u_char *)p->pkt);
    p->iph  = (IPHdr *)((u_char *)p->eh + ETHERNET_HEADER_LEN);
    p->data = (u_char *)p->iph + sizeof(IPHdr);

    /*
    **  Set the ethernet header with our cooked values.
    */
    ((EtherHdr *)p->eh)->ether_type = htons(0x0800);
    memcpy(((EtherHdr *)p->eh)->ether_dst, mac_addr, 6);
    memcpy(((EtherHdr *)p->eh)->ether_src, mac_addr, 6);

    g_tmp_pkt = p;

    return 0;
}

void PortscanCleanExitFunction(int signal, void *foo)
{
    ps_cleanup();
    free((void *)g_tmp_pkt->pkth);
    free(g_tmp_pkt);
    g_tmp_pkt = NULL;
    PortscanFreeConfigs(portscan_config);
    portscan_config = NULL;
}


void PortscanRestartFunction(int signal, void *foo)
{
    ps_cleanup();
    free((void *)g_tmp_pkt->pkth);
    free(g_tmp_pkt);
    g_tmp_pkt = NULL;
    PortscanFreeConfigs(portscan_config);
    portscan_config = NULL;
}

static void PortscanResetFunction(int signal, void *foo)
{
    ps_reset();
}

static void PortscanResetStatsFunction(int signal, void *foo)
{
    return;
}

/*
**  NAME
**    MakeProtoInfo::
*/
/**
**  This routine makes the portscan payload for the events.  The listed
**  info is:
**    - priority count (number of error transmissions RST/ICMP UNREACH)
**    - connection count (number of protocol connections SYN)
**    - ip count (number of IPs that communicated with host)
**    - ip range (low to high range of IPs)
**    - port count (number of port changes that occurred on host)
**    - port range (low to high range of ports connected too)
**
**  @return integer
**
**  @retval -1 buffer not large enough
**  @retval  0 successful
*/
static int MakeProtoInfo(PS_PROTO *proto, u_char *buffer, u_int *total_size)
{
    int             dsize;
#ifdef SUP_IP6
    sfip_t          *ip1, *ip2;
#else
    struct          in_addr ip1, ip2;
#endif


    if(!total_size || !buffer)
        return -1;

    dsize = (IP_MAXPACKET - *total_size);

    if(dsize < PROTO_BUFFER_SIZE)
       return -1; 

#ifdef SUP_IP6
    ip1 = &proto->low_ip;
    ip2 = &proto->high_ip;
#else
    /* low & high are already in host order.  Make them network
     * order so we can use inet_ntoa below */
    ip1.s_addr = htonl(proto->low_ip);
    ip2.s_addr = htonl(proto->high_ip);
#endif

    if(proto->alerts == PS_ALERT_PORTSWEEP ||
       proto->alerts == PS_ALERT_PORTSWEEP_FILTERED)
    {
        SnortSnprintf((char *)buffer, PROTO_BUFFER_SIZE,
                      "Priority Count: %d\n"
                      "Connection Count: %d\n"
                      "IP Count: %d\n"
                      "Scanned IP Range: %s:",
                      proto->priority_count,
                      proto->connection_count,
                      proto->u_ip_count,
                      inet_ntoa(ip1));

        /* Now print the high ip into the buffer.  This saves us
         * from having to copy the results of inet_ntoa (which is
         * a static buffer) to avoid the reuse of that buffer when
         * more than one use of inet_ntoa is within the same printf.
         */
        SnortSnprintfAppend((char *)buffer, PROTO_BUFFER_SIZE,
                      "%s\n"
                      "Port/Proto Count: %d\n"
                      "Port/Proto Range: %d:%d\n",
                      inet_ntoa(ip2),
                      proto->u_port_count,
                      proto->low_p,
                      proto->high_p);
    }
    else
    {
        SnortSnprintf((char *)buffer, PROTO_BUFFER_SIZE,
                      "Priority Count: %d\n"
                      "Connection Count: %d\n"
                      "IP Count: %d\n"
                      "Scanner IP Range: %s:",
                      proto->priority_count,
                      proto->connection_count,
                      proto->u_ip_count,
                      inet_ntoa(ip1)
                      );

        /* Now print the high ip into the buffer.  This saves us
         * from having to copy the results of inet_ntoa (which is
         * a static buffer) to avoid the reuse of that buffer when
         * more than one use of inet_ntoa is within the same printf.
         */
        SnortSnprintfAppend((char *)buffer, PROTO_BUFFER_SIZE,
                      "%s\n"
                      "Port/Proto Count: %d\n"
                      "Port/Proto Range: %d:%d\n",
                      inet_ntoa(ip2),
                      proto->u_port_count,
                      proto->low_p,
                      proto->high_p);
    }

    dsize = SnortStrnlen((const char *)buffer, PROTO_BUFFER_SIZE);
    *total_size += dsize;

    /*
    **  Set the payload size.  This is protocol independent.
    */
    g_tmp_pkt->dsize = dsize;

    return 0;
}

static int LogPortscanAlert(Packet *p, char *msg, uint32_t event_id,
        uint32_t event_ref, uint32_t gen_id, uint32_t sig_id)
{
    char timebuf[TIMEBUF_SIZE];
    snort_ip_p src_addr;
    snort_ip_p dst_addr;

    if(!p->iph)
        return -1;

    /* Do not log if being suppressed */
    src_addr = GET_SRC_IP(p);
    dst_addr = GET_DST_IP(p);

    if( sfthreshold_test(gen_id, sig_id, src_addr, dst_addr, p->pkth->ts.tv_sec) )
    {
        return 0;
    }

    ts_print((struct timeval *)&p->pkth->ts, timebuf);

    fprintf(g_logfile, "Time: %s\n", timebuf);

    if(event_id)
        fprintf(g_logfile, "event_id: %u\n", event_id);
    else
        fprintf(g_logfile, "event_ref: %u\n", event_ref);

    fprintf(g_logfile, "%s ", inet_ntoa(GET_SRC_ADDR(p)));
    fprintf(g_logfile, "-> %s %s\n", inet_ntoa(GET_DST_ADDR(p)), msg);
    fprintf(g_logfile, "%.*s\n", p->dsize, p->data);

    fflush(g_logfile);

    return 0;
}

static int GeneratePSSnortEvent(Packet *p,uint32_t gen_id,uint32_t sig_id, 
        uint32_t sig_rev, uint32_t class, uint32_t priority, char *msg)
{
    unsigned int event_id;

    event_id = GenerateSnortEvent(p,gen_id,sig_id,sig_rev,class,priority,msg);

    if(g_logfile)
        LogPortscanAlert(p, msg, event_id, 0, gen_id, sig_id);

    return event_id;
}

/*
**  NAME
**    GenerateOpenPortEvent::
*/
/**
**  We have to generate open port events differently because we tag these
**  to the original portscan event.
**
**  @return int
**
**  @retval 0 success
*/
static int GenerateOpenPortEvent(Packet *p, uint32_t gen_id, uint32_t sig_id,
        uint32_t sig_rev, uint32_t class, uint32_t pri, 
        uint32_t event_ref, struct timeval *event_time, char *msg)
{
    Event event;

    /*
    **  This means that we logged an open port, but we don't have a event
    **  reference for it, so we don't log a snort event.  We still keep
    **  track of it though.
    */
    if(!event_ref)
        return 0;

    /* reset the thresholding subsystem checks for this packet */
    sfthreshold_reset();
            
    SetEvent(&event, gen_id, sig_id, sig_rev, class, pri, event_ref);
    //CallAlertFuncs(p,msg,NULL,&event);

    event.ref_time.tv_sec  = event_time->tv_sec;
    event.ref_time.tv_usec = event_time->tv_usec;

    if(p)
    {
        /*
         * Do threshold test for suppression and thresholding.  We have to do it
         * here since these are tagged packets, which aren't subject to thresholding,
         * but we want to do it for open port events.
         */
        if( sfthreshold_test(gen_id, sig_id, GET_SRC_IP(p),
                            GET_DST_IP(p), p->pkth->ts.tv_sec) )
        {
            return 0;
        }

        CallLogFuncs(p,msg,NULL,&event);
    } 
    else 
    {
        return -1;
    }

    if(g_logfile)
        LogPortscanAlert(p, msg, 0, event_ref, gen_id, sig_id);

    return event.event_id;
}

/*
**  NAME
**    MakeOpenPortInfo::
*/
/** 
**  Write out the open ports info for open port alerts.
**
**  @return integer
*/
static int MakeOpenPortInfo(PS_PROTO *proto, u_char *buffer, u_int *total_size,
         void *user)
{
    int dsize;

    if(!total_size || !buffer)
        return -1;

    dsize = (IP_MAXPACKET - *total_size);

    if(dsize < PROTO_BUFFER_SIZE)
       return -1; 

    SnortSnprintf((char *)buffer, PROTO_BUFFER_SIZE,
                  "Open Port: %u\n", *((unsigned short *)user));

    dsize = SnortStrnlen((const char *)buffer, PROTO_BUFFER_SIZE);
    *total_size += dsize;

    /*
    **  Set the payload size.  This is protocol independent.
    */
    g_tmp_pkt->dsize = dsize;

    return 0;
}

/*
**  NAME
**    MakePortscanPkt::
*/
/*
**  We have to create this fake packet so portscan data can be passed
**  through the unified output.
**
**  We want to copy the network and transport layer headers into our
**  fake packet.
**  
*/
static int MakePortscanPkt(PS_PKT *ps_pkt, PS_PROTO *proto, int proto_type,
        void *user)
{
    Packet *p;
    unsigned int   hlen;
    unsigned int   ip_size = 0;
    struct pcap_pkthdr *pkth = (struct pcap_pkthdr *)g_tmp_pkt->pkth;

    if(!ps_pkt && proto_type != PS_PROTO_OPEN_PORT)
       return -1;

    if(ps_pkt)
    { 
        p = (Packet *)ps_pkt->pkt;

        /* Reset this to NULL in case previous alert had vlan and this one doesn't
         * The packet passed in might be a g_tmp_pkt that's already been run through
         * here with the same "wire" packet so only reset if it's not.
         * In particular from PortscanAlertTcp where the open port logging is done
         * after the general TCP portscan packet has been created and logged */
        if (p != g_tmp_pkt)
            g_tmp_pkt->vh = NULL;

        //copy vlan headers if present
        if(p->vh != NULL)
        {
            /* Need to do this here since if there is not a vlan header in
             * original packet we want it to be NULL */
            g_tmp_pkt->vh = (VlanTagHdr *)((u_char *)g_tmp_pkt->pkth + sizeof(struct pcap_pkthdr));
            *(VlanTagHdr *)g_tmp_pkt->vh = *p->vh;
        }

        //copy ethernet headers if present
        if(p->eh != NULL)
        {
            //Should copy ethernet source and destination addresses also.
            
            //ethernet type can be 0x8100 (VLAN) or 0x86DD (IPv6)
            ((EtherHdr *)g_tmp_pkt->eh)->ether_type = p->eh->ether_type;
        }

        if(IS_IP4(p))    
        {
            hlen = GET_IPH_HLEN(p)<<2;

            if ( p->iph != g_tmp_pkt->iph )
            {
                /*
                 * it happen that ps_pkt->pkt can be the same
                 * as g_tmp_pkt. Avoid overlapping copy then.
                 */
                memcpy((IPHdr *)g_tmp_pkt->iph, p->iph, hlen);
            }

            if(ps_pkt->reverse_pkt)
            {
                uint32_t tmp_addr = p->iph->ip_src.s_addr;
                ((IPHdr *)g_tmp_pkt->iph)->ip_src.s_addr = p->iph->ip_dst.s_addr;
                ((IPHdr *)g_tmp_pkt->iph)->ip_dst.s_addr = tmp_addr;
            }

            ip_size += hlen;

            ((IPHdr *)g_tmp_pkt->iph)->ip_proto = 0xff;
            ((IPHdr *)g_tmp_pkt->iph)->ip_ttl = 0x00;
            g_tmp_pkt->data = (uint8_t *)((uint8_t *)g_tmp_pkt->iph + hlen);

            pkth->ts.tv_sec = p->pkth->ts.tv_sec;
            pkth->ts.tv_usec = p->pkth->ts.tv_usec;
#ifdef SUP_IP6
            sfiph_build(g_tmp_pkt, g_tmp_pkt->iph, p->family);
#endif
        }
#ifdef SUP_IP6
        else if (IS_IP6(p))
        {
            sfiph_build(g_tmp_pkt, p->iph, p->family);

            if (ps_pkt->reverse_pkt)
            {
                sfip_t tmp_addr;
                sfip_set_ip(&tmp_addr, &p->inner_ip6h.ip_src);
                sfip_set_ip(&g_tmp_pkt->inner_ip6h.ip_src, &p->inner_ip6h.ip_dst);
                sfip_set_ip(&g_tmp_pkt->inner_ip6h.ip_dst, &tmp_addr);
            }

            ip_size += sizeof(IP6RawHdr);
            g_tmp_pkt->inner_ip6h.next = 0xff;
            g_tmp_pkt->inner_ip6h.hop_lmt = 0x00;
            g_tmp_pkt->data = (uint8_t *)((uint8_t *)g_tmp_pkt->iph + sizeof(IP6RawHdr));
            g_tmp_pkt->ip6h = &g_tmp_pkt->inner_ip6h;

            pkth->ts.tv_sec = p->pkth->ts.tv_sec;
            pkth->ts.tv_usec = p->pkth->ts.tv_usec;
        }
#endif
        else
        {
            return -1;
        }
    }

    switch(proto_type)
    {
        case PS_PROTO_TCP:
        case PS_PROTO_UDP:
        case PS_PROTO_ICMP:
        case PS_PROTO_IP:
            if(MakeProtoInfo(proto, (u_char *)g_tmp_pkt->data, &ip_size))
                return -1;

            break;

        case PS_PROTO_OPEN_PORT:
            if(MakeOpenPortInfo(proto, (u_char *)g_tmp_pkt->data, &ip_size, user))
                return -1;

            break;

        default:
            return -1;
    }

    /*
    **  Let's finish up the IP header and checksum.
    */
    if(IS_IP4(g_tmp_pkt))    
    {
        ((IPHdr *)g_tmp_pkt->iph)->ip_len = htons((short)ip_size);
#ifdef SUP_IP6
        g_tmp_pkt->inner_ip4h.ip_len = ((IPHdr *)g_tmp_pkt->iph)->ip_len;
#endif
        g_tmp_pkt->actual_ip_len = ip_size;
        ((IPHdr *)g_tmp_pkt->iph)->ip_csum = 0;
        ((IPHdr *)g_tmp_pkt->iph)->ip_csum = 
            in_chksum_ip((u_short *)g_tmp_pkt->iph, (IP_HLEN(g_tmp_pkt->iph)<<2));
    }
#ifdef SUP_IP6
    else if (IS_IP6(g_tmp_pkt))
    {
        int tmp_size = ip_size - 20;
        g_tmp_pkt->inner_ip6h.len = htons((short)tmp_size);

        g_tmp_pkt->actual_ip_len = ip_size;
        /* No checksum here */
    }
#endif

    /*
    **  And we set the pcap headers correctly so they decode.
    */

    pkth->caplen = ip_size + ETHERNET_HEADER_LEN;
    pkth->len    = pkth->caplen;

    return 0;
}

static int PortscanAlertTcp(Packet *p, PS_PROTO *proto, int proto_type)
{
    int iCtr;
    unsigned int event_ref;
    int portsweep = 0;
    
    if(!proto)
        return -1;

    switch(proto->alerts)
    {
        case PS_ALERT_ONE_TO_ONE:
            event_ref = GeneratePSSnortEvent(p, GENERATOR_PSNG, 
                    PSNG_TCP_PORTSCAN, 0, 0, 3, PSNG_TCP_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_DECOY:
            event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_TCP_DECOY_PORTSCAN,0,0,3,PSNG_TCP_DECOY_PORTSCAN_STR);
            break;

        case PS_ALERT_PORTSWEEP:
           event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                   PSNG_TCP_PORTSWEEP, 0, 0, 3, PSNG_TCP_PORTSWEEP_STR);
           portsweep = 1;
           
           break;

        case PS_ALERT_DISTRIBUTED:
            event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_TCP_DISTRIBUTED_PORTSCAN, 0, 0, 3, 
                    PSNG_TCP_DISTRIBUTED_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_FILTERED:
            event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_TCP_FILTERED_PORTSCAN,0,0,3, 
                    PSNG_TCP_FILTERED_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_DECOY_FILTERED:
            event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_TCP_FILTERED_DECOY_PORTSCAN, 0,0,3, 
                    PSNG_TCP_FILTERED_DECOY_PORTSCAN_STR);
            break;

        case PS_ALERT_PORTSWEEP_FILTERED:
           event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                   PSNG_TCP_PORTSWEEP_FILTERED,0,0,3,
                   PSNG_TCP_PORTSWEEP_FILTERED_STR);
           portsweep = 1;

           return 0;

        case PS_ALERT_DISTRIBUTED_FILTERED:
            event_ref = GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_TCP_FILTERED_DISTRIBUTED_PORTSCAN, 0, 0, 3, 
                    PSNG_TCP_FILTERED_DISTRIBUTED_PORTSCAN_STR);
            break;

        default:
            return 0;
    }

    /*
    **  Set the current event reference information for any open ports.
    */
    proto->event_ref  = event_ref;
    proto->event_time.tv_sec  = p->pkth->ts.tv_sec;
    proto->event_time.tv_usec = p->pkth->ts.tv_usec;

    /*
    **  Only log open ports for portsweeps after the alert has been
    **  generated.
    */
    if(proto->open_ports_cnt && !portsweep)
    {
        for(iCtr = 0; iCtr < proto->open_ports_cnt; iCtr++)
        {
            struct pcap_pkthdr *pkth = (struct pcap_pkthdr *)g_tmp_pkt->pkth;
            PS_PKT ps_pkt;            
            
            memset(&ps_pkt, 0x00, sizeof(PS_PKT));
            ps_pkt.pkt = (void *)p;

            if(MakePortscanPkt(&ps_pkt, proto, PS_PROTO_OPEN_PORT, 
                        (void *)&proto->open_ports[iCtr]))
                return -1;

            pkth->ts.tv_usec += 1;
            GenerateOpenPortEvent(g_tmp_pkt,GENERATOR_PSNG,PSNG_OPEN_PORT,
                    0,0,3, proto->event_ref, &proto->event_time, 
                    PSNG_OPEN_PORT_STR);
        }
    }

    return 0;
}

static int PortscanAlertUdp(Packet *p, PS_PROTO *proto, int proto_type)
{
    if(!proto)
        return -1;

    switch(proto->alerts)
    {
        case PS_ALERT_ONE_TO_ONE:
            GeneratePSSnortEvent(p, GENERATOR_PSNG, PSNG_UDP_PORTSCAN, 0, 0, 3,
                    PSNG_UDP_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_DECOY:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_UDP_DECOY_PORTSCAN, 0, 0, 3,
                    PSNG_UDP_DECOY_PORTSCAN_STR);
            break;

        case PS_ALERT_PORTSWEEP:
           GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_UDP_PORTSWEEP, 0, 0, 3,
                    PSNG_UDP_PORTSWEEP_STR);
            break;

        case PS_ALERT_DISTRIBUTED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_UDP_DISTRIBUTED_PORTSCAN, 
                    0, 0, 3, PSNG_UDP_DISTRIBUTED_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_FILTERED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_UDP_FILTERED_PORTSCAN,0,0,3,
                    PSNG_UDP_FILTERED_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_DECOY_FILTERED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_UDP_FILTERED_DECOY_PORTSCAN,
                    0,0,3, PSNG_UDP_FILTERED_DECOY_PORTSCAN_STR);
            break;

        case PS_ALERT_PORTSWEEP_FILTERED:
           GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_UDP_PORTSWEEP_FILTERED,0,0,3,
                    PSNG_UDP_PORTSWEEP_FILTERED_STR);
            break;

        case PS_ALERT_DISTRIBUTED_FILTERED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_UDP_FILTERED_DISTRIBUTED_PORTSCAN, 0, 0, 3, 
                    PSNG_UDP_FILTERED_DISTRIBUTED_PORTSCAN_STR);
            break;

        default:
            break;
    }

    return 0;
}

static int PortscanAlertIp(Packet *p, PS_PROTO *proto, int proto_type)
{
    if(!proto)
        return -1;

    switch(proto->alerts)
    {
        case PS_ALERT_ONE_TO_ONE:
            GeneratePSSnortEvent(p, GENERATOR_PSNG, PSNG_IP_PORTSCAN, 0, 0, 3,
                    PSNG_IP_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_DECOY:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_IP_DECOY_PORTSCAN, 0, 0, 3,
                    PSNG_IP_DECOY_PORTSCAN_STR);
            break;

        case PS_ALERT_PORTSWEEP:
           GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_IP_PORTSWEEP, 0, 0, 3,
                    PSNG_IP_PORTSWEEP_STR);
            break;

        case PS_ALERT_DISTRIBUTED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_IP_DISTRIBUTED_PORTSCAN, 
                    0, 0, 3, PSNG_IP_DISTRIBUTED_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_FILTERED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_IP_FILTERED_PORTSCAN,0,0,3,
                    PSNG_IP_FILTERED_PORTSCAN_STR);
            break;

        case PS_ALERT_ONE_TO_ONE_DECOY_FILTERED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_IP_FILTERED_DECOY_PORTSCAN,
                    0,0,3, PSNG_IP_FILTERED_DECOY_PORTSCAN_STR);
            break;

        case PS_ALERT_PORTSWEEP_FILTERED:
           GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_IP_PORTSWEEP_FILTERED,0,0,3,
                    PSNG_IP_PORTSWEEP_FILTERED_STR);
            break;

        case PS_ALERT_DISTRIBUTED_FILTERED:
            GeneratePSSnortEvent(p,GENERATOR_PSNG,
                    PSNG_IP_FILTERED_DISTRIBUTED_PORTSCAN, 0, 0, 3, 
                    PSNG_IP_FILTERED_DISTRIBUTED_PORTSCAN_STR);
            break;

        default:
            break;
    }

    return 0;
}

static int PortscanAlertIcmp(Packet *p, PS_PROTO *proto, int proto_type)
{
    if(!proto)
        return -1;

    switch(proto->alerts)
    {
        case PS_ALERT_PORTSWEEP:
           GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_ICMP_PORTSWEEP, 0, 0, 3,
                    PSNG_ICMP_PORTSWEEP_STR);
            break;

        case PS_ALERT_PORTSWEEP_FILTERED:
           GeneratePSSnortEvent(p,GENERATOR_PSNG,PSNG_ICMP_PORTSWEEP_FILTERED,0,0,3,
                    PSNG_ICMP_PORTSWEEP_FILTERED_STR);
            break;

        default:
            break;
    }

    return 0;
}

static int PortscanAlert(PS_PKT *ps_pkt, PS_PROTO *proto, int proto_type)
{
    Packet *p;

    if(!ps_pkt || !ps_pkt->pkt)
        return -1;

    p = (Packet *)ps_pkt->pkt;
    
    if(proto->alerts == PS_ALERT_OPEN_PORT)
    {
        if(MakePortscanPkt(ps_pkt, proto, PS_PROTO_OPEN_PORT, (void *)&p->sp))
            return -1;

        GenerateOpenPortEvent(g_tmp_pkt,GENERATOR_PSNG,PSNG_OPEN_PORT,0,0,3,
                proto->event_ref, &proto->event_time, PSNG_OPEN_PORT_STR);
    }
    else
    {
        if(MakePortscanPkt(ps_pkt, proto, proto_type, NULL))
            return -1;

        switch(proto_type)
        {
            case PS_PROTO_TCP:
                PortscanAlertTcp(g_tmp_pkt, proto, proto_type);
                break;

            case PS_PROTO_UDP:
                PortscanAlertUdp(g_tmp_pkt, proto, proto_type);
                break;

            case PS_PROTO_ICMP:
                PortscanAlertIcmp(g_tmp_pkt, proto, proto_type);
                break;

            case PS_PROTO_IP:
                PortscanAlertIp(g_tmp_pkt, proto, proto_type);
                break;
        }
    }

    sfthreshold_reset();

    return 0;
}

static void PortscanDetect(Packet *p, void *context)
{
    PS_PKT ps_pkt;
    tSfPolicyId policy_id = getRuntimePolicy();
    PortscanConfig *pPolicyConfig = NULL;
    PROFILE_VARS;
    sfPolicyUserPolicySet (portscan_config, policy_id);
    pPolicyConfig = (PortscanConfig *)sfPolicyUserDataGetCurrent(portscan_config); 
    if ( pPolicyConfig == NULL)
        return;

    portscan_eval_config = pPolicyConfig;

    if(!p || !IPH_IS_VALID(p) || (p->packet_flags & PKT_REBUILT_STREAM))
        return;

    PREPROC_PROFILE_START(sfpsPerfStats);

    memset(&ps_pkt, 0x00, sizeof(PS_PKT));
    ps_pkt.pkt = (void *)p;

    /* See if there is already an exisiting node in the hash table */

    ps_detect(&ps_pkt);

    if (ps_pkt.scanner && ps_pkt.scanner->proto.alerts &&
       (ps_pkt.scanner->proto.alerts != PS_ALERT_GENERATED))
    {
        PortscanAlert(&ps_pkt, &ps_pkt.scanner->proto, ps_pkt.proto);
    }

    if (ps_pkt.scanned && ps_pkt.scanned->proto.alerts &&
        (ps_pkt.scanned->proto.alerts != PS_ALERT_GENERATED))
    {
        PortscanAlert(&ps_pkt, &ps_pkt.scanned->proto, ps_pkt.proto);
    }

    PREPROC_PROFILE_END(sfpsPerfStats);
}

NORETURN static void FatalErrorNoOption(u_char *option)
{
    FatalError("%s(%d) => No argument to '%s' config option.\n", 
            file_name, file_line, option);
}

NORETURN static void FatalErrorNoEnd(char *option)
{
    FatalError("%s(%d) => No ending brace to '%s' config option.\n", 
            file_name, file_line, option);
}

NORETURN static void FatalErrorInvalidArg(char *option)
{
    FatalError("%s(%d) => Invalid argument to '%s' config option.\n", 
            file_name, file_line, option);
}

NORETURN static void FatalErrorInvalidOption(char *option)
{
    FatalError("%s(%d) => Invalid option '%s' to portscan preprocessor.\n", 
            file_name, file_line, option);
}

static void ParseProtos(int *protos, char **savptr)
{
    char *pcTok;

    if(!protos)
        return;

    *protos = 0;

    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    while(pcTok)
    {
        if(!strcasecmp(pcTok, "tcp"))
            *protos |= PS_PROTO_TCP;
        else if(!strcasecmp(pcTok, "udp"))
            *protos |= PS_PROTO_UDP;
        else if(!strcasecmp(pcTok, "icmp"))
            *protos |= PS_PROTO_ICMP;
        else if(!strcasecmp(pcTok, "ip"))
            *protos |= PS_PROTO_IP;
        else if(!strcasecmp(pcTok, "all"))
            *protos = PS_PROTO_ALL;
        else if(!strcasecmp(pcTok, TOKEN_ARG_END))
            return;
        else
            FatalErrorInvalidArg("proto");

        pcTok = strtok_r(NULL, DELIMITERS, savptr);
    }

    if(!pcTok)
        FatalErrorNoEnd("proto");

    return;
}

static void ParseScanType(int *scan_types, char **savptr)
{
    char *pcTok;
    
    if(!scan_types)
        return;

    *scan_types = 0;

    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    while(pcTok)
    {
        if(!strcasecmp(pcTok, "portscan"))
            *scan_types |= PS_TYPE_PORTSCAN;
        else if(!strcasecmp(pcTok, "portsweep"))
            *scan_types |= PS_TYPE_PORTSWEEP;
        else if(!strcasecmp(pcTok, "decoy_portscan"))
            *scan_types |= PS_TYPE_DECOYSCAN;
        else if(!strcasecmp(pcTok, "distributed_portscan"))
            *scan_types |= PS_TYPE_DISTPORTSCAN;
        else if(!strcasecmp(pcTok, "all"))
            *scan_types = PS_TYPE_ALL;
        else if(!strcasecmp(pcTok, TOKEN_ARG_END))
            return;
        else
            FatalErrorInvalidArg("scan_type");

        pcTok = strtok_r(NULL, DELIMITERS, savptr);
    }

    if(!pcTok)
        FatalErrorNoEnd("scan_type");

    return;
}

static void ParseSenseLevel(int *sense_level, char **savptr)
{
    char *pcTok;
    
    if(!sense_level)
        return;

    *sense_level = 0;

    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    while(pcTok)
    {
        if(!strcasecmp(pcTok, "low"))
            *sense_level = PS_SENSE_LOW;
        else if(!strcasecmp(pcTok, "medium"))
            *sense_level = PS_SENSE_MEDIUM;
        else if(!strcasecmp(pcTok, "high"))
            *sense_level = PS_SENSE_HIGH;
        else if(!strcmp(pcTok, TOKEN_ARG_END))
            return;
        else
            FatalErrorInvalidArg("sense_level");

        pcTok = strtok_r(NULL, DELIMITERS, savptr);
    }

    if(!pcTok)
        FatalErrorNoEnd("sense_level");

    return;
}

static void ParseIpList(IPSET **ip_list, char *option, char **savptr)
{
    char *pcTok;

    if(!ip_list)
        return;

    pcTok = strtok_r(NULL, TOKEN_ARG_END, savptr);
    if(!pcTok)
        FatalErrorInvalidArg(option);

#ifdef SUP_IP6
    *ip_list = ipset_new();
#else
    *ip_list = ipset_new(IPV4_FAMILY);
#endif
    if(!*ip_list)
        FatalError("Failed to initialize ip_list in portscan preprocessor.\n");

    if(ipset_parse(*ip_list, pcTok))
        FatalError("%s(%d) => Invalid ip_list to '%s' option.\n",
                file_name, file_line, option);

    return;
}

static void ParseMemcap(int *memcap, char **savptr)
{
    char *pcTok;

    if(!memcap)
        return;
    
    *memcap = 0;
    
    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    if(!pcTok)
        FatalErrorNoEnd("memcap");

    *memcap = atoi(pcTok);

    if(*memcap <= 0)
        FatalErrorInvalidArg("memcap");

    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    if(!pcTok)
        FatalErrorNoEnd("memcap");

    if(strcmp(pcTok, TOKEN_ARG_END))
        FatalErrorInvalidArg("memcap");
    
    return;
}

#ifdef SUP_IP6
static void PrintIPPortSet(IP_PORT *p)
{
    char ip_str[80], output_str[80];
    PORTRANGE *pr;

    SnortSnprintf(ip_str, sizeof(ip_str), "%s", sfip_to_str(&p->ip));

    if(p->notflag)
        SnortSnprintf(output_str, sizeof(output_str), "        !%s", ip_str);
    else
        SnortSnprintf(output_str, sizeof(output_str), "        %s", ip_str);

    if (((p->ip.family == AF_INET6) && (p->ip.bits != 128)) ||
        ((p->ip.family == AF_INET ) && (p->ip.bits != 32 )))
        SnortSnprintfAppend(output_str, sizeof(output_str), "/%d", p->ip.bits);

    pr=(PORTRANGE*)sflist_first(&p->portset.port_list);
    if ( pr && pr->port_lo != 0 )
        SnortSnprintfAppend(output_str, sizeof(output_str), " : ");
    for( ; pr != 0;
        pr=(PORTRANGE*)sflist_next(&p->portset.port_list) )
    {
        if ( pr->port_lo != 0)
        {
            SnortSnprintfAppend(output_str, sizeof(output_str), "%d", pr->port_lo);
            if ( pr->port_hi != pr->port_lo )
            {
                SnortSnprintfAppend(output_str, sizeof(output_str), "-%d", pr->port_hi);
            }
            SnortSnprintfAppend(output_str, sizeof(output_str), " ");
        }
    }
    LogMessage("%s\n", output_str);
}
#else
static void PrintCIDRBLOCK(CIDRBLOCK *p)
{
    char ip_str[80], mask_str[80], output_str[80];
    PORTRANGE *pr;

    output_str[0] = '\0';
    ip4_sprintx(ip_str, sizeof(ip_str), &p->ip);
    ip4_sprintx(mask_str, sizeof(mask_str), &p->mask);

    if(p->notflag)
    {
        SnortSnprintfAppend(output_str, sizeof(output_str),
            "        !%s / %s", ip_str, mask_str);
    }
    else
    {
        SnortSnprintfAppend(output_str, sizeof(output_str),
            "        %s / %s", ip_str, mask_str);
    }

    pr=(PORTRANGE*)sflist_first(&p->portset.port_list);
    if ( pr && pr->port_lo != 0 )
        SnortSnprintfAppend(output_str, sizeof(output_str), " : ");
    for( ; pr != 0;
        pr=(PORTRANGE*)sflist_next(&p->portset.port_list) )
    {
        if ( pr->port_lo != 0 )
        {
            SnortSnprintfAppend(output_str, sizeof(output_str), "%d", pr->port_lo);
            if ( pr->port_hi != pr->port_lo )
            {
                SnortSnprintfAppend(output_str, sizeof(output_str), "-%d", pr->port_hi);
            }   
            SnortSnprintfAppend(output_str, sizeof(output_str), " ");
        }
    }
    LogMessage("%s\n", output_str);
}
#endif

static void PrintPortscanConf(int detect_scans, int detect_scan_type,
        int sense_level, IPSET *scanner, IPSET *scanned, IPSET *watch,
        int memcap, char *logpath)
{
    char buf[STD_BUF + 1];
    int proto_cnt = 0;
#ifdef SUP_IP6
    IP_PORT *p;
#else
    CIDRBLOCK *p;
#endif

    LogMessage("Portscan Detection Config:\n");
    
    memset(buf, 0, STD_BUF + 1);
    SnortSnprintf(buf, STD_BUF + 1, "    Detect Protocols:  ");
    if(detect_scans & PS_PROTO_TCP)  { sfsnprintfappend(buf, STD_BUF, "TCP ");  proto_cnt++; }
    if(detect_scans & PS_PROTO_UDP)  { sfsnprintfappend(buf, STD_BUF, "UDP ");  proto_cnt++; }
    if(detect_scans & PS_PROTO_ICMP) { sfsnprintfappend(buf, STD_BUF, "ICMP "); proto_cnt++; }
    if(detect_scans & PS_PROTO_IP)   { sfsnprintfappend(buf, STD_BUF, "IP");    proto_cnt++; }
    LogMessage("%s\n", buf);

    memset(buf, 0, STD_BUF + 1);
    SnortSnprintf(buf, STD_BUF + 1, "    Detect Scan Type:  ");
    if(detect_scan_type & PS_TYPE_PORTSCAN)
        sfsnprintfappend(buf, STD_BUF, "portscan ");
    if(detect_scan_type & PS_TYPE_PORTSWEEP)
        sfsnprintfappend(buf, STD_BUF, "portsweep ");
    if(detect_scan_type & PS_TYPE_DECOYSCAN)
        sfsnprintfappend(buf, STD_BUF, "decoy_portscan ");
    if(detect_scan_type & PS_TYPE_DISTPORTSCAN)
        sfsnprintfappend(buf, STD_BUF, "distributed_portscan");
    LogMessage("%s\n", buf);

    memset(buf, 0, STD_BUF + 1);
    SnortSnprintf(buf, STD_BUF + 1, "    Sensitivity Level: ");
    if(sense_level == PS_SENSE_HIGH)
        sfsnprintfappend(buf, STD_BUF, "High/Experimental");
    if(sense_level == PS_SENSE_MEDIUM)
        sfsnprintfappend(buf, STD_BUF, "Medium");
    if(sense_level == PS_SENSE_LOW)
        sfsnprintfappend(buf, STD_BUF, "Low");
    LogMessage("%s\n", buf);

    LogMessage("    Memcap (in bytes): %d\n", memcap);
    LogMessage("    Number of Nodes:   %d\n",
            memcap / (sizeof(PS_PROTO)*proto_cnt-1));

    if (logpath != NULL)
        LogMessage("    Logfile:           %s\n", logpath); 

    if(scanner)
    {
        LogMessage("    Ignore Scanner IP List:\n");
#ifdef SUP_IP6
        for(p = (IP_PORT*)sflist_first(&scanner->ip_list);
            p;
            p = (IP_PORT*)sflist_next(&scanner->ip_list))
        {
            PrintIPPortSet(p);
        }
#else
        for(p = (CIDRBLOCK*)sflist_first(&scanner->cidr_list);
            p;
            p = (CIDRBLOCK*)sflist_next(&scanner->cidr_list))
        {
            PrintCIDRBLOCK(p);
        }
#endif
    }

    if(scanned)
    {
        LogMessage("    Ignore Scanned IP List:\n");
#ifdef SUP_IP6
        for(p = (IP_PORT*)sflist_first(&scanned->ip_list);
            p;
            p = (IP_PORT*)sflist_next(&scanned->ip_list))
        {
            PrintIPPortSet(p);
        }
#else
        for(p = (CIDRBLOCK*)sflist_first(&scanned->cidr_list);
            p;
            p = (CIDRBLOCK*)sflist_next(&scanned->cidr_list))
        {
            PrintCIDRBLOCK(p);
        }
#endif
    }

    if(watch)
    {
        LogMessage("    Watch IP List:\n");
#ifdef SUP_IP6
        for(p = (IP_PORT*)sflist_first(&watch->ip_list);
            p;
            p = (IP_PORT*)sflist_next(&watch->ip_list))
        {
            PrintIPPortSet(p);
        }
#else
        for(p = (CIDRBLOCK*)sflist_first(&watch->cidr_list);
            p;
            p = (CIDRBLOCK*)sflist_next(&watch->cidr_list))
        {
            PrintCIDRBLOCK(p);
        }
#endif
    }
}

static void ParseLogFile(PortscanConfig *config, char **savptr)
{
    char *pcTok;

    if (config == NULL)
        return;

    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    if (pcTok == NULL)
    {
        FatalError("%s(%d) => No ending brace to '%s' config option.\n", 
                   file_name, file_line, "logfile");
    }

    config->logfile = ProcessFileOption(snort_conf_for_parsing, pcTok);

    pcTok = strtok_r(NULL, DELIMITERS, savptr);
    if (pcTok == NULL)
    {
        FatalError("%s(%d) => No ending brace to '%s' config option.\n", 
                   file_name, file_line, "logfile");
    }

    if (strcmp(pcTok, TOKEN_ARG_END) != 0)
    {
        FatalError("%s(%d) => Invalid argument to '%s' config option.\n", 
                   file_name, file_line, "logfile");
    }
}
    
static void PortscanInit(char *args)
{
    tSfPolicyId policy_id = getParserPolicy();
    PortscanConfig *pPolicyConfig = NULL; 

    if (portscan_config == NULL)
    {
        portscan_config = sfPolicyConfigCreate();
        PortscanPacketInit();

        AddFuncToPreprocCleanExitList(PortscanCleanExitFunction, NULL, PRIORITY_SCANNER, PP_SFPORTSCAN);
        AddFuncToPreprocRestartList(PortscanRestartFunction, NULL, PRIORITY_SCANNER, PP_SFPORTSCAN);    
        AddFuncToPreprocResetList(PortscanResetFunction, NULL, PRIORITY_SCANNER, PP_SFPORTSCAN);    
        AddFuncToPreprocResetStatsList(PortscanResetStatsFunction, NULL, PRIORITY_SCANNER, PP_SFPORTSCAN);    
        AddFuncToPreprocPostConfigList(PortscanOpenLogFile, NULL);

#ifdef PERF_PROFILING
        RegisterPreprocessorProfile("sfportscan", &sfpsPerfStats, 0, &totalPerfStats);
#endif
    }

    if ((policy_id != 0) && (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config)) == NULL))
    {
        ParseError("Portscan: Must configure default policy if other "
                   "policies are going to be configured.");
    }

    sfPolicyUserPolicySet (portscan_config, policy_id);
    pPolicyConfig = (PortscanConfig *)sfPolicyUserDataGetCurrent(portscan_config);
    if (pPolicyConfig)
    {
        ParseError("Can only configure sfportscan once.\n");
    }
    pPolicyConfig = (PortscanConfig *)SnortAlloc(sizeof(PortscanConfig));
    if (!pPolicyConfig)
    {
        ParseError("SFPORTSCAN preprocessor: memory allocate failed.\n");
    }

    sfPolicyUserDataSetCurrent(portscan_config, pPolicyConfig);
    ParsePortscan(pPolicyConfig, args);

    if (policy_id == 0)
    {
        ps_init_hash(pPolicyConfig->memcap);
    }
    else
    {
        pPolicyConfig->memcap = ((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config))->memcap;

        if (pPolicyConfig->logfile != NULL)
        {
            ParseError("Portscan:  logfile can only be configured in "
                       "default policy.\n");
        }
    }

    AddFuncToPreprocList(PortscanDetect, PRIORITY_SCANNER, PP_SFPORTSCAN,
                         PortscanGetProtoBits(pPolicyConfig->detect_scans));
}

void SetupSfPortscan(void)
{
#ifndef SNORT_RELOAD
    RegisterPreprocessor("sfportscan", PortscanInit);
#else
    RegisterPreprocessor("sfportscan", PortscanInit, PortscanReload,
                         PortscanReloadSwap, PortscanReloadSwapFree);
#endif
}

static void ParsePortscan(PortscanConfig *config, char *args)
{
    int    sense_level = PS_SENSE_LOW;
    int    protos      = (PS_PROTO_TCP | PS_PROTO_UDP);
    int    scan_types  = PS_TYPE_ALL;
    int    memcap      = 1048576;
    IPSET *ignore_scanners = NULL;
    IPSET *ignore_scanned = NULL;
    IPSET *watch_ip = NULL;
    char  *pcTok, *savpcTok = NULL;
    int    iRet;

    if (args != NULL)
    {
        pcTok = strtok_r(args, DELIMITERS, &savpcTok);
        //pcTok = strtok(args, DELIMITERS);
        while(pcTok)
        {
            if(!strcasecmp(pcTok, "proto"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                //pcTok = strtok(NULL, DELIMITERS);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"proto");

                ParseProtos(&protos, &savpcTok);
            }
            else if(!strcasecmp(pcTok, "scan_type"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"scan_type");

                ParseScanType(&scan_types, &savpcTok);
            }
            else if(!strcasecmp(pcTok, "sense_level"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"sense_level");

                ParseSenseLevel(&sense_level, &savpcTok);
            }
            else if(!strcasecmp(pcTok, "ignore_scanners"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"ignore_scanners");

                ParseIpList(&ignore_scanners, "ignore_scanners", &savpcTok);
            }
            else if(!strcasecmp(pcTok, "ignore_scanned"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"ignore_scanned");

                ParseIpList(&ignore_scanned, "ignore_scanned", &savpcTok);
            }
            else if(!strcasecmp(pcTok, "watch_ip"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"watch_ip");

                ParseIpList(&watch_ip, "watch_ip", &savpcTok);
            }
            else if(!strcasecmp(pcTok, "print_tracker"))
            {
                config->print_tracker = 1;
            }
            else if(!strcasecmp(pcTok, "memcap"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"memcap");

                ParseMemcap(&memcap, &savpcTok);
            }
            else if(!strcasecmp(pcTok, "logfile"))
            {
                pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
                if(!pcTok || strcmp(pcTok, TOKEN_ARG_BEGIN))
                    FatalErrorNoOption((u_char *)"logfile");

                ParseLogFile(config, &savpcTok);
            }
            else if(!strcasecmp(pcTok, "include_midstream"))
            {
                /* Do not ignore packets in sessions picked up mid-stream */
                config->include_midstream = 1;
            }
            else if(!strcasecmp(pcTok, "detect_ack_scans"))
            {
                /* 
                 *  We will only see ack scan packets if we are looking at sessions that the
                 *    have been flagged as being picked up mid-stream
                 */
                config->include_midstream = 1;
            }
            else
            {
                FatalErrorInvalidOption(pcTok);
            }

            pcTok = strtok_r(NULL, DELIMITERS, &savpcTok);
        }
    }

    iRet = ps_init(config, protos, scan_types, sense_level, ignore_scanners,
                   ignore_scanned, watch_ip, memcap);
    if (iRet)
    {
        if(iRet == -2)
        {
            FatalError("%s(%d) => 'memcap' limit not sufficient to run "
                       "sfportscan preprocessor.  Please increase this "
                       "value or keep the default memory usage.\n", 
                       file_name, file_line);
        }

        FatalError("Failed to initialize the sfportscan detection module.  "
                   "Please check your configuration before submitting a "
                   "bug.\n");
    }

    PrintPortscanConf(protos, scan_types, sense_level, ignore_scanners,
                      ignore_scanned, watch_ip, memcap, config->logfile);
}

static void PortscanOpenLogFile(void *data)
{
    PortscanConfig *pPolicyConfig = (PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config) ;
    if ((  pPolicyConfig== NULL) ||
        (pPolicyConfig->logfile == NULL))
    {
        return;
    }

    g_logfile = fopen(pPolicyConfig->logfile, "a+");
    if (g_logfile == NULL)
    {
        FatalError("Portscan log file '%s' could not be opened: %s.\n", 
                   pPolicyConfig->logfile, strerror(errno));
    }
}

static int PortscanFreeConfigPolicy(tSfPolicyUserContextId config,tSfPolicyId policyId, void* pData )
{
    PortscanConfig *pPolicyConfig = (PortscanConfig *)pData;
    sfPolicyUserDataClear (config, policyId);
    PortscanFreeConfig(pPolicyConfig);
    return 0;
}
static void PortscanFreeConfigs(tSfPolicyUserContextId config)
{

    if (config == NULL)
        return;

    sfPolicyUserDataIterate (config, PortscanFreeConfigPolicy);
    sfPolicyConfigDelete(config);

}

static void PortscanFreeConfig(PortscanConfig *config)
{
    if (config == NULL)
        return;

    if (config->logfile)
        free(config->logfile);

    if (config->ignore_scanners != NULL)
        ipset_free(config->ignore_scanners);

    if (config->ignore_scanned != NULL)
        ipset_free(config->ignore_scanned);

    if (config->watch_ip != NULL)
        ipset_free(config->watch_ip);

    free(config);
}

static int PortscanGetProtoBits(int detect_scans)
{
    int proto_bits = 0;

    if (detect_scans & PS_PROTO_IP)
        proto_bits |= PROTO_BIT__IP;

    if (detect_scans & PS_PROTO_UDP)
        proto_bits |= PROTO_BIT__UDP;

    if (detect_scans & PS_PROTO_ICMP)
        proto_bits |= PROTO_BIT__ICMP;

    if (detect_scans & PS_PROTO_TCP)
        proto_bits |= PROTO_BIT__TCP;

    return proto_bits;
}

#ifdef SNORT_RELOAD
static void PortscanReload(char *args)
{
    tSfPolicyId policy_id = getParserPolicy();
    PortscanConfig *pPolicyConfig = NULL;

    if (portscan_swap_config == NULL)
    {
         portscan_swap_config = sfPolicyConfigCreate();
    }

    if ((policy_id != 0) && (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config)) == NULL))
    {
        ParseError("Portscan: Must configure default policy if other "
                   "policies are going to be configured.");
    }

       
    sfPolicyUserPolicySet (portscan_swap_config, policy_id);

    pPolicyConfig = (PortscanConfig *)sfPolicyUserDataGetCurrent(portscan_swap_config);
    if (pPolicyConfig)
    { 
	ParseError("Can only configure sfportscan once.\n");
    }

    pPolicyConfig = (PortscanConfig *)SnortAlloc(sizeof(PortscanConfig));
    if (!pPolicyConfig)
    {
        ParseError("SFPORTSCAN preprocessor: memory allocate failed.\n");
    }
 

    sfPolicyUserDataSetCurrent(portscan_swap_config, pPolicyConfig);
    ParsePortscan(pPolicyConfig, args);

    if (policy_id != 0)
    {
        pPolicyConfig->memcap = ((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config))->memcap;

        if (pPolicyConfig->logfile != NULL)
        {
            ParseError("Portscan:  logfile can only be configured in "
                       "default policy.\n");
        }
    }

    AddFuncToPreprocList(PortscanDetect, PRIORITY_SCANNER, PP_SFPORTSCAN,
                         PortscanGetProtoBits(pPolicyConfig->detect_scans));

    AddFuncToPreprocReloadVerifyList(PortscanReloadVerify);
}

static int PortscanReloadVerify(void)
{
    if ((portscan_swap_config == NULL) || (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config)) == NULL) ||
        (portscan_config == NULL) || (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config)) == NULL))
    {
        return 0;
    }

    if (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config))->memcap != ((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config))->memcap)
    {
        PortscanFreeConfigs(portscan_swap_config);
        portscan_swap_config = NULL;
        return -1;
    }

    if ((((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config))->logfile != NULL) &&
        (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config))->logfile != NULL))
    {
        if (strcasecmp(((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config))->logfile,
                       ((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config))->logfile) != 0)
        {
            PortscanFreeConfigs(portscan_swap_config);
            portscan_swap_config = NULL;
            return -1;
        }
    }
    else if (((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_swap_config))->logfile != ((PortscanConfig *)sfPolicyUserDataGetDefault(portscan_config))->logfile)
    {
        PortscanFreeConfigs(portscan_swap_config);
        portscan_swap_config = NULL;
        return -1;
    }

    return 0;
}

static void * PortscanReloadSwap(void)
{
    tSfPolicyUserContextId old_config = portscan_config;

    if (portscan_swap_config == NULL)
        return NULL;

    portscan_config = portscan_swap_config;
    portscan_swap_config = NULL;

    return (void *)old_config;
}

static void PortscanReloadSwapFree(void *data)
{
    if (data == NULL)
        return;

    PortscanFreeConfigs((tSfPolicyUserContextId)data);
}
#endif

