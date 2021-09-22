/* $Id: endpoint.c 3553 2011-05-05 06:14:19Z nanang $ */
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
#include <pjmedia/endpoint.h>
#include <pjmedia/errno.h>
#include <pjmedia/sdp.h>
#include <pjmedia-codec/types.h>
/*ycw-pjsip-delete sound device*/
#if 0
#include <pjmedia-audiodev/audiodev.h>
#endif
#include <pj/assert.h>
#include <pj/ioqueue.h>
#include <pj/log.h>
#include <pj/os.h>
#include <pj/pool.h>
#include <pj/sock.h>
#include <pj/string.h>
/*by yuchuwei,resolve the warning:
implicit declaration of function 'pjsua_perror'
*/
#include <pjsua.h>
#include "cmsip_assert.h"

#define THIS_FILE   "endpoint.c"

static const pj_str_t STR_AUDIO = { "audio", 5};
static const pj_str_t STR_VIDEO = { "video", 5};
static const pj_str_t STR_IN = { "IN", 2 };
static const pj_str_t STR_IP4 = { "IP4", 3};
static const pj_str_t STR_IP6 = { "IP6", 3};
static const pj_str_t STR_RTP_AVP = { "RTP/AVP", 7 };
static const pj_str_t STR_SDP_NAME = { "pjmedia", 7 };
static const pj_str_t STR_SENDRECV = { "sendrecv", 8 };
/*ycw-pjsip*/
static const pj_str_t STR_IMAGE = { "image", 5};
static const pj_str_t STR_UDPTL = { "udptl", 5};
static const pj_str_t STR_PTIME = { "ptime", 5};




/* Config to control rtpmap inclusion for static payload types */
pj_bool_t pjmedia_add_rtpmap_for_static_pt = 
	    PJMEDIA_ADD_RTPMAP_FOR_STATIC_PT;



/* Worker thread proc. */
static int PJ_THREAD_FUNC worker_proc(void*);


#define MAX_THREADS	16


/** Concrete declaration of media endpoint. */
struct pjmedia_endpt
{
    /** Pool. */
    pj_pool_t		 *pool;

    /** Pool factory. */
    pj_pool_factory	 *pf;

	/*codec manager*/
	/*ycw-pjsip-codec*/
	/*We must create two codec manager, one is full_codec_mgr, and another
	* is lite_codec_mgr. The full_codec_mgr contains all the codecs we need,but the
	* lite_codec_mgr only has PCMU and PCMA. When the call is from voip to
	* pstn or from pstn to voip, we use lite_codec_mgr, or, we use full_codec_mgr.
	*/
	#if 0
    /** Codec manager. */
    pjmedia_codec_mgr	  codec_mgr;
	#else
	pjmedia_codec_mgr* codec_mgr;
	pjmedia_codec_mgr full_codec_mgr;
#	if (defined(INCLUDE_PSTN_GATEWAY) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP)&& \
				defined(INCLUDE_USB_VOICEMAIL))) || (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
	pjmedia_codec_mgr lite_codec_mgr;
#	endif
	#endif

    /** IOqueue instance. */
    pj_ioqueue_t 	 *ioqueue;

    /** Do we own the ioqueue? */
    pj_bool_t		  own_ioqueue;

    /** Number of threads. */
    unsigned		  thread_cnt;

    /** IOqueue polling thread, if any. */
    pj_thread_t		 *thread[MAX_THREADS];

    /** To signal polling thread to quit. */
    pj_bool_t		  quit_flag;

    /** Is telephone-event enable */
    pj_bool_t		  has_telephone_event;

#if defined(SUPPORT_METABOLIC_MEDIA_PORT) && SUPPORT_METABOLIC_MEDIA_PORT!=0
	/** next RTP/RTCP port */
	pj_uint16_t		mediaPort;
#endif
};

/**
 * Initialize and get the instance of media endpoint.
 */
