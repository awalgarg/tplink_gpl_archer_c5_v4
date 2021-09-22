/* $Id: pjsua_media.c 3553 2011-05-05 06:14:19Z nanang $ */
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
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>
#include "cmsip_assert.h"
#include <errno.h>
#include <net/if.h>

#define THIS_FILE		"pjsua_media.c"

#define DEFAULT_RTP_PORT	4000

#define NULL_SND_DEV_ID		-99

#ifndef PJSUA_REQUIRE_CONSECUTIVE_RTCP_PORT
#   define PJSUA_REQUIRE_CONSECUTIVE_RTCP_PORT	0
#endif


/* Next RTP port to be used */
static pj_uint16_t next_rtp_port;

/* Open sound dev */
/*ycw-pjsip-delete sound device*/
#if 0
static pj_status_t open_snd_dev(pjmedia_snd_port_param *param);

/* Close existing sound device */
static void close_snd_dev(void);

/* Create audio device param */
static pj_status_t create_aud_param(pjmedia_aud_param *param,
				    pjmedia_aud_dev_index capture_dev,
				    pjmedia_aud_dev_index playback_dev,
				    unsigned clock_rate,
				    unsigned channel_count,
				    unsigned samples_per_frame,
				    unsigned bits_per_sample);
#endif

/*ycw-pjsip. Compare two stream info*/
#if 1
static pj_bool_t pjsua_media_stream_info_equal(pjmedia_stream_info* stream1, 
																				pjmedia_stream_info* stream2);
#endif

static void pjsua_media_config_dup(pj_pool_t *pool,
				   pjsua_media_config *dst,
				   const pjsua_media_config *src)
{
    pj_memcpy(dst, src, sizeof(*src));
#if	PJSUA_ADD_ICE_TAGS
    pj_strdup(pool, &dst->turn_server, &src->turn_server);
    pj_stun_auth_cred_dup(pool, &dst->turn_auth_cred, &src->turn_auth_cred);
#endif
}

/**
 * Init media subsystems.
 */
pj_status_t pjsua_media_subsys_init(const pjsua_media_config *cfg)
{
    pj_str_t codec_id = {NULL, 0};
	/*ycw-pjsip-delete conference bridge*/
	#if 0
    unsigned opt;
	#endif
    pj_status_t status;

    /* To suppress warning about unused var when all codecs are disabled */
    PJ_UNUSED_ARG(codec_id);

    /* Specify which audio device settings are save-able */
	/*ycw-pjsip-20110610--delete sound device*/
	#if 0
    pjsua_var.aud_svmask = 0xFFFFFFFF;
    /* These are not-settable */
    pjsua_var.aud_svmask &= ~(PJMEDIA_AUD_DEV_CAP_EXT_FORMAT |
			      PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER |
			      PJMEDIA_AUD_DEV_CAP_OUTPUT_SIGNAL_METER);
    /* EC settings use different API */
    pjsua_var.aud_svmask &= ~(PJMEDIA_AUD_DEV_CAP_EC |
			      PJMEDIA_AUD_DEV_CAP_EC_TAIL);
	#endif
    /* Copy configuration */
    pjsua_media_config_dup(pjsua_var.pool, &pjsua_var.media_cfg, cfg);

    /* Normalize configuration */
    if (pjsua_var.media_cfg.snd_clock_rate == 0) {
	pjsua_var.media_cfg.snd_clock_rate = pjsua_var.media_cfg.clock_rate;
    }

    if (pjsua_var.media_cfg.has_ioqueue &&
	pjsua_var.media_cfg.thread_cnt == 0)
    {
	pjsua_var.media_cfg.thread_cnt = 1;
    }

    if (pjsua_var.media_cfg.max_media_ports < pjsua_var.ua_cfg.max_calls) {
	pjsua_var.media_cfg.max_media_ports = pjsua_var.ua_cfg.max_calls + 2;
    }

    /* Create media endpoint. */
    status = pjmedia_endpt_create(&pjsua_var.cp.factory, 
				  pjsua_var.media_cfg.has_ioqueue? NULL :
				     pjsip_endpt_get_ioqueue(pjsua_var.endpt),
				  pjsua_var.media_cfg.thread_cnt,
				  &pjsua_var.med_endpt);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, 
		     "Media stack initialization has returned error", 
		     status);
	return status;
    }

    /* Register all codecs */

#if PJMEDIA_HAS_SPEEX_CODEC
    /* Register speex. */
    status = pjmedia_codec_speex_init(pjsua_var.med_endpt,  
				      0, 
				      pjsua_var.media_cfg.quality,  
				      -1);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing Speex codec",
		     status);
	return status;
    }

    /* Set speex/16000 to higher priority*/
    codec_id = pj_str("speex/16000");
    pjmedia_codec_mgr_set_codec_priority( 
	pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt),
	&codec_id, PJMEDIA_CODEC_PRIO_NORMAL+2);

    /* Set speex/8000 to next higher priority*/
    codec_id = pj_str("speex/8000");
    pjmedia_codec_mgr_set_codec_priority( 
	pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt),
	&codec_id, PJMEDIA_CODEC_PRIO_NORMAL+1);



#endif /* PJMEDIA_HAS_SPEEX_CODEC */

#if PJMEDIA_HAS_ILBC_CODEC
    /* Register iLBC. */
    status = pjmedia_codec_ilbc_init( pjsua_var.med_endpt, 
				      pjsua_var.media_cfg.ilbc_mode);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing iLBC codec",
		     status);
	return status;
    }
#endif /* PJMEDIA_HAS_ILBC_CODEC */

#if PJMEDIA_HAS_GSM_CODEC
    /* Register GSM */
    status = pjmedia_codec_gsm_init(pjsua_var.med_endpt);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing GSM codec",
		     status);
	return status;
    }
#endif /* PJMEDIA_HAS_GSM_CODEC */

#if PJMEDIA_HAS_G711_CODEC
    /* Register PCMA and PCMU */
    status = pjmedia_codec_g711_init(pjsua_var.med_endpt);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing G711 codec",
		     status);
	return status;
    }

#endif	/* PJMEDIA_HAS_G711_CODEC */

#if PJMEDIA_HAS_G722_CODEC
    status = pjmedia_codec_g722_init( pjsua_var.med_endpt );
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing G722 codec",
		     status);
	return status;
    }


#endif  /* PJMEDIA_HAS_G722_CODEC */

/*ycw-pjsip. t38*/
#if PJMEDIA_HAS_T38_CODEC
	status = pjmedia_codec_t38_init(pjsua_var.med_endpt);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error initializing T38 fax relay", status);
		return status;
	}

#endif /*PJMEDIA_HAS_T38_CODEC*/

#if PJMEDIA_HAS_INTEL_IPP
    /* Register IPP codecs */
    status = pjmedia_codec_ipp_init(pjsua_var.med_endpt);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing IPP codecs",
		     status);
	return status;
    }

#endif /* PJMEDIA_HAS_INTEL_IPP */

#if PJMEDIA_HAS_PASSTHROUGH_CODECS
    /* Register passthrough codecs */
    {
	unsigned aud_idx;
	unsigned ext_fmt_cnt = 0;
	pjmedia_format ext_fmts[32];
	pjmedia_codec_passthrough_setting setting;

	/* List extended formats supported by audio devices */
	for (aud_idx = 0; aud_idx < pjmedia_aud_dev_count(); ++aud_idx) {
	    pjmedia_aud_dev_info aud_info;
	    unsigned i;
	    
	    status = pjmedia_aud_dev_get_info(aud_idx, &aud_info);
	    if (status != PJ_SUCCESS) {
		pjsua_perror(THIS_FILE, "Error querying audio device info",
			     status);
		return status;
	    }
	    
	    /* Collect extended formats supported by this audio device */
	    for (i = 0; i < aud_info.ext_fmt_cnt; ++i) {
		unsigned j;
		pj_bool_t is_listed = PJ_FALSE;

		/* See if this extended format is already in the list */
		for (j = 0; j < ext_fmt_cnt && !is_listed; ++j) {
		    if (ext_fmts[j].id == aud_info.ext_fmt[i].id &&
			ext_fmts[j].bitrate == aud_info.ext_fmt[i].bitrate)
		    {
			is_listed = PJ_TRUE;
		    }
		}
		
		/* Put this format into the list, if it is not in the list */
		if (!is_listed)
		    ext_fmts[ext_fmt_cnt++] = aud_info.ext_fmt[i];

		pj_assert(ext_fmt_cnt <= PJ_ARRAY_SIZE(ext_fmts));
	    }
	}

	/* Init the passthrough codec with supported formats only */
	setting.fmt_cnt = ext_fmt_cnt;
	setting.fmts = ext_fmts;
	setting.ilbc_mode = cfg->ilbc_mode;
	status = pjmedia_codec_passthrough_init2(pjsua_var.med_endpt, &setting);
	if (status != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Error initializing passthrough codecs",
			 status);
	    return status;
	}
    }
#endif /* PJMEDIA_HAS_PASSTHROUGH_CODECS */

#if PJMEDIA_HAS_G7221_CODEC
    /* Register G722.1 codecs */
    status = pjmedia_codec_g7221_init(pjsua_var.med_endpt);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing G722.1 codec",
		     status);
	return status;
    }
#endif /* PJMEDIA_HAS_G7221_CODEC */

#if PJMEDIA_HAS_L16_CODEC
    /* Register L16 family codecs, but disable all */
    status = pjmedia_codec_l16_init(pjsua_var.med_endpt, 0);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing L16 codecs",
		     status);
	return status;
    }

    /* Disable ALL L16 codecs */
    codec_id = pj_str("L16");
    pjmedia_codec_mgr_set_codec_priority( 
	pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt),
	&codec_id, PJMEDIA_CODEC_PRIO_DISABLED);

#endif	/* PJMEDIA_HAS_L16_CODEC */

/*ycw-pjsip*/
#if PJMEDIA_HAS_G726_CODEC
	status = pjmedia_codec_g726_32_init(pjsua_var.med_endpt);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error initializing G726-32 codecs", status);
		return status;
	}

#endif
#if PJMEDIA_HAS_G721_CODEC
	status = pjmedia_codec_g721_init(pjsua_var.med_endpt);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error initializing G721 codecs", status);
		return status;
	}

#endif
#if PJMEDIA_HAS_G729_CODEC
	status = pjmedia_codec_g729_init(pjsua_var.med_endpt);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error initializing G729 codecs", status);
		return status;
	}

#endif

    /* Save additional conference bridge parameters for future
     * reference.
     */
    pjsua_var.mconf_cfg.channel_count = pjsua_var.media_cfg.channel_count;
    pjsua_var.mconf_cfg.bits_per_sample = 16;
    pjsua_var.mconf_cfg.samples_per_frame = pjsua_var.media_cfg.clock_rate * 
					    pjsua_var.mconf_cfg.channel_count *
					    pjsua_var.media_cfg.audio_frame_ptime / 
					    1000;
/*ycw-pjsip-delete conference bridge*/
#if 0
    /* Init options for conference bridge. */
    opt = PJMEDIA_CONF_NO_DEVICE;
    if (pjsua_var.media_cfg.quality >= 3 &&
	pjsua_var.media_cfg.quality <= 4)
    {
	opt |= PJMEDIA_CONF_SMALL_FILTER;
    }
    else if (pjsua_var.media_cfg.quality < 3) {
	opt |= PJMEDIA_CONF_USE_LINEAR;
    }
	

    /* Init conference bridge. */
    status = pjmedia_conf_create(pjsua_var.pool, 
				 pjsua_var.media_cfg.max_media_ports,
				 pjsua_var.media_cfg.clock_rate, 
				 pjsua_var.mconf_cfg.channel_count,
				 pjsua_var.mconf_cfg.samples_per_frame, 
				 pjsua_var.mconf_cfg.bits_per_sample, 
				 opt, &pjsua_var.mconf);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error creating conference bridge", 
		     status);
	return status;
    }

    /* Are we using the audio switchboard (a.k.a APS-Direct)? */
    pjsua_var.is_mswitch = pjmedia_conf_get_master_port(pjsua_var.mconf)
			    ->info.signature == PJMEDIA_CONF_SWITCH_SIGNATURE;
#endif
    /* Create null port just in case user wants to use null sound. */
/*ycw-pjsip--delete null port*/
#if 0
    status = pjmedia_null_port_create(pjsua_var.pool, 
				      pjsua_var.media_cfg.clock_rate,
				      pjsua_var.mconf_cfg.channel_count,
				      pjsua_var.mconf_cfg.samples_per_frame,
				      pjsua_var.mconf_cfg.bits_per_sample,
				      &pjsua_var.null_port);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    /* Initialize SRTP library. */
    status = pjmedia_srtp_init_lib();
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error initializing SRTP library", 
		     status);
	return status;
    }
#endif

    return PJ_SUCCESS;
}

/* 
 * Create RTP and RTCP socket pair, and possibly resolve their public
 * address via STUN.
 */
static pj_status_t create_rtp_rtcp_sock(const pjsua_transport_config *cfg,
					pjmedia_sock_info *skinfo)
{
    enum
	{ 
		RTP_RETRY = 100
    };
	 
    int i;
    pj_sockaddr_in bound_addr;
    pj_sockaddr_in mapped_addr[2];
    pj_status_t status = PJ_SUCCESS;
    pj_sock_t sock[2];
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
    /*ycw-pjsip.Enable bind address reuse*/
   	int on = 1;
#	endif
#if SUPPORT_STUN	
    /* Make sure STUN server resolution has completed */
    status = resolve_stun_server(PJ_TRUE);
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "Error resolving STUN server", status);
		return status;
    }
