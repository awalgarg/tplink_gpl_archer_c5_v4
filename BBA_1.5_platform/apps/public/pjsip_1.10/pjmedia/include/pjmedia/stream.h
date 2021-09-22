/* $Id: stream.h 3553 2011-05-05 06:14:19Z nanang $ */
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
#ifndef __PJMEDIA_STREAM_H__
#define __PJMEDIA_STREAM_H__


/**
 * @file stream.h
 * @brief Media Stream.
 */

#include <pjmedia/codec.h>
#include <pjmedia/endpoint.h>
/* 
 * brief	yuchuwei@20120413
 */
#	if 0
#include <pjmedia/jbuf.h>
#include <pjmedia/port.h>
#	endif
#include <pjmedia/rtcp.h>
#include <pjmedia/transport.h>
#include <pj/sock.h>
#include <pjlib-util.h>

PJ_BEGIN_DECL

/*ycw-pjsip*/
#define DSP_RTP_PORT_START 10000
extern pj_uint16_t g_dspPort;

/**
 * @defgroup PJMED_STRM Streams
 * @ingroup PJMEDIA_PORT
 * @brief Communicating with remote peer via the network
 * @{
 *
 * A media stream is a bidirectional multimedia communication between two
 * endpoints. It corresponds to a media description (m= line) in SDP
 * session descriptor.
 *
 * A media stream consists of two unidirectional channels:
 *  - encoding channel, which transmits unidirectional media to remote, and
 *  - decoding channel, which receives unidirectional media from remote.
 *
 * A media stream exports media port interface (see @ref PJMEDIA_PORT)
 * and application normally uses this interface to interconnect the stream
 * to other PJMEDIA components.
 *
 * A media stream internally manages the following objects:
 *  - an instance of media codec (see @ref PJMEDIA_CODEC),
 *  - an @ref PJMED_JBUF,
 *  - two instances of RTP sessions (#pjmedia_rtp_session, one for each
 *    direction),
 *  - one instance of RTCP session (#pjmedia_rtcp_session),
 *  - and a reference to media transport to send and receive packets
 *    to/from the network (see @ref PJMEDIA_TRANSPORT).
 *
 * Streams are created by calling #pjmedia_stream_create(), specifying
 * #pjmedia_stream_info structure in the parameter. Application can construct
 * the #pjmedia_stream_info structure manually, or use 
 * #pjmedia_stream_info_from_sdp() or #pjmedia_session_info_from_sdp() 
 * functions to construct the #pjmedia_stream_info from local and remote 
 * SDP session descriptors.
 *
 * Application can also use @ref PJMEDIA_SESSION to indirectly create the
 * streams.
 */

#if 0
/**
 * Opaque declaration for media channel.
 * Media channel is unidirectional flow of media from sender to
 * receiver.
 */
typedef struct pjmedia_channel pjmedia_channel;
#endif

/** 
 * This structure describes media stream information. Each media stream
 * corresponds to one "m=" line in SDP session descriptor, and it has
 * its own RTP/RTCP socket pair.
 */