pj_status_t pjmedia_endpt_create(pj_pool_factory *pf,
					 pj_ioqueue_t *ioqueue,
					 unsigned worker_cnt,
					 pjmedia_endpt **p_endpt)
{
    pj_pool_t *pool;
    pjmedia_endpt *endpt;
    unsigned i;
    pj_status_t status;

    status = pj_register_strerror(PJMEDIA_ERRNO_START, PJ_ERRNO_SPACE_SIZE,
				  &pjmedia_strerror);
    pj_assert(status == PJ_SUCCESS);

    PJ_ASSERT_RETURN(pf && p_endpt, PJ_EINVAL);
    PJ_ASSERT_RETURN(worker_cnt <= MAX_THREADS, PJ_EINVAL);

    pool = pj_pool_create(pf, "med-ept", 512, 512, NULL);
    if (!pool)
	return PJ_ENOMEM;

    endpt = PJ_POOL_ZALLOC_T(pool, struct pjmedia_endpt);
    endpt->pool = pool;
    endpt->pf = pf;
    endpt->ioqueue = ioqueue;
    endpt->thread_cnt = worker_cnt;
	 /*ycw-pjsip*/
	 /*this member must be set according to CM's configuration.
	 default must PJ_FALSE*/
	 #if 0
    endpt->has_telephone_event = PJ_TRUE;
	 #else
	 endpt->has_telephone_event = PJ_FALSE;
	 #endif

    /* Sound */
	/*ycw-pjsip-20110610--delete sound device*/
	#if 0
    status = pjmedia_aud_subsys_init(pf);
    if (status != PJ_SUCCESS)
	goto on_error;
	#endif
	
    /* Init codec manager. */
	/*ycw-pjsip-codec*/
	#if 0
    status = pjmedia_codec_mgr_init(&endpt->codec_mgr, endpt->pf);
    if (status != PJ_SUCCESS)
	goto on_error;
	 #else
	 status = pjmedia_codec_mgr_init(&endpt->full_codec_mgr, endpt->pf, "full_codec_mgr");
    if (status != PJ_SUCCESS)
	 {
	 	goto on_error;
    }

#	if (defined(INCLUDE_PSTN_GATEWAY) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP)&& \
				defined(INCLUDE_USB_VOICEMAIL))) || (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
	 status = pjmedia_codec_mgr_init(&endpt->lite_codec_mgr, endpt->pf, "lite_codec_mgr");
	 if (status != PJ_SUCCESS)
	 {
	 	goto on_error;
	 }
#	endif

	 endpt->codec_mgr = &endpt->full_codec_mgr;
	 #endif

    /* Create ioqueue if none is specified. */

    if (endpt->ioqueue == NULL) {

	
	endpt->own_ioqueue = PJ_TRUE;

	status = pj_ioqueue_create( endpt->pool, PJ_IOQUEUE_MAX_HANDLES,
				    &endpt->ioqueue);
	if (status != PJ_SUCCESS)
	    goto on_error;

	if (worker_cnt == 0) {
	    PJ_LOG(4,(THIS_FILE, "Warning: no worker thread is created in"  
				 "media endpoint for internal ioqueue"));
	}
    }
    /* Create worker threads if asked. */
    for (i=0; i<worker_cnt; ++i) {
	status = pj_thread_create( endpt->pool, "media", &worker_proc,
				   endpt, 0, 0, &endpt->thread[i]);
	if (status != PJ_SUCCESS)
	    goto on_error;
    }
#	ifdef SUPPORT_METABOLIC_MEDIA_PORT
	endpt->mediaPort = PJMEDIA_PORT_LOWER_BOUND;
#	endif

    *p_endpt = endpt;
    return PJ_SUCCESS;

on_error:

    /* Destroy threads */
    for (i=0; i<endpt->thread_cnt; ++i) {
	if (endpt->thread[i]) {
	    pj_thread_destroy(endpt->thread[i]);
	}
    }

    /* Destroy internal ioqueue */
    if (endpt->ioqueue && endpt->own_ioqueue)
	pj_ioqueue_destroy(endpt->ioqueue);

	/*ycw-pjsip-codec*/
	#if 0
    pjmedia_codec_mgr_destroy(&endpt->codec_mgr);
	#else
	endpt->codec_mgr = NULL;
	pjmedia_codec_mgr_destroy(&endpt->full_codec_mgr);
#	if (defined(INCLUDE_PSTN_GATEWAY) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP)&& \
				defined(INCLUDE_USB_VOICEMAIL))) || (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
	pjmedia_codec_mgr_destroy(&endpt->lite_codec_mgr);
	#endif
	#endif
	
		/*ycw-pjsip-20110610--delete sound device*/
	#if 0
    pjmedia_aud_subsys_shutdown();
	#endif
	
    pj_pool_release(pool);
    return status;
}

/**
 * Get the codec manager instance.
 */
pjmedia_codec_mgr* pjmedia_endpt_get_codec_mgr(pjmedia_endpt *endpt)
{
	/*ycw-pjsip-codec*/
	#if 0
    return &endpt->codec_mgr;
	#else
	return endpt->codec_mgr;
	#endif
}

/*ycw-pjsip-codec*/
#if 1
pjmedia_codec_mgr* pjmedia_endpt_get_full_codec_mgr(pjmedia_endpt* endpt)
{
	return &endpt->full_codec_mgr;
}

