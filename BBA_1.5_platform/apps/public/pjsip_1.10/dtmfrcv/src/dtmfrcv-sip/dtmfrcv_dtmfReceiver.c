/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		dtmfrcv_dtmfReceiver.c
 * brief		dtmfReceiver with switch that controlled by cm
 * details		fuctions and variables in this file can only be used in SIP's modules,
 *				CM's dtmf interfaces are in file 'usbvm_cmCtrl.c'
 * author		Huang Lei
 * version		0.9
 * date		14Oct11
 *
 * history 	\arg	Separate the DTMF receiver from usbVM, to use it when usbVM is not working, 
 */


#include <cmsip_msgdef.h>
#include <cmsip_transport.h>
#include <dtmfrcv_dtmfReceiver.h>
#ifdef INCLUDE_USB_VOICEMAIL
#include <usbvm_remoteRecord.h>
#endif
#include <arpa/inet.h>
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

#define DTMF_DETECT_PACKET_LEN 160		/* the min size to detect in inband mode */
#define MAX_DETECT_DTMF_BUFF_LEN (MAX_FRAME_SIZE + DTMF_DETECT_PACKET_LEN)
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
/* 
 * brief	DTMF number map with event 
 */
typedef struct _DTMF_MAP
{
	int id;  /* DTMF number */
	int evt; /* DTMF event */
} DTMF_MAP;
/* 
 * brief	in RFC 2833, this struct is used to check the "end" DTMF packets.
 */
typedef struct
{
	unsigned char  event; 
	unsigned char  volume;
	unsigned short duration;
} NTE_PAYLOAD;

/* 
 * brief	mode of dtmf reciever,has 3 modes
 */