#endif /* SUPPORT_STUN */
    if (next_rtp_port == 0)
    {
		next_rtp_port = (pj_uint16_t)cfg->port;
    }

    for (i=0; i<2; ++i)
    {
		sock[i] = PJ_INVALID_SOCKET;
    }

    bound_addr.sin_addr.s_addr = PJ_INADDR_ANY;
	/*ycw-pjsip. There must be bound address.*/
	CMSIP_ASSERT(cfg->bound_addr.slen);
    if (cfg->bound_addr.slen)
	{
		status = pj_sockaddr_in_set_str_addr(&bound_addr, &cfg->bound_addr);
		if (status != PJ_SUCCESS)
		{
		   	pjsua_perror(THIS_FILE, "Unable to resolve transport bind address",
				 status);
	    	return status;
		}		
    }

    /* Loop retry to bind RTP and RTCP sockets. */
    for (i=0; i<RTP_RETRY; ++i, next_rtp_port += 2)
	{	
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
		/* Create RTP socket. */
		status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[0]);
		if (status != PJ_SUCCESS)
		{
		   	pjsua_perror(THIS_FILE, "socket() error", status);
		   	return status;
		}

		/* Apply QoS to RTP socket, if specified */
		status = pj_sock_apply_qos2(sock[0], cfg->qos_type, 
				    &cfg->qos_params, 
				    2, THIS_FILE, "RTP socket");

		/*ycw-pjsip.Enable bind address reuse.*/
		status = pj_sock_setsockopt(sock[0], pj_SOL_SOCKET(), pj_SO_REUSEADDR(), 
						&on, sizeof(on));

		/* Bind RTP socket */
		status=pj_sock_bind_in(sock[0], pj_ntohl(bound_addr.sin_addr.s_addr), 
			       next_rtp_port);
		if (status != PJ_SUCCESS)
		{
		   	pj_sock_close(sock[0]); 
	    	sock[0] = PJ_INVALID_SOCKET;
	    	continue;
		}

		/* Create RTCP socket. */
		status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[1]);
		if (status != PJ_SUCCESS)
		{
		   	pjsua_perror(THIS_FILE, "socket() error", status);
		   	pj_sock_close(sock[0]);
		   	return status;
		}

		/* Apply QoS to RTCP socket, if specified */
		status = pj_sock_apply_qos2(sock[1], cfg->qos_type, 
				    &cfg->qos_params, 
				    2, THIS_FILE, "RTCP socket");

		/*ycw-pjsip.Enable bind address reuse.*/
		status = pj_sock_setsockopt(sock[1], pj_SOL_SOCKET(), pj_SO_REUSEADDR(), 
						&on, sizeof(on));

		/* Bind RTCP socket */
		status=pj_sock_bind_in(sock[1], pj_ntohl(bound_addr.sin_addr.s_addr), 
			       (pj_uint16_t)(next_rtp_port+1));
		if (status != PJ_SUCCESS)
		{
	   	pj_sock_close(sock[0]); 
	   	sock[0] = PJ_INVALID_SOCKET;

	   	pj_sock_close(sock[1]); 
	   	sock[1] = PJ_INVALID_SOCKET;
	   	continue;
		}
#	endif
#if SUPPORT_STUN
	/*
	 * If we're configured to use STUN, then find out the mapped address,
	 * and make sure that the mapped RTCP port is adjacent with the RTP.
	 */
	if (pjsua_var.stun_srv.addr.sa_family != 0)
	{
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
	/*ycw-pjsip-note.如果开启stun，要注意这里用到sock，肯定要重新改*/
#	endif
	    char ip_addr[32];
	    pj_str_t stun_srv;

	    pj_ansi_strcpy(ip_addr, 
			   pj_inet_ntoa(pjsua_var.stun_srv.ipv4.sin_addr));
	    stun_srv = pj_str(ip_addr);

	    status=pjstun_get_mapped_addr(&pjsua_var.cp.factory, 2, sock,
					   &stun_srv, pj_ntohs(pjsua_var.stun_srv.ipv4.sin_port),
					   &stun_srv, pj_ntohs(pjsua_var.stun_srv.ipv4.sin_port),
					   mapped_addr);
	    if (status != PJ_SUCCESS)
		 {
			pjsua_perror(THIS_FILE, "STUN resolve error", status);
			goto on_error;
	    }

#if PJSUA_REQUIRE_CONSECUTIVE_RTCP_PORT
	    if (pj_ntohs(mapped_addr[1].sin_port) == 
		pj_ntohs(mapped_addr[0].sin_port)+1)
	    {
			/* Success! */
			break;
	    }

	    pj_sock_close(sock[0]); 
	    sock[0] = PJ_INVALID_SOCKET;

	    pj_sock_close(sock[1]); 
	    sock[1] = PJ_INVALID_SOCKET;
#else
	    if (pj_ntohs(mapped_addr[1].sin_port) != 
		pj_ntohs(mapped_addr[0].sin_port)+1)
	    {
		PJ_LOG(4,(THIS_FILE, 
			  "Note: STUN mapped RTCP port %d is not adjacent"
			  " to RTP port %d",
			  pj_ntohs(mapped_addr[1].sin_port),
			  pj_ntohs(mapped_addr[0].sin_port)));
	    }
	    /* Success! */
	    break;
#endif

	}
	else 
#endif
	if (cfg->public_addr.slen)
	{
	    status = pj_sockaddr_in_init(&mapped_addr[0], &cfg->public_addr,
					 (pj_uint16_t)next_rtp_port);
	    if (status != PJ_SUCCESS)
		goto on_error;

	    status = pj_sockaddr_in_init(&mapped_addr[1], &cfg->public_addr,
					 (pj_uint16_t)(next_rtp_port+1));
	    if (status != PJ_SUCCESS)
		goto on_error;

	    break;

	}
	else
	{
	    if (bound_addr.sin_addr.s_addr == 0)
		{
			pj_sockaddr addr;

			/* Get local IP address. */
			status = pj_gethostip(pj_AF_INET(), &addr);
			if (status != PJ_SUCCESS)
		   {
		   	goto on_error;
			}

			bound_addr.sin_addr.s_addr = addr.ipv4.sin_addr.s_addr;
	    }

	    for (i=0; i<2; ++i)
		{
			pj_sockaddr_in_init(&mapped_addr[i], NULL, 0);
			mapped_addr[i].sin_addr.s_addr = bound_addr.sin_addr.s_addr;
	    }

	    mapped_addr[0].sin_port=pj_htons((pj_uint16_t)next_rtp_port);
	    mapped_addr[1].sin_port=pj_htons((pj_uint16_t)(next_rtp_port+1));
	    break;
	}
    }

# if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
    if (sock[0] == PJ_INVALID_SOCKET)
	 {
		PJ_LOG(1,(THIS_FILE, 
		  "Unable to find appropriate RTP/RTCP ports combination"));
		goto on_error;
    }
#	endif

    skinfo->rtp_sock = sock[0];

    pj_memcpy(&skinfo->rtp_addr_name, 
	      &mapped_addr[0], sizeof(pj_sockaddr_in));

    skinfo->rtcp_sock = sock[1];

    pj_memcpy(&skinfo->rtcp_addr_name, 
	      &mapped_addr[1], sizeof(pj_sockaddr_in));

#if PJ_LOG_MAX_LEVEL >= 4
	char addr_buf[128] = {0};
#endif
    PJ_LOG(4,(THIS_FILE, "RTP socket[%d] reachable at %s", skinfo->rtp_sock,
	      pj_sockaddr_print(&skinfo->rtp_addr_name, addr_buf,
				sizeof(addr_buf), 3)));
    PJ_LOG(4,(THIS_FILE, "RTCP socket[%d] reachable at %s", skinfo->rtcp_sock,
	      pj_sockaddr_print(&skinfo->rtcp_addr_name, addr_buf,
				sizeof(addr_buf), 3)));

    next_rtp_port += 2;
    return PJ_SUCCESS;

on_error:

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
   for (i=0; i<2; ++i)
	{
		if (sock[i] != PJ_INVALID_SOCKET)
	   	 pj_sock_close(sock[i]);
   }
#	endif
   return status;
}

/*ycw-pjsip*/
#	if (defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
				defined(INCLUDE_PSTN_GATEWAY))
static pj_status_t create_dsp_rtp_rtcp_socket(const pjsua_transport_config* cfg,
																pjmedia_sock_info* skinfo)
{
	pj_status_t status;
	pj_sock_t sock[2];
	pj_sockaddr_in bind_addr;
	pj_sockaddr_in mapped_addr[2];
	pj_sockaddr_in tmpAddr;
	int len;
	pj_str_t str_ip = {"127.0.0.1", 9};

	PJ_UNUSED_ARG(cfg);
		
	status = pj_sockaddr_in_set_str_addr(&bind_addr, &str_ip);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Unable to resolve transport bind address", status);
	   return status;
	}

	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[0]);
	if (status != PJ_SUCCESS)
	{
   	pjsua_perror(THIS_FILE, "socket() error", status);
   	return status;
	}

	/*Port is 0. Generated by system.*/
	status = pj_sock_bind_in(sock[0], pj_ntohl(bind_addr.sin_addr.s_addr), 0);
	if (status != PJ_SUCCESS)
	{
	  	pj_sock_close(sock[0]); 
		return status;
	}

	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[1]);
	if (status != PJ_SUCCESS)
	{
		pj_sock_close(sock[0]);
   	pjsua_perror(THIS_FILE, "socket() error", status);
   	return status;
	}
	status = pj_sock_bind_in(sock[1], pj_ntohl(bind_addr.sin_addr.s_addr), 0);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "bind() error", status);
		goto on_error;
	}

	pj_sockaddr_in_init(&mapped_addr[0], NULL, 0);
	pj_sockaddr_in_init(&mapped_addr[1], NULL, 0);
	mapped_addr[0].sin_addr.s_addr = bind_addr.sin_addr.s_addr;
	mapped_addr[1].sin_addr.s_addr = bind_addr.sin_addr.s_addr;
	
	len = sizeof(tmpAddr);
	status = pj_sock_getsockname(sock[0], &tmpAddr, &len);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "getsockname() error", status);
		goto on_error;
	}
	mapped_addr[0].sin_port = tmpAddr.sin_port;

	status = pj_sock_getsockname(sock[1], &tmpAddr, &len);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "getsockname() error", status);
		goto on_error;
	}
	mapped_addr[1].sin_port = tmpAddr.sin_port;

	skinfo->rtp_sock = sock[0];
	pj_memcpy(&skinfo->rtp_addr_name, &mapped_addr[0], sizeof(pj_sockaddr));

	skinfo->rtcp_sock = sock[1];
	pj_memcpy(&skinfo->rtcp_addr_name, &mapped_addr[1], sizeof(pj_sockaddr));

	return PJ_SUCCESS;
		
on_error:
	pj_sock_close(sock[0]);
	pj_sock_close(sock[1]);
	return status;
}
#	endif
/* Check if sound device is idle. */
/*ycw-pjsip-delete sound device*/
#if 0
static void check_snd_dev_idle()
{
    unsigned call_cnt;

    /* Get the call count, we shouldn't close the sound device when there is
     * any calls active.
     */
    call_cnt = pjsua_call_get_count();

    /* When this function is called from pjsua_media_channel_deinit() upon
     * disconnecting call, actually the call count hasn't been updated/
     * decreased. So we put additional check here, if there is only one
     * call and it's in DISCONNECTED state, there is actually no active
     * call.
     */
    if (call_cnt == 1) {
	pjsua_call_id call_id;
	pj_status_t status;

	status = pjsua_enum_calls(&call_id, &call_cnt);
	if (status == PJ_SUCCESS && call_cnt > 0 &&
	    !pjsua_call_is_active(call_id))
	{
	    call_cnt = 0;
	}
    }

    /* Activate sound device auto-close timer if sound device is idle.
     * It is idle when there is no port connection in the bridge and
     * there is no active call.
     */
    if ((pjsua_var.snd_port!=NULL || pjsua_var.null_snd!=NULL) && 
	pjsua_var.snd_idle_timer.id == PJ_FALSE &&
	pjmedia_conf_get_connect_count(pjsua_var.mconf) == 0 &&
	call_cnt == 0 &&
	pjsua_var.media_cfg.snd_auto_close_time >= 0)
    {
	pj_time_val delay;

	delay.msec = 0;
	delay.sec = pjsua_var.media_cfg.snd_auto_close_time;

	pjsua_var.snd_idle_timer.id = PJ_TRUE;
	pjsip_endpt_schedule_timer(pjsua_var.endpt, &pjsua_var.snd_idle_timer, 
				   &delay);
    }
}

/* Timer callback to close sound device */
static void close_snd_timer_cb( pj_timer_heap_t *th,
				pj_timer_entry *entry)
{
    PJ_UNUSED_ARG(th);

    PJSUA_LOCK();
    if (entry->id) {
	PJ_LOG(4,(THIS_FILE,"Closing sound device after idle for %d seconds", 
		  pjsua_var.media_cfg.snd_auto_close_time));

	entry->id = PJ_FALSE;

	close_snd_dev();
    }
    PJSUA_UNLOCK();
}
#endif


/*
 * Start pjsua media subsystem.
 */
pj_status_t pjsua_media_subsys_start(void)
{
    pj_status_t status;

    /* Create media for calls, if none is specified */
    if (pjsua_var.calls[0].med_tp == NULL)
	 {
		pjsua_transport_config transport_cfg;

		/* Create default transport config */
		pjsua_transport_config_default(&transport_cfg);
		transport_cfg.port = DEFAULT_RTP_PORT;

		status = pjsua_media_transports_create(&transport_cfg);
		if (status != PJ_SUCCESS)
		{
	   	return status;
		}
    }

	/*ycw-pjsip-delete sound device*/
	#if 0
    pj_timer_entry_init(&pjsua_var.snd_idle_timer, PJ_FALSE, NULL, 
			&close_snd_timer_cb);
	#endif

#if SUPPORT_STUN
    /* Perform NAT detection */
    pjsua_detect_nat_type();
#endif /* SUPPORT_STUN */

    return PJ_SUCCESS;
}


/*
 * Destroy pjsua media subsystem.
 */