pjmedia_codec_mgr* pjmedia_endpt_codec_mgr_switch_to_full(pjmedia_endpt* endpt)
{
	if (endpt->codec_mgr != &endpt->full_codec_mgr)
	{
		endpt->codec_mgr = &endpt->full_codec_mgr;
	}

	return endpt->codec_mgr;
}
#	if (defined(INCLUDE_PSTN_GATEWAY) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
				defined(INCLUDE_USB_VOICEMAIL))) || (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
pjmedia_codec_mgr* pjmedia_endpt_get_lite_codec_mgr(pjmedia_endpt* endpt)
{
	return &endpt->lite_codec_mgr;
}

pjmedia_codec_mgr* pjmedia_endpt_codec_mgr_switch_to_lite(pjmedia_endpt* endpt)
{
	if (endpt->codec_mgr != &endpt->lite_codec_mgr)
	{
		endpt->codec_mgr = &endpt->lite_codec_mgr;
	}

	return endpt->codec_mgr;
}
#	endif

#endif

/**
 * Deinitialize media endpoint.
 */
pj_status_t pjmedia_endpt_destroy (pjmedia_endpt *endpt)
{
    PJ_ASSERT_RETURN(endpt, PJ_EINVAL);

	pjmedia_endpt_stop_threads(endpt);
	
    /* Destroy internal ioqueue */
    if (endpt->ioqueue && endpt->own_ioqueue) {
		pj_ioqueue_destroy(endpt->ioqueue);
		endpt->ioqueue = NULL;
    }

    endpt->pf = NULL;

	/*ycw-pjsip-codec*/
	#if 0
    pjmedia_codec_mgr_destroy(&endpt->codec_mgr);
	#else
	endpt->codec_mgr = NULL;
	pjmedia_codec_mgr_destroy(&endpt->full_codec_mgr);
#	if (defined(INCLUDE_PSTN_GATEWAY) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP)&& \
				defined(INCLUDE_USB_VOICEMAIL))) \
				|| (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
	pjmedia_codec_mgr_destroy(&endpt->lite_codec_mgr);	
	#endif
	#endif
	
	/*ycw-pjsip--delete sound device*/
	#if 0
    pjmedia_aud_subsys_shutdown();
	#endif
    pj_pool_release (endpt->pool);

    return PJ_SUCCESS;
}

pj_status_t pjmedia_endpt_set_flag( pjmedia_endpt *endpt,
					    pjmedia_endpt_flag flag,
					    const void *value)
{
    PJ_ASSERT_RETURN(endpt, PJ_EINVAL);

    switch (flag) {
    case PJMEDIA_ENDPT_HAS_TELEPHONE_EVENT_FLAG:
	endpt->has_telephone_event = *(pj_bool_t*)value;

	break;
    default:
	return PJ_EINVAL;
    }

    return PJ_SUCCESS;
}

pj_status_t pjmedia_endpt_get_flag( pjmedia_endpt *endpt,
					    pjmedia_endpt_flag flag,
					    void *value)
{
    PJ_ASSERT_RETURN(endpt, PJ_EINVAL);

    switch (flag) {
    case PJMEDIA_ENDPT_HAS_TELEPHONE_EVENT_FLAG:
	*(pj_bool_t*)value = endpt->has_telephone_event;
	break;
    default:
	return PJ_EINVAL;
    }

    return PJ_SUCCESS;
}

/**
 * Get the ioqueue instance of the media endpoint.
 */
pj_ioqueue_t* pjmedia_endpt_get_ioqueue(pjmedia_endpt *endpt)
{
    PJ_ASSERT_RETURN(endpt, NULL);
    return endpt->ioqueue;
}

/**
 * Get the number of worker threads in media endpoint.
 */
unsigned pjmedia_endpt_get_thread_count(pjmedia_endpt *endpt)
{
    PJ_ASSERT_RETURN(endpt, 0);
    return endpt->thread_cnt;
}

/**
 * Get a reference to one of the worker threads of the media endpoint 
 */
pj_thread_t* pjmedia_endpt_get_thread(pjmedia_endpt *endpt, 
					      unsigned index)
{
    PJ_ASSERT_RETURN(endpt, NULL);
    PJ_ASSERT_RETURN(index < endpt->thread_cnt, NULL);

    /* here should be an assert on index >= 0 < endpt->thread_cnt */

    return endpt->thread[index];
}

/** 
 * Stop and destroy the worker threads of the media endpoint 
 */ 
