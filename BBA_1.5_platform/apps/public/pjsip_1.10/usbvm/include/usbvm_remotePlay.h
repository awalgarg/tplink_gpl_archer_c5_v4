/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_remotePlay.h
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		19Jul11
 *
 * history 	\arg	
 */

#ifndef __USBVM_REMOTEPLAY_H__
#define __USBVM_REMOTEPLAY_H__

#include "usbvm_types.h"
#include <usbvm_glbdef.h>
#include <pjsip-ua/sip_inv.h>
#include <cmsip_msgdef.h>
#include <pjsua-lib/pjsua_internal.h>
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

#define MAX_USBVM_RTP_LENGTH 640
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

typedef enum
{
	NEG_REMOTE_SDP_NOTEXIST,
	NEG_REMOTE_SDP_NOTSUPPORT_G711,
	NEG_LOCAL_SDP_NOTNEED_CHANGE,
	NEG_LOCAL_SDP_NEED_CHANGE,
	NEG_ERROR
	
} NEG_STAS;

typedef struct _SleepMsWithCNContext
{
	pjsua_call		*call;
	int 			recordId;
	UINT32 			ssrc;
	UINT32 			*ts;
	UINT16 			*seq;
	int 			TX_pktPeriod;
	int				TX_pktSize;
	int				*maker;
} SleepMsWithCNContext, *PSleepMsWithCNContext;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn		BOOL usbvm_getPayloadTypeByCodec( CODEC_TYPE eptCodec, unsigned int *rtpPayLoadType )
 * brief	Get rtp payload type according to codec type
 * details	
 *
 * param[in]	eptCodec  codec type
 * param[out]	*rtpPayLoadType  rtp payload type
 *
 * return	match return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
BOOL usbvm_getPayloadTypeByCodec( CODEC_TYPE eptCodec, unsigned int *rtpPayLoadType );

/* 
 * fn		BOOL usbvm_forceNegToG711( pjsip_inv_session *inv )
 * brief	Force negotiation to G.711 when using USB voicemail
 * details	
 *
 * param[in]	*inv  invite session handle
 * param[out]	
 *
 * return	Success return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
BOOL usbvm_forceNegToG711( pjsip_inv_session *inv );


/* 
 * fn		BOOL usbvm_enterRemoteUSBvoiceMail( int endpt, int cid, int recordId )
 * brief	Enter remote USB voicemail module
 * details	
 *
 * param[in]	endpt:    endpoint number
 * param[in]	cid:      call id
 * param[in]	recordId: record id
 * param[out]	
 *
 * return	Success return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
BOOL usbvm_enterRemoteUSBvoiceMail( CMSIP_USBVM_ENTER *pUsbVmEnter);

/* 
 * fn		void usbvm_exitRemoteUSBvoiceMail( int recordId )
 * brief	Exit remote USB voicemail module
 * details	
 *
 * param[in]	recordId:  record id
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_exitRemoteUSBvoiceMail( int recordId );


/* 
 * fn		void usbvm_remoteRecPlayInit(int handle)
 * brief	Initialize RTP control block info
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_remoteRecPlayInit( void );


/* 
 * fn		int usbvm_remotePlayThreadCreate( int recordId )
 * brief	Create remote play task
 * details	
 *
 * param[in]	recordId  record id
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_remotePlayThreadCreate( int recordId );
/* 
 * fn		void usbvm_remoteRcvRtpThreadCreate(int recordId)
 * brief	create remote rtp recieving thread.
 * details	
 *
 * param[in]	int recordId, 
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_remoteRcvRtpThreadCreate(int recordId);
#if 0
/* 
 * fn		int usbvm_checkRemoteRecPlayIsIDLE( void )
 * brief	Get remote record and play status to check whether USB voicemail is idle
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	If USB voicemail is idle return 0, else return -1
 * retval	
 *
 * note		
 */
int usbvm_checkRemoteRecPlayIsIDLE( void );
#endif
/* 
 * fn		int usbvm_checkRemoteRecFileIsIdle(void)
 * brief	check whether any record file is in use
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	0: not in use; not 0: is in use;
 * retval	
 *
 * note		
 */
int usbvm_checkRemoteRecFileIsIdle(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif	/* __USBVM_REMOTEPLAY_H__ */


