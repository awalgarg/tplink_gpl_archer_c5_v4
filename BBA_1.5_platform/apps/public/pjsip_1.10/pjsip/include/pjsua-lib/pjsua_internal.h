/* $Id: pjsua_internal.h 3553 2011-05-05 06:14:19Z nanang $ */
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
#ifndef __PJSUA_INTERNAL_H__
#define __PJSUA_INTERNAL_H__

/*ycw-pjsip*/
#include <sys/time.h>

/*ycw-firewall*/
#if defined(PJSIP_HAS_FIREWALL_FILTER) && PJSIP_HAS_FIREWALL_FILTER!=0
#include <pjlib-util.h>
#endif

/** 
 * This is the private header used by pjsua library implementation. 
 * Applications should not include this file.
 */

PJ_BEGIN_DECL

/** 
 * Media transport state.
 */
typedef enum pjsua_med_tp_st
{
    /** Not initialized */
    PJSUA_MED_TP_IDLE,

    /** Initialized (media_create() has been called) */
    PJSUA_MED_TP_INIT,

    /** Running (media_start() has been called) */
    PJSUA_MED_TP_RUNNING,

	/** Disabled (transport is initialized, but media is being disabled) */ 
 	PJSUA_MED_TP_DISABLED 

} pjsua_med_tp_st;


/** 
 * Structure to be attached to invite dialog. 
 * Given a dialog "dlg", application can retrieve this structure
 * by accessing dlg->mod_data[pjsua.mod.id].
 */
typedef struct pjsua_call
{
    unsigned					index;	    /**< Index in pjsua array.		    */
	int							seq;	/*Unique Call ID between cm and pjsip*/
#	if 	(defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
	pj_bool_t	isFaxPassthrough;
#	endif
#	if defined(INCLUDE_PSTN_GATEWAY) 
	 pj_bool_t					isPstn; /*ycw-pjsip.This call is from pstn to voip or from voip to pstn*/
#	endif
#	if (defined(NUM_VOICEAPP_CHANNELS) && 0!=NUM_VOICEAPP_CHANNELS)
	pj_bool_t 				isVoiceapp;
#endif
    pjsip_inv_session		*inv;	    /**< The invite session.		    */
    void							*user_data; /**< User/application data.		    */
    pjsip_status_code	 	last_code; /**< Last status code seen.		    */
    pj_str_t		 		 	last_text; /**< Last status text seen.		    */
    pj_time_val		 	 	start_time;/**< First INVITE sent/received.	    */
    pj_time_val		 	 	res_time;  /**< First response sent/received.	    */
    pj_time_val		 	 	conn_time; /**< Connected/confirmed time.	    */
    pj_time_val		 	 	dis_time;  /**< Disconnect time.		    */
    pjsua_acc_id	 		 	acc_id;    /**< Account index being used.	    */
    int			 			 	secure_level;/**< Signaling security level.	    */
    pjsua_call_hold_type 	call_hold_type; /**< How to do call hold.	    */
    pj_bool_t		 		 	local_hold;/**< Flag for call-hold by local.	    */
    pjsua_call_media_status media_st;/**< Media state.			    */
    pjmedia_dir				media_dir; /**< Media direction.		    */
    pjmedia_session			*session;   /**< The media session.		    */
    int							audio_idx; /**< Index of m=audio in SDP.	    */
    pj_uint32_t				ssrc;	    /**< RTP SSRC			    */

#	if 0
    pj_uint32_t				rtp_tx_ts; /**< Initial RTP timestamp for sender.  */
    pj_uint16_t				rtp_tx_seq;/**< Initial RTP sequence for sender.   */
    pj_uint8_t					rtp_tx_seq_ts_set;
				    /**< Bitmask flags if initial RTP sequence
				         and/or timestamp for sender are set.
					 bit 0/LSB : sequence flag 
					 bit 1     : timestamp flag 	    */
#	endif

	/*ycw-pjsip-delete conference bridge*/
#	if 0
    int			 conf_slot; /**< Slot # in conference bridge.	    */
#	endif

#	if (!(defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) || \
				defined(INCLUDE_PSTN_GATEWAY))
	pjmedia_transport			*dsp_med_tp;
