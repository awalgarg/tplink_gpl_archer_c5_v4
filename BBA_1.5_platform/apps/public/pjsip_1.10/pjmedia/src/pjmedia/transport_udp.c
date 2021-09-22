/* $Id: transport_udp.c 3553 2011-05-05 06:14:19Z nanang $ */
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
#include <pjmedia/transport_udp.h>
#include <pj/addr_resolv.h>
#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/ioqueue.h>
#include <pj/log.h>
#include <pj/pool.h>
#include <pj/rand.h>
#include <pj/string.h>
#include "cmsip_assert.h"

/* Maximum size of incoming RTP packet */
#define RTP_LEN	    PJMEDIA_MAX_MTU

/* Maximum size of incoming RTCP packet */
#define RTCP_LEN    600

/* Maximum pending write operations */
#define MAX_PENDING 4

static const pj_str_t ID_RTP_AVP  = { "RTP/AVP", 7 };
/*ycw-pjsip. t38*/
static const pj_str_t ID_UDPTL    = { "udptl", 5 };


/* Pending write buffer */
typedef struct pending_write
{
    char		buffer[RTP_LEN];
    pj_ioqueue_op_key_t	op_key;
} pending_write;


struct transport_udp
{
    pjmedia_transport	base;		/**< Base transport.		    */

    pj_pool_t	       *pool;		/**< Memory pool		    */
    unsigned		options;	/**< Transport options.		    */
    unsigned		media_options;	/**< Transport media options.	    */
    void	       *user_data;	/**< Only valid when attached	    */
    pj_bool_t		attached;	/**< Has attachment?		    */
    pj_sockaddr		rem_rtp_addr;	/**< Remote RTP address		    */
    pj_sockaddr		rem_rtcp_addr;	/**< Remote RTCP address	    */
    int			addr_len;	/**< Length of addresses.	    */
    void  (*rtp_cb)(	void*,		/**< To report incoming RTP.	    */
			void*,
			pj_ssize_t);
    void  (*rtcp_cb)(	void*,		/**< To report incoming RTCP.	    */
			void*,
			pj_ssize_t);

    unsigned		tx_drop_pct;	/**< Percent of tx pkts to drop.    */
    unsigned		rx_drop_pct;	/**< Percent of rx pkts to drop.    */

    pj_sock_t	        rtp_sock;	/**< RTP socket			    */
    pj_sockaddr		rtp_addr_name;	/**< Published RTP address.	    */
    pj_ioqueue_key_t   *rtp_key;	/**< RTP socket key in ioqueue	    */
    pj_ioqueue_op_key_t	rtp_read_op;	/**< Pending read operation	    */
    unsigned		rtp_write_op_id;/**< Next write_op to use	    */
    pending_write	rtp_pending_write[MAX_PENDING];  /**< Pending write */
    pj_sockaddr		rtp_src_addr;	/**< Actual packet src addr.	    */
    unsigned		rtp_src_cnt;	/**< How many pkt from this addr.   */
    int			rtp_addrlen;	/**< Address length.		    */
    char		rtp_pkt[RTP_LEN];/**< Incoming RTP packet buffer    */

    pj_sock_t		rtcp_sock;	/**< RTCP socket		    */
    pj_sockaddr		rtcp_addr_name;	/**< Published RTCP address.	    */
    pj_sockaddr		rtcp_src_addr;	/**< Actual source RTCP address.    */
    int			rtcp_addr_len;	/**< Length of RTCP src address.    */
    pj_ioqueue_key_t   *rtcp_key;	/**< RTCP socket key in ioqueue	    */
    pj_ioqueue_op_key_t rtcp_read_op;	/**< Pending read operation	    */
    pj_ioqueue_op_key_t rtcp_write_op;	/**< Pending write operation	    */
    char		rtcp_pkt[RTCP_LEN];/**< Incoming RTCP packet buffer */
};



static void on_rx_rtp( pj_ioqueue_key_t *key, 
                       pj_ioqueue_op_key_t *op_key, 
                       pj_ssize_t bytes_read);
static void on_rx_rtcp(pj_ioqueue_key_t *key, 
                       pj_ioqueue_op_key_t *op_key, 
                       pj_ssize_t bytes_read);

/*
 * These are media transport operations.
 */
static pj_status_t transport_get_info (pjmedia_transport *tp,
				       pjmedia_transport_info *info);
static pj_status_t transport_attach   (pjmedia_transport *tp,
				       void *user_data,
				       const pj_sockaddr_t *rem_addr,
				       const pj_sockaddr_t *rem_rtcp,
				       unsigned addr_len,
				       void (*rtp_cb)(void*,
						      void*,
						      pj_ssize_t),
				       void (*rtcp_cb)(void*,
						       void*,
						       pj_ssize_t));
static void	   transport_detach   (pjmedia_transport *tp,
				       void *strm);
static pj_status_t transport_send_rtp( pjmedia_transport *tp,
				       const void *pkt,
				       pj_size_t size);
static pj_status_t transport_send_rtcp(pjmedia_transport *tp,
				       const void *pkt,
				       pj_size_t size);
static pj_status_t transport_send_rtcp2(pjmedia_transport *tp,
				       const pj_sockaddr_t *addr,
				       unsigned addr_len,
				       const void *pkt,
				       pj_size_t size);
static pj_status_t transport_media_create(pjmedia_transport *tp,
				       pj_pool_t *pool,
				       unsigned options,
				       const pjmedia_sdp_session *sdp_remote,
				       unsigned media_index);
static pj_status_t transport_encode_sdp(pjmedia_transport *tp,
				        pj_pool_t *pool,
				        pjmedia_sdp_session *sdp_local,
				        const pjmedia_sdp_session *rem_sdp,
				        unsigned media_index);
static pj_status_t transport_media_start (pjmedia_transport *tp,
				       pj_pool_t *pool,
				       const pjmedia_sdp_session *sdp_local,
				       const pjmedia_sdp_session *sdp_remote,
				       unsigned media_index);
static pj_status_t transport_media_stop(pjmedia_transport *tp);
static pj_status_t transport_simulate_lost(pjmedia_transport *tp,
				       pjmedia_dir dir,
				       unsigned pct_lost);
static pj_status_t transport_destroy  (pjmedia_transport *tp);


static pjmedia_transport_op transport_udp_op = 
{
    &transport_get_info,
    &transport_attach,
    &transport_detach,
    &transport_send_rtp,
    &transport_send_rtcp,
    &transport_send_rtcp2,
    &transport_media_create,
    &transport_encode_sdp,
    &transport_media_start,
    &transport_media_stop,
    &transport_simulate_lost,
    &transport_destroy
};