struct pjmedia_stream_info
{
    pjmedia_type	type;	    /**< Media type (audio, video)	    */
    pjmedia_tp_proto	proto;	    /**< Transport protocol (RTP/AVP, etc.) */
    pjmedia_dir		dir;	    /**< Media direction.		    */
	pj_sockaddr		local_addr; /*store local rtp network address. YuChuwei@20130809*/
	pj_sockaddr		local_rtcp; /*store local rtcp network address. YuChuwei@20130809*/
    pj_sockaddr		rem_addr;   /**< Remote RTP address		    */
    pj_sockaddr		rem_rtcp;   /**< Optional remote RTCP address. If
					 sin_family is zero, the RTP address
					 will be calculated from RTP.	    */
#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR != 0)
    pj_bool_t		rtcp_xr_enabled;
				    /**< Specify whether RTCP XR is enabled.*/
    pj_uint32_t		rtcp_xr_interval; /**< RTCP XR interval.            */
    pj_sockaddr		rtcp_xr_dest;/**<Additional remote RTCP XR address.
				         This is useful for third-party (e.g:
					 network monitor) to monitor the 
					 stream. If sin_family is zero, 
					 this will be ignored.		    */
#endif
    pjmedia_codec_info	fmt;	    /**< Incoming codec format info.	    */
    pjmedia_codec_param *param;	    /**< Optional codec param.		    */
    unsigned		tx_pt;	    /**< Outgoing codec paylaod type.	    */
    unsigned		tx_maxptime;/**< Outgoing codec max ptime.	    */
    int		        tx_event_pt;/**< Outgoing pt for telephone-events.  */
    int			rx_event_pt;/**< Incoming pt for telephone-events.  */
	 #if 0
    pj_uint32_t		ssrc;	    /**< RTP SSRC.			    */
    pj_uint32_t		rtp_ts;	    /**< Initial RTP timestamp.		    */
    pj_uint16_t		rtp_seq;    /**< Initial RTP sequence number.	    */
    pj_uint8_t		rtp_seq_ts_set;
				    /**< Bitmask flags if initial RTP sequence 
				         and/or timestamp for sender are set.
					 bit 0/LSB : sequence flag 
					 bit 1     : timestamp flag 	    */
    int			jb_init;    /**< Jitter buffer init delay in msec.  
					 (-1 for default).		    */
    int			jb_min_pre; /**< Jitter buffer minimum prefetch
					 delay in msec (-1 for default).    */
    int			jb_max_pre; /**< Jitter buffer maximum prefetch
					 delay in msec (-1 for default).    */
    int			jb_max;	    /**< Jitter buffer max delay in msec.   */
	#endif
	 /*ycw-pjsip. t38*/
    unsigned    ulptime; /**< Atheros:codec specific ptime */
	unsigned	dlptime; /**For MTK DSP parameter*/
	 int         vad;
    int         cng;


#if defined(PJMEDIA_STREAM_ENABLE_KA) && PJMEDIA_STREAM_ENABLE_KA!=0
    pj_bool_t		use_ka;	    /**< Stream keep-alive and NAT hole punch
					 (see #PJMEDIA_STREAM_ENABLE_KA)
					 is enabled?			    */
#endif
};


/**
 * @see pjmedia_stream_info.
 */
typedef struct pjmedia_stream_info pjmedia_stream_info;



/*ycw-pjsip t38*/
#define MAX_UDPTL   16     /* max number of incoming UDPTL packet burst */
#define MAX_UDPTL_BUF_SIZE   284         /*This matches T38 definition FR38_MAX_T38_PACKET */
struct pjmedia_udptl_buf
{
    int buf_len;
    void *pbuf;
};

struct dtmf
{
    int		    event;
    pj_uint32_t	    duration;
};


/*ycw-pjsip.*/
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP==0
typedef enum _PJMEDIA_TRANSPORT_TYPE
{
	PJMEDIA_TRANSPORT_GENERIC,
#	if defined(INCLUDE_USB_VOICEMAIL)
	PJMEDIA_TRANSPORT_USBVM,
#	endif
#	if defined(INCLUDE_PSTN_GATEWAY)
	PJMEDIA_TRANSPORT_VOIP2PSTN,
#	endif
}PJMEDIA_TRANSPORT_TYPE;
#	endif

/*
 * Forward declaration for stream (needed by transport).
 */
typedef struct pjmedia_stream pjmedia_stream;


/**
 * This structure describes media stream.
 * A media stream is bidirectional media transmission between two endpoints.
 * It consists of two channels, i.e. encoding and decoding channels.
 * A media stream corresponds to a single "m=" line in a SDP session
 * description.
 */
struct pjmedia_stream
{
    pjmedia_endpt	    *endpt;	    /**< Media endpoint.	    */
    pjmedia_codec_mgr	    *codec_mgr;	    /**< Codec manager instance.    */

	/*ycw-pjsip--delete pjmedia_port*/
	#if 0
    pjmedia_port	     port;	    /**< Port interface.	    */
	#else
	pj_str_t port_info_name;
	#endif

	#if 0
    pjmedia_channel	    *enc;	    /**< Encoding channel.	    */
    pjmedia_channel	    *dec;	    /**< Decoding channel.	    */
	#endif

    pjmedia_dir		     dir;	    /**< Stream direction.	    */
    void		    *user_data;	    /**< User data.		    */
	 #if 0
    pj_str_t		     cname;	    /**< SDES CNAME		    */
	 #endif

    pjmedia_transport	    *transport;	    /**< Stream transport.	    */

#	if (!(defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) || \
			defined(INCLUDE_PSTN_GATEWAY))
	 /*ycw-pjsip*/
	 pjmedia_transport		*dsp_transport;	/** <Stream DSP transport*/
