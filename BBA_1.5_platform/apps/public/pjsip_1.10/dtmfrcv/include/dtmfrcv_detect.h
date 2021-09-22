/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		dtmfrcv_detect.h
 * brief		
 * details	
 *
 * author		Huang Lei
 * version	
 * date		30Oct11
 *
 * history 	\arg	
 */
#ifndef __DTMFRCV_DETECT_H__
#define __DTMFRCV_DETECT_H__


#include <pjmedia/sdp.h>
#include <pjmedia/sdp_neg.h>
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>
#include <cmsip_transport.h>
#include <usbvm_glbdef.h>
#include "usbvm_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/* If 1,can print the dtmf list  */
/* If not defined, no debug print*/
/*
#define HL_DEBUG 0
*/



#ifdef HL_DEBUG
#define HL_DR_PRINT(args...) {printf("++++ HL_DR ++++");printf(args);printf("[%s:%d]\n",__FUNCTION__,__LINE__);}
#else
#define HL_DR_PRINT(args...)
#endif
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define MAX_FRAME_SIZE	480			/* when ptime=30ms, the frame size is 30*8 */
#define MAX_LOOPS	160 			/* max count for DFT loops */
#define TAPS_LEN		(8*2)
#define ENERGY_LEN	(8)
#define DTMF_NUMBERS	(16)		/* '0'-'9' and 'A'-'F' */
#define THR_SIG 	3200	//	2800	//threshold for tone (-23dB) :YDN-065
#define THR_PAU		800	//	400		//threshold for pause energy
#define THR_TWI1	(800*32768/10000)	//threshold for twist (-10dB) : <<YDN-065(-6dB)>>
#define THR_TWI2	(800*32768/10000)	//threshold for twist (-10dB) : <<YDN-065(-6dB)>>
#define THR_ROWREL	(5000*32768/10000)	//threshold for row's relative peak ratio
#define THR_COLREL	(5000*32768/10000)	//threshold for col's relative peak ratio
#define PAUSE_TIME	1
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/* 
 * brief  used for goertzel algorithm, in inband mode.
 */
typedef struct
{
	int goertzelCnt;
	unsigned short digitLast;
	int pauseCnt;
	short taps[TAPS_LEN];
	short energy[ENERGY_LEN];
} DR_Det_t;

/* 
 * brief	Paload types in SDP
 */
typedef struct _DTMF_RECEIVER_RTPTYPE
{
	int SdpPayloadType;			/* the encode type  */
	int SdpTelEvtPt_RX;	/* incoming telephone event type */
	int SdpTelEvtPt_TX;	/* outgoing telephone event type */
}DTMF_RECEIVER_RTPTYPE;
/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

/* 
 * fn		void DR_DetectInit() 
 * brief	call init() to initialize a DR_Det_t tab
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
void DR_DetectInit();


/* 
 * fn		BOOL DR_Detect(unsigned char *framBuf, int frameLen, unsigned int rtpPayloadType, unsigned short *pNum) 
 * brief	Parse a dtmf number from a frame buffer.
 * details	
 *
 * param[in]	framBuf 
 * param[in]	frameLen 
 * param[in]	rtpPayloadType
 * param[out]	pNum : dtmf number.
 *
 * return	
 * retval	
 *
 * note		
 */
BOOL DR_Detect(unsigned char *framBuf, int frameLen, unsigned int rtpPayloadType, unsigned short *pNum);


/* 
 * fn		static void DR_getNegPayloadType( int cid, int *rtpPayloadType, int *TelEvtPayloadType )
 * brief	Get negotiation rtp payload type and telephone event type from 'pjsua_var'
 * details	
 *
 * param[in]	cid  call id
 * param[out]	*rtpPayloadType  pointer of rtp payload type
 * param[out]	*telEvtPayloadType  pointer of telephone event type
 *
 * return	
 * retval	
 *
 * note		
 */
void DR_getNegPayloadType( pjmedia_session *session, DTMF_RECEIVER_RTPTYPE* pPayloadTypeDR );


/* 
 * fn		BOOL DR_sendCmMsgDTMF() 
 * brief	
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
void DR_sendCmMsgDTMF(int cnxId, unsigned short dtmfNum, CMSIP_DTMF_SEND_TYPE dtmfType);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* __DTMFRCV_DETECT_H__  */