typedef enum _DTMF_RCV_MODE
{
	DTMF_MODE_NULL=0,
	DTMF_MODE_INBAND=1,
	DTMF_MODE_RFC2833=2,
	DTMF_MODE_SIPINFO=3
} DTMF_RCV_MODE;
/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
#if defined(HL_DEBUG) && HL_DEBUG==1
unsigned short g_dbg_dtmflist[100];
int dtmfLength=0;
#endif
int g_dtmf_receiver_switch[PJSUA_MAX_CALLS]; /* DTMF receiver switch, on:=1,off:=0 */
DTMF_RECEIVER_RTPTYPE g_DR_payloadType[PJSUA_MAX_CALLS];
static unsigned int l_DR_Mode[PJSUA_MAX_CALLS];
static char l_DR_DetectBuf[PJSUA_MAX_CALLS][MAX_DETECT_DTMF_BUFF_LEN];
#ifdef INCLUDE_USB_VOICEMAIL
BOOL g_usbvm_started[PJSUA_MAX_CALLS];		 /* 当检测dtmf时判断是否启动了usbvm  */
#endif
const DTMF_MAP DR_TelEvtToneMap[] =
{
   { RTP_NTE_DTMF0, '0' },
   { RTP_NTE_DTMF1, '1' },
   { RTP_NTE_DTMF2, '2' },
   { RTP_NTE_DTMF3, '3' },
   { RTP_NTE_DTMF4, '4' },
   { RTP_NTE_DTMF5, '5' },
   { RTP_NTE_DTMF6, '6' },
   { RTP_NTE_DTMF7, '7' },
   { RTP_NTE_DTMF8, '8' },
   { RTP_NTE_DTMF9, '9' },
   { RTP_NTE_DTMFS, '*' },
   { RTP_NTE_DTMFH, '#' },
   { RTP_NTE_DTMFA, 'A' },
   { RTP_NTE_DTMFB, 'B' },
   { RTP_NTE_DTMFC, 'C' },
   { RTP_NTE_DTMFD, 'D' },
   {UNKNOWN, UNKNOWN} /* end of map */
};

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
BOOL DR_parseDTMFSipInfo( int cid, char *signalStr, unsigned short *dtmfNum )
{
	int dtmf = -1;
	DTMF_MAP *map = (DTMF_MAP *)DR_TelEvtToneMap;	
	char *p = NULL;

#ifdef HL_DEBUG
	printf("HL_DBG_RD: entering %s:%d\n",__FUNCTION__,__LINE__);
#endif

	if(g_dtmf_receiver_switch[cid]==DTMF_RECEIVER_OFF)
	{
		return FALSE;
	}
	if ((cid < 0) || (cid >= PJSUA_MAX_CALLS))
	{
		return FALSE;
	}
	
	/* Lookup the digit. */
	if ((p = strstr( signalStr, "Signal=" )) != NULL)
	{
		while ((map->evt != UNKNOWN) && (map->evt != *(p + strlen("Signal="))))
	   {
	      map++;
	   }
   		
	   dtmf = map->id;	   
	   HL_DR_PRINT("cisco format sip info DTMF(%d)!",dtmf);
	}
	else if((p = strstr(signalStr,"Digits"))!= NULL)
	{
		if((p = strstr(signalStr,"d="))!= NULL)
		{
			while ((map->evt != UNKNOWN) && (map->evt != *(p + strlen("d="))))
		   {
		      map++;
		   }
	   
		   dtmf = map->id;
		    HL_DR_PRINT("nortel format sip info DTMF(%d)!",dtmf);
		}
	}
	if ((dtmf != -1) && (dtmf <= RTP_NTE_DTMFD) && (dtmf >= RTP_NTE_DTMF0))
	{

		if (l_DR_Mode[cid] == DTMF_MODE_NULL)
		{	/* If mode == Inband, the rtp is recieved before sipinfo, and the dtmf has already
			sent to cm.We only change mode and do not send it again.*/
			l_DR_Mode[cid] = DTMF_MODE_SIPINFO;
		}

		if (l_DR_Mode[cid] == DTMF_MODE_SIPINFO)
		{
			*dtmfNum = (unsigned short)dtmf;
			/* put dtmf in fifo and send it to cm*/
#ifdef INCLUDE_USB_VOICEMAIL
	if(g_usbvm_started[cid])
		{
/*20130618_update*/
			TT_PRINT("recordId(%d) cid(%d) dtmf(%d/%d)  remoteStatus(%d/3) "
			  		"pkgLen(%d) dtmfDur(%d) gotH(%d:0-in,1-2833,2-info)",
				usbvm_remoteRecord_getRecordIdFromCnxId(cid),
				cid,
				*dtmfNum, RTP_NTE_DTMFH,
				g_usbvm_recordHandle[cid].remoteStatus,
				g_usbvm_recordHandle[cid].rtpPacketBufLen,
				g_usbvm_recordHandle[cid].dtmfTimeDuration,
				g_usbvm_recordHandle[cid].nGotDtmfHashInRecording);

			
			/*<< BosaZhong@09Jun2013, add, skip dtmf # when recording. */
			int recordId = 0;
			recordId = usbvm_remoteRecord_getRecordIdFromCnxId(cid);
			if (recordId >= 0)
			{
				usbvm_remoteRecord_setGotDtmfHashInRecording(recordId, *dtmfNum, 0, CMSIP_DTMF_SIPINFO);
			}
			/*>> endof BosaZhong@09Jun2013, add, skip dtmf # when recording. */
			
			/* only active when recording  */
			dtmf_FIFOPut(cid, *dtmfNum, CMSIP_DTMF_SIPINFO);
		}
		else
		{
			DR_sendCmMsgDTMF(cid,*dtmfNum, CMSIP_DTMF_SIPINFO);
		}
#else
		DR_sendCmMsgDTMF(cid,*dtmfNum, CMSIP_DTMF_SIPINFO);
#endif 
#ifdef HL_DEBUG
		HL_DR_PRINT("cnxId:%d, SIPInfo MODE dtmf:%d\n", cid, dtmf);
	#if HL_DEBUG==1
		HL_DBG_pushDTMF(dtmfLength,*dtmfNum);	
		dtmfLength+=1;
		HL_DBG_printDTMF(&dtmfLength);
	#endif
#endif
			return TRUE;

		}
		/* if DTMF SIPINFO happens subsequently with DTMF INBAND,use SIPINFO instead */
		else if (l_DR_Mode[cid] == DTMF_MODE_INBAND)
		{
			l_DR_Mode[cid] = DTMF_MODE_SIPINFO;
			HL_DR_PRINT("Change from INBAND MODE to SIPINFO MODE");
			return FALSE;
		}
	}

	return FALSE;
}

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
					CMSIP_DTMF_SEND_TYPE * dtmfType)
{
	char *buf = (char*)rtpPacket;
	unsigned char  bPaddingByte = 0;	/* padding octet				   */
	unsigned short wExtensionByte = 0;	/* extension octet				   */
	unsigned short wCSRCByte = 0;		/* CSRC octet					   */
	unsigned char *pPayload = NULL;
	int payloadLen = 0;
	NTE_PAYLOAD *ntep;
	static unsigned int timeStampNew = 0;
	static unsigned int timeStampOld = 0;
	static int dtmfDetectBufLen = 0;
	int ret = -1;
	int count = 0;
	if(g_dtmf_receiver_switch[cnxId]==DTMF_RECEIVER_OFF)
	{
		HL_DR_PRINT("DTMF switch OFF!! cnxId=%d",cnxId);
		return -1;
	}
	if((g_DR_payloadType[cnxId].SdpPayloadType==-1)
		&&(g_DR_payloadType[cnxId].SdpTelEvtPt_RX == -1)
		&&(g_DR_payloadType[cnxId].SdpTelEvtPt_TX == -1))
	{
		HL_DR_PRINT("cid[%d],parse DTMF without payloadType got!!!",cnxId);
		return -1;
	}
/*ycw-usbvm*/
# if 0
	/* If the paload type is unkown,get it by 'cnxId',from 'pjsua_var'.Only have to get once */
	if((g_DR_payloadType[cnxId].SdpPayloadType==-1)
		&&(g_DR_payloadType[cnxId].SdpTelEvtPt_RX == -1)
		&&(g_DR_payloadType[cnxId].SdpTelEvtPt_TX == -1))
	{			
		DR_getNegPayloadType2(cnxId,&g_DR_payloadType[cnxId]);		
	}
#	endif
	/* Find padding byte */
	if ( rtpPacket->p != 0) /* p == 1 */
	{
	  /* Last octet of the payload contains the padding size */
	  bPaddingByte = buf[length-1];
	}
	
	/* Find CSRC source */
	if (rtpPacket->cc != 0) /* cc == 0 to 15 */
	{
	  wCSRCByte = (unsigned short)rtpPacket->cc;
	  wCSRCByte <<= 2;	 /* cc * 4 */
	}
	else
	{
	  wCSRCByte = 0;
	}
	
	/* Find extension byte */
	if (rtpPacket->x != 0) /* x == 1 */
	{
	  unsigned short extlen;
	
	  /* Extension length */
	  extlen = *(unsigned short*)&buf[sizeof(pjmedia_rtp_hdr) + wCSRCByte + 2];
	
	  /*
	  ** number of 16 bit byte = (number of 32 bit word + 1) x 2
	  ** (+1 comes from the extension descriptor.  Please see section 5.3.1 RFC1889)
	  */
	  wExtensionByte = (unsigned short) (((unsigned short)(extlen) + 1) << 1);
	}
	pPayload = (unsigned char*)rtpPacket + sizeof(pjmedia_rtp_hdr) + wCSRCByte + wExtensionByte;
	payloadLen = length - (sizeof(pjmedia_rtp_hdr) + wCSRCByte + wExtensionByte);

	/* InBand MODE, if SdpTelEvtPt_TX != -1,means not in inband,we should not get dtmf*/
	if ((g_DR_payloadType[cnxId].SdpPayloadType == rtpPacket->pt)
			&&((g_DR_payloadType[cnxId].SdpTelEvtPt_TX==-1)||(g_DR_payloadType[cnxId].SdpTelEvtPt_RX==-1)))
	{
		if ((l_DR_Mode[cnxId] == DTMF_MODE_NULL) || (l_DR_Mode[cnxId] == DTMF_MODE_INBAND))
		{
			if ((payloadLen > 0) && ((dtmfDetectBufLen + payloadLen) < MAX_DETECT_DTMF_BUFF_LEN))
			{
				memcpy((l_DR_DetectBuf[cnxId] + dtmfDetectBufLen), pPayload, payloadLen);
				dtmfDetectBufLen += payloadLen;
			}
			
			BOOL detRet=FALSE;
			while (dtmfDetectBufLen >= DTMF_DETECT_PACKET_LEN)
			{
				pPayload = (unsigned char *)l_DR_DetectBuf[cnxId] + count;
				payloadLen = DTMF_DETECT_PACKET_LEN;
				
				detRet=DR_Detect(pPayload, payloadLen, rtpPacket->pt, dtmfNum);

				if (detRet)
				{
					if (l_DR_Mode[cnxId] == DTMF_MODE_NULL)
					{
						l_DR_Mode[cnxId] = DTMF_MODE_INBAND;
					}
					if (l_DR_Mode[cnxId] == DTMF_MODE_INBAND)	
					{
/*20130618_update*/
						/*<< BosaZhong@09Jun2013, add, skip dtmf # when recording. */
						if (dtmfType != NULL)
						{
							*dtmfType = CMSIP_DTMF_INBAND;
						}
						/*>> endof BosaZhong@09Jun2013, add, skip dtmf # when recording. */
						
#ifdef INCLUDE_USB_VOICEMAIL
	if(g_usbvm_started[cnxId])
		{
			/* only active when recording  */
			dtmf_FIFOPut(cnxId, *dtmfNum, CMSIP_DTMF_INBAND);
		}
	else
		{
			DR_sendCmMsgDTMF(cnxId,*dtmfNum, CMSIP_DTMF_INBAND);
		}
#else
		DR_sendCmMsgDTMF(cnxId,*dtmfNum, CMSIP_DTMF_INBAND);
#endif 
						HL_DR_PRINT("HL_DBG_RD:cnxId:%d, INBAND MODE dtmf:%d ,ts=%x", cnxId, *dtmfNum,rtpPacket->ts);
	#if defined(HL_DEBUG) && HL_DEBUG==1
		HL_DBG_pushDTMF(dtmfLength,*dtmfNum);	
		dtmfLength+=1;
		HL_DBG_printDTMF(&dtmfLength);
	#endif

						ret  = 0; /*has detected the inband dtmf */
					}
				}
				count += payloadLen;
				dtmfDetectBufLen -= payloadLen;
			}

			if ((dtmfDetectBufLen > 0) && ((dtmfDetectBufLen + count) < MAX_DETECT_DTMF_BUFF_LEN))
			{
				memcpy(l_DR_DetectBuf[cnxId], l_DR_DetectBuf[cnxId] + count, dtmfDetectBufLen);
			}
		}
/*20130618_update*/
		/*<< BosaZhong@13Jun2013, add, for skip dtmf #. */
		else /* if (DTMF_MODE_SIPINFO == l_DR_Mode[cnxId]) */
		{
			/*
			 * Note: we just want to parse dtmf but don't send message to CM.
			 */
			if ((payloadLen > 0) && ((dtmfDetectBufLen + payloadLen) < MAX_DETECT_DTMF_BUFF_LEN))
			{
				memcpy((l_DR_DetectBuf[cnxId] + dtmfDetectBufLen), pPayload, payloadLen);
				dtmfDetectBufLen += payloadLen;
			}
	/*20130618_update*/		
			BOOL detRet=FALSE;
			while (dtmfDetectBufLen >= DTMF_DETECT_PACKET_LEN)
			{
				pPayload = (unsigned char *)l_DR_DetectBuf[cnxId] + count;
				payloadLen = DTMF_DETECT_PACKET_LEN;

				detRet=DR_Detect(pPayload, payloadLen, rtpPacket->pt, dtmfNum);
/*20130618_update*/
				if (detRet)
				{
					if (dtmfType != NULL)
					{
						*dtmfType = CMSIP_DTMF_INBAND;
					}
	/*20130618_update*/					
					ret  = 0; /*has detected the inband dtmf */
				}
				count += payloadLen;
				dtmfDetectBufLen -= payloadLen;
			}
/*20130618_update*/
			if ((dtmfDetectBufLen > 0) && ((dtmfDetectBufLen + count) < MAX_DETECT_DTMF_BUFF_LEN))
			{
				memcpy(l_DR_DetectBuf[cnxId], l_DR_DetectBuf[cnxId] + count, dtmfDetectBufLen);
			}
		}
		/*>> endof BosaZhong@13Jun2013, add, for skip dtmf #. */
	}
	/* RFC2833 MODE */
	else if (
		((rtpPacket->pt == g_DR_payloadType[cnxId].SdpTelEvtPt_RX)
		||(rtpPacket->pt == g_DR_payloadType[cnxId].SdpTelEvtPt_TX))
		&&(rtpPacket->pt >= 96)
		&&(rtpPacket->pt <= 127))
	{
		timeStampNew = ntohl(rtpPacket->ts);

		if (l_DR_Mode[cnxId] == DTMF_MODE_NULL)
		{
			l_DR_Mode[cnxId] = DTMF_MODE_RFC2833;
		}
		
		if (l_DR_Mode[cnxId] == DTMF_MODE_RFC2833)
		{
			if (timeStampOld != timeStampNew)
			{
				ntep = (NTE_PAYLOAD*)pPayload;
/*20130618_update*/
				/*<< BosaZhong@09Jun2013, add, skip dtmf # when recording. */
				if (dtmfStartNum != NULL 
				 && ntep->event <= RTP_NTE_DTMFD
				 && (ntep->volume & 0x80) == 0)
				{
					/* this is dtmf start packet */
					*dtmfStartNum = (unsigned short)(ntep->event);
				}

				if (dtmfType != NULL)
				{
					*dtmfType = CMSIP_DTMF_RFC2833;
				}
				/*>> endof BosaZhong@09Jun2013, add, skip dtmf # when recording. */
				
				if ((ntep->event <= RTP_NTE_DTMFD)&&((ntep->volume & 0x80)!=0))
				{	/* 首位为1表示是end包 */
					*dtmfNum = ( unsigned short)(ntep->event);
#ifdef INCLUDE_USB_VOICEMAIL
	if(g_usbvm_started[cnxId])
		{
			/* only active when recording  */
			dtmf_FIFOPut(cnxId, *dtmfNum, CMSIP_DTMF_RFC2833);
		}
	else
		{
			DR_sendCmMsgDTMF(cnxId,*dtmfNum, CMSIP_DTMF_RFC2833);
		}
#else
		DR_sendCmMsgDTMF(cnxId,*dtmfNum,CMSIP_DTMF_RFC2833);
#endif 
					HL_DR_PRINT("HL_DBG_RD:cnxId:%d, RFC2833 MODE dtmf:%d ", cnxId, *dtmfNum);
	#if defined(HL_DEBUG) && HL_DEBUG==1
		HL_DBG_pushDTMF(dtmfLength,*dtmfNum);
		dtmfLength+=1;
		HL_DBG_printDTMF(&dtmfLength);
	#endif
					ret = ntep->duration;  /*has detected the RFC2833 dtmf */
					timeStampOld = timeStampNew;
				}
			}
		}
		/* If DTMF RFC2833 happens subsequently with DTMF INBAND,use RFC2833 instead */
		else if (l_DR_Mode[cnxId] == DTMF_MODE_INBAND)
		{
			HL_DR_PRINT("HL_DBG_RD:Change from INBAND MODE to RFC2833 MODE ");
			l_DR_Mode[cnxId] = DTMF_MODE_RFC2833;
			ret = -1;
		}
		else
		{
			HL_DR_PRINT("HL_DBG_RD:Unkown dtmf mode, check it out! %s:%d ",__FUNCTION__,__LINE__);
		}

	}
	return ret;

}