pj_status_t pjmedia_endpt_stop_threads(pjmedia_endpt *endpt) 
{
	unsigned i;

	PJ_ASSERT_RETURN(endpt, PJ_EINVAL);

	endpt->quit_flag = 1;
	/* Destroy threads */
	for (i=0; i<endpt->thread_cnt; ++i) {
		if (endpt->thread[i]) {
			pj_thread_join(endpt->thread[i]);
			pj_thread_destroy(endpt->thread[i]);
			endpt->thread[i] = NULL;
		}
	}
	return PJ_SUCCESS;
}

/**
 * Worker thread proc.
 */
static int PJ_THREAD_FUNC worker_proc(void *arg)
{
    pjmedia_endpt *endpt = (pjmedia_endpt*) arg;

	 /*ycw-pjsip. 20111120*/
	 /*For the realtime media data, we set the RTP/RTCP thread's priority to maximum*/
	 #if 1
	int max = pj_thread_get_prio_max(pj_thread_this());
	 if (max > 0)
	 {
	 	pj_thread_set_prio(pj_thread_this(), max);
	 }
	 #endif
	 
    while (!endpt->quit_flag)
	 {
		pj_time_val timeout = { 0, 500 };
		pj_ioqueue_poll(endpt->ioqueue, &timeout);
    }

    return 0;
}

/**
 * Create pool.
 */
pj_pool_t* pjmedia_endpt_create_pool( pjmedia_endpt *endpt,
					      const char *name,
					      pj_size_t initial,
					      pj_size_t increment)
{
    pj_assert(endpt != NULL);

    return pj_pool_create(endpt->pf, name, initial, increment, NULL);
}

/**
 * Create a SDP session description that describes the endpoint
 * capability.
 */
pj_status_t pjmedia_endpt_create_sdp( pjmedia_endpt *endpt,
					      pj_pool_t *pool,
					      unsigned stream_cnt,
					      const pjmedia_sock_info sock_info[],
					      /*ycw-pjsip-ptime*/
							int ptime,
							pj_bool_t has_televt,
					      pjmedia_sdp_session **p_sdp )
{
    pj_time_val tv;
    unsigned i;
    const pj_sockaddr *addr0;
    pjmedia_sdp_session *sdp;
    pjmedia_sdp_media *m;
    pjmedia_sdp_attr *attr;
	 /*ycw-pjsip-ptime*/
	 #if 1
	 char tmp[3];
	 #endif

    /* Sanity check arguments */
    PJ_ASSERT_RETURN(endpt && pool && p_sdp && stream_cnt, PJ_EINVAL);

    /* Check that there are not too many codecs */
	 /*ycw-pjsip-codec*/
	 #if 0
    PJ_ASSERT_RETURN(endpt->codec_mgr.codec_cnt <= PJMEDIA_MAX_SDP_FMT, PJ_ETOOMANY);
	 #else
	 PJ_ASSERT_RETURN(endpt->codec_mgr->codec_cnt <= PJMEDIA_MAX_SDP_FMT, PJ_ETOOMANY);
	 #endif

    /* Create and initialize basic SDP session */
    sdp = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_session);

    addr0 = &sock_info[0].rtp_addr_name;

    pj_gettimeofday(&tv);
    sdp->origin.user = pj_str("-");
    sdp->origin.version = sdp->origin.id = tv.sec + 2208988800UL;
    sdp->origin.net_type = STR_IN;

    if (addr0->addr.sa_family == pj_AF_INET())
	 {
		sdp->origin.addr_type = STR_IP4;
		pj_strdup2(pool, &sdp->origin.addr, pj_inet_ntoa(addr0->ipv4.sin_addr));
    }
	 else if (addr0->addr.sa_family == pj_AF_INET6())
	 {
		char tmp_addr[PJ_INET6_ADDRSTRLEN];

		sdp->origin.addr_type = STR_IP6;
		pj_strdup2(pool, &sdp->origin.addr, pj_sockaddr_print(addr0, tmp_addr, sizeof(tmp_addr), 0));

    }
	 else
	 {
		pj_assert(!"Invalid address family");
		return PJ_EAFNOTSUP;
    }

    sdp->name = STR_SDP_NAME;

    /* Since we only support one media stream at present, put the
     * SDP connection line in the session level.
     */
    sdp->conn = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_conn);
    sdp->conn->net_type = sdp->origin.net_type;
    sdp->conn->addr_type = sdp->origin.addr_type;
    sdp->conn->addr = sdp->origin.addr;


    /* SDP time and attributes. */
    sdp->time.start = sdp->time.stop = 0;
    sdp->attr_count = 0;

    /* Create media stream 0: */

    sdp->media_count = 1;
    m = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_media);
    sdp->media[0] = m;

    /* Standard media info: */
    pj_strdup(pool, &m->desc.media, &STR_AUDIO);
    m->desc.port = pj_sockaddr_get_port(addr0);
    m->desc.port_count = 1;
    pj_strdup (pool, &m->desc.transport, &STR_RTP_AVP);

    /* Init media line and attribute list. */
    m->desc.fmt_count = 0;
    m->attr_count = 0;

    /* Add "rtcp" attribute */
