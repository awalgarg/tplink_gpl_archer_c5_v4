/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		dtmrcv_dtmfReceiver.h
 * brief		dtmf Receiver with switch that controlled by cm
 * details		fuctions and variables in this file can only be used in SIP's modules,
 *				CM's dtmf interfaces are in file 'usbvm_cmCtrl.c'
 *
 * author		Huang Lei
 * version		0.5
 * date		14Oct11
 *
 * history 	\arg	
 */
#ifndef __DTMFRCV_DTMFRECEIVER_H__
#define __DTMFRCV_DTMFRECEIVER_H__

#include <pthread.h>
#include <semaphore.h>
#include <pjmedia/rtp.h>       /* pjmedia_rtp_hdr */
#include <pjmedia/stream.h>    /* pjmedia_stream */
#include <pjsip-ua/sip_inv.h>
#include <usbvm_glbdef.h>
#include "usbvm_types.h"

#include <dtmfrcv_detect.h>
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
#if defined(HL_DEBUG) && HL_DEBUG==1
extern unsigned short g_dbg_dtmflist[10];
extern int dtmfLength;
#endif
extern int g_dtmf_receiver_switch[PJSUA_MAX_CALLS]; /* DTMF receiver switch, on:=1,off:=0 */
extern DTMF_RECEIVER_RTPTYPE g_DR_payloadType[PJSUA_MAX_CALLS];
#ifdef INCLUDE_USB_VOICEMAIL
extern BOOL g_usbvm_started[PJSUA_MAX_CALLS];		 /* 当检测dtmf时判断是否启动了usbvm  */
#endif
/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn		BOOL DR_parseDTMFSipInfo( int cid, char *signalStr, unsigned short *dtmfNum )
 * brief	Parse SIP INFO DTMF
 * details	
 *
 * param[in]	cid  connection id
 * param[in]	*signalStr  pointer of SIP INFO message buffer 
 * param[out]	*dtmfNum  pointer of DTMF number
 *
 * return	Detect a valid DTMF return TRUE, else return FALSE 
 * retval	
 *
 * note		
 */
int DR_parseDTMFSipInfo( int cid, char *signalStr, unsigned short *dtmfNum );
/* 
 * fn		 BOOL DR_parseDTMFRtpPacket( int cnxId, pjmedia_rtp_hdr *rtpPacket,
 *								int length, unsigned short *dtmfNum )
 * brief	Parse INBAND and RFC2833 mode DTMF from rtp packet
 * details	
 *
 * param[in]	cnxId  the current connection id 
 * param[in]	*rtpPacket  pointer of rtp packet
 * param[in]	length  rtp packet length
 * param[out]	*dtmfNum  pointer of DTMF number
 * param[out]*dtmfStartNum pointer of DTMF START number.
 * param[out]*dtmfType pointer of CMSIP_DTMF_SEND_TYPE.
 *
 * return	
 * retval	-1:failed, >0:the duration that a dtmf spend(RFC2833), 0:found dtmf
 *			!=-1:success
 * note		1. BosaZhong@09Jul2013, add, add param dtmfStartNum and dtmfType.
 */
int DR_parseDTMFRtpPacket( int cnxId, pjmedia_rtp_hdr *rtpPacket,
					int length, unsigned short *dtmfNum,
					unsigned short *dtmfStartNum,
					CMSIP_DTMF_SEND_TYPE * dtmfType);


					
/* 
 * fn		void DR_receiverInit()
 * brief	When pjsua start, set the global to default
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
void DR_receiverInit();
/* 
 * fn		void DR_resetVar(int cid)
 * brief	Reset Variables to default, should be used after the cid's switch is turned off
 * details	When the switch is still on, do nothing.
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void DR_resetVar(int cid);

#if defined(HL_DEBUG) && HL_DEBUG==1	/* define HL_DEBUG 0 to turn off it  */
void HL_DBG_pushDTMF(int length,unsigned short dtmf);
void HL_DBG_printDTMF(int* pLength);
void HL_DBG_flushDTMF();
#endif

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* __USBVM_DTMFRECEIVER_H__ */