pj_status_t pjsua_media_subsys_destroy(void)
{
    unsigned i;

    PJ_LOG(4,(THIS_FILE, "Shutting down media.."));

	/* bugfix#1658 Stop media endpoint's worker threads first when destroying media subsystem */
	if (pjsua_var.med_endpt) {
		/* Wait for media endpoint's worker threads to quit. */
		pjmedia_endpt_stop_threads(pjsua_var.med_endpt);
	}

	/*ycw-pjsip-delete sound device*/
	#if 0
    close_snd_dev();
	#endif

	/*ycw-pjsip-delete conference bridge*/
	#if 0
    if (pjsua_var.mconf) {
	pjmedia_conf_destroy(pjsua_var.mconf);
	pjsua_var.mconf = NULL;
    }
	#endif

/*ycw-pjsip--delete null port*/
#if 0
    if (pjsua_var.null_port) {
	pjmedia_port_destroy(pjsua_var.null_port);
	pjsua_var.null_port = NULL;
    }
#endif

	/*ycw-pjsip-delete media port*/
#if 0
    /* Destroy file players */
    for (i=0; i<PJ_ARRAY_SIZE(pjsua_var.player); ++i) {
	if (pjsua_var.player[i].port) {
	    pjmedia_port_destroy(pjsua_var.player[i].port);
	    pjsua_var.player[i].port = NULL;
	}
    }

    /* Destroy file recorders */
    for (i=0; i<PJ_ARRAY_SIZE(pjsua_var.recorder); ++i) {
	if (pjsua_var.recorder[i].port) {
	    pjmedia_port_destroy(pjsua_var.recorder[i].port);
	    pjsua_var.recorder[i].port = NULL;
	}
    }
#endif
    /* Close media transports */
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i)
	{
		if (pjsua_var.calls[i].med_tp_st != PJSUA_MED_TP_IDLE)
		{
	   	 pjsua_media_channel_deinit(i);
		}
		if (pjsua_var.calls[i].med_tp && pjsua_var.calls[i].med_tp_auto_del)
		{
	   	pjmedia_transport_close(pjsua_var.calls[i].med_tp);
		}
		pjsua_var.calls[i].med_tp = NULL;
    }

    /* Destroy media endpoint. */
    if (pjsua_var.med_endpt) {

	/* Shutdown all codecs: */
#	if PJMEDIA_HAS_SPEEX_CODEC
	    pjmedia_codec_speex_deinit();
#	endif /* PJMEDIA_HAS_SPEEX_CODEC */

#	if PJMEDIA_HAS_GSM_CODEC
	    pjmedia_codec_gsm_deinit();
#	endif /* PJMEDIA_HAS_GSM_CODEC */

#	if PJMEDIA_HAS_G711_CODEC
	    pjmedia_codec_g711_deinit();
#	endif	/* PJMEDIA_HAS_G711_CODEC */

#	if PJMEDIA_HAS_G722_CODEC
	    pjmedia_codec_g722_deinit();
#	endif	/* PJMEDIA_HAS_G722_CODEC */

#	if PJMEDIA_HAS_INTEL_IPP
	    pjmedia_codec_ipp_deinit();
#	endif	/* PJMEDIA_HAS_INTEL_IPP */

#	if PJMEDIA_HAS_PASSTHROUGH_CODECS
	    pjmedia_codec_passthrough_deinit();
#	endif /* PJMEDIA_HAS_PASSTHROUGH_CODECS */

#	if PJMEDIA_HAS_G7221_CODEC
	    pjmedia_codec_g7221_deinit();
#	endif /* PJMEDIA_HAS_G7221_CODEC */

#	if PJMEDIA_HAS_L16_CODEC
	    pjmedia_codec_l16_deinit();
#	endif	/* PJMEDIA_HAS_L16_CODEC */

/*ycw-pjsip. Add the codecs deinit of G726-31/G729/T38*/
#if PJMEDIA_HAS_T38_CODEC
		pjmedia_codec_t38_deinit();
#endif

#if PJMEDIA_HAS_G726_CODEC
		pjmedia_codec_g726_32_deinit();
#endif

#if PJMEDIA_HAS_G721_CODEC
	pjmedia_codec_g721_deinit();
#endif

#if PJMEDIA_HAS_G729_CODEC
		pjmedia_codec_g729_deinit();
#endif


	pjmedia_endpt_destroy(pjsua_var.med_endpt);
	pjsua_var.med_endpt = NULL;

	/* Deinitialize sound subsystem */
	// Not necessary, as pjmedia_snd_deinit() should have been called
	// in pjmedia_endpt_destroy().
	//pjmedia_snd_deinit();
    }

    /* Reset RTP port */
    next_rtp_port = 0;

    return PJ_SUCCESS;
}


# if (defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0)||\
		((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
			defined(INCLUDE_PSTN_GATEWAY))
/* 
 * brief	ycw-pjsip. create local rtp/rtcp socket interactive with DSP.
 *	we need not create ICE version of this function, because this media 
 *	port only communicate with dsp, not network!
 */
 /*ycw-pjsip:create transit media transport for DSP*/
static pj_status_t create_dsp_media_transports(pjsua_transport_config* cfg)
{
	int i;
	pjmedia_sock_info skinfo;
	pj_status_t status;

	for (i = 0; i < pjsua_var.ua_cfg.max_calls; ++i)
	{
		status = create_dsp_rtp_rtcp_socket(cfg, &skinfo);
		if (status != PJ_SUCCESS)
		{
			pjsua_perror(THIS_FILE, "Unable to create RTP/RTCP socket", status);
			goto on_error;
		}

		status = pjmedia_dsp_transport_udp_attach(pjsua_var.med_endpt, NULL, 
			&skinfo, 0, &pjsua_var.calls[i].dsp_med_tp);
		if (status != PJ_SUCCESS)
		{
			pjsua_perror(THIS_FILE, "Unable to create media transport", status);
			goto on_error;
		}
	}

	PJ_LOG(4, (THIS_FILE, "create dsp media transport successfully!"));
	return PJ_SUCCESS;
	
on_error:
	for (i = 0; i < pjsua_var.ua_cfg.max_calls; ++i)
	{
		if (pjsua_var.calls[i].dsp_med_tp != NULL)
		{
			pjmedia_transport_close(pjsua_var.calls[i].dsp_med_tp);
			pjsua_var.calls[i].dsp_med_tp = NULL;
		}
	}
	
	return status;
}
#	endif

/* Create normal UDP media transports */
static pj_status_t create_udp_media_transports(pjsua_transport_config *cfg)
{
    unsigned i;
    pjmedia_sock_info skinfo;
    pj_status_t status;

    /* Create each media transport */
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i)
	 {
		status = create_rtp_rtcp_sock(cfg, &skinfo);
		if (status != PJ_SUCCESS)
		{
		   	pjsua_perror(THIS_FILE, "Unable to create RTP/RTCP socket",
			         status);
		   	goto on_error;
		}

		status = pjmedia_transport_udp_attach(pjsua_var.med_endpt, NULL,
					      &skinfo, 0,
					      &pjsua_var.calls[i].med_tp);

		if (status != PJ_SUCCESS)
		{
		   	pjsua_perror(THIS_FILE, "Unable to create media transport",
			         status);
		   	goto on_error;
		}

		pjmedia_transport_simulate_lost(pjsua_var.calls[i].med_tp,
					PJMEDIA_DIR_ENCODING,
					pjsua_var.media_cfg.tx_drop_pct);

		pjmedia_transport_simulate_lost(pjsua_var.calls[i].med_tp,
					PJMEDIA_DIR_DECODING,
					pjsua_var.media_cfg.rx_drop_pct);

    }

    return PJ_SUCCESS;

on_error:
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i)
	{
		if (pjsua_var.calls[i].med_tp != NULL)
		{
		   	pjmedia_transport_close(pjsua_var.calls[i].med_tp);
		   	pjsua_var.calls[i].med_tp = NULL;
		}
    }

    return status;
}

#if	PJSUA_ADD_ICE_TAGS
/* This callback is called when ICE negotiation completes */
static void on_ice_complete(pjmedia_transport *tp, 
			    pj_ice_strans_op op,
			    pj_status_t result)
{
    unsigned id;
    pj_bool_t found = PJ_FALSE;

    /* Find call which has this media transport */
    PJSUA_LOCK();

    for (id=0; id<pjsua_var.ua_cfg.max_calls; ++id)
	{
		if (pjsua_var.calls[id].med_tp == tp ||
		    pjsua_var.calls[id].med_orig == tp) 
		{
		    found = PJ_TRUE;
		    break;
		}
    }

    PJSUA_UNLOCK();

    if (!found)
	return;

    switch (op) {
    case PJ_ICE_STRANS_OP_INIT:
	pjsua_var.calls[id].med_tp_ready = result;
	break;
    case PJ_ICE_STRANS_OP_NEGOTIATION:
	if (result != PJ_SUCCESS) {
	    pjsua_var.calls[id].media_st = PJSUA_CALL_MEDIA_ERROR;
	    pjsua_var.calls[id].media_dir = PJMEDIA_DIR_NONE;

	    if (pjsua_var.ua_cfg.cb.on_call_media_state) {
		pjsua_var.ua_cfg.cb.on_call_media_state(id);
	    }
	} else {
	    /* Send UPDATE if default transport address is different than
	     * what was advertised (ticket #881)
	     */
	    pjmedia_transport_info tpinfo;
	    pjmedia_ice_transport_info *ii = NULL;
	    unsigned i;

	    pjmedia_transport_info_init(&tpinfo);
	    pjmedia_transport_get_info(tp, &tpinfo);
	    for (i=0; i<tpinfo.specific_info_cnt; ++i) {
		if (tpinfo.spc_info[i].type==PJMEDIA_TRANSPORT_TYPE_ICE) {
		    ii = (pjmedia_ice_transport_info*)
			 tpinfo.spc_info[i].buffer;
		    break;
		}
	    }

	    if (ii && ii->role==PJ_ICE_SESS_ROLE_CONTROLLING &&
		pj_sockaddr_cmp(&tpinfo.sock_info.rtp_addr_name,
				&pjsua_var.calls[id].med_rtp_addr))
	    {
		pj_bool_t use_update;
		const pj_str_t STR_UPDATE = { "UPDATE", 6 };
		pjsip_dialog_cap_status support_update;
		pjsip_dialog *dlg;

		dlg = pjsua_var.calls[id].inv->dlg;
		support_update = pjsip_dlg_remote_has_cap(dlg, PJSIP_H_ALLOW,
							  NULL, &STR_UPDATE);
		use_update = (support_update == PJSIP_DIALOG_CAP_SUPPORTED);

		PJ_LOG(4,(THIS_FILE, 
		          "ICE default transport address has changed for "
			  "call %d, sending %s", id,
			  (use_update ? "UPDATE" : "re-INVITE")));

		if (use_update)
		    pjsua_call_update(id, 0, NULL);
		else
		    pjsua_call_reinvite(id, 0, NULL);
	    }
	}
	break;
    case PJ_ICE_STRANS_OP_KEEP_ALIVE:
	if (result != PJ_SUCCESS) {
	    PJ_PERROR(4,(THIS_FILE, result,
		         "ICE keep alive failure for transport %d", id));
	}
	if (pjsua_var.ua_cfg.cb.on_ice_transport_error) {
	    (*pjsua_var.ua_cfg.cb.on_ice_transport_error)(id, op, result,
							  NULL);
	}
	break;
    }
}



/* Parse "HOST:PORT" format */
static pj_status_t parse_host_port(const pj_str_t *host_port,
				   pj_str_t *host, pj_uint16_t *port)
{
    pj_str_t str_port;

    str_port.ptr = pj_strchr(host_port, ':');
    if (str_port.ptr != NULL) {
	int iport;

	host->ptr = host_port->ptr;
	host->slen = (str_port.ptr - host->ptr);
	str_port.ptr++;
	str_port.slen = host_port->slen - host->slen - 1;
	iport = (int)pj_strtoul(&str_port);
	if (iport < 1 || iport > 65535)
	    return PJ_EINVAL;
	*port = (pj_uint16_t)iport;
    } else {
	*host = *host_port;
	*port = 0;
    }

    return PJ_SUCCESS;
}

/* Create ICE media transports (when ice is enabled) */
static pj_status_t create_ice_media_transports(pjsua_transport_config *cfg)
{
    char stunip[PJ_INET6_ADDRSTRLEN];
    pj_ice_strans_cfg ice_cfg;
    unsigned i;
    pj_status_t status;

    /* Make sure STUN server resolution has completed */
    status = resolve_stun_server(PJ_TRUE);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Error resolving STUN server", status);
	return status;
    }

    /* Create ICE stream transport configuration */
    pj_ice_strans_cfg_default(&ice_cfg);
    pj_stun_config_init(&ice_cfg.stun_cfg, &pjsua_var.cp.factory, 0,
		        pjsip_endpt_get_ioqueue(pjsua_var.endpt),
			pjsip_endpt_get_timer_heap(pjsua_var.endpt));
    
    ice_cfg.af = pj_AF_INET();
    ice_cfg.resolver = pjsua_var.resolver;
    
    ice_cfg.opt = pjsua_var.media_cfg.ice_opt;

    /* Configure STUN settings */
    if (pj_sockaddr_has_addr(&pjsua_var.stun_srv)) {
	pj_sockaddr_print(&pjsua_var.stun_srv, stunip, sizeof(stunip), 0);
	ice_cfg.stun.server = pj_str(stunip);
	ice_cfg.stun.port = pj_sockaddr_get_port(&pjsua_var.stun_srv);
    }
    if (pjsua_var.media_cfg.ice_max_host_cands >= 0)
	ice_cfg.stun.max_host_cands = pjsua_var.media_cfg.ice_max_host_cands;

    /* Copy QoS setting to STUN setting */
    ice_cfg.stun.cfg.qos_type = cfg->qos_type;
    pj_memcpy(&ice_cfg.stun.cfg.qos_params, &cfg->qos_params,
	      sizeof(cfg->qos_params));

    /* Configure TURN settings */
    if (pjsua_var.media_cfg.enable_turn) {
	status = parse_host_port(&pjsua_var.media_cfg.turn_server,
				 &ice_cfg.turn.server,
				 &ice_cfg.turn.port);
	if (status != PJ_SUCCESS || ice_cfg.turn.server.slen == 0) {
	    PJ_LOG(1,(THIS_FILE, "Invalid TURN server setting"));
	    return PJ_EINVAL;
	}
	if (ice_cfg.turn.port == 0)
	    ice_cfg.turn.port = 3479;
	ice_cfg.turn.conn_type = pjsua_var.media_cfg.turn_conn_type;
	pj_memcpy(&ice_cfg.turn.auth_cred, 
		  &pjsua_var.media_cfg.turn_auth_cred,
		  sizeof(ice_cfg.turn.auth_cred));

	/* Copy QoS setting to TURN setting */
	ice_cfg.turn.cfg.qos_type = cfg->qos_type;
	pj_memcpy(&ice_cfg.turn.cfg.qos_params, &cfg->qos_params,
		  sizeof(cfg->qos_params));
    }

    /* Create each media transport */
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i) {
	pjmedia_ice_cb ice_cb;
	char name[32];
	unsigned comp_cnt;

	pj_bzero(&ice_cb, sizeof(pjmedia_ice_cb));
	ice_cb.on_ice_complete = &on_ice_complete;
	pj_ansi_snprintf(name, sizeof(name), "icetp%02d", i);
	pjsua_var.calls[i].med_tp_ready = PJ_EPENDING;

	comp_cnt = 1;
	if (PJMEDIA_ADVERTISE_RTCP && !pjsua_var.media_cfg.ice_no_rtcp)
	    ++comp_cnt;

	status = pjmedia_ice_create(pjsua_var.med_endpt, name, comp_cnt,
				    &ice_cfg, &ice_cb,
				    &pjsua_var.calls[i].med_tp);
	if (status != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Unable to create ICE media transport",
		         status);
	    goto on_error;
	}

	/* Wait until transport is initialized, or time out */
	PJSUA_UNLOCK();
	while (pjsua_var.calls[i].med_tp_ready == PJ_EPENDING) {
	    pjsua_handle_events(100);
	}
	PJSUA_LOCK();
	if (pjsua_var.calls[i].med_tp_ready != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Error initializing ICE media transport",
		         pjsua_var.calls[i].med_tp_ready);
	    status = pjsua_var.calls[i].med_tp_ready;
	    goto on_error;
	}

	pjmedia_transport_simulate_lost(pjsua_var.calls[i].med_tp,
				        PJMEDIA_DIR_ENCODING,
				        pjsua_var.media_cfg.tx_drop_pct);

	pjmedia_transport_simulate_lost(pjsua_var.calls[i].med_tp,
				        PJMEDIA_DIR_DECODING,
				        pjsua_var.media_cfg.rx_drop_pct);
    }

    return PJ_SUCCESS;

