/* $Id: session.c 3553 2011-05-05 06:14:19Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <pjmedia/session.h>
#include <pjmedia/errno.h>
/*ycw-pjsip*/
#include <pjmedia/transport_udp.h>
/*ycw-pjsip. t38*/
#include <pjmedia-codec/types.h>
#include <pj/log.h>
#include <pj/os.h> 
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/assert.h>
#include <pj/ctype.h>
#include <pj/rand.h>
/*ycw-pjsip*/
#include "cmsip_transport.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*ycw-usbvm @ 2012-04-04*/
#include "dtmfrcv_dtmfReceiver.h"

/* Remove to session.h, by sirrain zhang */
#if 0
struct pjmedia_session
{
    pj_pool_t		   *pool;
    pjmedia_endpt	   *endpt;
    unsigned		    stream_cnt;
    pjmedia_stream_info	    stream_info[PJMEDIA_MAX_SDP_MEDIA];
    pjmedia_stream	   *stream[PJMEDIA_MAX_SDP_MEDIA];
    void		   *user_data;
	 pj_bool_t isLocalTransit;
};
#endif

#define THIS_FILE		"session.c"

#ifndef PJMEDIA_SESSION_SIZE
#   define PJMEDIA_SESSION_SIZE	(10*1024)
#endif

#ifndef PJMEDIA_SESSION_INC
#   define PJMEDIA_SESSION_INC	1024
#endif

/*ycw-pjsip*/
static const pj_str_t ID_T38 = {"t38", 3};
static const pj_str_t ID_IMAGE = {"image", 5};
static const pj_str_t ID_AUDIO = { "audio", 5};
static const pj_str_t ID_VIDEO = { "video", 5};
static const pj_str_t ID_APPLICATION = { "application", 11};
static const pj_str_t ID_IN = { "IN", 2 };
static const pj_str_t ID_IP4 = { "IP4", 3};
static const pj_str_t ID_IP6 = { "IP6", 3};
static const pj_str_t ID_UDPTL = { "udptl", 5 };
static const pj_str_t ID_RTP_AVP = { "RTP/AVP", 7 };
static const pj_str_t ID_RTP_SAVP = { "RTP/SAVP", 8 };
//static const pj_str_t ID_SDP_NAME = { "pjmedia", 7 };
static const pj_str_t ID_RTPMAP = { "rtpmap", 6 };
static const pj_str_t ID_TELEPHONE_EVENT = { "telephone-event", 15 };
/*ycw-pjsip. t38*/
static const pj_str_t ID_VOICE_RED = { "red", 3 };

static const pj_str_t STR_INACTIVE = { "inactive", 8 };
static const pj_str_t STR_SENDRECV = { "sendrecv", 8 };
static const pj_str_t STR_SENDONLY = { "sendonly", 8 };
static const pj_str_t STR_RECVONLY = { "recvonly", 8 };


/*
 * Parse fmtp for specified format/payload type.
 */
static void parse_fmtp( pj_pool_t *pool,
			const pjmedia_sdp_media *m,
			unsigned pt,
			pjmedia_codec_fmtp *fmtp)
{
    const pjmedia_sdp_attr *attr;
    pjmedia_sdp_fmtp sdp_fmtp;
    char *p, *p_end, fmt_buf[8];
    pj_str_t fmt;

    pj_assert(m && fmtp);

    pj_bzero(fmtp, sizeof(pjmedia_codec_fmtp));

    /* Get "fmtp" attribute for the format */
    pj_ansi_sprintf(fmt_buf, "%d", pt);
    fmt = pj_str(fmt_buf);
    attr = pjmedia_sdp_media_find_attr2(m, "fmtp", &fmt);
    if (attr == NULL)
	return;

    /* Parse "fmtp" attribute */
    if (pjmedia_sdp_attr_get_fmtp(attr, &sdp_fmtp) != PJ_SUCCESS)
	return;

    /* Prepare parsing */
    p = sdp_fmtp.fmt_param.ptr;
    p_end = p + sdp_fmtp.fmt_param.slen;

    /* Parse */
    while (p < p_end) {
	char *token, *start, *end;

	/* Skip whitespaces */
	while (p < p_end && (*p == ' ' || *p == '\t')) ++p;
	if (p == p_end)
	    break;

	/* Get token */
	start = p;
	while (p < p_end && *p != ';' && *p != '=') ++p;
	end = p - 1;

	/* Right trim */
	while (end >= start && (*end == ' '  || *end == '\t' || 
				*end == '\r' || *end == '\n' ))
	    --end;

	/* Forward a char after trimming */
	++end;

	/* Store token */
	if (end > start) {
	    token = (char*)pj_pool_alloc(pool, end - start);
	    pj_ansi_strncpy(token, start, end - start);
	    if (*p == '=')
		/* Got param name */
		pj_strset(&fmtp->param[fmtp->cnt].name, token, end - start);
	    else
		/* Got param value */
		pj_strset(&fmtp->param[fmtp->cnt++].val, token, end - start);
	} else if (*p != '=') {
	    ++fmtp->cnt;
	}

	/* Next */
	++p;
    }
}


/*
 * Create stream info from SDP media line.
 */