#	endif

#if 0
    pjmedia_codec	    *codec;	    /**< Codec instance being used. */
    pjmedia_codec_param	     codec_param;   /**< Codec param.		    */
    pj_int16_t		    *enc_buf;	    /**< Encoding buffer, when enc's
						 ptime is different than dec.
						 Otherwise it's NULL.	    */

    unsigned		     enc_samples_per_pkt;
    unsigned		     enc_buf_size;  /**< Encoding buffer size, in
						 samples.		    */
    unsigned		     enc_buf_pos;   /**< First position in buf.	    */
    unsigned		     enc_buf_count; /**< Number of samples in the
						 encoding buffer.	    */

    unsigned		     plc_cnt;	    /**< # of consecutive PLC frames*/
    unsigned		     max_plc_cnt;   /**< Max # of PLC frames	    */

    unsigned		     vad_enabled;   /**< VAD enabled in param.	    */
    unsigned		     frame_size;    /**< Size of encoded base frame.*/
    pj_bool_t		     is_streaming;  /**< Currently streaming?. This
						 is used to put RTP marker
						 bit.			    */
    pj_uint32_t		     ts_vad_disabled;/**< TS when VAD was disabled. */
    pj_uint32_t		     tx_duration;   /**< TX duration in timestamp.  */
#endif

#if 0 /*ycw-pjsip*/
    pj_mutex_t		    *jb_mutex;
    pjmedia_jbuf	    *jb;	    /**< Jitter buffer.		    */
    char		     jb_last_frm;   /**< Last frame type from jb    */
    unsigned		     jb_last_frm_cnt;/**< Last JB frame type counter*/
#endif

#if 0
	/*ycw-pjsip. t38*/
    struct pjmedia_udptl_buf   udptl_buf[MAX_UDPTL];           /* T38 packet buffer*/
    int udptl_rindex, udptl_windex, udptl_n, total_udptl_rx, total_udptl_tx;
#endif

#if 0
    pjmedia_rtcp_session     rtcp;	    /**< RTCP for incoming RTP.	    */
    pj_uint32_t		     rtcp_last_tx;  /**< RTCP tx time in timestamp  */
    pj_uint32_t		     rtcp_interval; /**< Interval, in timestamp.    */
    pj_bool_t		     initial_rr;    /**< Initial RTCP RR sent	    */
#endif

#if 0
    /* RFC 2833 DTMF transmission queue: */
    int			     tx_event_pt;   /**< Outgoing pt for dtmf.	    */
    int			     tx_dtmf_count; /**< # of digits in tx dtmf buf.*/
    struct dtmf		     tx_dtmf_buf[32];/**< Outgoing dtmf queue.	    */

    /* Incoming DTMF: */
    int			     rx_event_pt;   /**< Incoming pt for dtmf.	    */
    int			     last_dtmf;	    /**< Current digit, or -1.	    */
    pj_uint32_t		     last_dtmf_dur; /**< Start ts for cur digit.    */
    unsigned		     rx_dtmf_count; /**< # of digits in dtmf rx buf.*/
    char		     rx_dtmf_buf[32];/**< Incoming DTMF buffer.	    */

    /* DTMF callback */
    void		    (*dtmf_cb)(pjmedia_stream*, void*, int);
    void		     *dtmf_cb_user_data;
#endif

#if defined(PJMEDIA_HANDLE_G722_MPEG_BUG) && (PJMEDIA_HANDLE_G722_MPEG_BUG!=0)
    /* Enable support to handle codecs with inconsistent clock rate
     * between clock rate in SDP/RTP & the clock rate that is actually used.
     * This happens for example with G.722 and MPEG audio codecs.
     */
    pj_bool_t		     has_g722_mpeg_bug;
					    /**< Flag to specify whether 
						 normalization process 
						 is needed		    */
    unsigned		     rtp_tx_ts_len_per_pkt;
					    /**< Normalized ts length per packet
						 transmitted according to 
						 'erroneous' definition	    */
    unsigned		     rtp_rx_ts_len_per_frame;
					    /**< Normalized ts length per frame
						 received according to 
						 'erroneous' definition	    */
    pj_uint32_t		     rtp_rx_last_ts;/**< Last received RTP timestamp
						 for timestamp checking	    */
    unsigned		     rtp_rx_last_cnt;/**< Nb of frames in last pkt  */
    unsigned		     rtp_rx_check_cnt;
					    /**< Counter of remote timestamp
						 checking */