on_error:
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i) {
	if (pjsua_var.calls[i].med_tp != NULL) {
	    pjmedia_transport_close(pjsua_var.calls[i].med_tp);
	    pjsua_var.calls[i].med_tp = NULL;
	}
    }

    return status;
}
#endif

/*
 * Create UDP media transports for all the calls. This function creates
 * one UDP media transport for each call.
 */
pj_status_t pjsua_media_transports_create(
			const pjsua_transport_config *app_cfg)
{
    pjsua_transport_config cfg;
    unsigned i;
    pj_status_t status;


    /* Make sure pjsua_init() has been called */
    PJ_ASSERT_RETURN(pjsua_var.ua_cfg.max_calls>0, PJ_EINVALIDOP);
    PJSUA_LOCK();

    /* Delete existing media transports */
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i)
	{
		if (pjsua_var.calls[i].med_tp != NULL && 
	   	 pjsua_var.calls[i].med_tp_auto_del) 
		{
	   	 pjmedia_transport_close(pjsua_var.calls[i].med_tp);
	   	 pjsua_var.calls[i].med_tp = NULL;
	   	 pjsua_var.calls[i].med_orig = NULL;
		}
    }

    /* Copy config */
    pjsua_transport_config_dup(pjsua_var.pool, &cfg, app_cfg);

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP==0
#	if defined(INCLUDE_USB_VOICEMAIL) || defined(INCLUDE_PSTN_GATEWAY)
	pjsua_transport_config_dup(pjsua_var.pool, &pjsua_var.medTpCfg, app_cfg);
#	endif
#	endif

    /* Create the transports */
#if	PJSUA_ADD_ICE_TAGS
    if (pjsua_var.media_cfg.enable_ice)
	{
		status = create_ice_media_transports(&cfg);
    }
	else
#endif
	{
		status = create_udp_media_transports(&cfg);
    }

#	if (defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
				defined(INCLUDE_PSTN_GATEWAY))
	 status = create_dsp_media_transports(&cfg);
#	endif

    /* Set media transport auto_delete to True */
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i)
	 {
		pjsua_var.calls[i].med_tp_auto_del = PJ_TRUE;
    }

    PJSUA_UNLOCK();
    return status;
}

/*
 * Attach application's created media transports.
 */
pj_status_t pjsua_media_transports_attach(pjsua_media_transport tp[],
						  unsigned count,
						  pj_bool_t auto_delete)
{
    unsigned i;

    PJ_ASSERT_RETURN(tp && count==pjsua_var.ua_cfg.max_calls, PJ_EINVAL);

    /* Assign the media transports */
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i) {
	if (pjsua_var.calls[i].med_tp != NULL && 
	    pjsua_var.calls[i].med_tp_auto_del) 
	{
	    pjmedia_transport_close(pjsua_var.calls[i].med_tp);
	}

	pjsua_var.calls[i].med_tp = tp[i].transport;
	pjsua_var.calls[i].med_tp_auto_del = auto_delete;
    }

    return PJ_SUCCESS;
}

/*ycw-pjsip.假如有多个audio、video或image等多个"m="行且不以prefer_srtp为选择依据，
选择的是第一个。这样子将会是是错误的。*/
int find_audio_index(const pjmedia_sdp_session *sdp, 
			    pj_bool_t prefer_srtp)
{
    unsigned i;
    int audio_idx = -1;

	 CMSIP_ASSERT(sdp!=NULL);
	CMSIP_PRINT("--------sdp media count(%d)------------", sdp->media_count);
    for (i=0; i<sdp->media_count; ++i)
	 {
		const pjmedia_sdp_media *m = sdp->media[i];

		/* Skip if media is not audio */
		if (pj_stricmp2(&m->desc.media, "audio") != 0 &&
				pj_stricmp2(&m->desc.media, "image") != 0)/*ycw-pjsip. t38*/
	   	continue;

		/* Skip if media is disabled */
		if (m->desc.port == 0)
		    continue;

		/* Skip if transport is not supported */
		if (pj_stricmp2(&m->desc.transport, "RTP/AVP") != 0 &&
		    pj_stricmp2(&m->desc.transport, "RTP/SAVP") != 0 &&
		    pj_stricmp2(&m->desc.transport, "udptl") != 0)/*ycw-pjsip. t38*/
		{
		    continue;
		}

		if (audio_idx == -1)
		{
		    audio_idx = i;
		}
		else
		{
	    	/* We've found multiple candidates. This could happen
	     	* e.g. when remote is offering both RTP/SAVP and RTP/AVP,
	     	* or when remote for some reason offers two audio.
	     	*/

		    if (prefer_srtp && pj_stricmp2(&m->desc.transport, "RTP/SAVP")==0)
		    {
				/* Prefer RTP/SAVP when our media transport is SRTP */
				audio_idx = i;
				break;
		    }
			 else if (!prefer_srtp && pj_stricmp2(&m->desc.transport, "RTP/AVP")==0)
		    {
				/* Prefer RTP/AVP when our media transport is NOT SRTP */
				audio_idx = i;
		    }
		}
    }

    return audio_idx;
}


pj_status_t pjsua_media_channel_init(pjsua_call_id call_id,
				     pjsip_role_e role,
				     int security_level,
				     pj_pool_t *tmp_pool,
				     const pjmedia_sdp_session *rem_sdp,
				     int *sip_err_code)
{
    pjsua_call *call = &pjsua_var.calls[call_id];
    pj_status_t status;

/** ycw-pjsip-note
* 因为只有定义SRTP的时候才会用到账号索引，而如果是在没有账号时播出IP呼叫则没有账号索引，这是call
* 对象的acc_id成员的值为-1，所以涉及到该成员的地方都需要修改!!!
* yuchuwei,20120229--今天是个特殊的日子，纪念一下!
*/
#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    pjsua_acc *acc = NULL;
    pjmedia_srtp_setting srtp_opt;
    pjmedia_transport *srtp = NULL;
#endif

    PJ_UNUSED_ARG(role);
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT==0
	CMSIP_ASSERT(call->acc_id>=0);
#	endif

#	if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	if (call->acc_id >= 0)
	{
#	endif
		acc = &pjsua_var.acc[call->acc_id];
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	}
#	endif
#	endif

    /* Return error if media transport has not been created yet
     * (e.g. application is starting)
     */
    if (call->med_tp == NULL)
	 {
		if (sip_err_code)
	    {
		   	*sip_err_code = PJSIP_SC_INTERNAL_SERVER_ERROR;
		}
		return PJ_EBUSY;
    }

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    /* This function may be called when SRTP transport already exists 
     * (e.g: in re-invite, update), don't need to destroy/re-create.
     */
    if (!call->med_orig || call->med_tp == call->med_orig)
	{
		/* Check if SRTP requires secure signaling */
		if (
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
			acc &&
#	endif
			acc->cfg.use_srtp != PJMEDIA_SRTP_DISABLED
		)
		{
		   	if (security_level < acc->cfg.srtp_secure_signaling)
			{
				if (sip_err_code)
				    *sip_err_code = PJSIP_SC_NOT_ACCEPTABLE;
				return PJSIP_ESESSIONINSECURE;
		    }
		}

		/* Always create SRTP adapter */
		pjmedia_srtp_setting_default(&srtp_opt);
		srtp_opt.close_member_tp = PJ_FALSE;
		/* If media session has been ever established, let's use remote's 
		 * preference in SRTP usage policy, especially when it is stricter.
		 */
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
		if (acc)
		{
#	endif
			if (call->rem_srtp_use > acc->cfg.use_srtp)
			    srtp_opt.use = call->rem_srtp_use;
			else
			    srtp_opt.use = acc->cfg.use_srtp;
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
		}
#	endif

		status = pjmedia_transport_srtp_create(pjsua_var.med_endpt, 
						       call->med_tp,
						       &srtp_opt, &srtp);
		if (status != PJ_SUCCESS)
		{
		    if (sip_err_code)
				*sip_err_code = PJSIP_SC_INTERNAL_SERVER_ERROR;
		    return status;
		}

		/* Set SRTP as current media transport */
		call->med_orig = call->med_tp;
		call->med_tp = srtp;
    }
#else
    call->med_orig = call->med_tp;
    PJ_UNUSED_ARG(security_level);
#endif

    /* Find out which media line in SDP that we support. If we are offerer,
     * audio will be initialized at index 0 in SDP. 
     */
    if (rem_sdp == NULL)
	{
		call->audio_idx = 0;
    } 
    /* Otherwise find out the candidate audio media line in SDP */
    else
	{
		pj_bool_t srtp_active;

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
		if (acc)
		{
#	endif
			srtp_active = acc->cfg.use_srtp;
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
		}
		else
		{
			srtp_active = PJ_FALSE;
		}
#	endif

#else
		srtp_active = PJ_FALSE;
#endif

		/* Media count must have been checked */
		pj_assert(rem_sdp->media_count != 0);

		call->audio_idx = find_audio_index(rem_sdp, srtp_active);
    }

    /* Reject offer if we couldn't find a good m=audio line in offer */
    if (call->audio_idx < 0)
	 {
		if (sip_err_code)
		{
			*sip_err_code = PJSIP_SC_NOT_ACCEPTABLE_HERE;
		}
		pjsua_media_channel_deinit(call_id);
		return PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_NOT_ACCEPTABLE_HERE);
    }

    PJ_LOG(4,(THIS_FILE, "Media index %d selected for call %d",
	      call->audio_idx, call->index));

    /* Create the media transport */
    status = pjmedia_transport_media_create(call->med_tp, tmp_pool, 0,
					    rem_sdp, call->audio_idx);
    if (status != PJ_SUCCESS)
	 {
		if (sip_err_code)
		{
			*sip_err_code = PJSIP_SC_NOT_ACCEPTABLE;
		}
		pjsua_media_channel_deinit(call_id);
		return status;
    }

    call->med_tp_st = PJSUA_MED_TP_INIT;
    return PJ_SUCCESS;
}