#	endif

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_USB_VOICEMAIL)
	pj_bool_t					isUsbVm; /*UsbVm answer this call*/
#	endif
#	endif
	
    pjsip_evsub				*xfer_sub;  /**< Xfer server subscription, if this
					 call was triggered by xfer.	    */
    pjmedia_transport		*med_tp;    /**< Current media transport.	    */
    pj_status_t				med_tp_ready;/**< Media transport status.	    */
    pjmedia_transport		*med_orig;  /**< Original media transport	    */
    pj_bool_t					med_tp_auto_del; /**< May delete media transport   */
    pjsua_med_tp_st			med_tp_st; /**< Media transport state		    */
    pj_sockaddr				med_rtp_addr; /**< Current RTP source address
					    (used to update ICE default
					    address)			    */
    pj_stun_nat_type			rem_nat_type; /**< NAT type of remote endpoint.    */
    pjmedia_srtp_use			rem_srtp_use; /**< Remote's SRTP usage policy.	    */

    char							last_text_buf_[128];    /**< Buffer for last_text.		    */

    struct
	 {
		pj_timer_entry			reinv_timer;/**< Reinvite retry timer.		    */
		pj_uint32_t				sdp_ver;    /**< SDP version of the bad answer     */
		int						retry_cnt;  /**< Retry count.			    */
      pj_bool_t				pending;    /**< Pending until CONFIRMED state     */
    } lock_codec;		     /**< Data for codec locking when answer
					  contains multiple codecs.	    */

	/*ycw-pjsip*/
    unsigned request_channel_mode; /* Atheros: see pj_channel_mode*/
    unsigned actual_channel_mode;

	 /*ycw-pjsip:*/
	 pj_bool_t finish_progressing; /*For transfer attended(180), transferee will notify 180
	 											and 200. In order to prevent the case that pjsip send 
	 											bye to cm twice(this may be cause VOIP down), I add this
	 											flag. If its value is TRUE, then send no bye to cm.*/
	 											
	 pj_bool_t cmHangup;/*If this call is hanged up by CM's command, the value of this field
												is TRUE.ycw-pjsip*/
	 pj_bool_t busy; /*If the call id is occupyed, busy is true, or false.ycw-pjsip*/
	 pj_timer_entry callReleaseAckTimeout; /*After pjSIP send call release request to cm,
	 					cm will send ACK back. And pjSIP will wait the ACK response. If cm do not
	 					send ACK back and this timer fire, pjSIP will not wait ACK.ycw-pjsip*/
#	if (defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP) && \
		(defined(INCLUDE_PSTN_GATEWAY) || defined(INCLUDE_USB_VOICEMAIL))
	 pj_bool_t medTpReady; 
#	endif

	PJ_FIREWALL_RULE fwRule[2];

} pjsua_call;

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
/**
 * Server presence subscription list head.
 */
struct pjsua_srv_pres
{
    PJ_DECL_LIST_MEMBER(struct pjsua_srv_pres);
    pjsip_evsub	    *sub;	    /**< The evsub.			    */
    char	    *remote;	    /**< Remote URI.			    */
    int		     acc_id;	    /**< Account ID.			    */
    pjsip_dialog    *dlg;	    /**< Dialog.			    */
    int		     expires;	    /**< "expires" value in the request.    */
};
#	endif

/* 
 * brief	yuchuwei@20120420
 */
typedef struct host_ip_node
{
	char host_ip[MAX_IPADDR_LEN];
	struct host_ip_node* next;
}host_ip_node;

/*
*brief	yuchuwei@20120621
* unReg state
*/
typedef enum __ACC_UNREG_STATE
{
	ACC_UNREG_NONE, /*to register or registering or registered*/
	ACC_UNREG_ACTIVE, /*unregistering*/
	ACC_UNREG_SLEEP	/*unregister done*/
}ACC_UNREG_STATE;

/**
 * Account
 */