#if defined(PJMEDIA_HAS_RTCP_IN_SDP) && PJMEDIA_HAS_RTCP_IN_SDP!=0
    if (sock_info->rtcp_addr_name.addr.sa_family != 0)
	 {
		attr = pjmedia_sdp_attr_create_rtcp(pool, &sock_info->rtcp_addr_name);
		if (attr)
	   {
	   	pjmedia_sdp_attr_add(&m->attr_count, m->attr, attr);
		}
    }
#endif

    /* Add format, rtpmap, and fmtp (when applicable) for each codec */
	/*ycw-pjsip-codec*/
	#if 0
    for (i=0; i<endpt->codec_mgr.codec_cnt; ++i)
	#else
	for (i = 0; i < endpt->codec_mgr->codec_cnt; ++i)
	#endif
	 {

		pjmedia_codec_info *codec_info;
		pjmedia_sdp_rtpmap rtpmap;
		char tmp_param[3];
		pjmedia_sdp_attr *attr;
		pjmedia_codec_param codec_param;
		pj_str_t *fmt;

		/*ycw-pjsip-codec*/
		#if 0
		if (endpt->codec_mgr.codec_desc[i].prio == PJMEDIA_CODEC_PRIO_DISABLED)
	   {
	   	break;
		}

		codec_info = &endpt->codec_mgr.codec_desc[i].info;
		pjmedia_codec_mgr_get_default_param(&endpt->codec_mgr, codec_info, &codec_param);
		#else
		if (endpt->codec_mgr->codec_desc[i].prio == PJMEDIA_CODEC_PRIO_DISABLED)
	   {
	   	break;
		}

		codec_info = &endpt->codec_mgr->codec_desc[i].info;
		pjmedia_codec_mgr_get_default_param(endpt->codec_mgr, codec_info, &codec_param);
		#endif

		/*ycw-pjsip. t38*/
		if (codec_param.info.pt == PJMEDIA_RTP_PT_T38)
		{
			continue;
		}
	
		fmt = &m->desc.fmt[m->desc.fmt_count++];

		fmt->ptr = (char*) pj_pool_alloc(pool, 8);
		fmt->slen = pj_utoa(codec_info->pt, fmt->ptr);

		rtpmap.pt = *fmt;
		rtpmap.enc_name = codec_info->encoding_name;

#if defined(PJMEDIA_HANDLE_G722_MPEG_BUG) && (PJMEDIA_HANDLE_G722_MPEG_BUG != 0)
		if (codec_info->pt == PJMEDIA_RTP_PT_G722)
		{
			rtpmap.clock_rate = 8000;
		}
		else
		{
			rtpmap.clock_rate = codec_info->clock_rate;
		}
#else
		rtpmap.clock_rate = codec_info->clock_rate;
#endif

		/* For audio codecs, rtpmap parameters denotes the number
		 * of channels, which can be omited if the value is 1.
		 */
		if (codec_info->type == PJMEDIA_TYPE_AUDIO && codec_info->channel_cnt > 1)
		{
		    /* Can only support one digit channel count */
		    pj_assert(codec_info->channel_cnt < 10);

		    tmp_param[0] = (char)('0' + codec_info->channel_cnt);

		    rtpmap.param.ptr = tmp_param;
		    rtpmap.param.slen = 1;
		}
		else
		{
		    rtpmap.param.ptr = NULL;
		    rtpmap.param.slen = 0;
		}

		if (codec_info->pt >= 96 || pjmedia_add_rtpmap_for_static_pt)
		{
		    pjmedia_sdp_rtpmap_to_attr(pool, &rtpmap, &attr);
		    m->attr[m->attr_count++] = attr;
		}

		/* Add fmtp params */
		if (codec_param.setting.dec_fmtp.cnt > 0)
		{
	   	enum { MAX_FMTP_STR_LEN = 160 };
	   	char buf[MAX_FMTP_STR_LEN];
	   	unsigned buf_len = 0, i;
	   	pjmedia_codec_fmtp *dec_fmtp = &codec_param.setting.dec_fmtp;

	    	/* Print codec PT */
	    	buf_len += pj_ansi_snprintf(buf, 
					MAX_FMTP_STR_LEN - buf_len, 
					"%d", 
					codec_info->pt);

	    	for (i = 0; i < dec_fmtp->cnt; ++i)
			{
				unsigned test_len = 2;

				/* Check if buf still available */
				test_len = dec_fmtp->param[i].val.slen + dec_fmtp->param[i].name.slen;
				if (test_len + buf_len >= MAX_FMTP_STR_LEN)
				{
		    		return PJ_ETOOBIG;
				}

				/* Print delimiter */
				buf_len += pj_ansi_snprintf(&buf[buf_len], MAX_FMTP_STR_LEN - buf_len,
					    								(i == 0?" ":";"));

				/* Print an fmtp param */
				if (dec_fmtp->param[i].name.slen)
		    		buf_len += pj_ansi_snprintf(
					    &buf[buf_len],
					    MAX_FMTP_STR_LEN - buf_len,
					    "%.*s=%.*s",
					    (int)dec_fmtp->param[i].name.slen,
					    pj_strnull(dec_fmtp->param[i].name.ptr),
					    (int)dec_fmtp->param[i].val.slen,
					    pj_strnull(dec_fmtp->param[i].val.ptr));
				else
				    buf_len += pj_ansi_snprintf(&buf[buf_len], 
							    MAX_FMTP_STR_LEN - buf_len,
							    "%.*s", 
							    (int)dec_fmtp->param[i].val.slen,
							    pj_strnull(dec_fmtp->param[i].val.ptr));
	    	}

	    	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);

		   attr->name = pj_str("fmtp");
		   attr->value = pj_strdup3(pool, buf);
		   m->attr[m->attr_count++] = attr;
		}
    }

    /* Add sendrecv attribute. */
    attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
    attr->name = STR_SENDRECV;
    m->attr[m->attr_count++] = attr;

	/*ycw-pjsip*/
	 /*Add ptime attribute*/
	 snprintf(tmp, sizeof(tmp), "%d", ptime);
	 CMSIP_PRINT("ptime str[%s]\n", tmp);
	 attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	 attr->name = STR_PTIME;
	 attr->value = pj_strdup3(pool, tmp);
	 m->attr[m->attr_count++] = attr;
	