/* 
 * fn		void DR_receiverInit()
 * brief	When pjsua start, set the globals of all calls to default
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
void DR_receiverInit()
{
	int i=0;
	/* for all available call_ids  */
	for (i = 0; i < PJSUA_MAX_CALLS; i++)
	{	
		g_DR_payloadType[i].SdpPayloadType = -1;
		g_DR_payloadType[i].SdpTelEvtPt_TX = -1;
		g_DR_payloadType[i].SdpTelEvtPt_RX = -1;
		/*default status of dtmf_receiver is off*/
		g_dtmf_receiver_switch[i] = DTMF_RECEIVER_OFF;
		l_DR_Mode[i] = DTMF_MODE_NULL;
#ifdef INCLUDE_USB_VOICEMAIL
		g_usbvm_started[i] = FALSE;		 /* 当检测dtmf时判断是否启动了usbvm  */

	}
			dtmf_FIFOInit();
#else
	}
#endif 
}
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
 * note	[2012-5-14]DTMF-switch is always on, need not check.Remove codes that is useless. BY HL.	
 */
void DR_resetVar(int cid)
{
	l_DR_Mode[cid] = DTMF_MODE_NULL;
#ifdef INCLUDE_USB_VOICEMAIL
if(g_usbvm_started[cid] == TRUE)
{
	dtmf_FIFOReset(cid);
}
#endif 
	#if defined(HL_DEBUG) && HL_DEBUG==1
	HL_DBG_flushDTMF();
#endif 
}

#if defined(HL_DEBUG) && HL_DEBUG==1
void HL_DBG_pushDTMF(int length,unsigned short dtmf)
{
	if(length>=0&&length<100)
	{
		g_dbg_dtmflist[length]=dtmf;
	}
}
void HL_DBG_printDTMF(int* pLength)
{
	int i;
	int length=*pLength;
	if(length>=100)
	{/* reset list  */
		printf("+++ HL_DBG +++ DTMF_LIST too long\n");
		memset(g_dbg_dtmflist,0,100);
		*pLength = 0;
		return;
	}
	printf("+++ HL_DBG +++\nDTMF_LIST: {");
	for(i=0;i<length;i++)
	{
		printf("%d",g_dbg_dtmflist[i]);
		if(i%5==0)
		{
			printf("\b");
		}
	}
	printf("}\n");
}
void HL_DBG_flushDTMF()
{
	memset(g_dbg_dtmflist,0,100);
	dtmfLength = 0;
}
#endif
#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