pj_status_t pjsua_media_channel_create_sdp(pjsua_call_id call_id, 
					   pj_pool_t *pool,
					   const pjmedia_sdp_session *rem_sdp,
					   pjmedia_sdp_session **p_sdp,
					   int *sip_status_code,
					   pj_bool_t createFwRule)
{
    enum { MAX_MEDIA = 1 };
    pjmedia_sdp_session *sdp;
    pjmedia_transport_info tpinfo;
    pjsua_call *call = &pjsua_var.calls[call_id];
    pjmedia_sdp_neg_state sdp_neg_state = PJMEDIA_SDP_NEG_STATE_NULL;
    pj_status_t status;
	/*ycw-pjsip-codec*/
	#if 1
	pjsua_acc* acc = NULL;
	unsigned i;
	pj_str_t* codec = NULL;
	pjsua_codec_info c[32];
	unsigned count = PJ_ARRAY_SIZE(c);/*ycw-pjsip. Must init, or there would be a error.*/
	#endif
	pj_bool_t offer_has_telephone_evt = PJ_FALSE;
#	ifdef SUPPORT_METABOLIC_MEDIA_PORT
	pj_uint16_t	medport;
	pj_bool_t portAvaible = PJ_TRUE;
#	endif
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	char localIp[CMSIP_STR_16];
	pj_uint16_t localPort;
	pj_uint16_t localRtcpPort;
	pj_str_t ip;
	PJ_FIREWALL_RULE* pFwRule = NULL;
#endif

#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	if (call->acc_id>=0)
	{
#	endif
		acc = &pjsua_var.acc[call->acc_id];
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	}
#	endif

	 CMSIP_PRINT("--call[%d],account[%d], rem_sdp[%p]", call_id, call->acc_id, rem_sdp);

    /* Return error if media transport has not been created yet
     * (e.g. application is starting)
     */
    if (call->med_tp == NULL)
	 {
		return PJ_EBUSY;
    }

	 /*ycw-pjsip-codec*/
	 if (
#	if defined(INCLUDE_PSTN_GATEWAY) 		
	 	(PJ_TRUE == call->isPstn)
#	else
		0
#	endif
		||
#	if (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
		(PJ_TRUE == call->isFaxPassthrough)
#	else
		0
#	endif
	 	||
#	if (defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
			defined(INCLUDE_USB_VOICEMAIL)
	 	PJ_TRUE == call->isUsbVm
#	else
		0
#	endif
		||
#	if (defined(NUM_VOICEAPP_CHANNELS) && 0!=NUM_VOICEAPP_CHANNELS)
	 	PJ_TRUE == call->isVoiceapp
#	else
		0
#	endif
	 	)
	 {
#	if defined(INCLUDE_PSTN_GATEWAY) || \
			((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP)&& defined(INCLUDE_USB_VOICEMAIL))\
		 || (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0) || (defined(NUM_VOICEAPP_CHANNELS) && 0!=NUM_VOICEAPP_CHANNELS)
	 	pjsua_codec_mgr_switch_to_lite();
#	endif
	 }
	 else
	 {
	 	pjsua_codec_mgr_switch_to_full();
	 }	
	 
	 pjsua_enum_codecs(c, &count);
	 for(i = 0; i < count; ++i)
	 {
	 	codec = &c[i].codec_id;
		status = pjsua_codec_set_priority(codec, PJMEDIA_CODEC_PRIO_NORMAL);
		if (status != PJ_SUCCESS)
		{
			pjsua_perror(THIS_FILE, "Error reset codecs' priority!", status);
			return status;
		}
	 }
	 
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	if (acc)
	{
#	endif
		CMSIP_PRINT("------------haha,I can use account(%d)---", acc->index);
	 for (i = 0, count = 0; i < acc->cfg.codec_cnt; ++i)
	 {	 	
	 	codec = &acc->cfg.codecs[i];

		if (PJ_TRUE == pjsua_codec_mgr_codec_is_exist(codec))
		{
			status = pjsua_codec_set_priority(codec, PJMEDIA_CODEC_PRIO_HIGHEST - i);
			if (status != PJ_SUCCESS)
			{
				pjsua_perror(THIS_FILE, "Error reset codecs' priority!", status);
				return status;
			}
			++count;
		}
	 }	 
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	}
#	endif

    if (rem_sdp && rem_sdp->media_count != 0)
	{
		pj_bool_t srtp_active;

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	if (acc)
	{
#	endif
      srtp_active = pjsua_var.acc[call->acc_id].cfg.use_srtp;
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
	}
	else
	{
		srtp_active = PJ_FALSE;
	}
#	endif
#else
		srtp_active = PJ_FALSE;
#endif

		call->audio_idx = find_audio_index(rem_sdp, srtp_active);
		if (call->audio_idx == -1)
		{
	   	/* No audio in the offer. We can't accept this */
	   	PJ_LOG(4,(THIS_FILE,
		      "Unable to accept SDP offer without audio for call %d",
		      call_id));
	    	return PJMEDIA_SDP_EINMEDIA;
		}

		CMSIP_PRINT("---select Media Index[%d] for call[%d]", call->audio_idx, call->index);
    }

    /* Media index must have been determined before */
    pj_assert(call->audio_idx != -1);

    /* Create media if it's not created. This could happen when call is
     * currently on-hold
     */
    if (call->med_tp_st == PJSUA_MED_TP_IDLE)
	{
	 	/*ycw-pjsip:如果需要动态创建传输，可以在这儿实现*/
		pjsip_role_e role;
		role = (rem_sdp ? PJSIP_ROLE_UAS : PJSIP_ROLE_UAC);
		status = pjsua_media_channel_init(call_id, role, call->secure_level, 
					  pool, rem_sdp, sip_status_code);
		if (status != PJ_SUCCESS)
	   	{
	   		return status;
		}
    }

    /* Get SDP negotiator state */
    if (call->inv && call->inv->neg)
    {
		sdp_neg_state = pjmedia_sdp_neg_get_state(call->inv->neg);
    }

    /* Get media socket info */
    pjmedia_transport_info_init(&tpinfo);
	
#	ifdef SUPPORT_METABOLIC_MEDIA_PORT
	for (i = 0; i < PJMEDIA_PORT_TEST_COUNT; ++i)
	{
		medport = pjmedia_endpt_get_mediaPort(pjsua_var.med_endpt);
		CMSIP_PRINT("-------Media port is (%d)----", medport);
		pjmedia_endpt_set_mediaPort(pjsua_var.med_endpt);
		/*test RTP media port*/
		status = pjmedia_endpt_test_media_port(pjsua_var.BoundIp, medport, &portAvaible);
		if (PJ_FALSE==portAvaible && EADDRINUSE == errno)
		{
			CMSIP_PRINT("----address is already used---------");
			errno = 0;
			continue;
		}
		else if (PJ_SUCCESS != status)
		{
			CMSIP_PRINT("--------test media port error----------");
			return status;
		}

		/*test RTCP media port*/
		status = pjmedia_endpt_test_media_port(pjsua_var.BoundIp, (medport+1), &portAvaible);
		if (PJ_FALSE==portAvaible && EADDRINUSE == errno)
		{
			CMSIP_PRINT("-------address is already used-------");
			errno = 0;
			continue;
		}
		else if (PJ_SUCCESS != status)
		{
			CMSIP_PRINT("----------test media port error-----------");
			return status;
		}
		else
		{
			break;
		}
	}

	if (i >= PJMEDIA_PORT_TEST_COUNT) 
	{
		CMSIP_PRINT("--------test media port error----------");
		return status;
	}
	pjmedia_transport_udp_set_mediaPort(call->med_tp, medport);	
#	endif
	
    pjmedia_transport_get_info(call->med_tp, &tpinfo);

#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	if (createFwRule)
	{
		for (i = 0; i < PJ_ARRAY_SIZE(call->fwRule); ++i)
		{
			CMSIP_PRINT("--------ready to delete iptables rule--------");
			pFwRule = &call->fwRule[i];
			if (pFwRule->destination.slen > 0 && pFwRule->dport > 0)
			{
				pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, PJ_TRANSPORT_UDP, 
					&pFwRule->destination, NULL, pFwRule->dport, 
					NULL, NULL, -1, pjsua_var.fwType);
		}
	}
	pjmedia_transport_udp_local_info(call->med_tp, localIp, &localPort, &localRtcpPort);
	ip = pj_str(localIp);
	CMSIP_PRINT("-----add iptables rule(dstIp(%s), localPort(%d), localRtcpPort(%d))",
		localIp, localPort, localRtcpPort);	
	/*开启DMZ，呼出时，假如对方的rtp先到达，会使得本来到达DUT VOIP的rtp数据包之后直接
	随DMZ的iptables规则转发，本地DUT VOIP无法接收到数据包。所以这里需要在发出INVITE之
	前就开启相应端口。by yuchuwei*/
	/*now, only support UDP*/
	status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_UDP, 
		&ip, NULL, localPort, NULL, NULL, -1, pjsua_var.fwType);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
		return status;
	}
	CMSIP_PRINT("=============add iptables rule end====================================");
	pFwRule = &call->fwRule[0];
	memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
	pFwRule->protocol = PJ_TRANSPORT_UDP;
	strcpy(pFwRule->dstBuf, localIp);
	pFwRule->destination = pj_str(pFwRule->dstBuf);
	pFwRule->dport = localPort;
	pFwRule->sport = -1;
	
	status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_UDP, 
		&ip, NULL, localRtcpPort, NULL, NULL, -1, pjsua_var.fwType);
	if (status != PJ_SUCCESS)
	{
		pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, PJ_TRANSPORT_UDP, 
		&ip, NULL, localPort, NULL, NULL, -1, pjsua_var.fwType);
		pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
		return status;
	}	
	CMSIP_PRINT("=============add iptables rule end====================================");
	pFwRule = &call->fwRule[1];
	memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
	pFwRule->protocol = PJ_TRANSPORT_UDP;
	strcpy(pFwRule->dstBuf, localIp);
	pFwRule->destination = pj_str(pFwRule->dstBuf);
	pFwRule->dport = localRtcpPort;
	pFwRule->sport = -1;
	}
#endif
    /* Create SDP */
	 /*ycw-pjsip.Current only one media*/
	 if (rem_sdp && rem_sdp->media_count > 0)
	 {
	 	int k;
	 	pjmedia_sdp_media* m = rem_sdp->media[0];
		pjmedia_sdp_attr* attr = NULL;
		pjmedia_sdp_rtpmap rtpmap;

		for (k = 0; k < m->desc.fmt_count; ++k)
		{
			attr = pjmedia_sdp_media_find_attr2(m, "rtpmap", &m->desc.fmt[k]);
			if (!attr) continue;
			pjmedia_sdp_attr_get_rtpmap(attr, &rtpmap);

			if (!pj_stricmp2(&rtpmap.enc_name, "telephone-event"))
			{
				offer_has_telephone_evt = PJ_TRUE;
			}
		}
	 }
	 /*ycw-pjsip-ptime*/
	 #if 0
    status = pjmedia_endpt_create_sdp(pjsua_var.med_endpt, pool, MAX_MEDIA,
				      &tpinfo.sock_info, &sdp);
	 #else
	 status = pjmedia_endpt_create_sdp(pjsua_var.med_endpt, pool, MAX_MEDIA,
				      &tpinfo.sock_info, 
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
				      acc ? acc->cfg.ptime : PJSUA_DEFAULT_LOCAL_PTIME, 
#	else
						acc->cfg.ptime,
#	endif
				      offer_has_telephone_evt, &sdp);
	 #endif
    if (status != PJ_SUCCESS)
	 {
		if (sip_status_code)
		{
			*sip_status_code = 500;
		}
		return status;
    }

    /* If we're answering or updating the session with a new offer,
     * and the selected media is not the first media
     * in SDP, then fill in the unselected media with with zero port. 
     * Otherwise we'll crash in transport_encode_sdp() because the media
     * lines are not aligned between offer and answer.
     */
    if (call->audio_idx != 0 && (rem_sdp || sdp_neg_state==PJMEDIA_SDP_NEG_STATE_DONE))
    {
		unsigned i;
		const pjmedia_sdp_session *ref_sdp = rem_sdp;

		if (!ref_sdp)
		{
		    /* We are updating session with a new offer */
		    status = pjmedia_sdp_neg_get_active_local(call->inv->neg, &ref_sdp);
		    pj_assert(status == PJ_SUCCESS);
		}

		CMSIP_PRINT("--ref sdp's media count[%d]", ref_sdp->media_count);
		for (i=0; i<ref_sdp->media_count; ++i)
		{
		   	const pjmedia_sdp_media *ref_m = ref_sdp->media[i];
		   	pjmedia_sdp_media *m;

		   	if ((int)i == call->audio_idx)
		   	{
					continue;
		   	}

	    	m = pjmedia_sdp_media_clone_deactivate(pool, ref_m);
	    	if (i==sdp->media_count)
	    	{
				sdp->media[sdp->media_count++] = m;
	    	}
	    	else
			{
				pj_array_insert(sdp->media, sizeof(sdp->media[0]),
						sdp->media_count, i, &m);
				++sdp->media_count;
	    	}
		}
    }
#if SUPPORT_STUN
    /* Add NAT info in the SDP */
    if (pjsua_var.ua_cfg.nat_type_in_sdp)
	 {
		pjmedia_sdp_attr *a;
		pj_str_t value;
		char nat_info[80];

		value.ptr = nat_info;
		if (pjsua_var.ua_cfg.nat_type_in_sdp == 1)
		{
	   	value.slen = pj_ansi_snprintf(nat_info, sizeof(nat_info),
					  "%d", pjsua_var.nat_type);
		}
		else
		{
	   	const char *type_name = pj_stun_get_nat_name(pjsua_var.nat_type);
	   	value.slen = pj_ansi_snprintf(nat_info, sizeof(nat_info),
					  "%d %s",
					  pjsua_var.nat_type,
					  type_name);
		}

		a = pjmedia_sdp_attr_create(pool, "X-nat", &value);

		pjmedia_sdp_attr_add(&sdp->attr_count, sdp->attr, a);

    }
#endif /* SUPPORT_STUN */
    /* Give the SDP to media transport */
    status = pjmedia_transport_encode_sdp(call->med_tp, pool, sdp, rem_sdp, 
					  call->audio_idx);
    if (status != PJ_SUCCESS)
	 {
		if (sip_status_code)
		{
			*sip_status_code = PJSIP_SC_NOT_ACCEPTABLE;
		}
		return status;
    }

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    /* Check if SRTP is in optional mode and configured to use duplicated
     * media, i.e: secured and unsecured version, in the SDP offer.
     */
    if (!rem_sdp && acc &&
	pjsua_var.acc[call->acc_id].cfg.use_srtp == PJMEDIA_SRTP_OPTIONAL &&
	pjsua_var.acc[call->acc_id].cfg.srtp_optional_dup_offer)
    {
		unsigned i;

	for (i = 0; i < sdp->media_count; ++i) {
	    pjmedia_sdp_media *m = sdp->media[i];

	    /* Check if this media is unsecured but has SDP "crypto" 
	     * attribute.
	     */
	    if (pj_stricmp2(&m->desc.transport, "RTP/AVP") == 0 &&
		pjmedia_sdp_media_find_attr2(m, "crypto", NULL) != NULL)
	    {
		if (i == (unsigned)call->audio_idx && 
		    sdp_neg_state == PJMEDIA_SDP_NEG_STATE_DONE)
		{
		    /* This is a session update, and peer has chosen the
		     * unsecured version, so let's make this unsecured too.
		     */
		    pjmedia_sdp_media_remove_all_attr(m, "crypto");
		} else {
		    /* This is new offer, duplicate media so we'll have
		     * secured (with "RTP/SAVP" transport) and and unsecured
		     * versions.
		     */
		    pjmedia_sdp_media *new_m;

		    /* Duplicate this media and apply secured transport */
		    new_m = pjmedia_sdp_media_clone(pool, m);
		    pj_strdup2(pool, &new_m->desc.transport, "RTP/SAVP");

		    /* Remove the "crypto" attribute in the unsecured media */
		    pjmedia_sdp_media_remove_all_attr(m, "crypto");

		    /* Insert the new media before the unsecured media */
		    if (sdp->media_count < PJMEDIA_MAX_SDP_MEDIA) {
			pj_array_insert(sdp->media, sizeof(new_m), 
					sdp->media_count, i, &new_m);
			++sdp->media_count;
			++i;
		    }
		}
	    }
	}
    }
#endif

    /* Update currently advertised RTP source address */
    pj_memcpy(&call->med_rtp_addr, &tpinfo.sock_info.rtp_addr_name, 
	      sizeof(pj_sockaddr));

    *p_sdp = sdp;
    return PJ_SUCCESS;
}