#if defined(PJMEDIA_RTP_PT_TELEPHONE_EVENTS) && \
    PJMEDIA_RTP_PT_TELEPHONE_EVENTS != 0
    /*
     * Add support telephony event
     */

    if (endpt->has_telephone_event || has_televt)
	 {
		m->desc.fmt[m->desc.fmt_count++] = pj_str(PJMEDIA_RTP_PT_TELEPHONE_EVENTS_STR);

		/* Add rtpmap. */
		attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
		attr->name = pj_str("rtpmap");
		attr->value = pj_str(PJMEDIA_RTP_PT_TELEPHONE_EVENTS_STR
				     " telephone-event/8000");
		m->attr[m->attr_count++] = attr;

		/* Add fmtp */
		attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
		attr->name = pj_str("fmtp");
		attr->value = pj_str(PJMEDIA_RTP_PT_TELEPHONE_EVENTS_STR " 0-15");
		m->attr[m->attr_count++] = attr;
    }
#endif

    /* Done */
    *p_sdp = sdp;

    return PJ_SUCCESS;

}

/*ycw-pjsip. t38*/
/**
* Create a SDP session description that describes the endpoint
* capability of t38.
*/
pj_status_t pjmedia_endpt_create_sdp_t38( pjmedia_endpt *endpt,
											     pj_pool_t *pool,
											     unsigned stream_cnt,
											     const pjmedia_sock_info sock_info[],
											     pjmedia_sdp_session **p_sdp )
{
	pj_time_val tv;
	//unsigned i;
	const pj_sockaddr *addr0;
	pjmedia_sdp_session *sdp;
	pjmedia_sdp_media *m;
	pjmedia_sdp_attr *attr;

	/* Sanity check arguments */
	PJ_ASSERT_RETURN(endpt && pool && p_sdp && stream_cnt, PJ_EINVAL);

	/* Check that there are not too many codecs */
	/*ycw-pjsip-codec*/
	#if 0
	PJ_ASSERT_RETURN(endpt->codec_mgr.codec_cnt <= PJMEDIA_MAX_SDP_FMT, PJ_ETOOMANY);
	#else
	PJ_ASSERT_RETURN(endpt->codec_mgr->codec_cnt <= PJMEDIA_MAX_SDP_FMT, PJ_ETOOMANY);
	#endif

	/* Create and initialize basic SDP session */
	sdp = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_session);

	addr0 = &sock_info[0].rtp_addr_name;

	pj_gettimeofday(&tv);
	sdp->origin.user = pj_str("-");
	sdp->origin.version = sdp->origin.id = tv.sec + 2208988800UL;
	sdp->origin.net_type = STR_IN;

	if (addr0->addr.sa_family == pj_AF_INET())
	{
		sdp->origin.addr_type = STR_IP4;
		pj_strdup2(pool, &sdp->origin.addr, 
			pj_inet_ntoa(addr0->ipv4.sin_addr));
	}
	else if (addr0->addr.sa_family == pj_AF_INET6())
	{
		char tmp_addr[PJ_INET6_ADDRSTRLEN];

		sdp->origin.addr_type = STR_IP6;
		pj_strdup2(pool, &sdp->origin.addr, 
			pj_sockaddr_print(addr0, tmp_addr, sizeof(tmp_addr), 0));

	}
	else
	{
		pj_assert(!"Invalid address family");
		return PJ_EAFNOTSUP;
	}

	sdp->name = STR_SDP_NAME;

	/* Since we only support one media stream at present, put the
	* SDP connection line in the session level.
	*/
	sdp->conn = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_conn);
	sdp->conn->net_type = sdp->origin.net_type;
	sdp->conn->addr_type = sdp->origin.addr_type;
	sdp->conn->addr = sdp->origin.addr;


	/* SDP time and attributes. */
	sdp->time.start = sdp->time.stop = 0;
	sdp->attr_count = 0;

	/* Create media stream 0: */

	sdp->media_count = 1;
	m = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_media);
	sdp->media[0] = m;

	/* Standard media info: */
	pj_strdup(pool, &m->desc.media, &STR_IMAGE);
	m->desc.port = pj_sockaddr_get_port(addr0);
	m->desc.port_count = 1;
	pj_strdup (pool, &m->desc.transport, &STR_UDPTL);

	/* Init media line and attribute list. */
	m->desc.fmt_count = 0;
	m->desc.fmt[m->desc.fmt_count++] = pj_str("t38");

	m->attr_count = 0;
	/* Add attr list for t38 */ // TODO: get system capability	
	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxVersion");
	attr->value = pj_str("0");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38MaxBitRate");
	attr->value = pj_str("14400");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxFillBitRemoval");
	attr->value = pj_str("0");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxTranscodingMMR");
	attr->value = pj_str("0");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxTranscodingJBIG");
	attr->value = pj_str("0");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxRateManagement");
	attr->value = pj_str("transferredTCF");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxMaxBuffer");
	attr->value = pj_str("800");
	m->attr[m->attr_count++] = attr;

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxMaxDatagram");
	attr->value = pj_str("284");
	m->attr[m->attr_count++] = attr;

	/*attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxUdpEC");
	attr->value = pj_str("t38UDPFEC");
	m->attr[m->attr_count++] = attr;*/

	attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
	if (!attr) return PJ_ENOMEM;
	attr->name = pj_str("T38FaxUdpEC");
	attr->value = pj_str("t38UDPRedundancy");
	m->attr[m->attr_count++] = attr;

	/* Done */
	*p_sdp = sdp;

    return PJ_SUCCESS;

}