pj_status_t pjmedia_stream_info_from_sdp(
					   pjmedia_stream_info *si,
					   pj_pool_t *pool,
					   pjmedia_endpt *endpt,
					   const pjmedia_sdp_session *local,
					   const pjmedia_sdp_session *remote,
					   unsigned stream_idx)
{
    pjmedia_codec_mgr *mgr;
    const pjmedia_sdp_attr *attr;
    const pjmedia_sdp_media *local_m;
    const pjmedia_sdp_media *rem_m;
    const pjmedia_sdp_conn *local_conn;
    const pjmedia_sdp_conn *rem_conn;
    int rem_af, local_af;
    pj_sockaddr local_addr;
    pjmedia_sdp_rtpmap *rtpmap;
    unsigned i, pt, fmti;
    pj_status_t status;

    
    /* Validate arguments: */
    PJ_ASSERT_RETURN(pool && si && local && remote, PJ_EINVAL);
    PJ_ASSERT_RETURN(stream_idx < local->media_count, PJ_EINVAL);
    PJ_ASSERT_RETURN(stream_idx < remote->media_count, PJ_EINVAL);


    /* Get codec manager. */
    mgr = pjmedia_endpt_get_codec_mgr(endpt);

    /* Keep SDP shortcuts */
    local_m = local->media[stream_idx];
    rem_m = remote->media[stream_idx];

    local_conn = local_m->conn ? local_m->conn : local->conn;
    if (local_conn == NULL)
    {
		return PJMEDIA_SDP_EMISSINGCONN;
    }

    rem_conn = rem_m->conn ? rem_m->conn : remote->conn;
    if (rem_conn == NULL)
    {
		return PJMEDIA_SDP_EMISSINGCONN;
    }


    /* Reset: */

    pj_bzero(si, sizeof(*si));

#if PJMEDIA_HAS_RTCP_XR && PJMEDIA_STREAM_ENABLE_XR
    /* Set default RTCP XR enabled/disabled */
    si->rtcp_xr_enabled = PJ_TRUE;
#endif

    /* Media type: */

    if (pj_stricmp(&local_m->desc.media, &ID_AUDIO) == 0)
	 {	 	
		si->type = PJMEDIA_TYPE_AUDIO;
    }
	 else if (pj_stricmp(&local_m->desc.media, &ID_VIDEO) == 0)
	 {
		si->type = PJMEDIA_TYPE_VIDEO;
    }
	 /*ycw-pjsip. t38*/
	 else if (pj_stricmp(&local_m->desc.media, &ID_IMAGE) == 0)
	 {	 	
	 	si->type = PJMEDIA_TYPE_IMAGE;
	 }
	 else
	 {
		si->type = PJMEDIA_TYPE_UNKNOWN;

		/* Avoid rejecting call because of unrecognized media, 
		 * just return PJ_SUCCESS, this media will be deactivated later.
		 */
		//return PJMEDIA_EINVALIMEDIATYPE;
		return PJ_SUCCESS;
    }

    /* Transport protocol */

    /* At this point, transport type must be compatible, 
     * the transport instance will do more validation later.
     */
    status = pjmedia_sdp_transport_cmp(&rem_m->desc.transport, &local_m->desc.transport);
    if (status != PJ_SUCCESS)
    {
		return PJMEDIA_SDPNEG_EINVANSTP;
    }

    if (pj_stricmp(&local_m->desc.transport, &ID_RTP_AVP) == 0)
	 {
		si->proto = PJMEDIA_TP_PROTO_RTP_AVP;
    }
	 else if (pj_stricmp(&local_m->desc.transport, &ID_RTP_SAVP) == 0)
	 {
		si->proto = PJMEDIA_TP_PROTO_RTP_SAVP;
    }
	 /*ycw-pjsip. t38*/
	 else if (pj_stricmp(&local_m->desc.transport, &ID_UDPTL) == 0)
	 {
	 	si->proto = PJMEDIA_TP_PROTO_UDPTL;
	 }
	 else
	 {
		si->proto = PJMEDIA_TP_PROTO_UNKNOWN;
		return PJ_SUCCESS;
    }

    /* Check address family in remote SDP */
    rem_af = pj_AF_UNSPEC();
    if (pj_stricmp(&rem_conn->net_type, &ID_IN)==0)
	 {
		if (pj_stricmp(&rem_conn->addr_type, &ID_IP4)==0)
		{
	   	rem_af = pj_AF_INET();
		}
		else if (pj_stricmp(&rem_conn->addr_type, &ID_IP6)==0)
		{
	   	rem_af = pj_AF_INET6();
		}
    }

    if (rem_af==pj_AF_UNSPEC())
	 {
		/* Unsupported address family */
		return PJ_EAFNOTSUP;
    }

    /* Set remote address: */
    status = pj_sockaddr_init(rem_af, &si->rem_addr, &rem_conn->addr, rem_m->desc.port);
    if (status != PJ_SUCCESS)
	 {
		/* Invalid IP address. */
		return PJMEDIA_EINVALIDIP;
    }

    /* Check address family of local info */
    local_af = pj_AF_UNSPEC();
    if (pj_stricmp(&local_conn->net_type, &ID_IN)==0)
	 {
		if (pj_stricmp(&local_conn->addr_type, &ID_IP4)==0)
		{
	   	local_af = pj_AF_INET();
		}
		else if (pj_stricmp(&local_conn->addr_type, &ID_IP6)==0)
		{
	   	local_af = pj_AF_INET6();
		}
    }

    if (local_af==pj_AF_UNSPEC())
	 {
		/* Unsupported address family */
		return PJ_SUCCESS;
    }

    /* Set remote address: */
    status = pj_sockaddr_init(local_af, &local_addr, &local_conn->addr, 
			      local_m->desc.port);
    if (status != PJ_SUCCESS)
	 {
		/* Invalid IP address. */
		return PJMEDIA_EINVALIDIP;
    }

    /* Local and remote address family must match */
    if (local_af != rem_af)
    {
		return PJ_EAFNOTSUP;
    }

    /* Media direction: */

    if (local_m->desc.port == 0 || 
	pj_sockaddr_has_addr(&local_addr)==PJ_FALSE ||
	pj_sockaddr_has_addr(&si->rem_addr)==PJ_FALSE ||
	pjmedia_sdp_media_find_attr(local_m, &STR_INACTIVE, NULL)!=NULL)
    {
		/* Inactive stream. */
		si->dir = PJMEDIA_DIR_NONE;
    }
	 else if (pjmedia_sdp_media_find_attr(local_m, &STR_SENDONLY, NULL)!=NULL)
	 {
		/* Send only stream. */
		si->dir = PJMEDIA_DIR_ENCODING;
    }
	 else if (pjmedia_sdp_media_find_attr(local_m, &STR_RECVONLY, NULL)!=NULL)
	 {
		/* Recv only stream. */
		si->dir = PJMEDIA_DIR_DECODING;
    }
	 else
	 {
		/* Send and receive stream. */
		si->dir = PJMEDIA_DIR_ENCODING_DECODING;
    }

    /* No need to do anything else if stream is rejected */
    if (local_m->desc.port == 0)
	 {
		return PJ_SUCCESS;
    }

    /* If "rtcp" attribute is present in the SDP, set the RTCP address
     * from that attribute. Otherwise, calculate from RTP address.
     */
    attr = pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr, "rtcp", NULL);
    if (attr)
	 {
		pjmedia_sdp_rtcp_attr rtcp;
		status = pjmedia_sdp_attr_get_rtcp(attr, &rtcp);
		if (status == PJ_SUCCESS)
		{
	   	if (rtcp.addr.slen)
			{
				status = pj_sockaddr_init(rem_af, &si->rem_rtcp, &rtcp.addr, (pj_uint16_t)rtcp.port);
	    	}
			else
			{
				pj_sockaddr_init(rem_af, &si->rem_rtcp, NULL, (pj_uint16_t)rtcp.port);
				pj_memcpy(pj_sockaddr_get_addr(&si->rem_rtcp), pj_sockaddr_get_addr(&si->rem_addr),
			  			pj_sockaddr_get_addr_len(&si->rem_addr));
	    	}
		}
    }
    
    if (!pj_sockaddr_has_addr(&si->rem_rtcp))
	 {
		int rtcp_port;

		pj_memcpy(&si->rem_rtcp, &si->rem_addr, sizeof(pj_sockaddr));
		rtcp_port = pj_sockaddr_get_port(&si->rem_addr) + 1;
		pj_sockaddr_set_port(&si->rem_rtcp, (pj_uint16_t)rtcp_port);
    }


    /* Get the payload number for receive channel. */
    /*
       Previously we used to rely on fmt[0] being the selected codec,
       but some UA sends telephone-event as fmt[0] and this would
       cause assert failure below.

       Thanks Chris Hamilton <chamilton .at. cs.dal.ca> for this patch.

    // And codec must be numeric!
    if (!pj_isdigit(*local_m->desc.fmt[0].ptr) || 
	!pj_isdigit(*rem_m->desc.fmt[0].ptr))
    {
	return PJMEDIA_EINVALIDPT;
    }

    pt = pj_strtoul(&local_m->desc.fmt[0]);
    pj_assert(PJMEDIA_RTP_PT_TELEPHONE_EVENTS==0 ||
	      pt != PJMEDIA_RTP_PT_TELEPHONE_EVENTS);
    */

    /* This is to suppress MSVC warning about uninitialized var */
    pt = 0;

    /* Find the first codec which is not telephone-event */
    for ( fmti = 0; fmti < local_m->desc.fmt_count; ++fmti )
	 {
	 	/*ycw-pjsip. t38*/
	 	if (pj_strcmp(&local_m->desc.fmt[fmti], &ID_T38) == 0)
	 	{
	 		pt = PJMEDIA_RTP_PT_T38;
			break;
	 	}
		
		if ( !pj_isdigit(*local_m->desc.fmt[fmti].ptr) )
	   {
	   	return PJMEDIA_EINVALIDPT;
		}
		
		pt = pj_strtoul(&local_m->desc.fmt[fmti]);
		if ( PJMEDIA_RTP_PT_TELEPHONE_EVENTS == 0 ||
		pt != PJMEDIA_RTP_PT_TELEPHONE_EVENTS )
		{
			break;
		}
    }
    if ( fmti >= local_m->desc.fmt_count )
		return PJMEDIA_EINVALIDPT;


	/*ycw-pjsip. t38. Build T38 codec*/
    if(pt == PJMEDIA_RTP_PT_T38)
	 {
        //const pjmedia_codec_info *p_info;

        /* Build codec format info: */
        si->fmt.type = si->type;
        si->fmt.pt = pt;
        si->fmt.encoding_name = pj_str("t38");
        si->fmt.clock_rate = 8000;
        si->fmt.channel_cnt = 1;
        
        /* symmetric payload for T38*/
        si->tx_pt = pt;

        /* Now that we have codec info, get the codec param. */
        si->param = PJ_POOL_ALLOC_T(pool, pjmedia_codec_param);
        status = pjmedia_codec_mgr_get_default_param(mgr, &si->fmt, si->param);  
        if (status != PJ_SUCCESS)
        {
        	CMSIP_PRINT("current codec manager does not suppport t38.%s", "^^");
        	return status;
        }
		  
	    attr = pjmedia_sdp_media_find_attr2(rem_m, "T38FaxMaxDatagram", NULL);
        if (attr)
		{
            si->param->info.max_datagram = (pj_uint32_t)pj_strtoul(&attr->value);
		  		CMSIP_PRINT("T38FaxMaxDatagram[%d]", si->param->info.max_datagram);

        }
		  else
		  {
		  		si->param->info.max_datagram = 0;
		  		CMSIP_PRINT("default T38FaxMaxDatagram[%d]", si->param->info.max_datagram);
		  }

		  /*ycw-pjsip. t38*/
		  attr = pjmedia_sdp_media_find_attr2(rem_m, "T38MaxBitRate", NULL);
		  if (attr)
		  {
		  		si->param->info.max_bitrate = (pj_uint32_t)pj_strtoul(&attr->value);
				CMSIP_PRINT("T38MaxBitRate[%d]", si->param->info.max_bitrate);
		  }
		  else
		  {
		  		si->param->info.max_bitrate = 0;
				CMSIP_PRINT("default T38MaxBitRate[%d]", si->param->info.max_bitrate);
		  }

		  attr = pjmedia_sdp_media_find_attr2(rem_m, "T38FaxUdpEC", NULL);
		  if (attr)
		  {
		  		si->param->info.enableUdpEc = PJ_TRUE;
		  }
		  else
		  {
		  		si->param->info.enableUdpEc = PJ_FALSE;
		  }
        return PJ_SUCCESS;
    }

    /* Get codec info.
     * For static payload types, get the info from codec manager.
     * For dynamic payload types, MUST get the rtpmap.
     */
    if (pt < 96)
	 {
		pj_bool_t has_rtpmap;

		rtpmap = NULL;
		has_rtpmap = PJ_TRUE;

		attr = pjmedia_sdp_media_find_attr(local_m, &ID_RTPMAP, &local_m->desc.fmt[fmti]);
		if (attr == NULL)
		{
	   	has_rtpmap = PJ_FALSE;
		}
		if (attr != NULL)
		{
	   	status = pjmedia_sdp_attr_to_rtpmap(pool, attr, &rtpmap);
	    	if (status != PJ_SUCCESS)
				has_rtpmap = PJ_FALSE;
		}

		/* Build codec format info: */
		if (has_rtpmap)
		{
		   	si->fmt.type = si->type;
	    	si->fmt.pt = pj_strtoul(&local_m->desc.fmt[fmti]);
	    	pj_strdup(pool, &si->fmt.encoding_name, &rtpmap->enc_name);
	    	si->fmt.clock_rate = rtpmap->clock_rate;
	    
#if defined(PJMEDIA_HANDLE_G722_MPEG_BUG) && (PJMEDIA_HANDLE_G722_MPEG_BUG != 0)
	    	/* The session info should have the actual clock rate, because 
	     	* this info is used for calculationg buffer size, etc in stream 
	     	*/
	    	if (si->fmt.pt == PJMEDIA_RTP_PT_G722)
				si->fmt.clock_rate = 16000;
#endif

	    	/* For audio codecs, rtpmap parameters denotes the number of
	     	* channels.
	     	*/
	    	if (si->type == PJMEDIA_TYPE_AUDIO && rtpmap->param.slen)
			{
				si->fmt.channel_cnt = (unsigned) pj_strtoul(&rtpmap->param);
	    	}
			else
			{
				si->fmt.channel_cnt = 1;
	    	}

		}
		else 
		{
	   	const pjmedia_codec_info *p_info;

	    	status = pjmedia_codec_mgr_get_codec_info( mgr, pt, &p_info);
	    	if (status != PJ_SUCCESS)
				return status;

	    	pj_memcpy(&si->fmt, p_info, sizeof(pjmedia_codec_info));
		}

		/* For static payload type, pt's are symetric */
		si->tx_pt = pt;

    }
	 else 
	 {
		attr = pjmedia_sdp_media_find_attr(local_m, &ID_RTPMAP, &local_m->desc.fmt[fmti]);
		if (attr == NULL)
	   {	
	   	return PJMEDIA_EMISSINGRTPMAP;
		}

		status = pjmedia_sdp_attr_to_rtpmap(pool, attr, &rtpmap);
		if (status != PJ_SUCCESS)
	   	return status;

		/* Build codec format info: */

		si->fmt.type = si->type;
		si->fmt.pt = pj_strtoul(&local_m->desc.fmt[fmti]);
		pj_strdup(pool, &si->fmt.encoding_name, &rtpmap->enc_name);
		si->fmt.clock_rate = rtpmap->clock_rate;

		/* For audio codecs, rtpmap parameters denotes the number of
		 * channels.
		 */
		if (si->type == PJMEDIA_TYPE_AUDIO && rtpmap->param.slen)
		{
	   	si->fmt.channel_cnt = (unsigned) pj_strtoul(&rtpmap->param);
		}
		else
		{
	   	si->fmt.channel_cnt = 1;
		}

		/* Determine payload type for outgoing channel, by finding
		 * dynamic payload type in remote SDP that matches the answer.
		 */
		si->tx_pt = 0xFFFF;
		for (i=0; i<rem_m->desc.fmt_count; ++i)
		{
	   	unsigned rpt;
		    pjmedia_sdp_attr *r_attr;
		    pjmedia_sdp_rtpmap r_rtpmap;

		    rpt = pj_strtoul(&rem_m->desc.fmt[i]);
		    if (rpt < 96)
				continue;

		    r_attr = pjmedia_sdp_media_find_attr(rem_m, &ID_RTPMAP,
							 &rem_m->desc.fmt[i]);
		    if (!r_attr)
				continue;

	    	 if (pjmedia_sdp_attr_get_rtpmap(r_attr, &r_rtpmap) != PJ_SUCCESS)
				continue;

		    if (!pj_stricmp(&rtpmap->enc_name, &r_rtpmap.enc_name) &&
			rtpmap->clock_rate == r_rtpmap.clock_rate)
		    {
				/* Found matched codec. */
				si->tx_pt = rpt;

				break;
	    	}

		}

		if (si->tx_pt == 0xFFFF)
		{
		    return PJMEDIA_EMISSINGRTPMAP;
    }
    }
  
	    /* Now that we have codec info, get the codec param. */
	    si->param = PJ_POOL_ALLOC_T(pool, pjmedia_codec_param);
	    status = pjmedia_codec_mgr_get_default_param(mgr, &si->fmt, si->param);

	    /* Get remote fmtp for our encoder. */
	    parse_fmtp(pool, rem_m, si->tx_pt, &si->param->setting.enc_fmtp);

	    /* Get local fmtp for our decoder. */
	    parse_fmtp(pool, local_m, si->fmt.pt, &si->param->setting.dec_fmtp);

	    /* Get the remote ptime for our encoder. */
	    attr = pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr,
					  "ptime", NULL);
    	if (attr)
		{

			pj_str_t tmp_val = attr->value;
			/*ycw-pjsip-ptime*/
			#if 0
			unsigned frm_per_pkt;
			#endif
 
			pj_strltrim(&tmp_val);

			/*ycw-pjsip-ptime*/
			#if 0
			/* Round up ptime when the specified is not multiple of frm_ptime */
			frm_per_pkt = (pj_strtoul(&tmp_val) + si->param->info.frm_ptime/2) /
				      si->param->info.frm_ptime;
			if (frm_per_pkt != 0)
			{
		       si->param->setting.frm_per_pkt = (pj_uint8_t)frm_per_pkt;
		   }
			#endif

			/*ycw-pjsip*/
			si->ulptime = pj_strtoul(&tmp_val);
    	}
		else
		{
			si->ulptime = 20;/*default is 20ms*/
		}

		/*Get the local ptime for our decoder*/
		attr = pjmedia_sdp_attr_find2(local_m->attr_count, local_m->attr, "ptime", NULL);
		if (attr)
		{
			pj_str_t tmp_val = attr->value;
			pj_strltrim(&tmp_val);
			si->dlptime = pj_strtoul(&tmp_val);
		}
		else
		{
			si->dlptime = 20;/*default is 20ms*/
    	}

    /* Get remote maxptime for our encoder. */
    attr = pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr,
				  "maxptime", NULL);
    if (attr) {

	pj_str_t tmp_val = attr->value;

	pj_strltrim(&tmp_val);
	si->tx_maxptime = pj_strtoul(&tmp_val);
    }

    /* When direction is NONE (it means SDP negotiation has failed) we don't
     * need to return a failure here, as returning failure will cause
     * the whole SDP to be rejected. See ticket #:
     *	http://
     *
     * Thanks Alain Totouom 
     */
    if (status != PJ_SUCCESS && si->dir != PJMEDIA_DIR_NONE)
	return status;

    /* Get incomming payload type for telephone-events */
    si->rx_event_pt = -1;
    for (i=0; i<local_m->attr_count; ++i) {

	pjmedia_sdp_rtpmap r;

	attr = local_m->attr[i];
	if (pj_strcmp(&attr->name, &ID_RTPMAP) != 0)
	    continue;
	if (pjmedia_sdp_attr_get_rtpmap(attr, &r) != PJ_SUCCESS)
	    continue;
	if (pj_strcmp(&r.enc_name, &ID_TELEPHONE_EVENT) == 0) {
	    si->rx_event_pt = pj_strtoul(&r.pt);
	    break;
	}
    }

    /* Get outgoing payload type for telephone-events */
    si->tx_event_pt = -1;
    for (i=0; i<rem_m->attr_count; ++i) {
	pjmedia_sdp_rtpmap r;

	attr = rem_m->attr[i];
	if (pj_strcmp(&attr->name, &ID_RTPMAP) != 0)
	    continue;
	if (pjmedia_sdp_attr_get_rtpmap(attr, &r) != PJ_SUCCESS)
	    continue;
	if (pj_strcmp(&r.enc_name, &ID_TELEPHONE_EVENT) == 0) {
	    si->tx_event_pt = pj_strtoul(&r.pt);
	    break;
	}
    }