typedef struct pjsua_acc
{
	pj_pool_t	    	*pool;	    /**< Pool for this account.		*/
	pjsua_acc_config cfg;	    /**< Account configuration.		*/
	pj_bool_t	     	valid;	    /**< Is this account valid?		*/

	int		     		index;	    /**< Index in accounts array.	*/
	int 					cmAcctIndex; /*map CM Account. This is the index of CM Account.ycw-pjsip*/
	pj_str_t	     	display;	    /**< Display name, if any.		*/
	pj_str_t	     	user_part;	    /**< User part of local URI.	*/
	pj_str_t	     	contact;	    /**< Our Contact header.		*/
	pj_str_t         reg_contact;   /**< Contact header for REGISTER.
				         It may be different than acc
				         contact if outbound is used    */

	/*ycw-pjsip. Actually, this is the proxy's address.*/
	pj_str_t	     	srv_domain;    /**< Host part of reg server.	*/
	int		     		srv_port;	    /**< Port number of reg server.	*/

	pjsip_regc	    	*regc;	    /**< Client registration session.   */
	pj_status_t	   reg_last_err;  /**< Last registration error.	*/
	int		     		reg_last_code; /**< Last status last register.	*/

	struct
	{
		pj_bool_t	 active;    /**< Flag of reregister status.	*/
		pj_timer_entry   timer;	    /**< Timer for reregistration.	*/
		void		*reg_tp;    /**< Transport for registration.	*/
		unsigned	 attempt_cnt; /**< Attempt counter.		*/
	} auto_rereg;		    /**< Reregister/reconnect data.	*/

	pj_timer_entry   ka_timer;	    /**< Keep-alive timer for UDP.	*/
	pjsip_transport *ka_transport;  /**< Transport for keep-alive.	*/
	pj_sockaddr	   ka_target;	    /**< Destination address for K-A	*/
	unsigned	     	ka_target_len; /**< Length of ka_target.		*/

#if !defined(INCLUDE_TFC_ES) 
	/*By YuChuwei, For Telefonica*/
	pjsip_route_hdr  route_set;	    /**< Complete route set inc. outbnd.*/
#endif
	/*yuchuwei@2012-04-05: we don't need global outbound proxy*/
#	if 0
	pj_uint32_t	   global_route_crc; /** CRC of global route setting. */
#	endif
	pj_uint32_t	   local_route_crc;  /** CRC of account route setting.*/

	unsigned         rfc5626_status;/**< SIP outbound status:
                                           0: not used
                                           1: requested
                                           2: acknowledged by servers   */
	pj_str_t	     	rfc5626_instprm;/**< SIP outbound instance param.  */
	pj_str_t         rfc5626_regprm;/**< SIP outbound reg param.        */

	unsigned	     	cred_cnt;	    /**< Number of credentials.		*/
	pjsip_cred_info  cred[PJSUA_ACC_MAX_PROXIES]; /**< Complete creds.	*/

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
	pj_bool_t	     	online_status; /**< Our online status.		*/
	pjrpid_element   rpid;	    /**< RPID element information.	*/
	pjsua_srv_pres   pres_srv_list; /**< Server subscription list.	*/
	pjsip_publishc  *publish_sess;  /**< Client publication session.	*/
	pj_bool_t	     	publish_state; /**< Last published online status	*/
#	endif

	pjsip_evsub	   *mwi_sub;	    /**< MWI client subscription	*/
	pjsip_dialog    	*mwi_dlg;	    /**< Dialog for MWI sub.		*/

	host_ip_node*		host_ip_list;
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
	int 					regDuration;
#	else
	pj_bool_t	regOK;
#	endif

	/*ycw-20120621*/
	/*This flag is used to indentified whether this account is unregisterd by 
	user(by Web UI Button) or not.*/
	ACC_UNREG_STATE unregManualState;
} pjsua_acc;


/**
 *Transport.
 */
typedef struct pjsua_transport_data
{
    int			     index;
    pjsip_transport_type_e   type;
    pjsip_host_port	     local_name;

    union
	 {
		pjsip_transport	    *tp;
		pjsip_tpfactory	    *factory;
		void		    *ptr;
    } data;

} pjsua_transport_data;

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
/** Maximum length of subscription termination reason. */
#define PJSUA_BUDDY_SUB_TERM_REASON_LEN	    32