/**
 * Create UDP stream transport.
 */
pj_status_t pjmedia_transport_udp_create( pjmedia_endpt *endpt,
						  const char *name,
						  int port,
						  unsigned options,
						  pjmedia_transport **p_tp)
{
    return pjmedia_transport_udp_create2(endpt, name, NULL, port, options, 
					p_tp);
}

/**
 * Create UDP stream transport.
 */
pj_status_t pjmedia_transport_udp_create2(pjmedia_endpt *endpt,
						  const char *name,
						  const pj_str_t *addr,
						  int port,
						  unsigned options,
						  pjmedia_transport **p_tp)
{
    return pjmedia_transport_udp_create3(endpt, pj_AF_INET(), name,
					 addr, port, options, p_tp);
}

/**
 * Create UDP stream transport.
 */
pj_status_t pjmedia_transport_udp_create3(pjmedia_endpt *endpt,
						  int af,
						  const char *name,
						  const pj_str_t *addr,
						  int port,
						  unsigned options,
						  pjmedia_transport **p_tp)
{
    pjmedia_sock_info si;
    pj_status_t status;

    
    /* Sanity check */
    PJ_ASSERT_RETURN(endpt && port && p_tp, PJ_EINVAL);


    pj_bzero(&si, sizeof(pjmedia_sock_info));
    si.rtp_sock = si.rtcp_sock = PJ_INVALID_SOCKET;

    /* Create RTP socket */
    status = pj_sock_socket(af, pj_SOCK_DGRAM(), 0, &si.rtp_sock);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Bind RTP socket */
    status = pj_sockaddr_init(af, &si.rtp_addr_name, addr, (pj_uint16_t)port);
    if (status != PJ_SUCCESS)
	goto on_error;

    status = pj_sock_bind(si.rtp_sock, &si.rtp_addr_name, 
			  pj_sockaddr_get_len(&si.rtp_addr_name));
    if (status != PJ_SUCCESS)
	goto on_error;


    /* Create RTCP socket */
    status = pj_sock_socket(af, pj_SOCK_DGRAM(), 0, &si.rtcp_sock);
    if (status != PJ_SUCCESS)
	goto on_error;

    /* Bind RTCP socket */
    status = pj_sockaddr_init(af, &si.rtcp_addr_name, addr, 
			      (pj_uint16_t)(port+1));
    if (status != PJ_SUCCESS)
	goto on_error;

    status = pj_sock_bind(si.rtcp_sock, &si.rtcp_addr_name,
			  pj_sockaddr_get_len(&si.rtcp_addr_name));
    if (status != PJ_SUCCESS)
	goto on_error;

    
    /* Create UDP transport by attaching socket info */
    return pjmedia_transport_udp_attach( endpt, name, &si, options, p_tp);


on_error:
    if (si.rtp_sock != PJ_INVALID_SOCKET)
	pj_sock_close(si.rtp_sock);
    if (si.rtcp_sock != PJ_INVALID_SOCKET)
	pj_sock_close(si.rtcp_sock);
    return status;
}


/**
 * Create UDP stream transport from existing socket info.
 */
pj_status_t pjmedia_transport_udp_attach( pjmedia_endpt *endpt,
						  const char *name,
						  const pjmedia_sock_info *si,
						  unsigned options,
						  pjmedia_transport **p_tp)
{
    struct transport_udp *tp;
    pj_pool_t *pool;
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
    pj_ioqueue_t *ioqueue;
    pj_ioqueue_callback rtp_cb, rtcp_cb;
#	endif
    pj_status_t status;


    /* Sanity check */
    PJ_ASSERT_RETURN(endpt && si && p_tp, PJ_EINVAL);

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
    /* Get ioqueue instance */
    ioqueue = pjmedia_endpt_get_ioqueue(endpt);
#	endif

    if (name==NULL)
		name = "udp%p";

    /* Create transport structure */
    pool = pjmedia_endpt_create_pool(endpt, name, 512, 512);
    if (!pool)
	return PJ_ENOMEM;

    tp = PJ_POOL_ZALLOC_T(pool, struct transport_udp);
    tp->pool = pool;
    tp->options = options;
    pj_memcpy(tp->base.name, pool->obj_name, PJ_MAX_OBJ_NAME);
    tp->base.op = &transport_udp_op;
    tp->base.type = PJMEDIA_TRANSPORT_TYPE_UDP;

    /* Copy socket infos */
    tp->rtp_sock = si->rtp_sock;
    tp->rtp_addr_name = si->rtp_addr_name;
    tp->rtcp_sock = si->rtcp_sock;
    tp->rtcp_addr_name = si->rtcp_addr_name;

    /* If address is 0.0.0.0, use host's IP address */
    if (!pj_sockaddr_has_addr(&tp->rtp_addr_name))
	 {
		pj_sockaddr hostip;
		status = pj_gethostip(tp->rtp_addr_name.addr.sa_family, &hostip);
		if (status != PJ_SUCCESS)
	   	 goto on_error;

		pj_memcpy(pj_sockaddr_get_addr(&tp->rtp_addr_name), 
		  pj_sockaddr_get_addr(&hostip),
		  pj_sockaddr_get_addr_len(&hostip));
    }

    /* Same with RTCP */
    if (!pj_sockaddr_has_addr(&tp->rtcp_addr_name))
	 {
		pj_memcpy(pj_sockaddr_get_addr(&tp->rtcp_addr_name),
		  pj_sockaddr_get_addr(&tp->rtp_addr_name),
		  pj_sockaddr_get_addr_len(&tp->rtp_addr_name));
    }

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP!=0
    /* Setup RTP socket with the ioqueue */
    pj_bzero(&rtp_cb, sizeof(rtp_cb));
    rtp_cb.on_read_complete = &on_rx_rtp;

    status = pj_ioqueue_register_sock(pool, ioqueue, tp->rtp_sock, tp,
				      &rtp_cb, &tp->rtp_key);
    if (status != PJ_SUCCESS)
	goto on_error;
    
    /* Disallow concurrency so that detach() and destroy() are
     * synchronized with the callback.
     */
    status = pj_ioqueue_set_concurrency(tp->rtp_key, PJ_FALSE);
    if (status != PJ_SUCCESS)
	goto on_error;

    pj_ioqueue_op_key_init(&tp->rtp_read_op, sizeof(tp->rtp_read_op));
    for (i=0; i<PJ_ARRAY_SIZE(tp->rtp_pending_write); ++i)
	pj_ioqueue_op_key_init(&tp->rtp_pending_write[i].op_key, 
			       sizeof(tp->rtp_pending_write[i].op_key));

    /* Kick of pending RTP read from the ioqueue */
    tp->rtp_addrlen = sizeof(tp->rtp_src_addr);
    size = sizeof(tp->rtp_pkt);
    status = pj_ioqueue_recvfrom(tp->rtp_key, &tp->rtp_read_op,
			         tp->rtp_pkt, &size, PJ_IOQUEUE_ALWAYS_ASYNC,
				 &tp->rtp_src_addr, &tp->rtp_addrlen);
    if (status != PJ_EPENDING)
	goto on_error;

    /* Setup RTCP socket with ioqueue */
    pj_bzero(&rtcp_cb, sizeof(rtcp_cb));
    rtcp_cb.on_read_complete = &on_rx_rtcp;

    status = pj_ioqueue_register_sock(pool, ioqueue, tp->rtcp_sock, tp,
				      &rtcp_cb, &tp->rtcp_key);
    if (status != PJ_SUCCESS)
	goto on_error;

    status = pj_ioqueue_set_concurrency(tp->rtcp_key, PJ_FALSE);
    if (status != PJ_SUCCESS)
	goto on_error;

    pj_ioqueue_op_key_init(&tp->rtcp_read_op, sizeof(tp->rtcp_read_op));
    pj_ioqueue_op_key_init(&tp->rtcp_write_op, sizeof(tp->rtcp_write_op));


    /* Kick of pending RTCP read from the ioqueue */
    size = sizeof(tp->rtcp_pkt);
    tp->rtcp_addr_len = sizeof(tp->rtcp_src_addr);
    status = pj_ioqueue_recvfrom( tp->rtcp_key, &tp->rtcp_read_op,
				  tp->rtcp_pkt, &size, PJ_IOQUEUE_ALWAYS_ASYNC,
				  &tp->rtcp_src_addr, &tp->rtcp_addr_len);
    if (status != PJ_EPENDING)
	goto on_error;
#	endif
    /* Done */
    *p_tp = &tp->base;
    return PJ_SUCCESS;


on_error:
    transport_destroy(&tp->base);
    return status;
}