#if 0
    /* Leave SSRC to random. */
    si->ssrc = pj_rand();

    /* Set default jitter buffer parameter. */
    si->jb_init = si->jb_max = si->jb_min_pre = si->jb_max_pre = -1;
#endif
    return PJ_SUCCESS;
}



/*
 * Initialize session info from SDP session descriptors.
 */
pj_status_t pjmedia_session_info_from_sdp( pj_pool_t *pool,
			       pjmedia_endpt *endpt,
			       unsigned max_streams,
			       pjmedia_session_info *si,
			       const pjmedia_sdp_session *local,
			       const pjmedia_sdp_session *remote)
{
    unsigned i;

    PJ_ASSERT_RETURN(pool && endpt && si && local && remote, PJ_EINVAL);

    si->stream_cnt = max_streams;
    if (si->stream_cnt > local->media_count)
    {
		si->stream_cnt = local->media_count;
    }

    for (i=0; i<si->stream_cnt; ++i)
	 {
		pj_status_t status;
		status = pjmedia_stream_info_from_sdp( &si->stream_info[i], pool,
					       endpt, local, remote, i);
		if (status != PJ_SUCCESS)
	   {
	   	return status;
		}
    }

    return PJ_SUCCESS;
}


/**
 * Create new session.
 */

/*ycw-pjsip:send CMSIP_SIP_RTPCREATEINFO*/

