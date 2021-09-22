/*  Copyright(c) 2009-2012 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		pjsua_mwi.c
 * brief		
 * details	
 *
 * author	Yu Chuwei
 * version	
 * date		12Apr12
 *
 * warning	
 *
 * history \arg	
 */

#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>

#define THIS_FILE   "pjsua_mwi.c"


/***************************************************************************
 * MWI
 */
static void mwi_evsub_on_state( pjsip_evsub *sub, pjsip_event *event)
{
    pjsua_acc *acc;

    PJ_UNUSED_ARG(event);

    /* Note: #937: no need to acuire PJSUA_LOCK here. Since the buddy has
     *   a dialog attached to it, lock_buddy() will use the dialog
     *   lock, which we are currently holding!
     */
    acc = (pjsua_acc*) pjsip_evsub_get_mod_data(sub, pjsua_var.mod.id);
    if (!acc)
    {
		return;
    }

    PJ_LOG(4,(THIS_FILE, 
	      "MWI subscription for %.*s is %s",
	      (int)acc->cfg.id.slen, acc->cfg.id.ptr, 
	      pjsip_evsub_get_state_name(sub)));

    if (pjsip_evsub_get_state(sub) == PJSIP_EVSUB_STATE_TERMINATED)
	 {
		/*ycw-pjsip:send CMSIP_SIP_MWISTATUS*/
		cmsip_send_mwistatus(acc->cmAcctIndex, CMSIP_MWI_TERMINATE);

		/* Clear subscription */
		acc->mwi_dlg = NULL;
		acc->mwi_sub = NULL;
		pjsip_evsub_set_mod_data(sub, pjsua_var.mod.id, NULL);
    }
	 /*ycw-pjsip:send CMSIP_SIP_MWISTATUS*/
	 else if (pjsip_evsub_get_state(sub) == PJSIP_EVSUB_STATE_ACCEPTED ||
	 	pjsip_evsub_get_state(sub) == PJSIP_EVSUB_STATE_ACTIVE)
	 {
	 	cmsip_send_mwistatus(acc->cmAcctIndex, CMSIP_MWI_SUCCESS);
	 }
}

/* Callback called when we receive NOTIFY */
static void mwi_evsub_on_rx_notify(pjsip_evsub *sub, 
				   pjsip_rx_data *rdata,
				   int *p_st_code,
				   pj_str_t **p_st_text,
				   pjsip_hdr *res_hdr,
				   pjsip_msg_body **p_body)
{
    pjsua_mwi_info mwi_info;
    pjsua_acc *acc;

    PJ_UNUSED_ARG(p_st_code);
    PJ_UNUSED_ARG(p_st_text);
    PJ_UNUSED_ARG(res_hdr);
    PJ_UNUSED_ARG(p_body);

    acc = (pjsua_acc*) pjsip_evsub_get_mod_data(sub, pjsua_var.mod.id);
    if (!acc)
    {
		return;
    }

    /* Construct mwi_info */
    pj_bzero(&mwi_info, sizeof(mwi_info));
    mwi_info.evsub = sub;
    mwi_info.rdata = rdata;

    /* Call callback */
    if (pjsua_var.ua_cfg.cb.on_mwi_info)
	 {
		(*pjsua_var.ua_cfg.cb.on_mwi_info)(acc->index, &mwi_info);
    }
}


/* Event subscription callback. */
static pjsip_evsub_user mwi_cb = 
{
    &mwi_evsub_on_state,  
    NULL,   /* on_tsx_state: not interested */
    NULL,   /* on_rx_refresh: don't care about SUBSCRIBE refresh, unless 
	     * we want to authenticate 
	     */

    &mwi_evsub_on_rx_notify,

    NULL,   /* on_client_refresh: Use default behaviour, which is to 
	     * refresh client subscription. */

    NULL,   /* on_server_timeout: Use default behaviour, which is to send 
	     * NOTIFY to terminate. 
	     */
};