/*ycw-pjsip*/
pj_status_t pjmedia_dsp_transport_udp_attach(pjmedia_endpt *endpt,
						  const char *name,
						  const pjmedia_sock_info *si,
						  unsigned options,
						  pjmedia_transport **p_tp)
{
    struct transport_udp *tp;
    pj_pool_t *pool;
    pj_ioqueue_t *ioqueue;
    pj_ioqueue_callback rtp_cb, rtcp_cb;
    pj_ssize_t size;
    unsigned i;
    pj_status_t status;


    /* Sanity check */
    PJ_ASSERT_RETURN(endpt && si && p_tp, PJ_EINVAL);

    /* Get ioqueue instance */
    ioqueue = pjmedia_endpt_get_ioqueue(endpt);

    if (name==NULL)
		name = "udp%p";

    /* Create transport structure */
    pool = pjmedia_endpt_create_pool(endpt, name, 512, 512);
    if (!pool)
	return PJ_ENOMEM;

    tp = PJ_POOL_ZALLOC_T(pool, struct transport_udp);
    tp->pool = pool;
    tp->options = options;
    pj_memcpy(tp->base.name, pool->obj_name, PJ_MAX_OBJ_NAME);
    tp->base.op = &transport_udp_op;
    tp->base.type = PJMEDIA_TRANSPORT_TYPE_UDP;

    /* Copy socket infos */
    tp->rtp_sock = si->rtp_sock;
    tp->rtp_addr_name = si->rtp_addr_name;
    tp->rtcp_sock = si->rtcp_sock;
    tp->rtcp_addr_name = si->rtcp_addr_name;

    /* If address is 0.0.0.0, use host's IP address */
    if (!pj_sockaddr_has_addr(&tp->rtp_addr_name))
	 {
		pj_sockaddr hostip;
		status = pj_gethostip(tp->rtp_addr_name.addr.sa_family, &hostip);
		if (status != PJ_SUCCESS)
	   	 goto on_error;

		pj_memcpy(pj_sockaddr_get_addr(&tp->rtp_addr_name), 
		  pj_sockaddr_get_addr(&hostip),
		  pj_sockaddr_get_addr_len(&hostip));
    }

    /* Same with RTCP */
    if (!pj_sockaddr_has_addr(&tp->rtcp_addr_name))
	 {
		pj_memcpy(pj_sockaddr_get_addr(&tp->rtcp_addr_name),
		  pj_sockaddr_get_addr(&tp->rtp_addr_name),
		  pj_sockaddr_get_addr_len(&tp->rtp_addr_name));
    }

    /* Setup RTP socket with the ioqueue */
    pj_bzero(&rtp_cb, sizeof(rtp_cb));
    rtp_cb.on_read_complete = &on_rx_rtp;

    status = pj_ioqueue_register_sock(pool, ioqueue, tp->rtp_sock, tp,
				      &rtp_cb, &tp->rtp_key);
    if (status != PJ_SUCCESS)
	goto on_error;
    
    /* Disallow concurrency so that detach() and destroy() are
     * synchronized with the callback.
     */
    status = pj_ioqueue_set_concurrency(tp->rtp_key, PJ_FALSE);
    if (status != PJ_SUCCESS)
	goto on_error;

    pj_ioqueue_op_key_init(&tp->rtp_read_op, sizeof(tp->rtp_read_op));
    for (i=0; i<PJ_ARRAY_SIZE(tp->rtp_pending_write); ++i)
	pj_ioqueue_op_key_init(&tp->rtp_pending_write[i].op_key, 
			       sizeof(tp->rtp_pending_write[i].op_key));

    /* Kick of pending RTP read from the ioqueue */
    tp->rtp_addrlen = sizeof(tp->rtp_src_addr);
    size = sizeof(tp->rtp_pkt);
    status = pj_ioqueue_recvfrom(tp->rtp_key, &tp->rtp_read_op,
			         tp->rtp_pkt, &size, PJ_IOQUEUE_ALWAYS_ASYNC,
				 &tp->rtp_src_addr, &tp->rtp_addrlen);
    if (status != PJ_EPENDING)
	goto on_error;

    /* Setup RTCP socket with ioqueue */
    pj_bzero(&rtcp_cb, sizeof(rtcp_cb));
    rtcp_cb.on_read_complete = &on_rx_rtcp;

    status = pj_ioqueue_register_sock(pool, ioqueue, tp->rtcp_sock, tp,
				      &rtcp_cb, &tp->rtcp_key);
    if (status != PJ_SUCCESS)
	goto on_error;

    status = pj_ioqueue_set_concurrency(tp->rtcp_key, PJ_FALSE);
    if (status != PJ_SUCCESS)
	goto on_error;

    pj_ioqueue_op_key_init(&tp->rtcp_read_op, sizeof(tp->rtcp_read_op));
    pj_ioqueue_op_key_init(&tp->rtcp_write_op, sizeof(tp->rtcp_write_op));


    /* Kick of pending RTCP read from the ioqueue */
    size = sizeof(tp->rtcp_pkt);
    tp->rtcp_addr_len = sizeof(tp->rtcp_src_addr);
    status = pj_ioqueue_recvfrom( tp->rtcp_key, &tp->rtcp_read_op,
				  tp->rtcp_pkt, &size, PJ_IOQUEUE_ALWAYS_ASYNC,
				  &tp->rtcp_src_addr, &tp->rtcp_addr_len);
    if (status != PJ_EPENDING)
	goto on_error;

    /* Done */
    *p_tp = &tp->base;
    return PJ_SUCCESS;


on_error:
    transport_destroy(&tp->base);
    return status;
}

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
pj_uint16_t pjmedia_transport_get_rtp_port(pjmedia_transport* med_tp)
{
	struct transport_udp* tp = (struct transport_udp*)med_tp;
	return pj_sockaddr_get_port(&tp->rtp_addr_name);
}