/**
 * Buddy data.
 */
typedef struct pjsua_buddy
{
    pj_pool_t		*pool;	    /**< Pool for this buddy.		*/
    unsigned		 index;	    /**< Buddy index.			*/
    void		*user_data; /**< Application data.		*/
    pj_str_t		 uri;	    /**< Buddy URI.			*/
    pj_str_t		 contact;   /**< Contact learned from subscrp.	*/
    pj_str_t		 name;	    /**< Buddy name.			*/
    pj_str_t		 display;   /**< Buddy display name.		*/
    pj_str_t		 host;	    /**< Buddy host.			*/
    unsigned		 port;	    /**< Buddy port.			*/
    pj_bool_t		 monitor;   /**< Should we monitor?		*/
    pjsip_dialog	*dlg;	    /**< The underlying dialog.		*/
    pjsip_evsub		*sub;	    /**< Buddy presence subscription	*/
    unsigned		 term_code; /**< Subscription termination code	*/
    pj_str_t		 term_reason;/**< Subscription termination reason */
    pjsip_pres_status	 status;    /**< Buddy presence status.		*/
    pj_timer_entry	 timer;	    /**< Resubscription timer		*/
} pjsua_buddy;
#	endif

/* 
 * brief	by yuchuwei@20120413
 */
# if 0
/**
 * File player/recorder data.
 */
typedef struct pjsua_file_data
{
    pj_bool_t	     type;  /* 0=player, 1=playlist */
    pjmedia_port    *port;
    pj_pool_t	    *pool;
    unsigned	     slot;
} pjsua_file_data;
#	endif

/**
 * Additional parameters for conference bridge.
 */
typedef struct pjsua_conf_setting
{
    unsigned	channel_count;
    unsigned	samples_per_frame;
    unsigned	bits_per_sample;
} pjsua_conf_setting;

typedef struct pjsua_stun_resolve
{
    PJ_DECL_LIST_MEMBER(struct pjsua_stun_resolve);

    pj_pool_t		*pool;	    /**< Pool		    */
    unsigned		 count;	    /**< # of entries	    */
    pj_str_t		*srv;	    /**< Array of entries   */
    unsigned		 idx;	    /**< Current index	    */
    void		*token;	    /**< App token	    */
    pj_stun_resolve_cb	 cb;	    /**< App callback	    */
    pj_bool_t		 blocking;  /**< Blocking?	    */
    pj_status_t		 status;    /**< Session status	    */
    pj_sockaddr		 addr;	    /**< Result		    */
    pj_stun_sock	*stun_sock; /**< Testing STUN sock  */
} pjsua_stun_resolve;


/**
 * Global pjsua application data.
 */
struct pjsua_data
{

    /* Control: */
    pj_caching_pool	 cp;	    /**< Global pool factory.		*/
    pj_pool_t		*pool;	    /**< pjsua's private pool.		*/
    pj_mutex_t		*mutex;	    /**< Mutex protection for this data	*/

#if (1 <= PJ_LOG_MAX_LEVEL)
    /* Logging: */
    pjsua_logging_config log_cfg;   /**< Current logging config.	*/
    pj_oshandle_t	 log_file;  /**<Output log file handle		*/
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */

    /* SIP: */
    pjsip_endpoint	*endpt;	    /**< Global endpoint.		*/
    pjsip_module	 mod;	    /**< pjsua's PJSIP module.		*/

    pjsua_transport_data tpdata[8]; /**< Array of transports.		*/
    pjsip_tp_state_callback old_tp_cb; /**< Old transport callback.	*/

    /* Threading: */
    pj_bool_t		 thread_quit_flag;  /**< Thread quit flag.	*/
    pj_thread_t		*thread[4];	    /**< Array of threads.	*/

#if SUPPORT_STUN
    /* STUN and resolver */
    pj_stun_config	 stun_cfg;  /**< Global STUN settings.		*/
    pj_sockaddr		 stun_srv;  /**< Resolved STUN server address	*/
    pj_status_t		 stun_status; /**< STUN server status.		*/
    pjsua_stun_resolve	 stun_res;  /**< List of pending STUN resolution*/
#endif
    pj_dns_resolver	*resolver;  /**< DNS resolver.			*/