#endif

#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR != 0)
    pj_uint32_t		     rtcp_xr_last_tx;  /**< RTCP XR tx time 
					            in timestamp.           */
    pj_uint32_t		     rtcp_xr_interval; /**< Interval, in timestamp. */
    pj_sockaddr		     rtcp_xr_dest;     /**< Additional remote RTCP XR 
						    dest. If sin_family is 
						    zero, it will be ignored*/
    unsigned		     rtcp_xr_dest_len; /**< Length of RTCP XR dest
					            address		    */
#endif

#if defined(PJMEDIA_STREAM_ENABLE_KA) && PJMEDIA_STREAM_ENABLE_KA!=0
    pj_bool_t		     use_ka;	       /**< Stream keep-alive with non-
						    codec-VAD mechanism is
						    enabled?		    */
    pj_timestamp	     last_frm_ts_sent; /**< Timestamp of last sending
					            packet		    */
#endif

/*ycw-pjsip--delete Jitter Buffer*/
#if 0
#if TRACE_JB
    pj_oshandle_t	    trace_jb_fd;	    /**< Jitter tracing file handle.*/
    char		   *trace_jb_buf;	    /**< Jitter tracing buffer.	    */
#endif
#endif
/* Added by sirrain zhang */
	int callId;

	/*ycw-firewall*/
	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	PJ_FIREWALL_RULE fwRule[2];
	#endif

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP==0
	PJMEDIA_TRANSPORT_TYPE	mediaTpType; /*media data transport type*/
#	endif
};



/**
 * Create a media stream based on the specified parameter. After the stream
 * has been created, application normally would want to get the media port 
 * interface of the streams, by calling pjmedia_stream_get_port(). The 
 * media port interface exports put_frame() and get_frame() function, used
 * to transmit and receive media frames from the stream.
 *
 * Without application calling put_frame() and get_frame(), there will be 
 * no media frames transmitted or received by the stream.
 *
 * @param endpt		Media endpoint.
 * @param pool		Pool to allocate memory for the stream. A large
 *			number of memory may be needed because jitter
 *			buffer needs to preallocate some storage.
 * @param info		Stream information.
 * @param tp		Stream transport instance used to transmit 
 *			and receive RTP/RTCP packets to/from the underlying 
 *			transport. 
 * @param user_data	Arbitrary user data (for future callback feature).
 * @param p_stream	Pointer to receive the media stream.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_create(pjmedia_endpt *endpt,
					   pj_pool_t *pool,
					   const pjmedia_stream_info *info,
					   pjmedia_transport *tp,
#	if (!(defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) ||\
			defined(INCLUDE_PSTN_GATEWAY))
					   pjmedia_transport* dsp_tp,
#	endif
#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
						PJMEDIA_TRANSPORT_TYPE mediaTpType,
#	endif
					   void *user_data,
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
						PJ_FIREWALLCFG_CTRL fwCtrl,
#endif
					   pjmedia_stream **p_stream);

/**
 * Destroy the media stream.
 *
 * @param stream	The media stream.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_destroy(pjmedia_stream *stream
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
	, PJ_FIREWALLCFG_CTRL ctrl
#endif
);


#if 0
/**
 * Get the last frame type retreived from the jitter buffer.
 *
 * @param stream	The media stream.
 *
 * @return		Jitter buffer frame type.
 */
char pjmedia_stream_get_last_jb_frame_type(pjmedia_stream *stream);
#endif

/**
 * Get the media port interface of the stream. The media port interface
 * declares put_frame() and get_frame() function, which is the only 
 * way for application to transmit and receive media frames from the
 * stream.
 *
 * @param stream	The media stream.
 * @param p_port	Pointer to receive the port interface.
 *
 * @return		PJ_SUCCESS on success.
 */
 /*ycw-pjsip--delete pjmedia_port*/
#if 0
pj_status_t pjmedia_stream_get_port(pjmedia_stream *stream,
					     pjmedia_port **p_port );
#endif

/**
 * Get the media transport object associated with this stream.
 *
 * @param st		The media stream.
 *
 * @return		The transport object being used by the stream.
 */