pj_uint16_t pjmedia_transport_get_rtcp_port(pjmedia_transport* med_tp)
{
	struct transport_udp* tp = (struct transport_udp*)med_tp;
	return pj_sockaddr_get_port(&tp->rtcp_addr_name);
}
pj_status_t pjmedia_transport_udp_attach_for_single_call(pjmedia_endpt* endpt,
																			pjmedia_transport* med_tp,
																			pj_sock_t sock[2])
{
CMSIP_PRINT("--------attach media transport only for single call---------------------");
	pj_ioqueue_t *ioqueue;
   pj_ioqueue_callback rtp_cb, rtcp_cb;
	unsigned i;
	pj_ssize_t size;
	pj_status_t status;
	struct transport_udp* tp = (struct transport_udp*)med_tp;

	tp->rtp_sock = sock[0];
	tp->rtcp_sock = sock[1];

	 /* Get ioqueue instance */
    ioqueue = pjmedia_endpt_get_ioqueue(endpt);

    /* Setup RTP socket with the ioqueue */
    pj_bzero(&rtp_cb, sizeof(rtp_cb));
    rtp_cb.on_read_complete = &on_rx_rtp;

	CMSIP_PRINT("pool[%p], ioqueue[%p], socket[%d]", tp->pool, ioqueue, tp->rtp_sock);

    status = pj_ioqueue_register_sock(tp->pool, ioqueue, tp->rtp_sock, tp,
				      &rtp_cb, &tp->rtp_key);
    if (status != PJ_SUCCESS)
    {
    	CMSIP_PRINT("status[%d]", status);
		return status;
    }
    
    /* Disallow concurrency so that detach() and destroy() are
     * synchronized with the callback.
     */
    status = pj_ioqueue_set_concurrency(tp->rtp_key, PJ_FALSE);
    if (status != PJ_SUCCESS)
    {
     	CMSIP_PRINT("status[%d]", status);
		return status;
    }

    pj_ioqueue_op_key_init(&tp->rtp_read_op, sizeof(tp->rtp_read_op));
    for (i=0; i<PJ_ARRAY_SIZE(tp->rtp_pending_write); ++i)
	pj_ioqueue_op_key_init(&tp->rtp_pending_write[i].op_key, 
			       sizeof(tp->rtp_pending_write[i].op_key));

    /* Kick of pending RTP read from the ioqueue */
    tp->rtp_addrlen = sizeof(tp->rtp_src_addr);
    size = sizeof(tp->rtp_pkt);
    status = pj_ioqueue_recvfrom(tp->rtp_key, &tp->rtp_read_op,
			         tp->rtp_pkt, &size, PJ_IOQUEUE_ALWAYS_ASYNC,
				 &tp->rtp_src_addr, &tp->rtp_addrlen);
    if (status != PJ_SUCCESS && status != PJ_EPENDING)
    {
     	CMSIP_PRINT("status[%d]", status);
		return status;
    }

    /* Setup RTCP socket with ioqueue */
    pj_bzero(&rtcp_cb, sizeof(rtcp_cb));
    rtcp_cb.on_read_complete = &on_rx_rtcp;

	CMSIP_PRINT("pool[%p], ioqueue[%p], socket[%d]", tp->pool, ioqueue, tp->rtcp_sock);

    status = pj_ioqueue_register_sock(tp->pool, ioqueue, tp->rtcp_sock, tp,
				      &rtcp_cb, &tp->rtcp_key);
    if (status != PJ_SUCCESS)
    {
     	CMSIP_PRINT("status[%d]", status);

		return status;
    }

    status = pj_ioqueue_set_concurrency(tp->rtcp_key, PJ_FALSE);
    if (status != PJ_SUCCESS)
    {
     	CMSIP_PRINT("status[%d]", status);

		return status;
    }

    pj_ioqueue_op_key_init(&tp->rtcp_read_op, sizeof(tp->rtcp_read_op));
    pj_ioqueue_op_key_init(&tp->rtcp_write_op, sizeof(tp->rtcp_write_op));


    /* Kick of pending RTCP read from the ioqueue */
    size = sizeof(tp->rtcp_pkt);
    tp->rtcp_addr_len = sizeof(tp->rtcp_src_addr);
    status = pj_ioqueue_recvfrom( tp->rtcp_key, &tp->rtcp_read_op,
				  tp->rtcp_pkt, &size, PJ_IOQUEUE_ALWAYS_ASYNC,
				  &tp->rtcp_src_addr, &tp->rtcp_addr_len);
    if (status != PJ_SUCCESS && status != PJ_EPENDING)
    {
     	CMSIP_PRINT("status[%d]", status);

		return status;
    }

	 return PJ_SUCCESS;
}