/*ycw-pjsip. t38*/
pj_status_t pjsua_media_channel_create_sdp_t38(pjsua_call_id call_id, 
										       pj_pool_t *pool,
										       const pjmedia_sdp_session *rem_sdp,
										       pjmedia_sdp_session **p_sdp,
										       int *sip_status_code,
					   pj_bool_t createFwRule)
{
	enum { MAX_MEDIA = 1 };
	pjmedia_sdp_session *sdp;
	pjmedia_transport_info tpinfo;
	pjsua_call *call = &pjsua_var.calls[call_id];
	pj_status_t status;
	int i;
#	ifdef SUPPORT_METABOLIC_MEDIA_PORT
	pj_uint16_t	medport;
	pj_bool_t portAvaible = PJ_FALSE;
#	endif
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	char localIp[CMSIP_STR_16];
	pj_uint16_t localPort;
	pj_uint16_t localRtcpPort;
	pj_str_t ip;
	PJ_FIREWALL_RULE* pFwRule = NULL;
#endif


	/* Return error if media transport has not been created yet
	* (e.g. application is starting)
	*/
	if (call->med_tp == NULL)
	{
		return PJ_EBUSY;
	}

	/* Media index must have been determined before */
	pj_assert(call->audio_idx != -1);

	/* Create media if it's not created. This could happen when call is
	* currently on-hold
	*/
	if (call->med_tp_st == PJSUA_MED_TP_IDLE)
	{
		pjsip_role_e role;
		role = (rem_sdp ? PJSIP_ROLE_UAS : PJSIP_ROLE_UAC);
		status = pjsua_media_channel_init(call_id, role, call->secure_level, 
			pool, rem_sdp, sip_status_code);
		if (status != PJ_SUCCESS)
		{
			return status;
		}
	}

	/* Get media socket info */
	pjmedia_transport_info_init(&tpinfo);
#	ifdef SUPPORT_METABOLIC_MEDIA_PORT
	for (i = 0; i < PJMEDIA_PORT_TEST_COUNT; ++i)
	{
		medport = pjmedia_endpt_get_mediaPort(pjsua_var.med_endpt);
		pjmedia_endpt_set_mediaPort(pjsua_var.med_endpt);
		/*test RTP media port*/
		status = pjmedia_endpt_test_media_port(pjsua_var.BoundIp, medport, &portAvaible);
		if (PJ_FALSE==portAvaible && EADDRINUSE == errno)
		{
			CMSIP_PRINT("-------address is already used-------");
			errno = 0;
			continue;
		}
		else if (PJ_SUCCESS != status)
		{
			CMSIP_PRINT("----------test media port error-----------");
			return status;
		}

		/*test RTCP media port*/
		status = pjmedia_endpt_test_media_port(pjsua_var.BoundIp, (medport+1), &portAvaible);
		if (PJ_FALSE==portAvaible && EADDRINUSE == errno)
		{
			CMSIP_PRINT("-------address is already used-------");
			errno = 0;
			continue;
		}
		else if (PJ_SUCCESS != status)
		{
			CMSIP_PRINT("----------test media port error-----------");
			return status;
		}
		else
		{
			break;
		}

	}

	if (i >= PJMEDIA_PORT_TEST_COUNT) 
	{
		return status;
	}
	pjmedia_transport_udp_set_mediaPort(call->med_tp, medport);	
#	endif

	pjmedia_transport_get_info(call->med_tp, &tpinfo);

#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	if (createFwRule)
	{
	for (i = 0; i < PJ_ARRAY_SIZE(call->fwRule); ++i)
	{
		CMSIP_PRINT("--------ready to delete iptables rule--------");
		pFwRule = &call->fwRule[i];
		if (pFwRule->destination.slen > 0 && pFwRule->dport > 0)
		{
			pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, PJ_TRANSPORT_UDP, 
				&pFwRule->destination, NULL, pFwRule->dport, 
				NULL, NULL, -1, pjsua_var.fwType);
		}
	}
	pjmedia_transport_udp_local_info(call->med_tp, localIp, &localPort, &localRtcpPort);
	ip = pj_str(localIp);
	CMSIP_PRINT("-----add iptables rule(dstIp(%s), localPort(%d), localRtcpPort(%d))",
		localIp, localPort, localRtcpPort);	
	/*开启DMZ，呼出时，假如对方的rtp先到达，会使得本来到达DUT VOIP的rtp数据包之后直接
	随DMZ的iptables规则转发，本地DUT VOIP无法接收到数据包。所以这里需要在发出INVITE之
	前就开启相应端口。by yuchuwei*/
	/*now, only support UDP*/
	status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_UDP, 
		&ip, NULL, localPort, NULL, NULL, -1, pjsua_var.fwType);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
		return status;
	}
	CMSIP_PRINT("=============add iptables rule end====================================");
	pFwRule = &call->fwRule[0];
	memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
	pFwRule->protocol = PJ_TRANSPORT_UDP;
	strcpy(pFwRule->dstBuf, localIp);
	pFwRule->destination = pj_str(pFwRule->dstBuf);
	pFwRule->dport = localPort;
	pFwRule->sport = -1;
	
	status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_UDP, 
		&ip, NULL, localRtcpPort, NULL, NULL, -1, pjsua_var.fwType);
	if (status != PJ_SUCCESS)
	{
		pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, PJ_TRANSPORT_UDP, 
		&ip, NULL, localPort, NULL, NULL, -1, pjsua_var.fwType);
		pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
		return status;
	}
	CMSIP_PRINT("=============add iptables rule end====================================");
	pFwRule = &call->fwRule[1];
	memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
	pFwRule->protocol = PJ_TRANSPORT_UDP;
	strcpy(pFwRule->dstBuf, localIp);
	pFwRule->destination = pj_str(pFwRule->dstBuf);
	pFwRule->dport = localRtcpPort;
	pFwRule->sport = -1;
	}
#endif


	/* Create SDP */
	status = pjmedia_endpt_create_sdp_t38(pjsua_var.med_endpt, pool, MAX_MEDIA,
		&tpinfo.sock_info, &sdp);
	if (status != PJ_SUCCESS)
	{
		if (sip_status_code)
		{
			*sip_status_code = 500;
		}
		return status;
	}

	/* If we're answering and the selected media is not the first media
	* in SDP, then fill in the unselected media with with zero port. 
	* Otherwise we'll crash in transport_encode_sdp() because the media
	* lines are not aligned between offer and answer.
	*/
	if (rem_sdp && call->audio_idx != 0)
	{
		unsigned i;

		for (i=0; i<rem_sdp->media_count; ++i)
		{
			const pjmedia_sdp_media *rem_m = rem_sdp->media[i];
			pjmedia_sdp_media *m;
			const pjmedia_sdp_attr *a;

			if ((int)i == call->audio_idx)
				continue;

			m = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_media);
			pj_strdup(pool, &m->desc.media, &rem_m->desc.media);
			pj_strdup(pool, &m->desc.transport, &rem_m->desc.transport);
			m->desc.port = 0;

			/* Add one format, copy from the offer. And copy the corresponding
			* rtpmap and fmtp attributes too.
			*/
			m->desc.fmt_count = 1;
			pj_strdup(pool, &m->desc.fmt[0], &rem_m->desc.fmt[0]);
			if ((a=pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr,
				"rtpmap", &m->desc.fmt[0])) != NULL)
			{
				m->attr[m->attr_count++] = pjmedia_sdp_attr_clone(pool, a);
			}
			if ((a=pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr,
				"fmtp", &m->desc.fmt[0])) != NULL)
			{
				m->attr[m->attr_count++] = pjmedia_sdp_attr_clone(pool, a);
			}

			if (i==sdp->media_count)
				sdp->media[sdp->media_count++] = m;
			else {
				pj_array_insert(sdp->media, sizeof(sdp->media[0]),
					sdp->media_count, i, &m);
				++sdp->media_count;
			}
		}
	}
#if SUPPORT_STUN
	/* Add NAT info in the SDP */
	if (pjsua_var.ua_cfg.nat_type_in_sdp)
	{
		pjmedia_sdp_attr *a;
		pj_str_t value;
		char nat_info[80];

		value.ptr = nat_info;
		if (pjsua_var.ua_cfg.nat_type_in_sdp == 1)
		{
			value.slen = pj_ansi_snprintf(nat_info, sizeof(nat_info),
				"%d", pjsua_var.nat_type);
		}
		else
		{
			const char *type_name = pj_stun_get_nat_name(pjsua_var.nat_type);
			value.slen = pj_ansi_snprintf(nat_info, sizeof(nat_info),
				"%d %s",
				pjsua_var.nat_type,
				type_name);
		}

		a = pjmedia_sdp_attr_create(pool, "X-nat", &value);

		pjmedia_sdp_attr_add(&sdp->attr_count, sdp->attr, a);

	}
#endif /* SUPPORT_STUN */
	/* Give the SDP to media transport */
	status = pjmedia_transport_encode_sdp(call->med_tp, pool, sdp, rem_sdp, call->audio_idx);
	if (status != PJ_SUCCESS)
	{
		if (sip_status_code) *sip_status_code = PJSIP_SC_NOT_ACCEPTABLE;
		return status;
	}

	/* Update currently advertised RTP source address */
	pj_memcpy(&call->med_rtp_addr, &tpinfo.sock_info.rtp_addr_name, sizeof(pj_sockaddr));

	*p_sdp = sdp;
	return PJ_SUCCESS;
}


static void stop_media_session(pjsua_call_id call_id)
{
	CMSIP_PRINT("--Now, stop media session-------");
/*ycw-pjsip:here*/
    pjsua_call *call = &pjsua_var.calls[call_id];

	/*ycw-pjsip-delete conference*/
	#if 0
    if (call->conf_slot != PJSUA_INVALID_ID) {
	if (pjsua_var.mconf) {
	    pjsua_conf_remove_port(call->conf_slot);
	}
	call->conf_slot = PJSUA_INVALID_ID;
    }
	#endif

	/*ycw-pjsip*/
	PJSUA_LOCK();

    if (call->session)
	 {
		/*ycw-pjsip.20111104*/
		#if 0
		pjmedia_rtcp_stat stat;

		if ((call->media_dir & PJMEDIA_DIR_ENCODING)
			#if 0
			&&
	    (pjmedia_session_get_stream_stat(call->session, 0, &stat) 
	     == PJ_SUCCESS)
	     #endif
		  )
		{
		    /* Save RTP timestamp & sequence, so when media session is 
		     * restarted, those values will be restored as the initial 
		     * RTP timestamp & sequence of the new media session. So in 
		     * the same call session, RTP timestamp and sequence are 
		     * guaranteed to be contigue.
		     */
		     #if 0
		    call->rtp_tx_seq_ts_set = 1 | (1 << 1);
		    call->rtp_tx_seq = stat.rtp_tx_last_seq;
		    call->rtp_tx_ts = stat.rtp_tx_last_ts;
			 #endif
		}
		#endif

		if (pjsua_var.ua_cfg.cb.on_stream_destroyed)
		{
		    pjsua_var.ua_cfg.cb.on_stream_destroyed(call_id, call->session, 0);
		}
		
		pjmedia_session_destroy(call->session, call_id, call->cmHangup
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
		, call->isUsbVm
#	endif
#	endif
#	if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
		,pjsua_var.fwType
#	endif
			);
		
		call->session = NULL;

		PJ_LOG(4,(THIS_FILE, "Media session for call %d is destroyed", 
				     call_id));

    }

    call->media_st = PJSUA_CALL_MEDIA_NONE;
	 
	 /*ycw-pjsip*/
	 PJSUA_UNLOCK();

	 CMSIP_PRINT("---stop media session. media state is none");
}

pj_status_t pjsua_media_channel_deinit(pjsua_call_id call_id)
{
    pjsua_call *call = &pjsua_var.calls[call_id];

    stop_media_session(call_id);

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (
#	if defined(INCLUDE_USB_VOICEMAIL)
		call->isUsbVm == PJ_TRUE
#	else
		0
#	endif
		||
#	if defined(INCLUDE_PSTN_GATEWAY)
		call->isPstn
#	else
		0
#	endif
		)
	{
#	if defined(INCLUDE_USB_VOICEMAIL) || defined(INCLUDE_PSTN_GATEWAY)
		pjsua_media_transport_destroy_for_single_call(call_id);
#	endif
	}
#	endif

    if (call->med_tp_st != PJSUA_MED_TP_IDLE) {
	pjmedia_transport_media_stop(call->med_tp);
	call->med_tp_st = PJSUA_MED_TP_IDLE;
    }

    if (call->med_orig && call->med_tp && call->med_tp != call->med_orig) {
	pjmedia_transport_close(call->med_tp);
	call->med_tp = call->med_orig;
    }

/*ycw-pjsip-delete sound device*/
#if 0
    check_snd_dev_idle();
#endif

#	if (defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
		(defined(INCLUDE_PSTN_GATEWAY) || defined(INCLUDE_USB_VOICEMAIL))
	call->medTpReady = PJ_FALSE;
#	endif

    return PJ_SUCCESS;
}


#	if 0
/*
 * DTMF callback from the stream.
 */
static void dtmf_callback(pjmedia_stream *strm, void *user_data,
			  int digit)
{
    PJ_UNUSED_ARG(strm);

    /* For discussions about call mutex protection related to this 
     * callback, please see ticket #460:
     *	http://trac.pjsip.org/repos/ticket/460#comment:4
     */
    if (pjsua_var.ua_cfg.cb.on_dtmf_digit) {
	pjsua_call_id call_id;

	call_id = (pjsua_call_id)(long)user_data;
	pjsua_var.ua_cfg.cb.on_dtmf_digit(call_id, digit);
    }
}
#	endif