static int notifyStreamInfoToCM(	int callIndex, 
														const pjmedia_stream_info *stm, 
														const pjmedia_transport* tp
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
														, pj_bool_t isLocalTransit
#	endif
														)
{
CMSIP_PRINT("-----notify stream Info to CM!");
	CMSIP_MEDIA_TYPE mediaType;
	CMSIP_MEDIA_TPPROTO mediaProto;
	CMSIP_MEDIA_DIR	mediaDir;
	char codecName[CMSIP_MAX_CODEC_NAME_LEN];
	char localIp[CMSIP_STR_40];
	pj_uint16_t	localPort;
	pj_uint16_t localRtcpPort;
	char remoteIp[CMSIP_STR_40];
	pj_uint16_t remotePort;
	pj_uint16_t	remoteRtcpPort;

	switch (stm->type)
	{
	case PJMEDIA_TYPE_NONE:
		mediaType = CMSIP_MEDIA_TYPE_NONE;
	break;
	case PJMEDIA_TYPE_AUDIO:
		mediaType = CMSIP_MEDIA_TYPE_AUDIO;
	break;
	case PJMEDIA_TYPE_VIDEO:
		mediaType = CMSIP_MEDIA_TYPE_VIDEO;
	break;
	case PJMEDIA_TYPE_UNKNOWN:
		mediaType = CMSIP_MEDIA_TYPE_UNKNOWN;
	break;
	case PJMEDIA_TYPE_APPLICATION:
		mediaType = CMSIP_MEDIA_TYPE_APPLICATION;
	break;
	case PJMEDIA_TYPE_IMAGE:
		mediaType = CMSIP_MEDIA_TYPE_IMAGE;
	break;
	default:
		PJ_LOG(4, (THIS_FILE, "wrong media type!!!"));
		CMSIP_PRINT("wrong media type[%d]", stm->type);
		return -1;
	}

	switch (stm->proto)
	{
	case PJMEDIA_TP_PROTO_NONE:
		mediaProto = CMSIP_MEDIA_TPPROTO_NONE;
	break;
	case PJMEDIA_TP_PROTO_RTP_AVP:
		mediaProto = CMSIP_MEDIA_TPPROTO_RTP_AVP;
	break;
	case PJMEDIA_TP_PROTO_RTP_SAVP:
		mediaProto = CMSIP_MEDIA_TPPROTO_RTP_SAVP;
	break;
	case PJMEDIA_TP_PROTO_UDPTL:
		mediaProto = CMSIP_MEDIA_TPPROTO_UDPTL;
	break;
	case PJMEDIA_TP_PROTO_UNKNOWN:
		mediaProto = CMSIP_MEDIA_TPPROTO_UNKNOWN;
	break;
	default:
		PJ_LOG(4, (THIS_FILE, "wrong media protocol!!!"));
		return -1;
	break;
	}

	switch (stm->dir)
	{
	case PJMEDIA_DIR_NONE:
		mediaDir = CMSIP_MEDIA_DIR_NONE;
	break;
	case PJMEDIA_DIR_ENCODING:
		mediaDir = CMSIP_MEDIA_DIR_ENCODING;
	break;
	case PJMEDIA_DIR_DECODING:
		mediaDir = CMSIP_MEDIA_DIR_DECODING;
	break;
	case PJMEDIA_DIR_ENCODING_DECODING:
		mediaDir = CMSIP_MEDIA_DIR_ENCODING_DECODING;
	break;
	default:
		PJ_LOG(4, (THIS_FILE, "wrong direction!!!"));
		return -1;
	break;
	}

	memcpy(codecName, stm->fmt.encoding_name.ptr, stm->fmt.encoding_name.slen);
	codecName[stm->fmt.encoding_name.slen] = 0;

	if (tp)
	{
		pjmedia_transport_udp_local_info(tp, localIp, &localPort, &localRtcpPort);
		pjmedia_transport_udp_remote_info(tp, remoteIp, &remotePort, &remoteRtcpPort);
	}
	else
	{
		localIp[0] = 0;
		localPort = 0;
		localRtcpPort = 0;

		remoteIp[0] = 0;
		remotePort = 0;
		remoteRtcpPort = 0;
	}

	/*ycw-pjsip:要注意的是这儿是默认DSP与本地socket通信的。如果直接与外部网络通信，与此相反*/
	if (
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
		PJ_TRUE
#	else
		isLocalTransit
#	endif
		)
	{
		return cmsip_send_rtpcreate(callIndex, mediaType, mediaProto, mediaDir, 
					codecName, strlen(codecName),
					stm->fmt.clock_rate, stm->fmt.channel_cnt,
					stm->ulptime,
					stm->dlptime,
					stm->tx_pt, stm->fmt.pt, stm->tx_event_pt, stm->rx_event_pt,
					stm->param->info.max_datagram,
					stm->param->info.max_bitrate,
					stm->param->info.enableUdpEc,
					remoteIp, strlen(remoteIp),remotePort,
					localIp, strlen(localIp), localPort, localRtcpPort);
	}
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	return cmsip_send_rtpcreate(callIndex, mediaType, mediaProto, mediaDir, 
				codecName, strlen(codecName),
				stm->fmt.clock_rate, stm->fmt.channel_cnt,
				stm->ulptime,
				stm->dlptime,
				stm->tx_pt, stm->fmt.pt, stm->tx_event_pt, stm->rx_event_pt,
				stm->param->info.max_datagram,
				stm->param->info.max_bitrate,
				stm->param->info.enableUdpEc,
				localIp, strlen(localIp), localPort,
				remoteIp, strlen(remoteIp), remotePort, remoteRtcpPort);	
#	endif
	
}