pj_status_t pjmedia_transport_udp_destroy_for_single_call(pjmedia_transport* tp)
{

CMSIP_PRINT("destroy current call media transport--------------------------");
    struct transport_udp *udp = (struct transport_udp*) tp;

    /* Sanity check */
    PJ_ASSERT_RETURN(tp, PJ_EINVAL);

    /* Must not close while application is using this */
    //PJ_ASSERT_RETURN(!udp->attached, PJ_EINVALIDOP);
    
    if (udp->rtp_key)
	 {
	 	CMSIP_PRINT("transport rtp key not null----------------------------");
		/* This will block the execution if callback is still
		 * being called.
		 */
		pj_ioqueue_unregister(udp->rtp_key);
		udp->rtp_key = NULL;
		udp->rtp_sock = PJ_INVALID_SOCKET;
    }
	 else if (udp->rtp_sock != PJ_INVALID_SOCKET)
	 {
	 	CMSIP_PRINT("close rtp socket--------------------------------------");
		pj_sock_close(udp->rtp_sock);
		udp->rtp_sock = PJ_INVALID_SOCKET;
    }

    if (udp->rtcp_key)
	 {
 	 	CMSIP_PRINT("transport rtcp key not null----------------------------");

		pj_ioqueue_unregister(udp->rtcp_key);
		udp->rtcp_key = NULL;
		udp->rtcp_sock = PJ_INVALID_SOCKET;
    }
	 else if (udp->rtcp_sock != PJ_INVALID_SOCKET)
	 {
 	 	CMSIP_PRINT("close rtcp socket--------------------------------------");

		pj_sock_close(udp->rtcp_sock);
		udp->rtcp_sock = PJ_INVALID_SOCKET;
    }

    return PJ_SUCCESS;
}
#	endif

#	ifdef SUPPORT_METABOLIC_MEDIA_PORT
pj_status_t pjmedia_transport_udp_set_mediaPort(pjmedia_transport* med_tp, 
			pj_uint16_t port)
{
	pj_status_t status;
	struct transport_udp* tp = (struct transport_udp*)med_tp;
	status = pj_sockaddr_set_port(&tp->rtp_addr_name, port);
	if (status != PJ_SUCCESS) return PJ_EINVAL;

	status = pj_sockaddr_set_port(&tp->rtcp_addr_name, port+1);
	if (status != PJ_SUCCESS) return PJ_EINVAL;

	return PJ_SUCCESS;
}
#	endif


/*brief ycw-pjsip-note. Get local ip and port info. But this may be public address.
I think you must get the actual address.*/
/*ycw-pjsip-note*/
void  pjmedia_transport_udp_local_info(const pjmedia_transport* tp, char* localIp, 
																pj_uint16_t* localRtpPort, pj_uint16_t* localRtcpPort)
{
	pj_sockaddr tmpAddr;
	char lRtpIP[64];
	pj_uint16_t lRtpPort;
	pj_uint16_t lRtcpPort;
	struct transport_udp* udp_tp = (struct transport_udp*)tp;
	
	/*get remote rtp socket's IP:Port*/
	tmpAddr = udp_tp->rtp_addr_name;
   pj_inet_ntop(tmpAddr.addr.sa_family, pj_sockaddr_get_addr(&tmpAddr), lRtpIP, sizeof(lRtpIP));
	lRtpPort = pj_sockaddr_get_port(&tmpAddr);

	/*get remote rtcp socket's port*/
	tmpAddr = udp_tp->rtcp_addr_name;
   if (pj_sockaddr_has_addr(&tmpAddr))
	{
		/*ycw-pjsip:假如RTCP真的有另外一个地址，怎么办?
		RarLink的参数不需要!
		*/
		lRtcpPort = pj_sockaddr_get_port(&tmpAddr);
   }
	else
	{	
		lRtcpPort = lRtpPort + 1;
   }

	strcpy(localIp, lRtpIP);
	*localRtpPort = lRtpPort;
	*localRtcpPort = lRtcpPort;

#if 0
	struct pjmedia_transport_info	tpInfo;
	pj_sock_t	localRtpSock;
	pj_sock_t	localRtcpSock;
	pj_sockaddr	localAddress;
	int namelen;
	pj_uint16_t	lPort;
	pj_uint16_t lRtcpPort;
	char localIp_tmp[CMSIP_STR_64];
	
	pjmedia_transport_get_info(tp, &tpInfo);

	/*get local rtp socket's IP:Port*/	
	localRtpSock = tpInfo.sock_info.rtp_sock;
	namelen = sizeof(localAddress);
	pj_sock_getsockname(localRtpSock, &localAddress, &namelen);
	pj_inet_ntop(localAddress.addr.sa_family, pj_sockaddr_get_addr(&localAddress), localIp_tmp, sizeof(localIp_tmp));
	if (localAddress.addr.sa_family == pj_AF_INET())
	{
		lPort = pj_ntohs(localAddress.ipv4.sin_port); 
	}
	else
	{
		lPort = pj_ntohs(localAddress.ipv6.sin6_port);
	}

	/*get local rtcp socket's IP:Port*/	
	localRtcpSock = tpInfo.sock_info.rtcp_sock;
	namelen = sizeof(localAddress);
	pj_sock_getsockname(localRtcpSock, &localAddress, &namelen);
	if (localAddress.addr.sa_family == pj_AF_INET())
	{
		lRtcpPort = pj_ntohs(localAddress.ipv4.sin_port); 
	}
	else
	{
		lRtcpPort = pj_ntohs(localAddress.ipv6.sin6_port);
	}

	strcpy(localIp, localIp_tmp);
	*localRtpPort = lPort;
	*localRtcpPort = lRtcpPort;
#endif
}


/* 
 * brief	get remote info(ip,rtp port,rtcp port) of pjmeia transport!!!
 */
void pjmedia_transport_udp_remote_info(const pjmedia_transport *tp,
							char* remoteIp, pj_uint16_t *rtpPort, pj_uint16_t *rtcpPort)
{
	pj_sockaddr tmpAddr;
	char remoteRtpIP[64];
	pj_uint16_t remoteRtpPort;
	pj_uint16_t remoteRtcpPort;
	struct transport_udp* udp_tp = (struct transport_udp*)tp;
	
	/*get remote rtp socket's IP:Port*/
	tmpAddr = udp_tp->rem_rtp_addr;	
   pj_inet_ntop(tmpAddr.addr.sa_family, pj_sockaddr_get_addr(&tmpAddr), remoteRtpIP, sizeof(remoteRtpIP));
	remoteRtpPort = pj_sockaddr_get_port(&tmpAddr);

	/*get remote rtcp socket's port*/
	tmpAddr = udp_tp->rem_rtcp_addr;	
   if (pj_sockaddr_has_addr(&tmpAddr))
	{
		//TODO:假如RTCP真的有另外一个地址，怎么办?	RarLink的参数不需要!
		remoteRtcpPort = pj_sockaddr_get_port(&tmpAddr);
   }
	else
	{	
		remoteRtcpPort = remoteRtpPort + 1;
   }

	strcpy(remoteIp, remoteRtpIP);
	*rtpPort = remoteRtpPort;
	*rtcpPort = remoteRtcpPort;

}

/*ycw-pjsip*/
char* pjmedia_transport_udp_get_rtp_pkt(pjmedia_transport *tp)
{
	struct transport_udp* udp = (struct transport_udp*)tp;
	return udp->rtp_pkt;
}
/*ycw-pjsip*/
char* pjmedia_transport_udp_get_rtcp_pkt(pjmedia_transport *tp)
{
	struct transport_udp* udp = (struct transport_udp*)tp;
	return udp->rtcp_pkt;
}