    /* Detected NAT type */
    pj_stun_nat_type	 nat_type;	/**< NAT type.			*/
    pj_status_t		 nat_status;	/**< Detection status.		*/
    pj_bool_t		 nat_in_progress; /**< Detection in progress	*/

    /* List of outbound proxies: */
#	if 0
    pjsip_route_hdr	 outbound_proxy;
#	endif

    /* Account: */
    unsigned		 acc_cnt;	     /**< Number of accounts.	*/
#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
    pjsua_acc_id	 default_acc;	     /**< Default account ID	*/
#	endif
    pjsua_acc		 acc[PJSUA_MAX_ACC]; /**< Account array.	*/
    pjsua_acc_id	 acc_ids[PJSUA_MAX_ACC]; /**< Acc sorted by prio*/

    /* Calls: */
    pjsua_config	 ua_cfg;		/**< UA config.		*/
    unsigned		 call_cnt;		/**< Call counter.	*/
    pjsua_call		 calls[PJSUA_MAX_CALLS];/**< Calls array.	*/
    pjsua_call_id	 next_call_id;		/**< Next call id to use*/

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
    /* Buddy; */
    unsigned		 buddy_cnt;		    /**< Buddy count.	*/
    pjsua_buddy		 buddy[PJSUA_MAX_BUDDIES];  /**< Buddy array.	*/
#	endif

    /* Presence: */
    pj_timer_entry	 pres_timer;/**< Presence refresh timer.	*/

    /* Media: */
    pjsua_media_config   media_cfg; /**< Media config.			*/
    pjmedia_endpt	*med_endpt; /**< Media endpoint.		*/
    pjsua_conf_setting	 mconf_cfg; /**< Additionan conf. bridge. param */
	 
	 /*ycw-firewall*/
	 #if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	 PJ_FIREWALL_RULE	fwRule;
	 #endif

	 /*ycw-pjsip. Bound Interface's IP*/
	 pj_str_t BoundIp;
	 pj_str_t bindDev;

	 /*ycw-pjsip-usbvm*/
	 int cmEndpt_cnt;
	 USBVMENDPTCONFIG usbVmEndptConfig[PJSUA_MAX_CMENDPT];

	 pj_timer_entry regMonitorTimer;/*When timeout, check all accounts. If there is
	 					any unregistered account, then reregister it.*/

#	if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && PJ_MEDIA_TRANSIT_BY_PJSIP==0
#	if defined(INCLUDE_USB_VOICEMAIL) || defined(INCLUDE_PSTN_GATEWAY)
	pjsua_transport_config medTpCfg;
#	endif
#	endif

/*ycw-pjsip-enableT38*/
	pj_bool_t enableT38;

#	if defined(INCLUDE_USB_VOICEMAIL)
/* wlm: for voice mail remote access */
	pj_bool_t enableVMRemoteAcc;
	pj_thread_t * usbvmThread[PJSUA_USBVM_MAX_RECORDID][PJSUA_USBVM_MAX_THREADID];
#	endif

	/* to set iptables rule:
	0 -- don't set iptables rules;
	1 -- only set netfilter table's iptables rules;
	2 -- set iptables rules of both nat table and netfilter table.
	*/
	pj_uint8_t fwType;

#if INCLUDE_FWMARK
	pj_int32_t	fwmark;
#endif /* INCLUDE_FWMARK */
};


extern struct pjsua_data pjsua_var;

/**
 * Get the instance of pjsua
 */
struct pjsua_data* pjsua_get_var(void);


#	if defined(SUPPORT_IM) && SUPPORT_IM!=0
/**
 * IM callback data.
 */
typedef struct pjsua_im_data
{
    pjsua_acc_id     acc_id;
    pjsua_call_id    call_id;
    pj_str_t	     to;
    pj_str_t	     body;
    void	    *user_data;
} pjsua_im_data;


/**
 * Duplicate IM data.
 */