pj_status_t pjmedia_session_create( pjmedia_endpt *endpt, 
					    const pjmedia_session_info *si,
					    pjmedia_transport *transports[],					    
					    void *user_data,
						int callIndex,
#	if (!(defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) || \
			defined(INCLUDE_PSTN_GATEWAY))
						pjmedia_transport *dsptransports[],
#	endif
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
						pj_bool_t isUsbVm,
#	endif
#	if defined(INCLUDE_PSTN_GATEWAY)
						pj_bool_t isPstn,
#	endif
#	endif
#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
						PJ_FIREWALLCFG_CTRL fwCtrl,
#endif
					    pjmedia_session **p_session )
{
    pj_pool_t *pool;
    pjmedia_session *session;
    int i; /* Must be signed */
    pj_status_t status;

    /* Verify arguments. */
    PJ_ASSERT_RETURN(endpt && si && p_session, PJ_EINVAL);

    /* Create pool for the session. */
    pool = pjmedia_endpt_create_pool( endpt, "session", 
				      PJMEDIA_SESSION_SIZE, 
				      PJMEDIA_SESSION_INC);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    session = PJ_POOL_ZALLOC_T(pool, pjmedia_session);
    session->pool = pool;
    session->endpt = endpt;
    session->stream_cnt = si->stream_cnt;
    session->user_data = user_data;

    /* Copy stream info (this simple memcpy may break sometime) */
    pj_memcpy(session->stream_info, si->stream_info, si->stream_cnt * sizeof(pjmedia_stream_info));

    /*
     * Now create and start the stream!
     */
    for (i=0; i<(int)si->stream_cnt; ++i)
   {
		/* Create the stream */
		status = pjmedia_stream_create(endpt, session->pool,
				       &session->stream_info[i],
				       (transports?transports[i]:NULL),
#	if (!(defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) || \
			defined(INCLUDE_PSTN_GATEWAY))
				       (dsptransports?dsptransports[i]:NULL),	
#	endif
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	(
#	if defined(INCLUDE_USB_VOICEMAIL)
		(PJ_TRUE==isUsbVm) ? PJMEDIA_TRANSPORT_USBVM : 
								(
#	endif
#	if defined(INCLUDE_PSTN_GATEWAY)
									(PJ_TRUE==isPstn) ? PJMEDIA_TRANSPORT_VOIP2PSTN : 
#	endif
									PJMEDIA_TRANSPORT_GENERIC
#	if defined(INCLUDE_USB_VOICEMAIL)
								)
#	endif
	),
#	endif
				       session,
#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
						fwCtrl,
#endif
				       &session->stream[i]);

		if (status == PJ_SUCCESS)
		{
			status = pjmedia_stream_start(session->stream[i]);
			/* Added by sirrain zhang */
			session->stream[i]->callId = callIndex;
		}


		if (status != PJ_SUCCESS)
		{
		   for ( --i; i>=0; ++i)
		   {
				pjmedia_stream_destroy(session->stream[i]
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	, fwCtrl
#endif
					);
		   }

		   pj_pool_release(session->pool);

		   return status;
		}
		/*ycw-pjsip: send RTP info to CM.transport一定是UDP的，无需判断是ICE还是UDP。
		另外，如果DSP是直接连接到外部网络的话，就将原来与外部网络通信的socket信息发送给CM。
		如果DSP是与本地socket通信，则对于DSP来说，远端端口即是本地socket的端口，本地端口即
		是本地socket的远端端口。要注意这个特点。
		目前默认DSP是与本地socket通信的，以后要有一个全局开关，以利于程序判断!!!
		*/
		else
		{
			/*ycw-firewall*/
	 		#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
			PJ_TRANSPORT_PROTO protocol;
			char srcAddr[CMSIP_STR_16];
			char dstAddr[CMSIP_STR_16];
			pj_uint16_t	localFwPort;
			pj_uint16_t localFwRtcpPort;
			pj_uint16_t remoteFwPort;
			pj_uint16_t remoteFwRtcpPort;
			PJ_FIREWALL_RULE* pFwRule = NULL;
			pj_str_t src;
			pj_str_t dst;

			protocol = PJ_TRANSPORT_UDP;
			/*Now, get firewall filter information*/
			pjmedia_transport_udp_local_info(session->stream[i]->transport, 
				srcAddr, &localFwPort, &localFwRtcpPort);
			pjmedia_transport_udp_remote_info(session->stream[i]->transport, 
				dstAddr, &remoteFwPort, &remoteFwRtcpPort);

			src = pj_str(srcAddr);
			dst = pj_str(dstAddr);
			status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, protocol, 
					&src, NULL, localFwPort, &dst, NULL, remoteFwPort, fwCtrl);
			if (status != PJ_SUCCESS)
			{
			   for ( --i; i>=0; ++i)
			   {
					pjmedia_stream_destroy(session->stream[i]
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
					, fwCtrl
#endif
					);
		   		}
		   		pj_pool_release(session->pool);

				return status;
			}
			pFwRule = &session->stream[i]->fwRule[0];
			memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
			pFwRule->protocol = protocol;
			strcpy(pFwRule->dstBuf, srcAddr);
			pFwRule->destination = pj_str(pFwRule->dstBuf);
			pFwRule->dport = localFwPort;
			strcpy(pFwRule->srcBuf, dstAddr);
			pFwRule->source = pj_str(pFwRule->srcBuf);
			pFwRule->sport = remoteFwPort;			

			status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, protocol, 
					&src, NULL, localFwRtcpPort, &dst, NULL, remoteFwRtcpPort, fwCtrl);
			if (status != PJ_SUCCESS)
			{
				pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, pFwRule->protocol, 
					&pFwRule->destination, NULL, pFwRule->dport, 
					&pFwRule->source, NULL, pFwRule->sport, fwCtrl);
			   for ( --i; i>=0; ++i)
			   {
					pjmedia_stream_destroy(session->stream[i]
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
					, fwCtrl
#endif
					);
		   		}
		   		pj_pool_release(session->pool);

				return status;
			}
			pFwRule = &session->stream[i]->fwRule[1];
			memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
			pFwRule->protocol = protocol;
			strcpy(pFwRule->dstBuf, srcAddr);
			pFwRule->destination = pj_str(pFwRule->dstBuf);
			pFwRule->dport = localFwRtcpPort;
			strcpy(pFwRule->srcBuf, dstAddr);
			pFwRule->source = pj_str(pFwRule->srcBuf);
			pFwRule->sport = remoteFwRtcpPort;			
			#endif

			CMSIP_PRINT("notify CM abount media info of stream(%d)", i);

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
				notifyStreamInfoToCM(callIndex, &session->stream_info[i], 
								session->stream[i]->dsp_transport);