/**
 * Close UDP transport.
 */
static pj_status_t transport_destroy(pjmedia_transport *tp)
{
    struct transport_udp *udp = (struct transport_udp*) tp;

    /* Sanity check */
    PJ_ASSERT_RETURN(tp, PJ_EINVAL);

    /* Must not close while application is using this */
    //PJ_ASSERT_RETURN(!udp->attached, PJ_EINVALIDOP);
    
    if (udp->rtp_key) {
	/* This will block the execution if callback is still
	 * being called.
	 */
	pj_ioqueue_unregister(udp->rtp_key);
	udp->rtp_key = NULL;
	udp->rtp_sock = PJ_INVALID_SOCKET;
    } else if (udp->rtp_sock != PJ_INVALID_SOCKET) {
	pj_sock_close(udp->rtp_sock);
	udp->rtp_sock = PJ_INVALID_SOCKET;
    }

    if (udp->rtcp_key) {
	pj_ioqueue_unregister(udp->rtcp_key);
	udp->rtcp_key = NULL;
	udp->rtcp_sock = PJ_INVALID_SOCKET;
    } else if (udp->rtcp_sock != PJ_INVALID_SOCKET) {
	pj_sock_close(udp->rtcp_sock);
	udp->rtcp_sock = PJ_INVALID_SOCKET;
    }

    pj_pool_release(udp->pool);

    return PJ_SUCCESS;
}


/* Notification from ioqueue about incoming RTP packet */
static void on_rx_rtp( pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, pj_ssize_t bytes_read)
{
    struct transport_udp *udp;
    pj_status_t status;

    PJ_UNUSED_ARG(op_key);

    udp = (struct transport_udp*) pj_ioqueue_get_user_data(key);

    do
	 {		
		void (*cb)(void*,void*,pj_ssize_t);
		void *user_data;

		cb = udp->rtp_cb;
		user_data = udp->user_data;

		/*ycw-pjsip--*/
		#if 0
		/* Simulate packet lost on RX direction */		
		if (udp->rx_drop_pct)
		{
	   	 if ((pj_rand() % 100) <= (int)udp->rx_drop_pct)
			 {
				PJ_LOG(5,(udp->base.name, 
			  "RX RTP packet dropped because of pkt lost "
			  "simulation"));
				goto read_next_packet;
	    	 }
		}
		#endif


		if (udp->attached && cb)
		{
			/*ycw-pjsip*/
			#if 0
	   	(*cb)(user_data, udp->rtp_pkt, bytes_read);
			#else
			(*cb)(user_data, udp, bytes_read);
			#endif
		}

	/* See if source address of RTP packet is different than the 
	 * configured address, and switch RTP remote address to 
	 * source packet address after several consecutive packets
	 * have been received.
	 */
		if (bytes_read>0 && 
	    	(udp->options & PJMEDIA_UDP_NO_SRC_ADDR_CHECKING)==0) 
		{
			 if (0 && pj_sockaddr_cmp(&udp->rem_rtp_addr, &udp->rtp_src_addr) != 0)
			 {
			 	udp->rtp_src_cnt++;
				if (udp->rtp_src_cnt >= PJMEDIA_RTP_NAT_PROBATION_CNT)
				{		
#if 4 <= PJ_LOG_MAX_LEVEL
		    		char addr_text[80];
#endif

		    		/* Set remote RTP address to source address */
		    		pj_memcpy(&udp->rem_rtp_addr, &udp->rtp_src_addr,
			      	sizeof(pj_sockaddr));

		    		/* Reset counter */
		    		udp->rtp_src_cnt = 0;

		    		PJ_LOG(4,(udp->base.name,
			      "Remote RTP address switched to %s",
			      pj_sockaddr_print(&udp->rtp_src_addr, addr_text,
						sizeof(addr_text), 3)));

			    /* Also update remote RTCP address if actual RTCP source
			     * address is not heard yet.
			     */
			    if (!pj_sockaddr_has_addr(&udp->rtcp_src_addr))
				 {
					pj_uint16_t port;

					pj_memcpy(&udp->rem_rtcp_addr, &udp->rem_rtp_addr, sizeof(pj_sockaddr));
					pj_sockaddr_copy_addr(&udp->rem_rtcp_addr,
					      &udp->rem_rtp_addr);
					port = (pj_uint16_t)
			       (pj_sockaddr_get_port(&udp->rem_rtp_addr)+1);
					pj_sockaddr_set_port(&udp->rem_rtcp_addr, port);

					pj_memcpy(&udp->rtcp_src_addr, &udp->rem_rtcp_addr, 
				  		sizeof(pj_sockaddr));

					PJ_LOG(4,(udp->base.name,
				  "Remote RTCP address switched to %s",
				  pj_sockaddr_print(&udp->rtcp_src_addr, 
						    addr_text,
						    sizeof(addr_text), 3)));

		    	}
			}
	    }
	}

read_next_packet:
	bytes_read = sizeof(udp->rtp_pkt);
	udp->rtp_addrlen = sizeof(udp->rtp_src_addr);
	status = pj_ioqueue_recvfrom(udp->rtp_key, &udp->rtp_read_op,
				     udp->rtp_pkt, &bytes_read, 0,
				     &udp->rtp_src_addr, 
				     &udp->rtp_addrlen);

	if (status != PJ_EPENDING && status != PJ_SUCCESS)
	    bytes_read = -status;

    } while (status != PJ_EPENDING && status != PJ_ECANCELLED);
}