void pjsua_start_mwi(pjsua_acc *acc)
{
    pj_pool_t *tmp_pool = NULL;
    pj_str_t contact;
    pjsip_tx_data *tdata;
    pj_status_t status;

    if (!acc->cfg.mwi_enabled)
	 {
		if (acc->mwi_sub)
		{
	   	/* Terminate MWI subscription */
	   	pjsip_tx_data *tdata;
	   	pjsip_evsub *sub = acc->mwi_sub;

	    	/* Detach sub from this account */
	    	acc->mwi_sub = NULL;
	    	acc->mwi_dlg = NULL;
	    	pjsip_evsub_set_mod_data(sub, pjsua_var.mod.id, NULL);

	    	/* Unsubscribe */
			#	if 0
	    	status = pjsip_mwi_initiate(acc->mwi_sub, 0, &tdata);
	    	if (status == PJ_SUCCESS)
			{
				status = pjsip_mwi_send_request(acc->mwi_sub, tdata);
	    	}
			#	else
			status = pjsip_mwi_initiate(sub, 0, &tdata);
	    	if (status == PJ_SUCCESS)
			{
				status = pjsip_mwi_send_request(sub, tdata);
	    	}
			#	endif
		}
		return;
    }


    if (acc->mwi_sub)
	 {
		/* Subscription is already active */
		return;
    }

	 CMSIP_PRINT("---MWI Subscribing------------");

    /* Generate suitable Contact header unless one is already set in 
     * the account
     */
    if (acc->contact.slen)
	 {
		contact = acc->contact;
    }
	 else
	 {
		tmp_pool = pjsua_pool_create("tmpmwi", 512, 256);
		status = pjsua_acc_create_uac_contact(tmp_pool, &contact,
					      acc->index, &acc->cfg.id);
		if (status != PJ_SUCCESS)
		{
	   	pjsua_perror(THIS_FILE, "Unable to generate Contact header", 
		         status);
	   	pj_pool_release(tmp_pool);
	   	return;
		}
    }
    /* Create UAC dialog */
    status = pjsip_dlg_create_uac( pjsip_ua_instance(),
				   &acc->cfg.id,
				   &contact,
				   &acc->cfg.id,
				   NULL, &acc->mwi_dlg);
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "Unable to create dialog", status);
		if (tmp_pool)
		{
			pj_pool_release(tmp_pool);
		}
		return;
    }

    /* Increment the dialog's lock otherwise when presence session creation
     * fails the dialog will be destroyed prematurely.
     */
    pjsip_dlg_inc_lock(acc->mwi_dlg);
    /* Create UAC subscription */
    status = pjsip_mwi_create_uac(acc->mwi_dlg, &mwi_cb, 
				  PJSIP_EVSUB_NO_EVENT_ID, &acc->mwi_sub);
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "Error creating MWI subscription", status);
		if (tmp_pool)
		{
			pj_pool_release(tmp_pool);
		}
		
		if (acc->mwi_dlg)
		{
			pjsip_dlg_dec_lock(acc->mwi_dlg);
		}
		return;
    }

    /* If account is locked to specific transport, then lock dialog
     * to this transport too.
     */
    if (acc->cfg.transport_id != PJSUA_INVALID_ID)
	 {
		pjsip_tpselector tp_sel;

		pjsua_init_tpselector(acc->cfg.transport_id, &tp_sel);
		pjsip_dlg_set_transport(acc->mwi_dlg, &tp_sel);
    }

#if !defined(INCLUDE_TFC_ES) 
	/*By YuChuwei, For Telefonica*/
    /* Set route-set */
    if (!pj_list_empty(&acc->route_set))
	 {
		pjsip_dlg_set_route_set(acc->mwi_dlg, &acc->route_set);
    }
#endif

    /* Set credentials */
    if (acc->cred_cnt)
	 {
		pjsip_auth_clt_set_credentials( &acc->mwi_dlg->auth_sess, 
					acc->cred_cnt, acc->cred);
    }

    /* Set authentication preference */
    pjsip_auth_clt_set_prefs(&acc->mwi_dlg->auth_sess, &acc->cfg.auth_pref);

    pjsip_evsub_set_mod_data(acc->mwi_sub, pjsua_var.mod.id, acc);
    status = pjsip_mwi_initiate(acc->mwi_sub, -1, &tdata);
    if (status != PJ_SUCCESS)
	 {
		if (acc->mwi_dlg)
		{
			pjsip_dlg_dec_lock(acc->mwi_dlg);
		}
		
		if (acc->mwi_sub)
		{
			pjsip_mwi_terminate(acc->mwi_sub, PJ_FALSE);
		}
		acc->mwi_sub = NULL;
		acc->mwi_dlg = NULL;
		pjsua_perror(THIS_FILE, "Unable to create initial MWI SUBSCRIBE", 
		     status);
		if (tmp_pool)
		{
			pj_pool_release(tmp_pool);
		}
		return;
    }

    pjsua_process_msg_data(tdata, NULL);
	 status = pjsip_mwi_send_request(acc->mwi_sub, tdata);
    if (status != PJ_SUCCESS)
	 {
		if (acc->mwi_dlg)
		{
			pjsip_dlg_dec_lock(acc->mwi_dlg);
		}
		
		if (acc->mwi_sub)
		{
			pjsip_mwi_terminate(acc->mwi_sub, PJ_FALSE);
		}
		
		acc->mwi_sub = NULL;
		acc->mwi_dlg = NULL;
		pjsua_perror(THIS_FILE, "Unable to send initial MWI SUBSCRIBE", 
		     status);
		if (tmp_pool)
		{
			pj_pool_release(tmp_pool);
		}
		return;
    }

    pjsip_dlg_dec_lock(acc->mwi_dlg);
    if (tmp_pool)
	 {
	 	pj_pool_release(tmp_pool);
    }

}