#	else

#	if defined(INCLUDE_PSTN_GATEWAY) || defined(INCLUDE_USB_VOICEMAIL)
			if (
#		if defined(INCLUDE_PSTN_GATEWAY)
				PJ_TRUE == isPstn 
#		endif /* defined(INCLUDE_PSTN_GATEWAY) */
#	if defined(INCLUDE_USB_VOICEMAIL)
#	if defined(INCLUDE_PSTN_GATEWAY)
				 || 
#		endif /* defined(INCLUDE_PSTN_GATEWAY) */
				PJ_TRUE == isUsbVm
#		endif /* defined(INCLUDE_USB_VOICEMAIL)	*/
				)
				{
					notifyStreamInfoToCM(callIndex, &session->stream_info[i], 
#		if defined(INCLUDE_PSTN_GATEWAY)
					(PJ_TRUE == isPstn) ?
					session->stream[i]->dsp_transport :
#		endif
					NULL, 
					PJ_TRUE);
				}
				else
#	endif /* defined(INCLUDE_PSTN_GATEWAY) || defined(INCLUDE_USB_VOICEMAIL) */
				{
					notifyStreamInfoToCM(callIndex, &session->stream_info[i], 
						session->stream[i]->transport, PJ_FALSE);
				}

#	endif
		}
    }

    /* Done. */

	/*ycw-usbvm*/
	/* Add HL, should resetVar everytime session created*/
	 DR_resetVar(callIndex);
	 DR_getNegPayloadType(session, &g_DR_payloadType[callIndex]);
    *p_session = session;
    return PJ_SUCCESS;
}