pj_status_t pjsua_media_channel_update(pjsua_call_id call_id,
				       const pjmedia_sdp_session *local_sdp,
				       const pjmedia_sdp_session *remote_sdp)
{
    int prev_media_st = 0;
    pjsua_call *call = &pjsua_var.calls[call_id];
    pjmedia_session_info sess_info;
    pjmedia_stream_info *si = NULL;
    pj_status_t status;
	unsigned i;
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	PJ_FIREWALL_RULE* pFwRule = NULL;
#endif
	char localIp[CMSIP_STR_40] = {0};
	pj_uint16_t rtpPort = 0;
	pj_uint16_t rtcpPort = 0;
	pj_str_t tmpStr;

    if (!pjsua_var.med_endpt) 
   {
		/* We're being shutdown */
		return PJ_EBUSY;
    }

	/*ycw-pjsip*/
	/*we must compare the new stream with the old stram to decide to destroy the old stream or not*/
    /* Destroy existing media session, if any. */
	#if 0
    prev_media_st = call->media_st;
    stop_media_session(call->index);
	#endif

	memset(&sess_info, 0, sizeof(sess_info));
    /* Create media session info based on SDP parameters. 
     */    
    status = pjmedia_session_info_from_sdp( call->inv->pool_prov, 
					    pjsua_var.med_endpt, 
					    PJMEDIA_MAX_SDP_MEDIA, &sess_info,
					    local_sdp, remote_sdp);
    if (status != PJ_SUCCESS)
	{
		return status;
    }

	for (i = 0; i < sess_info.stream_cnt; ++i)
	{
		if (sess_info.stream_info[i].dir == PJMEDIA_DIR_NONE)
		{
			continue;
		}

		if (sess_info.stream_info[i].ulptime== 0)
		{
			sess_info.stream_info[i].ulptime = 20;
		}
	}

    /* Update audio index from the negotiated SDP */
    call->audio_idx = find_audio_index(local_sdp, PJ_TRUE);

	CMSIP_PRINT("---call audio index(%d)---", call->audio_idx);

    /* Find which session is audio */
    PJ_ASSERT_RETURN(call->audio_idx != -1, PJ_EBUG);
    PJ_ASSERT_RETURN(call->audio_idx < (int)sess_info.stream_cnt, PJ_EBUG);
    si = &sess_info.stream_info[call->audio_idx];
    
    /* Reset session info with only one media stream */
    sess_info.stream_cnt = 1;
	CMSIP_PRINT("-----stream cnt(%d)---", sess_info.stream_cnt);
	/*ycw-pjsip-t38. 目前只支持一个流*/
    if (si != &sess_info.stream_info[0])
	{
		pj_memcpy(&sess_info.stream_info[0], si, sizeof(pjmedia_stream_info));
		si = &sess_info.stream_info[0];
    }

	pjmedia_transport_udp_local_info(call->med_tp, localIp, &rtpPort, &rtcpPort);
	CMSIP_PRINT("Get local rtp network address:%s:%d", localIp, rtpPort);
	CMSIP_PRINT("Get local rtcp network address:%s:%d", localIp, rtcpPort);
	tmpStr = pj_str(localIp);
	pj_sockaddr_init(pj_AF_INET(), &si->local_addr, &tmpStr, rtpPort);
	pj_sockaddr_init(pj_AF_INET(), &si->local_rtcp, &tmpStr, rtcpPort);

	 /*ycw-pjsip-t38*/
	 /*for the new re-INVITE, we accept all. But, we must check the new stream equal the 
	 old stream or not. If they are same, we need not re-create the stream*/
	 if (call->session &&
	 		call->session->stream_cnt == sess_info.stream_cnt &&
	 		pjsua_media_stream_info_equal(&call->session->stream_info[0], si)
	 		/*now, just support one stream */)
	 {
	 	CMSIP_PRINT("----the existed session has the same transport info, do not re-create--");
	 	goto ON_SUCCESS;
	 }

	 CMSIP_PRINT("destroy old stream, create new stream.");

	/*ycw-pjsip*/
	prev_media_st = call->media_st;
    stop_media_session(call->index);


    /* Check if no media is active */
    if (sess_info.stream_cnt == 0 || si->dir == PJMEDIA_DIR_NONE)
    {
		/* Call media state */
		call->media_st = PJSUA_CALL_MEDIA_NONE;

		/* Call media direction */
		call->media_dir = PJMEDIA_DIR_NONE;

		/* Don't stop transport because we need to transmit keep-alives, and
		 * also to prevent restarting ICE negotiation. See
		 *  http://trac.pjsip.org/repos/ticket/1094
		 */
#if 0
		/* Shutdown transport's session */
		pjmedia_transport_media_stop(call->med_tp);
		call->med_tp_st = PJSUA_MED_TP_IDLE;

		/* No need because we need keepalive? */

		/* Close upper entry of transport stack */
		if (call->med_orig && (call->med_tp != call->med_orig)) {
		    pjmedia_transport_close(call->med_tp);
		    call->med_tp = call->med_orig;
		}
#endif

    }
	else 
	{
		pjmedia_transport_info tp_info;

		/* Start/restart media transport */
		CMSIP_PRINT("----start media transport-------------");
		status = pjmedia_transport_media_start(call->med_tp, 
						       call->inv->pool_prov,
						       local_sdp, remote_sdp,
						       call->audio_idx);
		if (status != PJ_SUCCESS)
		    return status;
		CMSIP_PRINT("-----media transport state is running---------");
		call->med_tp_st = PJSUA_MED_TP_RUNNING;

		/* Get remote SRTP usage policy */
		pjmedia_transport_info_init(&tp_info);
		pjmedia_transport_get_info(call->med_tp, &tp_info);
		if (tp_info.specific_info_cnt > 0)
		{
		    unsigned i;
		    for (i = 0; i < tp_info.specific_info_cnt; ++i)
			 {
				if (tp_info.spc_info[i].type == PJMEDIA_TRANSPORT_TYPE_SRTP) 
				{
				   	 pjmedia_srtp_info *srtp_info = 
							(pjmedia_srtp_info*) tp_info.spc_info[i].buffer;

				    call->rem_srtp_use = srtp_info->peer_use;
				   	break;
				}
		    }
		}


		/*ycw-pjsip*/
		#if 0
		/* Override ptime, if this option is specified. */
		if (pjsua_var.media_cfg.ptime != 0)
		{
		    si->param->setting.frm_per_pkt = (pj_uint8_t)
			(pjsua_var.media_cfg.ptime / si->param->info.frm_ptime);
		    if (si->param->setting.frm_per_pkt == 0)
			si->param->setting.frm_per_pkt = 1;
		}
		#endif

		/* Disable VAD, if this option is specified. */
		if (pjsua_var.media_cfg.no_vad)
		{
		    si->param->setting.vad = 0;
		}


		/* Optionally, application may modify other stream settings here
		 * (such as jitter buffer parameters, codec ptime, etc.)
		 */
		 #if 0
		si->jb_init = pjsua_var.media_cfg.jb_init;
		si->jb_min_pre = pjsua_var.media_cfg.jb_min_pre;
		si->jb_max_pre = pjsua_var.media_cfg.jb_max_pre;
		si->jb_max = pjsua_var.media_cfg.jb_max;
		#endif
	    /* ycw-pjsip: vbd support */
	    if (call->request_channel_mode == pj_channel_passthrough)
		 {
		 	#if 0
	        si->jb_min_pre = si->jb_max_pre = -2; 
			#endif
	        si->vad = si->cng = 0;
	    }

		/* Set SSRC */
		#if 0
		si->ssrc = call->ssrc;
		#endif

		/* Set RTP timestamp & sequence, normally these value are intialized
		 * automatically when stream session created, but for some cases (e.g:
		 * call reinvite, call update) timestamp and sequence need to be kept
		 * contigue.
		 */
		#if 0
		si->rtp_ts = call->rtp_tx_ts;
		si->rtp_seq = call->rtp_tx_seq;
		si->rtp_seq_ts_set = call->rtp_tx_seq_ts_set;
		#endif

#if defined(PJMEDIA_STREAM_ENABLE_KA) && PJMEDIA_STREAM_ENABLE_KA!=0
		/* Enable/disable stream keep-alive and NAT hole punch. */
		si->use_ka = (call->acc_id >= 0) ? pjsua_var.acc[call->acc_id].cfg.use_stream_ka : PJ_TRUE;
#endif

		/* Create session based on session info. */

		status = pjmedia_session_create( pjsua_var.med_endpt, &sess_info,
						 &call->med_tp, call, call_id, 
#	if (!(defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) || \
				defined(INCLUDE_PSTN_GATEWAY))
						 &call->dsp_med_tp,
#	endif
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
						call->isUsbVm,
#	endif
#	if defined(INCLUDE_PSTN_GATEWAY)
						call->isPstn,
#	endif
#	endif
#	if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
						pjsua_var.fwType,
#	endif
						 &call->session );
		
		if (status != PJ_SUCCESS)
		{
		    return status;
		}

#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
		for (i = 0; i < PJ_ARRAY_SIZE(call->fwRule); ++i)
		{
			pFwRule = &call->fwRule[i];
			if (pFwRule->destination.slen > 0 && pFwRule->dport > 0)
			{
			status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, pFwRule->protocol,
						&(pFwRule->destination), NULL, pFwRule->dport, NULL, NULL, -1,
						pjsua_var.fwType);
			if (status != PJ_SUCCESS)
			{
					printf("==%s,%d==firewall rule set error!!!!====", __FUNCTION__,
						__LINE__);
			}
			}
			memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
		}
#endif
	
	CMSIP_PRINT("----session create successfully--------------------------------");
	/* If DTMF callback is installed by application, install our
	 * callback to the session.
	 */
	if (pjsua_var.ua_cfg.cb.on_dtmf_digit)
	{
		#if 0
	    pjmedia_session_set_dtmf_callback(call->session, 0, 
					      &dtmf_callback, 
					      (void*)(long)(call->index));
		#endif
	}

	
		/* Call media direction */
		call->media_dir = si->dir;

		/* Call media state */
		if (call->local_hold)
		{
			call->media_st = PJSUA_CALL_MEDIA_LOCAL_HOLD;
		}
		else if (call->media_dir == PJMEDIA_DIR_DECODING)
		{
			call->media_st = PJSUA_CALL_MEDIA_REMOTE_HOLD;
		}
		else
		{
			call->media_st = PJSUA_CALL_MEDIA_ACTIVE;
		}
    }

ON_SUCCESS:

    /* Print info. */
    {
		char info[80];
		int info_len = 0;
		unsigned i;

		for (i = 0; i < sess_info.stream_cnt; ++i)
		{
		    int len;
		    const char *dir;
		    pjmedia_stream_info *strm_info = &sess_info.stream_info[i];

		    switch (strm_info->dir)
			 {
		    case PJMEDIA_DIR_NONE:
				dir = "inactive";
			 	break;
		    case PJMEDIA_DIR_ENCODING:
				dir = "sendonly";
			 	break;
		    case PJMEDIA_DIR_DECODING:
				dir = "recvonly";
			 	break;
		    case PJMEDIA_DIR_ENCODING_DECODING:
				dir = "sendrecv";
			 	break;
		    default:
				dir = "unknown";
			 	break;
		    }
			 
		    len = pj_ansi_sprintf( info+info_len,
					   ", stream #%d: %.*s (%s)", i,
					   (int)strm_info->fmt.encoding_name.slen,
					   strm_info->fmt.encoding_name.ptr,
					   dir);
		    if (len > 0)
		    {
				info_len += len;
		    }
		}
		PJ_LOG(4,(THIS_FILE,"Media updates%s", info));
    }

    return PJ_SUCCESS;
}



#	if 0
/*****************************************************************************
 * File player.
 */

static char* get_basename(const char *path, unsigned len)
{
    char *p = ((char*)path) + len;

    if (len==0)
	return p;

    for (--p; p!=path && *p!='/' && *p!='\\'; ) --p;

    return (p==path) ? p : p+1;
}
#	endif

/*****************************************************************************
 * Codecs.
 */

/*
 * Enum all supported codecs in the system.
 */
pj_status_t pjsua_enum_codecs( pjsua_codec_info id[],
				       unsigned *p_count )
{
    pjmedia_codec_mgr *codec_mgr;
    pjmedia_codec_info info[32];
    unsigned i, count, prio[32];
    pj_status_t status;

    codec_mgr = pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt);
    count = PJ_ARRAY_SIZE(info);
    status = pjmedia_codec_mgr_enum_codecs( codec_mgr, &count, info, prio);
    if (status != PJ_SUCCESS)
	 {
		*p_count = 0;
		return status;
    }

    if (count > *p_count) count = *p_count;

    for (i=0; i<count; ++i) {
	pjmedia_codec_info_to_id(&info[i], id[i].buf_, sizeof(id[i].buf_));
	id[i].codec_id = pj_str(id[i].buf_);
	id[i].priority = (pj_uint8_t) prio[i];
    }

    *p_count = count;

    return PJ_SUCCESS;
}


/*
 * Change codec priority.
 */
pj_status_t pjsua_codec_set_priority( const pj_str_t *codec_id,
					      pj_uint8_t priority )
{
    const pj_str_t all = { NULL, 0 };
    pjmedia_codec_mgr *codec_mgr;

    codec_mgr = pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt);

    if (codec_id->slen==1 && *codec_id->ptr=='*')
	codec_id = &all;

    return pjmedia_codec_mgr_set_codec_priority(codec_mgr, codec_id, 
					        priority);
}

/* 
 * brief	by yuchuwei@20120413
 */
#	if 0

/*
 * Get codec parameters.
 */
pj_status_t pjsua_codec_get_param( const pj_str_t *codec_id,
					   pjmedia_codec_param *param )
{
    const pj_str_t all = { NULL, 0 };
    const pjmedia_codec_info *info;
    pjmedia_codec_mgr *codec_mgr;
    unsigned count = 1;
    pj_status_t status;

    codec_mgr = pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt);

    if (codec_id->slen==1 && *codec_id->ptr=='*')
	codec_id = &all;

    status = pjmedia_codec_mgr_find_codecs_by_id(codec_mgr, codec_id,
						 &count, &info, NULL);
    if (status != PJ_SUCCESS)
	return status;

    if (count != 1)
	return PJ_ENOTFOUND;

    status = pjmedia_codec_mgr_get_default_param( codec_mgr, info, param);
    return status;
}

/*
 * Set codec parameters.
 */
pj_status_t pjsua_codec_set_param( const pj_str_t *codec_id,
					   const pjmedia_codec_param *param)
{
    const pjmedia_codec_info *info[2];
    pjmedia_codec_mgr *codec_mgr;
    unsigned count = 2;
    pj_status_t status;

    codec_mgr = pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt);

    status = pjmedia_codec_mgr_find_codecs_by_id(codec_mgr, codec_id,
						 &count, info, NULL);
    if (status != PJ_SUCCESS)
	return status;

    /* Codec ID should be specific, except for G.722.1 */
    if (count > 1 && 
	pj_strnicmp2(codec_id, "G7221/16", 8) != 0 &&
	pj_strnicmp2(codec_id, "G7221/32", 8) != 0)
    {
	pj_assert(!"Codec ID is not specific");
	return PJ_ETOOMANY;
    }

    status = pjmedia_codec_mgr_set_default_param(codec_mgr, info[0], param);
    return status;
}
#	endif

/*ycw-pjsip-codec*/
#if 1
pjmedia_codec_mgr* pjsua_codec_mgr_switch_to_full(void)
{
	return pjmedia_endpt_codec_mgr_switch_to_full(pjsua_var.med_endpt);
}

#	if (defined(INCLUDE_PSTN_GATEWAY) || ((defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && \
			0==PJ_MEDIA_TRANSIT_BY_PJSIP)&& defined(INCLUDE_USB_VOICEMAIL))) \
			|| (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
pjmedia_codec_mgr* pjsua_codec_mgr_switch_to_lite(void)
{
	return pjmedia_endpt_codec_mgr_switch_to_lite(pjsua_var.med_endpt);
}
#	endif

pj_status_t pjsua_codec_mgr_codec_is_exist(const pj_str_t* codec_id)
{
	PJ_ASSERT_RETURN( codec_id && codec_id->slen!=0, PJ_EINVAL);
	
	pjmedia_codec_mgr* mgr = pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt);
	return pjmedia_codec_mgr_codec_is_exist(mgr, codec_id);
}

int pjsua_codec_mgr_get_codec_count(void)
{
	pjmedia_codec_mgr* mgr = pjmedia_endpt_get_codec_mgr(pjsua_var.med_endpt);
	return pjmedia_codec_mgr_get_codec_count(mgr);
}

#endif