/* Notification from ioqueue about incoming RTCP packet */
static void on_rx_rtcp(pj_ioqueue_key_t *key, 
                       pj_ioqueue_op_key_t *op_key, 
                       pj_ssize_t bytes_read)
{
    struct transport_udp *udp;
    pj_status_t status;

    PJ_UNUSED_ARG(op_key);

    udp = (struct transport_udp*) pj_ioqueue_get_user_data(key);

    do
	 {
		void (*cb)(void*,void*,pj_ssize_t);
		void *user_data;

		cb = udp->rtcp_cb;
		user_data = udp->user_data;

		if (udp->attached && cb)
		{
			/*ycw-pjsip*/
			#if 0
	   	(*cb)(user_data, udp->rtcp_pkt, bytes_read);
			#else
			(*cb)(user_data, udp, bytes_read);
			#endif
		}


	/* Check if RTCP source address is the same as the configured
	 * remote address, and switch the address when they are
	 * different.
	 */
	if (0 && bytes_read>0 &&
	    (udp->options & PJMEDIA_UDP_NO_SRC_ADDR_CHECKING)==0 &&
	    pj_sockaddr_cmp(&udp->rem_rtcp_addr, &udp->rtcp_src_addr) != 0)
	{
#if 4 <= PJ_LOG_MAX_LEVEL
	    char addr_text[80];
#endif

	    pj_memcpy(&udp->rem_rtcp_addr, &udp->rtcp_src_addr,
		      sizeof(pj_sockaddr));

	    PJ_LOG(4,(udp->base.name,
		      "Remote RTCP address switched to %s",
		      pj_sockaddr_print(&udp->rtcp_src_addr, addr_text,
					sizeof(addr_text), 3)));
	}

	bytes_read = sizeof(udp->rtcp_pkt);
	udp->rtcp_addr_len = sizeof(udp->rtcp_src_addr);
	status = pj_ioqueue_recvfrom(udp->rtcp_key, &udp->rtcp_read_op,
				     udp->rtcp_pkt, &bytes_read, 0,
				     &udp->rtcp_src_addr, 
				     &udp->rtcp_addr_len);
	if (status != PJ_EPENDING && status != PJ_SUCCESS)
	    bytes_read = -status;

    } while (status != PJ_EPENDING && status != PJ_ECANCELLED);
}


/* Called to get the transport info */
static pj_status_t transport_get_info(pjmedia_transport *tp,
				      pjmedia_transport_info *info)
{
    struct transport_udp *udp = (struct transport_udp*)tp;
    PJ_ASSERT_RETURN(tp && info, PJ_EINVAL);

    info->sock_info.rtp_sock = udp->rtp_sock;
    info->sock_info.rtp_addr_name = udp->rtp_addr_name;
    info->sock_info.rtcp_sock = udp->rtcp_sock;
    info->sock_info.rtcp_addr_name = udp->rtcp_addr_name;

    /* Get remote address originating RTP & RTCP. */
    info->src_rtp_name  = udp->rtp_src_addr;
    info->src_rtcp_name = udp->rtcp_src_addr;

    return PJ_SUCCESS;
}