/*
 * Get session info.
 */
pj_status_t pjmedia_session_get_info( pjmedia_session *session,
					      pjmedia_session_info *info )
{
    PJ_ASSERT_RETURN(session && info, PJ_EINVAL);

    info->stream_cnt = session->stream_cnt;
    pj_memcpy(info->stream_info, session->stream_info,
	      session->stream_cnt * sizeof(pjmedia_stream_info));

    return PJ_SUCCESS;
}

/*
 * Get user data.
 */
void* pjmedia_session_get_user_data( pjmedia_session *session)
{
    return (session? session->user_data : NULL);
}


pj_status_t pjmedia_session_destroy (pjmedia_session *session, int callIndex,
																pj_bool_t cmHangup
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
																, pj_bool_t isUsbVm
#	endif
#	endif
#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
						,PJ_FIREWALLCFG_CTRL fwCtrl
#endif
																)
{
    unsigned i;

    PJ_ASSERT_RETURN(session, PJ_EINVAL);

    for (i=0; i<session->stream_cnt; ++i)
	 {	
		pjmedia_stream_destroy(session->stream[i]
#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
						,fwCtrl
#endif
			);
		/*ycw-pjsip:send CMSIP_REQUEST_SIP_RTPDESTROY*/
		if (PJ_FALSE == cmHangup &&
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
		 PJ_FALSE == isUsbVm
#	else
		 1
#	endif
#	endif
			)
		{
			cmsip_send_rtpdestroy(callIndex);
		}
    }

    pj_pool_release (session->pool);

    return PJ_SUCCESS;
}


/**
 * Activate all stream in media session.
 *
 */
pj_status_t pjmedia_session_resume(pjmedia_session *session,
					   pjmedia_dir dir)
{
    unsigned i;

    PJ_ASSERT_RETURN(session, PJ_EINVAL);

    for (i=0; i<session->stream_cnt; ++i) {
	pjmedia_session_resume_stream(session, i, dir);
    }

    return PJ_SUCCESS;
}