/***************************************************************************
 * Unsolicited MWI
 */
static pj_bool_t unsolicited_mwi_on_rx_request(pjsip_rx_data *rdata)
{
	CMSIP_PRINT("-------recv unsolicited notify message-----------");
    pjsip_msg *msg = rdata->msg_info.msg;
    pj_str_t EVENT_HDR  = { "Event", 5 };
    pj_str_t MWI = { "message-summary", 15 };
    pjsip_event_hdr *eh;

    if (pjsip_method_cmp(&msg->line.req.method, &pjsip_notify_method)!=0)
	 {
		/* Only interested with NOTIFY request */
		return PJ_FALSE;
    }

    eh = (pjsip_event_hdr*) pjsip_msg_find_hdr_by_name(msg, &EVENT_HDR, NULL);
    if (!eh)
	 {
		/* Something wrong with the request, it has no Event hdr */
		return PJ_FALSE;
    }

    if (pj_stricmp(&eh->event_type, &MWI) != 0)
	 {
		/* Not MWI event */
		return PJ_FALSE;
    }

    /* Got unsolicited MWI request, respond with 200/OK first */
    pjsip_endpt_respond(pjsua_get_pjsip_endpt(), NULL, rdata, 200, NULL,
			NULL, NULL, NULL);


    /* Call callback */
    if (pjsua_var.ua_cfg.cb.on_mwi_info)
	 {
		pjsua_acc_id acc_id;
		pjsua_mwi_info mwi_info;

		acc_id = pjsua_acc_find_for_incoming(rdata
#	if defined(SUPPORT_IPCALL_NO_ACCOUNT) && SUPPORT_IPCALL_NO_ACCOUNT!=0
		, NULL
#	endif
);
		CMSIP_PRINT("find account[%d]", acc_id);
		if (acc_id < 0)
		{
			return PJ_FALSE;
		}

		pj_bzero(&mwi_info, sizeof(mwi_info));
		mwi_info.rdata = rdata;

		(*pjsua_var.ua_cfg.cb.on_mwi_info)(acc_id, &mwi_info);
    }

    
    return PJ_TRUE;
}

/* The module instance. */
static pjsip_module pjsua_unsolicited_mwi_mod = 
{
    NULL, NULL,				/* prev, next.		*/
    { "mod-unsolicited-mwi", 19 },	/* Name.		*/
    -1,					/* Id			*/
    PJSIP_MOD_PRIORITY_APPLICATION,	/* Priority	        */
    NULL,				/* load()		*/
    NULL,				/* start()		*/
    NULL,				/* stop()		*/
    NULL,				/* unload()		*/
    &unsolicited_mwi_on_rx_request,	/* on_rx_request()	*/
    NULL,				/* on_rx_response()	*/
    NULL,				/* on_tx_request.	*/
    NULL,				/* on_tx_response()	*/
    NULL,				/* on_tsx_state()	*/
};

static pj_status_t enable_unsolicited_mwi(void)
{
	CMSIP_PRINT("---enable unsolicited mwi---");
    pj_status_t status;

    status = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), 
					 &pjsua_unsolicited_mwi_mod);
    if (status != PJ_SUCCESS)
    {
		pjsua_perror(THIS_FILE, "Error registering unsolicited MWI module", 
		     status);
    }

    return status;
}


/*
 * Init mwi
 */
pj_status_t pjsua_mwi_init(void)
{
    return PJ_SUCCESS;
}


/*
 * Start mwi subsystem.
 */
pj_status_t pjsua_mwi_start(void)
{
    if (pjsua_var.ua_cfg.enable_unsolicited_mwi)
	 {
		pj_status_t status = enable_unsolicited_mwi();
		if (status != PJ_SUCCESS)
	   {
	   	return status;
		}
    }

    return PJ_SUCCESS;
}


/*
 * Shutdown mwi.
 */
void pjsua_mwi_shutdown(void)
{
    return;
}

 