/* Called by application to initialize the transport */
static pj_status_t transport_attach(   pjmedia_transport *tp,
				       void *user_data,
				       const pj_sockaddr_t *rem_addr,
				       const pj_sockaddr_t *rem_rtcp,
				       unsigned addr_len,
				       void (*rtp_cb)(void*,
						      void*,
						      pj_ssize_t),
				       void (*rtcp_cb)(void*,
						       void*,
						       pj_ssize_t))
{
    struct transport_udp *udp = (struct transport_udp*) tp;
    const pj_sockaddr *rtcp_addr;

    /* Validate arguments */
    PJ_ASSERT_RETURN(tp && rem_addr && addr_len, PJ_EINVAL);

    /* Must not be "attached" to existing application */
    PJ_ASSERT_RETURN(!udp->attached, PJ_EINVALIDOP);

    /* Lock the ioqueue keys to make sure that callbacks are
     * not executed. See ticket #844 for details.
     */
	/*ycw-pjsip.对于由DSP直接发送版本来说，假如是普通呼叫，那么是不会将udp transport
	 注册到ioqueue去的，也就是说rtp_key和rtcp_key均为空；但假如启动了usbvm功能，那么就会
	 将udp transport注册到ioqueue。为了兼顾这两种情况，这里增加了判断。*/
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtp_key != NULL)
	{
#	endif
   	pj_ioqueue_lock_key(udp->rtp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtcp_key)
	{
#	endif
   	pj_ioqueue_lock_key(udp->rtcp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

    /* "Attach" the application: */

    /* Copy remote RTP address */
    pj_memcpy(&udp->rem_rtp_addr, rem_addr, addr_len);

    /* Copy remote RTP address, if one is specified. */
    rtcp_addr = (const pj_sockaddr*) rem_rtcp;
    if (rtcp_addr && pj_sockaddr_has_addr(rtcp_addr)) {
	pj_memcpy(&udp->rem_rtcp_addr, rem_rtcp, addr_len);

    } else {
	unsigned rtcp_port;

	/* Otherwise guess the RTCP address from the RTP address */
	pj_memcpy(&udp->rem_rtcp_addr, rem_addr, addr_len);
	rtcp_port = pj_sockaddr_get_port(&udp->rem_rtp_addr) + 1;
	pj_sockaddr_set_port(&udp->rem_rtcp_addr, (pj_uint16_t)rtcp_port);
    }
	 
    /* Save the callbacks */
    udp->rtp_cb = rtp_cb;
    udp->rtcp_cb = rtcp_cb;
    udp->user_data = user_data;

    /* Save address length */
    udp->addr_len = addr_len;

    /* Last, mark transport as attached */
    udp->attached = PJ_TRUE;

    /* Reset source RTP & RTCP addresses and counter */
    pj_bzero(&udp->rtp_src_addr, sizeof(udp->rtp_src_addr));
    pj_bzero(&udp->rtcp_src_addr, sizeof(udp->rtcp_src_addr));
    udp->rtp_src_cnt = 0;

    /* Unlock keys */
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtcp_key != NULL)
	{
#	endif
    	pj_ioqueue_unlock_key(udp->rtcp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtp_key != NULL)
	{
#	endif	
    	pj_ioqueue_unlock_key(udp->rtp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

    return PJ_SUCCESS;
}


/* Called by application when it no longer needs the transport */
static void transport_detach( pjmedia_transport *tp,
			      void *user_data)
{
    struct transport_udp *udp = (struct transport_udp*) tp;

    pj_assert(tp);

    if (udp->attached)
	 {
		/* Lock the ioqueue keys to make sure that callbacks are
	 	* not executed. See ticket #460 for details.
	 	*/
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtp_key != NULL)
	{
#	endif
   	pj_ioqueue_lock_key(udp->rtp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtcp_key)
	{
#	endif
   	pj_ioqueue_lock_key(udp->rtcp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

		/* User data is unreferenced on Release build */
		PJ_UNUSED_ARG(user_data);

		/* As additional checking, check if the same user data is specified */
		pj_assert(user_data == udp->user_data);

		/* First, mark transport as unattached */
		udp->attached = PJ_FALSE;

		/* Clear up application infos from transport */
		udp->rtp_cb = NULL;
		udp->rtcp_cb = NULL;
		udp->user_data = NULL;

		/* Unlock keys */
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtcp_key != NULL)
	{
#	endif
    	pj_ioqueue_unlock_key(udp->rtcp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	if (udp->rtp_key != NULL)
	{
#	endif	
    	pj_ioqueue_unlock_key(udp->rtp_key);
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
	}
#	endif
    }
}

/* Called by application to send RTP packet */
static pj_status_t transport_send_rtp( pjmedia_transport *tp,
				       const void *pkt,
				       pj_size_t size)
{
    struct transport_udp *udp = (struct transport_udp*)tp;
    pj_ssize_t sent;
    unsigned id;
    struct pending_write *pw;
    pj_status_t status;

    /* Must be attached */
	 /*ycw-pjsip. 20111104*/
	 #if 0
    PJ_ASSERT_RETURN(udp->attached, PJ_EINVALIDOP);
	 #else
	 if (!udp || PJ_FALSE == udp->attached)
	 {
	 	PJ_LOG(1, (udp->base.name, "This UDP transport has been destroyed!!!"));
	 	return PJ_EBUG;
	 }
	 #endif

    /* Check that the size is supported */
    PJ_ASSERT_RETURN(size <= RTP_LEN, PJ_ETOOBIG);

    /* Simulate packet lost on TX direction */
    if (udp->tx_drop_pct)
	 {
		if ((pj_rand() % 100) <= (int)udp->tx_drop_pct)
		{
	   	PJ_LOG(5,(udp->base.name, 
		      "TX RTP packet dropped because of pkt lost "
		      "simulation"));
	    	return PJ_SUCCESS;
		}
    }


    id = udp->rtp_write_op_id;
    pw = &udp->rtp_pending_write[id];

    /* We need to copy packet to our buffer because when the
     * operation is pending, caller might write something else
     * to the original buffer.
     */
    pj_memcpy(pw->buffer, pkt, size);

    sent = size;
    status = pj_ioqueue_sendto( udp->rtp_key, 
				&udp->rtp_pending_write[id].op_key,
				pw->buffer, &sent, 0,
				&udp->rem_rtp_addr, 
				udp->addr_len);

    udp->rtp_write_op_id = (udp->rtp_write_op_id + 1) %
			   PJ_ARRAY_SIZE(udp->rtp_pending_write);

    if (status==PJ_SUCCESS || status==PJ_EPENDING)
	return PJ_SUCCESS;

    return status;
}

/* Called by application to send RTCP packet */
static pj_status_t transport_send_rtcp(pjmedia_transport *tp,
				       const void *pkt,
				       pj_size_t size)
{
    return transport_send_rtcp2(tp, NULL, 0, pkt, size);
}


/* Called by application to send RTCP packet */
static pj_status_t transport_send_rtcp2(pjmedia_transport *tp,
					const pj_sockaddr_t *addr,
					unsigned addr_len,
				        const void *pkt,
				        pj_size_t size)
{
    struct transport_udp *udp = (struct transport_udp*)tp;
    pj_ssize_t sent;
    pj_status_t status;

    PJ_ASSERT_RETURN(udp->attached, PJ_EINVALIDOP);

    if (addr == NULL) {
	addr = &udp->rem_rtcp_addr;
	addr_len = udp->addr_len;
    }

    sent = size;
    status = pj_ioqueue_sendto( udp->rtcp_key, &udp->rtcp_write_op,
				pkt, &sent, 0, addr, addr_len);

    if (status==PJ_SUCCESS || status==PJ_EPENDING)
	return PJ_SUCCESS;

    return status;
}

static pj_status_t transport_media_create(pjmedia_transport *tp,
				  pj_pool_t *pool,
				  unsigned options,
				  const pjmedia_sdp_session *sdp_remote,
				  unsigned media_index)
{
    struct transport_udp *udp = (struct transport_udp*)tp;

    PJ_ASSERT_RETURN(tp && pool, PJ_EINVAL);
    udp->media_options = options;

    PJ_UNUSED_ARG(sdp_remote);
    PJ_UNUSED_ARG(media_index);

    return PJ_SUCCESS;
}

static pj_status_t transport_encode_sdp(pjmedia_transport *tp,
				        pj_pool_t *pool,
				        pjmedia_sdp_session *sdp_local,
				        const pjmedia_sdp_session *rem_sdp,
				        unsigned media_index)
{
    struct transport_udp *udp = (struct transport_udp*)tp;

    /* Validate media transport */
    /* By now, this transport only support RTP/AVP transport */
    if ((udp->media_options & PJMEDIA_TPMED_NO_TRANSPORT_CHECKING) == 0)
	 {
		pjmedia_sdp_media *m_rem, *m_loc;

		m_rem = rem_sdp? rem_sdp->media[media_index] : NULL;
		m_loc = sdp_local->media[media_index];

/*ycw-pjsip. t38*/
#if 0 // Atheros: was
	if (pj_stricmp(&m_loc->desc.transport, &ID_RTP_AVP) ||
	   (m_rem && pj_stricmp(&m_rem->desc.transport, &ID_RTP_AVP)))
#else
    if ((pj_stricmp(&m_loc->desc.transport, &ID_RTP_AVP) && pj_stricmp(&m_loc->desc.transport, &ID_UDPTL)) || // rtp/avp or udptl
        (m_rem && pj_stricmp(&m_rem->desc.transport, &ID_RTP_AVP) && pj_stricmp(&m_rem->desc.transport, &ID_UDPTL)))
#endif // Atheros: end
		{
	   	pjmedia_sdp_media_deactivate(pool, m_loc);
	   	return PJMEDIA_SDP_EINPROTO;
		}
    }

    return PJ_SUCCESS;
}

static pj_status_t transport_media_start(pjmedia_transport *tp,
				  pj_pool_t *pool,
				  const pjmedia_sdp_session *sdp_local,
				  const pjmedia_sdp_session *sdp_remote,
				  unsigned media_index)
{
    PJ_ASSERT_RETURN(tp && pool && sdp_local, PJ_EINVAL);

    PJ_UNUSED_ARG(tp);
    PJ_UNUSED_ARG(pool);
    PJ_UNUSED_ARG(sdp_local);
    PJ_UNUSED_ARG(sdp_remote);
    PJ_UNUSED_ARG(media_index);

    return PJ_SUCCESS;
}

static pj_status_t transport_media_stop(pjmedia_transport *tp)
{
    PJ_UNUSED_ARG(tp);

    return PJ_SUCCESS;
}

static pj_status_t transport_simulate_lost(pjmedia_transport *tp,
					   pjmedia_dir dir,
					   unsigned pct_lost)
{
    struct transport_udp *udp = (struct transport_udp*)tp;

    PJ_ASSERT_RETURN(tp && pct_lost <= 100, PJ_EINVAL);

    if (dir & PJMEDIA_DIR_ENCODING)
	udp->tx_drop_pct = pct_lost;
    
    if (dir & PJMEDIA_DIR_DECODING)
	udp->rx_drop_pct = pct_lost;

    return PJ_SUCCESS;
}