#if PJ_LOG_MAX_LEVEL >= 3
static const char *good_number(char *buf, pj_int32_t val)
{
    if (val < 1000) {
	pj_ansi_sprintf(buf, "%d", val);
    } else if (val < 1000000) {
	pj_ansi_sprintf(buf, "%d.%dK", 
			val / 1000,
			(val % 1000) / 100);
    } else {
	pj_ansi_sprintf(buf, "%d.%02dM", 
			val / 1000000,
			(val % 1000000) / 10000);
    }

    return buf;
}
#endif

pj_status_t pjmedia_endpt_dump(pjmedia_endpt *endpt)
{

#if PJ_LOG_MAX_LEVEL >= 3
    unsigned i, count;
    pjmedia_codec_info codec_info[32];
    unsigned prio[32];

    PJ_LOG(3,(THIS_FILE, "Dumping PJMEDIA capabilities:"));

    count = PJ_ARRAY_SIZE(codec_info);
	 /*ycw-pjsip-codec*/
	 #if 0
    if (pjmedia_codec_mgr_enum_codecs(&endpt->codec_mgr, 
				      &count, codec_info, prio) != PJ_SUCCESS)
	 #else
	 if (pjmedia_codec_mgr_enum_codecs(endpt->codec_mgr, &count, codec_info, prio) != PJ_SUCCESS)
	 #endif
    {
	PJ_LOG(3,(THIS_FILE, " -error: failed to enum codecs"));
	return PJ_SUCCESS;
    }

    PJ_LOG(3,(THIS_FILE, "  Total number of installed codecs: %d", count));
    for (i=0; i<count; ++i) {
	const char *type;
	pjmedia_codec_param param;
	char bps[32];

	switch (codec_info[i].type) {
	case PJMEDIA_TYPE_AUDIO:
	    type = "Audio"; break;
	case PJMEDIA_TYPE_VIDEO:
	    type = "Video"; break;
	default:
	    type = "Unknown type"; break;
	}

	/*ycw-pjsip-codec*/
	#if 0
	if (pjmedia_codec_mgr_get_default_param(&endpt->codec_mgr,
						&codec_info[i],
						&param) != PJ_SUCCESS)
	#else
	if (pjmedia_codec_mgr_get_default_param(endpt->codec_mgr,
						&codec_info[i],
						&param) != PJ_SUCCESS)
	#endif
	{
	    pj_bzero(&param, sizeof(pjmedia_codec_param));
	}

	/*ycw-pjsip. I delete the frm_per_pkt of param's setting*/
	#if 0
	PJ_LOG(3,(THIS_FILE, 
		  "   %s codec #%2d: pt=%d (%.*s @%dKHz/%d, %sbps, %dms%s%s%s%s%s)",
		  type, i, codec_info[i].pt,
		  (int)codec_info[i].encoding_name.slen,
		  codec_info[i].encoding_name.ptr,
		  codec_info[i].clock_rate/1000,
		  codec_info[i].channel_cnt,
		  good_number(bps, param.info.avg_bps), 
		  param.info.frm_ptime * param.setting.frm_per_pkt,
		  (param.setting.vad ? " vad" : ""),
		  (param.setting.cng ? " cng" : ""),
		  (param.setting.plc ? " plc" : ""),
		  (param.setting.penh ? " penh" : ""),
		  (prio[i]==PJMEDIA_CODEC_PRIO_DISABLED?" disabled":"")));
	#else
		PJ_LOG(3,(THIS_FILE, 
		  "   %s codec #%2d: pt=%d (%.*s @%dKHz/%d, %sbps, %s%s%s%s%s)",
		  type, i, codec_info[i].pt,
		  (int)codec_info[i].encoding_name.slen,
		  codec_info[i].encoding_name.ptr,
		  codec_info[i].clock_rate/1000,
		  codec_info[i].channel_cnt,
		  good_number(bps, param.info.avg_bps), 
		  (param.setting.vad ? " vad" : ""),
		  (param.setting.cng ? " cng" : ""),
		  (param.setting.plc ? " plc" : ""),
		  (param.setting.penh ? " penh" : ""),
		  (prio[i]==PJMEDIA_CODEC_PRIO_DISABLED?" disabled":"")));

	#endif
    }
#endif

    return PJ_SUCCESS;
}