/*ycw-pjsip. Compare two stream info*/
static pj_bool_t pjsua_media_stream_info_equal(pjmedia_stream_info* stream1, 
																				pjmedia_stream_info* stream2)
{
	PJ_ASSERT_RETURN(stream1!=NULL && stream2!=NULL, PJ_EINVAL);

#ifdef CMSIP_DEBUG

	char addr[40];
	int port;

	if (stream1->type == stream2->type)
	{
		CMSIP_PRINT("type equal");
	}
	else
	{
		CMSIP_PRINT("type not equal");
	}

	if (stream1->proto == stream2->proto)
	{
		CMSIP_PRINT("proto equal");
	}
	else
	{
		CMSIP_PRINT("proto not equal"); 
	}

	if (stream1->dir == stream2->dir)
	{
		CMSIP_PRINT("dir equal");
	}
	else
	{
		CMSIP_PRINT("dir not equal");
	}

	strcpy(addr, pj_inet_ntoa(pj_sockaddr_in_get_addr(&stream1->rem_addr.ipv4)));
	port = pj_sockaddr_in_get_port(&stream1->rem_addr.ipv4);
	CMSIP_PRINT("stream1 remote rtp : %s:%d", addr, port);

	strcpy(addr, pj_inet_ntoa(pj_sockaddr_in_get_addr(&stream2->rem_addr.ipv4)));
	port = pj_sockaddr_in_get_port(&stream2->rem_addr.ipv4);
	CMSIP_PRINT("stream2 remote rtcp: %s:%d", addr, port);	

	if (memcmp(&stream1->rem_addr, &stream2->rem_addr, sizeof(stream2->rem_addr)))
	{
		CMSIP_PRINT("rem_addr not equal");
	}
	else
	{
		CMSIP_PRINT("rem_addr equal");
	}	

	if (memcmp(&stream1->rem_rtcp, &stream2->rem_rtcp, sizeof(stream2->rem_rtcp)))
	{
		CMSIP_PRINT("rem_rtcp not equal");
	}
	else
	{
		CMSIP_PRINT("rem_rtcp equal");
	}

	strcpy(addr, pj_inet_ntoa(pj_sockaddr_in_get_addr(&stream1->local_addr.ipv4)));
	port = pj_sockaddr_in_get_port(&stream1->local_addr.ipv4);
	CMSIP_PRINT("stream1 remote rtp : %s:%d", addr, port);

	strcpy(addr, pj_inet_ntoa(pj_sockaddr_in_get_addr(&stream2->local_rtcp.ipv4)));
	port = pj_sockaddr_in_get_port(&stream2->local_rtcp.ipv4);
	CMSIP_PRINT("stream2 remote rtp: %s:%d", addr, port);	

	if (memcmp(&stream1->local_addr, &stream2->local_addr, sizeof(stream2->local_addr)))
	{
		CMSIP_PRINT("local_addr not equal");
	}
	else
	{
		CMSIP_PRINT("local_addr equal");
	}

	if (memcmp(&stream1->local_rtcp, &stream2->local_rtcp, sizeof(stream2->local_rtcp)))
	{
		CMSIP_PRINT("local_rtcp not equal");
	}
	else
	{
		CMSIP_PRINT("local_rtcp equal");
	}

	if (stream1->fmt.type != stream2->fmt.type)

	{
		CMSIP_PRINT("fmt type not equal");
	}
	else
	{
		CMSIP_PRINT("fmt type equal");
	}

	if (stream1->fmt.pt != stream2->fmt.pt)
	{
		CMSIP_PRINT("fmt pt not equal");
	}
	else
	{
		CMSIP_PRINT("fmt pt equal");
	}

	if (pj_stricmp(&stream1->fmt.encoding_name, &stream2->fmt.encoding_name))
	{
		CMSIP_PRINT("fmt encoding_name not equal");
	}
	else
	{
		CMSIP_PRINT("fmt encoding_name equal");
	}

	if (stream1->fmt.clock_rate != stream2->fmt.clock_rate)
	{
		CMSIP_PRINT("fmt clock_rate not equal");
	}
	else
	{
		CMSIP_PRINT("fmt clock_rate equal");
	}

	if (stream1->fmt.channel_cnt != stream2->fmt.channel_cnt)
	{
		CMSIP_PRINT("fmt channel_cnt not equal");
	}
	else
	{
		CMSIP_PRINT("fmt channel_cnt equal");
	}

	if (PJ_FALSE == pjmedia_codec_param_equal(stream1->param, stream2->param))
	{
		CMSIP_PRINT("param not equal");
	}
	else
	{
		CMSIP_PRINT("param equal");
	}

	if (stream1->tx_pt != stream2->tx_pt)
	{
		CMSIP_PRINT("tx_pt not equal");
	}
	else
	{
		CMSIP_PRINT("tx_pt equal");
	}

	if (stream1->tx_maxptime != stream2->tx_maxptime)
	{
		CMSIP_PRINT("tx_maxptime not equal");
	}
	else
	{
		CMSIP_PRINT("tx_maxptime equal");
	}

	if (stream1->tx_event_pt != stream2->tx_event_pt)
	{
		CMSIP_PRINT("tx_event_pt not equal");
	}
	else
	{
		CMSIP_PRINT("tx_event_pt equal");
	}

	if (stream1->rx_event_pt != stream2->rx_event_pt)
	{
		CMSIP_PRINT("rx_event_pt not equal");
	}
	else
	{
		CMSIP_PRINT("rx_event_pt equal");
	}

	if (stream1->ulptime != stream2->ulptime)
	{
		CMSIP_PRINT("ulptime not equal");
	}
	else
	{
		CMSIP_PRINT("ulptime equal");
	}

	if (stream1->vad != stream2->vad)
	{
		CMSIP_PRINT("vad not equal");
	}
	else
	{
		CMSIP_PRINT("vad equal");
	}

	if (stream1->cng != stream2->cng )
	{
		CMSIP_PRINT("cng not equal");
	}
	else
	{
		CMSIP_PRINT("cng equal");
	}	
#endif


	if (stream1->type != stream2->type ||
			stream1->proto != stream2->proto ||
			stream1->dir != stream2->dir ||
			memcmp(&stream1->local_addr, &stream2->local_addr, sizeof(stream2->local_addr)) ||
			memcmp(&stream1->local_rtcp, &stream2->local_rtcp, sizeof(stream2->local_rtcp)) ||
			memcmp(&stream1->rem_addr, &stream2->rem_addr, sizeof(stream2->rem_addr)) ||
			memcmp(&stream1->rem_rtcp, &stream2->rem_rtcp, sizeof(stream2->rem_rtcp)) ||
			#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR!=0)
			stream1->rtcp_xr_enabled != stream2->rtcp_xr_enabled ||
			stream1->rtcp_xr_interval != stream2->rtcp_xr_interval ||
			memcmp(&stream1->rtcp_xr_dest, stream2->rtcp_xr_dest, sizeof(stream2->rtcp_xr_dest)) ||
			#endif
			stream1->fmt.type != stream2->fmt.type ||
			stream1->fmt.pt != stream2->fmt.pt ||
			pj_stricmp(&stream1->fmt.encoding_name, &stream2->fmt.encoding_name) ||
			stream1->fmt.clock_rate != stream2->fmt.clock_rate ||
			stream1->fmt.channel_cnt != stream2->fmt.channel_cnt ||
			PJ_FALSE == pjmedia_codec_param_equal(stream1->param, stream2->param) ||
			stream1->tx_pt != stream2->tx_pt ||
			stream1->tx_maxptime != stream2->tx_maxptime ||
			stream1->tx_event_pt != stream2->tx_event_pt ||
			stream1->rx_event_pt != stream2->rx_event_pt ||
			stream1->ulptime != stream2->ulptime ||
			stream1->vad != stream2->vad ||
			stream1->cng != stream2->cng 
			#if defined(PJMEDIA_STREAM_ENABLE_KA) && (PJMEDIA_STREAM_ENABLE_KA!=0)
			||	stream1->use_ka != stream2->use_ka
			#endif			
			)
	{
		return PJ_FALSE;
	}

	return PJ_TRUE;
}

/*ycw-pjsip*/
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_PSTN_GATEWAY) || defined(INCLUDE_USB_VOICEMAIL)
pj_status_t pjsua_media_transport_create_for_single_call(pjsua_call_id index)
{
/*ycw-pjsip-note.
在create_rtp_rtcp_sock函数中，对于绑定的端口有一个循环，如果这个端口不行，那么+2，
然后再尝试。
而现在改成这样，是没有的，那么绑定失败的情况下要怎么办?所以这儿也需要改成循环的。
*/
CMSIP_PRINT("------------create socket and register to ioqueue------------------");
	pjsua_call* call = NULL;
	pj_status_t status;
	pj_sockaddr_in bound_addr;
	pjsua_transport_config* cfg = NULL;
	pj_uint16_t rtp_port;
	pj_uint16_t rtcp_port;
	pj_sock_t sock[2];
	pj_bool_t on = PJ_TRUE;
	struct ifreq boundIf;

	CMSIP_ASSERT(index>=0 && index<pjsua_var.ua_cfg.max_calls);

	call = &pjsua_var.calls[index];
	cfg = &pjsua_var.medTpCfg;
	bound_addr.sin_addr.s_addr = PJ_INADDR_ANY;
	snprintf(boundIf.ifr_name, IFNAMSIZ, "%.*s", (int)pjsua_var.bindDev.slen, 
				pj_strnull(pjsua_var.bindDev.ptr));

	
	/*ycw-pjsip. There must be bound address.*/
	CMSIP_ASSERT(cfg->bound_addr.slen);
   status = pj_sockaddr_in_set_str_addr(&bound_addr, &cfg->bound_addr);
	if (status != PJ_SUCCESS)
	{
	  	pjsua_perror(THIS_FILE, "Unable to resolve transport bind address", status);
	   return status;
	}		

	rtp_port = pjmedia_transport_get_rtp_port(call->med_tp);
	rtcp_port = pjmedia_transport_get_rtcp_port(call->med_tp);
	CMSIP_PRINT("get rtp port[%d], rtcp port[%d]", rtp_port, rtcp_port);

	/* Create RTP socket. */
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[0]);
	if (status != PJ_SUCCESS)
	{
	  	pjsua_perror(THIS_FILE, "socket() error", status);
	  	return status;
	}

	/* Apply QoS to RTP socket, if specified */
	status = pj_sock_apply_qos2(sock[0], cfg->qos_type, 
				    &cfg->qos_params, 
				    2, THIS_FILE, "RTP socket");

	/*ycw-pjsip.Enable bind address reuse.*/
	status = pj_sock_setsockopt(sock[0], pj_SOL_SOCKET(), pj_SO_REUSEADDR(), 
						&on, sizeof(on));

	/*Bind rtp socket to the BoundInterface. YuChuwei*/
	status = pj_sock_setsockopt(sock[0], pj_SOL_SOCKET(), SO_BINDTODEVICE, 
						&boundIf, sizeof(boundIf));

	/* Bind RTP socket */
	status = pj_sock_bind_in(sock[0], pj_ntohl(bound_addr.sin_addr.s_addr), rtp_port);
	if (status != PJ_SUCCESS)
	{
	  	pj_sock_close(sock[0]); 
	  	sock[0] = PJ_INVALID_SOCKET;
		pjsua_perror(THIS_FILE, "bind() error", status);
		return status;
	}

	/* Create RTCP socket. */
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[1]);
	if (status != PJ_SUCCESS)
	{
	  	pjsua_perror(THIS_FILE, "socket() error", status);
	  	pj_sock_close(sock[0]);
	  	return status;
	}

	/* Apply QoS to RTCP socket, if specified */
	status = pj_sock_apply_qos2(sock[1], cfg->qos_type, 
				    &cfg->qos_params, 
				    2, THIS_FILE, "RTCP socket");

	/*ycw-pjsip.Enable bind address reuse.*/
	status = pj_sock_setsockopt(sock[1], pj_SOL_SOCKET(), pj_SO_REUSEADDR(), 
						&on, sizeof(on));

	/*Bind rtp socket to the BoundInterface. YuChuwei*/
	status = pj_sock_setsockopt(sock[1], pj_SOL_SOCKET(), SO_BINDTODEVICE, 
						&boundIf, sizeof(boundIf));

	/* Bind RTCP socket */
	status=pj_sock_bind_in(sock[1], pj_ntohl(bound_addr.sin_addr.s_addr), 
			       rtcp_port);
	if (status != PJ_SUCCESS)
	{
	  	pj_sock_close(sock[0]); 
	  	sock[0] = PJ_INVALID_SOCKET;

	  	pj_sock_close(sock[1]); 
	  	sock[1] = PJ_INVALID_SOCKET;
		pjsua_perror(THIS_FILE, "bind() error", status);
		return status;
	}

	status = pjmedia_transport_udp_attach_for_single_call(pjsua_var.med_endpt,
			call->med_tp, sock);
	if (status != PJ_SUCCESS)
	{
		if (call->med_tp != NULL)
		{
	   	pjmedia_transport_close(call->med_tp);
	   	call->med_tp = NULL;
		}
		pjsua_perror(THIS_FILE, "attach() error", status);
		return status;

	}

	 return PJ_SUCCESS;

}

pj_status_t pjsua_media_transport_destroy_for_single_call(pjsua_call_id index)
{
	CMSIP_PRINT("--destroy media transport for call(%d)--", index);
	pjsua_call* call = NULL;
	pj_status_t status;
	
	CMSIP_ASSERT(index>=0 && index<pjsua_var.ua_cfg.max_calls);

	call = &pjsua_var.calls[index];

	if (call->med_tp)
	{
		status = pjmedia_transport_udp_destroy_for_single_call(call->med_tp);
		if (status != PJ_SUCCESS)
		{
			pjsua_perror(THIS_FILE, "udp transport destroy", status);
			return status;
		}
		return PJ_SUCCESS;
	}

	CMSIP_PRINT("===the transport has not been created===");
	
	return PJ_SUCCESS;
}
#	endif

#	if defined(INCLUDE_USB_VOICEMAIL)
pj_status_t pjsua_media_send_rtp_create(pjsua_call_id callIndex)
{
	pjsua_call* call;
	pj_status_t status;
	CMSIP_ASSERT(callIndex>=0 && callIndex<pjsua_var.ua_cfg.max_calls);

	call = &pjsua_var.calls[callIndex];
	CMSIP_PRINT("-----rtp_create-----call(%d) session(%p)------", call->index, call->session);
	if (call->session)
	{
	status = pjmedia_session_send_rtp_create(call->session, callIndex);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "send rtp create error", status);
		return status;
	}
	}

	return PJ_SUCCESS;
}
#	endif

#	endif