PJ_INLINE(pjsua_im_data*) pjsua_im_data_dup(pj_pool_t *pool, 
					    const pjsua_im_data *src)
{
    pjsua_im_data *dst;

    dst = (pjsua_im_data*) pj_pool_alloc(pool, sizeof(*dst));
    dst->acc_id = src->acc_id;
    dst->call_id = src->call_id;
    pj_strdup_with_null(pool, &dst->to, &src->to);
    dst->user_data = src->user_data;
    pj_strdup_with_null(pool, &dst->body, &src->body);

    return dst;
}
#	endif


#if 1
#define PJSUA_LOCK()	    pj_mutex_lock(pjsua_var.mutex)
#define PJSUA_TRY_LOCK()    pj_mutex_trylock(pjsua_var.mutex)
#define PJSUA_UNLOCK()	    pj_mutex_unlock(pjsua_var.mutex)
#else
#define PJSUA_LOCK()
#define PJSUA_TRY_LOCK()    PJ_SUCCESS
#define PJSUA_UNLOCK()
#endif

/******
 * STUN resolution
 */
/* Resolve the STUN server */
pj_status_t resolve_stun_server(pj_bool_t wait);

/** 
 * Normalize route URI (check for ";lr" and append one if it doesn't
 * exist and pjsua_config.force_lr is set.
 */
pj_status_t normalize_route_uri(pj_pool_t *pool, pj_str_t *uri);

/**
 * Handle incoming invite request.
 */
pj_bool_t pjsua_call_on_incoming(pjsip_rx_data *rdata);

/*
 * Media channel.
 */
pj_status_t pjsua_media_channel_init(pjsua_call_id call_id,
				     pjsip_role_e role,
				     int security_level,
				     pj_pool_t *tmp_pool,
				     const pjmedia_sdp_session *rem_sdp,
				     int *sip_err_code);
pj_status_t pjsua_media_channel_create_sdp(pjsua_call_id call_id, 
					   pj_pool_t *pool,
					   const pjmedia_sdp_session *rem_sdp,
					   pjmedia_sdp_session **p_sdp,
					   int *sip_err_code,
					   pj_bool_t createFwRule);
/*ycw-pjsip. t38*/
pj_status_t pjsua_media_channel_create_sdp_t38(pjsua_call_id call_id, 
										       pj_pool_t *pool,
										       const pjmedia_sdp_session *rem_sdp,
										       pjmedia_sdp_session **p_sdp,
										       int *sip_status_code,
					   pj_bool_t createFwRule);

pj_status_t pjsua_media_channel_update(pjsua_call_id call_id,
				       const pjmedia_sdp_session *local_sdp,
				       const pjmedia_sdp_session *remote_sdp);
pj_status_t pjsua_media_channel_deinit(pjsua_call_id call_id);

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
/**
 * Init presence.
 */
pj_status_t pjsua_pres_init();

/*
 * Start presence subsystem.
 */
pj_status_t pjsua_pres_start(void);

/**
 * Refresh presence subscriptions
 */
void pjsua_pres_refresh(void);

/*
 * Update server subscription (e.g. when our online status has changed)
 */
void pjsua_pres_update_acc(int acc_id, pj_bool_t force);

/*
 * Shutdown presence.
 */
void pjsua_pres_shutdown(void);

/**
 * Init presence for aoocunt.
 */
pj_status_t pjsua_pres_init_acc(int acc_id);

/**
 * Send PUBLISH
 */
pj_status_t pjsua_pres_init_publish_acc(int acc_id);

/**
 *  Send un-PUBLISH
 */
void pjsua_pres_unpublish(pjsua_acc *acc);

/**
 * Terminate server subscription for the account 
 */
void pjsua_pres_delete_acc(int acc_id);
#	endif


#	if defined(SUPPORT_IM) && SUPPORT_IM!=0
/**
 * Init IM module handler to handle incoming MESSAGE outside dialog.
 */
pj_status_t pjsua_im_init(void);
#	endif

/**
 * Start MWI subscription
 */