#ifdef SUPPORT_METABOLIC_MEDIA_PORT
pj_uint16_t pjmedia_endpt_get_mediaPort(pjmedia_endpt * endpt)
{
	return (endpt->mediaPort);
}

void pjmedia_endpt_set_mediaPort(pjmedia_endpt * endpt)
{
	pj_uint16_t	port;
	port = endpt->mediaPort;
	if (port + 2 >= PJMEDIA_PORT_UPPER_BOUND)
	{
		port = PJMEDIA_PORT_LOWER_BOUND;
	}
	else
	{
		port += 2;
	}
	endpt->mediaPort = port;
}

pj_status_t	pjmedia_endpt_test_media_port(pj_str_t boundIp, pj_uint16_t port, pj_bool_t* avaible)
{
	pj_sock_t	sockfd;
	pj_sockaddr_in bindaddr;
	pj_status_t	status;
	int on = 1;

	PJ_ASSERT_RETURN(boundIp.slen>0, PJ_EINVAL);

	*avaible = PJ_TRUE;

	status = pj_sockaddr_in_set_str_addr(&bindaddr, &boundIp);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "resolve address error", status);
		return status;
	}

	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sockfd);
	if (status != PJ_SUCCESS)
	{
	   	pjsua_perror(THIS_FILE, "socket() error", status);
	   	return status;
	}

	status = pj_sock_setsockopt(sockfd, pj_SOL_SOCKET(), pj_SO_REUSEADDR(), 
						&on, sizeof(on));
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "setsockopt() error", status);
		return status;
	}
	
	status = pj_sock_bind_in(sockfd, pj_ntohl(bindaddr.sin_addr.s_addr), port);
	if (status != PJ_SUCCESS)
	{
		*avaible = PJ_FALSE;
	   	pj_sock_close(sockfd); 
		pjsua_perror(THIS_FILE, "bind() error", status);
		return status;
	}

	pj_sock_close(sockfd);

	return PJ_SUCCESS;
}

#endif