pjmedia_transport* pjmedia_stream_get_transport(pjmedia_stream *st);


/**
 * Start the media stream. This will start the appropriate channels
 * in the media stream, depending on the media direction that was set
 * when the stream was created.
 *
 * @param stream	The media stream.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_start(pjmedia_stream *stream);


#if 0
/**
 * Get the stream statistics. See also
 * #pjmedia_stream_get_stat_jbuf()
 *
 * @param stream	The media stream.
 * @param stat		Media stream statistics.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_get_stat( const pjmedia_stream *stream,
					      pjmedia_rtcp_stat *stat);

/**
 * Reset the stream statistics.
 *
 * @param stream	The media stream.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_reset_stat(pjmedia_stream *stream);
#endif


#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR != 0)
/**
 * Get the stream extended report statistics (RTCP XR).
 *
 * @param stream	The media stream.
 * @param stat		Media stream extended report statistics.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_get_stat_xr( const pjmedia_stream *stream,
					         pjmedia_rtcp_xr_stat *stat);
#endif

#if 0
/**
 * Get current jitter buffer state. See also
 * #pjmedia_stream_get_stat()
 *
 * @param stream	The media stream.
 * @param state		Jitter buffer state.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_get_stat_jbuf(const pjmedia_stream *stream,
						  pjmedia_jb_state *state);
#endif

/**
 * Pause the individual channel in the stream.
 *
 * @param stream	The media channel.
 * @param dir		Which direction to pause.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_pause( pjmedia_stream *stream,
					   pjmedia_dir dir);

/**
 * Resume the individual channel in the stream.
 *
 * @param stream	The media channel.
 * @param dir		Which direction to resume.
 *
 * @return		PJ_SUCCESS on success;
 */
pj_status_t pjmedia_stream_resume(pjmedia_stream *stream,
					   pjmedia_dir dir);

#if 0
/**
 * Transmit DTMF to this stream. The DTMF will be transmitted uisng
 * RTP telephone-events as described in RFC 2833. This operation is
 * only valid for audio stream.
 *
 * @param stream	The media stream.
 * @param ascii_digit	String containing digits to be sent to remote.
 *			Currently the maximum number of digits are 32.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t pjmedia_stream_dial_dtmf(pjmedia_stream *stream,
					      const pj_str_t *ascii_digit);

/**
 * Check if the stream has incoming DTMF digits in the incoming DTMF
 * queue. Incoming DTMF digits received via RFC 2833 mechanism are
 * saved in the incoming digits queue.
 *
 * @param stream	The media stream.
 *
 * @return		Non-zero (PJ_TRUE) if the stream has received DTMF
 *			digits in the .
 */
pj_bool_t pjmedia_stream_check_dtmf(pjmedia_stream *stream);


/**
 * Retrieve the incoming DTMF digits from the stream, and remove the digits
 * from stream's DTMF buffer. Note that the digits buffer will not be NULL 
 * terminated.
 *
 * @param stream	The media stream.
 * @param ascii_digits	Buffer to receive the digits. The length of this
 *			buffer is indicated in the "size" argument.
 * @param size		On input, contains the maximum digits to be copied
 *			to the buffer.
 *			On output, it contains the actual digits that has
 *			been copied to the buffer.
 *
 * @return		Non-zero (PJ_TRUE) if the stream has received DTMF
 *			digits in the .
 */
pj_status_t pjmedia_stream_get_dtmf( pjmedia_stream *stream,
					      char *ascii_digits,
					      unsigned *size);

#endif

#if 0
/**
 * Set callback to be called upon receiving DTMF digits. If callback is
 * registered, the stream will not buffer incoming DTMF but rather call
 * the callback as soon as DTMF digit is received completely.
 *
 * @param stream	The media stream.
 * @param cb		Callback to be called upon receiving DTMF digits.
 *			The DTMF digits will be given to the callback as
 *			ASCII digits.
 * @param user_data	User data to be returned back when the callback
 *			is called.
 *
 * @return		PJ_SUCCESS on success.
 */
pj_status_t
pjmedia_stream_set_dtmf_callback(pjmedia_stream *stream,
				 void (*cb)(pjmedia_stream*, 
					    void *user_data, 
					    int digit), 
				 void *user_data);
#endif
/**
 * @}
 */

PJ_END_DECL


#endif	/* __PJMEDIA_STREAM_H__ */