/**
 * Suspend receipt and transmission of all stream in media session.
 *
 */
pj_status_t pjmedia_session_pause(pjmedia_session *session,
					  pjmedia_dir dir)
{
    unsigned i;

    PJ_ASSERT_RETURN(session, PJ_EINVAL);

    for (i=0; i<session->stream_cnt; ++i) {
	pjmedia_session_pause_stream(session, i, dir);
    }

    return PJ_SUCCESS;
}


/**
 * Suspend receipt and transmission of individual stream in media session.
 */
pj_status_t pjmedia_session_pause_stream( pjmedia_session *session,
						  unsigned index,
						  pjmedia_dir dir)
{
    PJ_ASSERT_RETURN(session && index < session->stream_cnt, PJ_EINVAL);

    return pjmedia_stream_pause(session->stream[index], dir);
}


/**
 * Activate individual stream in media session.
 *
 */
pj_status_t pjmedia_session_resume_stream( pjmedia_session *session,
						   unsigned index,
						   pjmedia_dir dir)
{
    PJ_ASSERT_RETURN(session && index < session->stream_cnt, PJ_EINVAL);

    return pjmedia_stream_resume(session->stream[index], dir);
}

/**
 * Enumerate media stream in the session.
 */
pj_status_t pjmedia_session_enum_streams(const pjmedia_session *session,
						 unsigned *count, 
						 pjmedia_stream_info info[])
{
    unsigned i;

    PJ_ASSERT_RETURN(session && count && *count && info, PJ_EINVAL);

    if (*count > session->stream_cnt)
	*count = session->stream_cnt;

    for (i=0; i<*count; ++i) {
	pj_memcpy(&info[i], &session->stream_info[i], 
                  sizeof(pjmedia_stream_info));
    }

    return PJ_SUCCESS;
}

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
pj_status_t pjmedia_session_send_rtp_create(pjmedia_session* session, int callIndex)
{	
	unsigned i;
	int ret;
	CMSIP_ASSERT(session!=NULL);

	CMSIP_PRINT("When usbvm recording, user offhook, notify CM abount media info");
	for (i = 0; i < session->stream_cnt; ++i)
	{
		ret = notifyStreamInfoToCM(callIndex, &session->stream_info[i], 
												session->stream[i]->transport
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
												, PJ_FALSE
#	endif
												);
		if (ret < 0)
		{
			return PJMEDIA_CMSIP_ENOTSEND;
		}
	}

	return PJ_SUCCESS;

}
#	endif
#	endif



/*
 * Get the port interface.
 */
 /*ycw-pjsip--delete pjmedia_port*/
#if 0
pj_status_t pjmedia_session_get_port(  pjmedia_session *session,
					       unsigned index,
					       pjmedia_port **p_port)
{
    return pjmedia_stream_get_port( session->stream[index], p_port);
}
#endif

#if 0
/*
 * Get statistics
 */
pj_status_t pjmedia_session_get_stream_stat( pjmedia_session *session,
						     unsigned index,
						     pjmedia_rtcp_stat *stat)
{
    PJ_ASSERT_RETURN(session && stat && index < session->stream_cnt, 
		     PJ_EINVAL);

    return pjmedia_stream_get_stat(session->stream[index], stat);
}


/**
 * Reset session statistics.
 */
pj_status_t pjmedia_session_reset_stream_stat( pjmedia_session *session,
						       unsigned index)
{
    PJ_ASSERT_RETURN(session && index < session->stream_cnt, PJ_EINVAL);

    return pjmedia_stream_reset_stat(session->stream[index]);
}


#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR != 0)
/*
 * Get extended statistics
 */
pj_status_t pjmedia_session_get_stream_stat_xr(
					     pjmedia_session *session,
					     unsigned index,
					     pjmedia_rtcp_xr_stat *stat_xr)
{
    PJ_ASSERT_RETURN(session && stat_xr && index < session->stream_cnt, 
		     PJ_EINVAL);

    return pjmedia_stream_get_stat_xr(session->stream[index], stat_xr);
}
#endif
#endif

#if 0
pj_status_t pjmedia_session_get_stream_stat_jbuf(
					    pjmedia_session *session,
					    unsigned index,
					    pjmedia_jb_state *state)
{
    PJ_ASSERT_RETURN(session && state && index < session->stream_cnt, 
		     PJ_EINVAL);

    return pjmedia_stream_get_stat_jbuf(session->stream[index], state);
}
#endif

#if 0
/*
 * Dial DTMF digit to the stream, using RFC 2833 mechanism.
 */
pj_status_t pjmedia_session_dial_dtmf( pjmedia_session *session,
					       unsigned index,
					       const pj_str_t *ascii_digits )
{
    PJ_ASSERT_RETURN(session && ascii_digits, PJ_EINVAL);
    return pjmedia_stream_dial_dtmf(session->stream[index], ascii_digits);
}
/*
 * Check if the specified stream has received DTMF digits.
 */
pj_status_t pjmedia_session_check_dtmf( pjmedia_session *session,
					        unsigned index )
{
    PJ_ASSERT_RETURN(session, PJ_EINVAL);
    return pjmedia_stream_check_dtmf(session->stream[index]);
}


/*
 * Retrieve DTMF digits from the specified stream.
 */
pj_status_t pjmedia_session_get_dtmf( pjmedia_session *session,
					      unsigned index,
					      char *ascii_digits,
					      unsigned *size )
{
    PJ_ASSERT_RETURN(session && ascii_digits && size, PJ_EINVAL);
    return pjmedia_stream_get_dtmf(session->stream[index], ascii_digits,
				   size);
}

/*
 * Install DTMF callback.
 */
pj_status_t pjmedia_session_set_dtmf_callback(pjmedia_session *session,
				  unsigned index,
				  void (*cb)(pjmedia_stream*, 
				 	     void *user_data, 
					     int digit), 
				  void *user_data)
{
    PJ_ASSERT_RETURN(session && index < session->stream_cnt, PJ_EINVAL);
    return pjmedia_stream_set_dtmf_callback(session->stream[index], cb,
					    user_data);
}
#endif