void pjsua_start_mwi(pjsua_acc *acc);
pj_status_t pjsua_mwi_init(void);
pj_status_t pjsua_mwi_start(void);
void pjsua_mwi_shutdown(void);

/**
 * Init call subsystem.
 */
pj_status_t pjsua_call_subsys_init(const pjsua_config *cfg);

/*ycw-pjsip. Start account reregister timer*/
pj_status_t pjsua_acc_subsys_start(void);


/**
 * Start call subsystem.
 */
pj_status_t pjsua_call_subsys_start(void);

/**
 * Init media subsystems.
 */
pj_status_t pjsua_media_subsys_init(const pjsua_media_config *cfg);

/**
 * Start pjsua media subsystem.
 */
pj_status_t pjsua_media_subsys_start(void);


/* 
 * brief	ycw-pjsip
 *			create local media transport interactive with DSP.
 */
pj_status_t pjsua_media_create_udpDspInteractive_transport(pjmedia_transport* med_tp);

/**
 * Destroy pjsua media subsystem.
 */
pj_status_t pjsua_media_subsys_destroy(void);

#	if defined(SUPPORT_IM) && SUPPORT_IM!=0
/**
 * Private: check if we can accept the message.
 *	    If not, then p_accept header will be filled with a valid
 *	    Accept header.
 */
pj_bool_t pjsua_im_accept_pager(pjsip_rx_data *rdata,
				pjsip_accept_hdr **p_accept_hdr);

/**
 * Private: process pager message.
 *	    This may trigger pjsua_ui_on_pager() or pjsua_ui_on_typing().
 */
void pjsua_im_process_pager(int call_id, const pj_str_t *from,
			    const pj_str_t *to, pjsip_rx_data *rdata);


/**
 * Create Accept header for MESSAGE.
 */
pjsip_accept_hdr* pjsua_im_create_accept(pj_pool_t *pool);
#	endif

/*
 * Add additional headers etc in msg_data specified by application
 * when sending requests.
 */
void pjsua_process_msg_data(pjsip_tx_data *tdata,
			    const pjsua_msg_data *msg_data);


/*
 * Add route_set to outgoing requests
 */
#if !defined(INCLUDE_TFC_ES) 
/*By YuChuwei, For Telefonica*/
void pjsua_set_msg_route_set( pjsip_tx_data *tdata,
			      const pjsip_route_hdr *route_set );
#endif


/*
 * Simple version of MIME type parsing (it doesn't support parameters)
 */
void pjsua_parse_media_type( pj_pool_t *pool,
			     const pj_str_t *mime,
			     pjsip_media_type *media_type);


/*
 * Internal function to init transport selector from transport id.
 */
void pjsua_init_tpselector(pjsua_transport_id tp_id,
			   pjsip_tpselector *sel);

pjsip_dialog* on_dlg_forked(pjsip_dialog *first_set, pjsip_rx_data *res);
#if defined(INCLUDE_TFC_ES) && PJ_RFC3960_SUPPORT
pj_status_t mute_dlg_forked(pjsip_dialog *first_set, pjsip_rx_data *res, pj_bool_t mute);
#endif
pj_status_t acquire_call(const char *title,
                         pjsua_call_id call_id,
                         pjsua_call **p_call,
                         pjsip_dialog **p_dlg);
const char *good_number(char *buf, pj_int32_t val);
void print_call(const char *title,
                int call_id,
                char *buf, pj_size_t size);

/*ycw-pjsip*/
# if defined(PJ_MEDIA_TRANSIT_BY_PJSIP) && 0==PJ_MEDIA_TRANSIT_BY_PJSIP
#	if defined(INCLUDE_PSTN_GATEWAY) || defined(INCLUDE_USB_VOICEMAIL)
pj_status_t pjsua_media_transport_create_for_single_call(pjsua_call_id index);
pj_status_t pjsua_media_transport_destroy_for_single_call(pjsua_call_id index);
#	endif

#	if defined(INCLUDE_USB_VOICEMAIL)
pj_status_t pjsua_media_send_rtp_create(pjsua_call_id callIndex);
#	endif
#	endif

PJ_END_DECL

#endif	/* __PJSUA_INTERNAL_H__ */

