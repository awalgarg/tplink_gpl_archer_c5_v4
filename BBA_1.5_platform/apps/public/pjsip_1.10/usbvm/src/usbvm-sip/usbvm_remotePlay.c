/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_remotePlay.c
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		19Jul11
 *
 * history 	\arg	
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>     /* getpid */
#include <sys/types.h>  /* getpid */
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <arpa/inet.h>  /* htonl ntohl */
#include <ctype.h>      /* isprint */
#include <sys/vfs.h>
#include <dirent.h>
#include <sys/select.h>

#include <usbvm_remoteRecord.h>
#include <usbvm_remotePlay.h>	 
#include <usbvm_recFileMang.h>
#include <usbvm_voiceNotify.h>
#include <usbvm_speechEncDec.h>
#include <pjmedia/rtp.h>       /* pjmedia_rtp_hdr */
#include <pjmedia/stream.h>    /* pjmedia_stream */
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>
#include <cmsip_msgdef.h>
#include <cmsip_transport.h>
#include <pjmedia/sdp.h>
#include <pjmedia/sdp_neg.h>
#include <pjsip/sip_uri.h>

#include <dtmfrcv_dtmfReceiver.h>
#include "voip_ioctl.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */




/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#if(defined(USBVM_DEBUG)&&(USBVM_DEBUG==1))
#define  MARK_PRINT(args...) {printf("============USBVM PLAY %d============", __LINE__);printf(args);}
#else
#define  MARK_PRINT(args...)
#endif

#define WAIT_FOR_RTP_FIFO_TIMEOUT 20		/* ms */

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
/* 
 * brief	RTP packet
 */
typedef struct
{
	pjmedia_rtp_hdr RTPheader; /* RTP packet header */
	char RTPdata[MAX_USBVM_RTP_LENGTH];         /* RTP packet payload */
} RTPDATA;

/* 
 * brief	Remote play error number
 */
typedef enum REMOTE_PLAY_ERRORNO
{
   NO_ERROR,                 /* no error */
   ERROR_STREAM_CLOSED,      /* rtp stream has been closed */
   ERROR_NOTIFY_NOT_EXIST,   /* voice notify does't exist */
   ERROR_REC_NOT_EXIST,      /* record file does't exist */
   ERROR_FILE_OPEN,          /* file open fail */
   ERROR_FILE_READ,          /* file read fail */
   ERROR_FILE_WRITE,         /* file write fail */
   ERROR_RECINFO_UPDATE,     /* record info update fail */
   ERROR_RECINFO_GET         /* get record info fail */

} REMOTE_PLAY_ERRONO;

/* 
 * brief	Sound format
 */
typedef struct
{
	UINT8 bytesPerFrame;       /* Size of a base frame */
	UINT8 msecPerFrame;        /* Rate of a base frame */
	UINT8 samplesPerMsec;      /* 8 for narrow band codec, and 16 for wide band codec */
} SOUNDFORMAT;

/* 
 * brief	Codec attribute
 */
typedef struct
{
	SOUNDFORMAT soundFormat;   /* sound format */
	UINT8 rtpPayloadType;      /* rtp payload type */
	CODEC_TYPE eptCodec;       /* codec type */
} CODEC_ATTRIBUTE;

/* 
 * brief	Voice notify 
 */
typedef struct
{
	BOOL bUseCustom;                 /* use custom or default voice notify */
	SOUNDFORMAT soundFormat;         /* sound format */
	unsigned int rtpPayLoadType;     /* rtp payload type */
	char voiceNotFile[30];           /* voice notify file name */
	unsigned int voiceNotFileLen;    /* voice notify file length */
	unsigned int voiceNotifyDurInMs; /* voide notify file duration */

}VOICE_NOTIFY;



/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
static void strProcessForSpecialChar( char *inputStr, char *outputStr );
#if 0
static void closeRtpStream( pjsua_call *call );
#endif
static BOOL chkRtpStreamReady( pjsua_call *call );
static BOOL getNotifyFileByPayLoad( int endpt, unsigned int rtpPayLoadType, int bUseCustom, 
									FILE **pfpVoiceNotify, unsigned int *pFileLen );
static BOOL calRtpPacketParams( int ptime, unsigned int rtpPayLoadType, int *pktPeriod,
								int *pktSize, int *tsInterval );
static int getRemoteSdpOfferCodec( pjsip_inv_session *inv, int *rtpPayloadType );
static void getRemoteSdpOfferPtime( pjsua_call *call, int *pTime );
static void modifyLocalSdpAnswer( pjsip_inv_session *inv, int rtpPayloadType, NEG_STAS negState );
static void getLocalSdpPtime(pjsua_call* call, int* accPtime);
static void getDisplayName(pjsip_fromto_hdr * phdr, char* displayName);
static void getSrcDstDisplayName( pjsua_call *call, char *srcName, char *dstName );
static void sipMsgModifyCMTimer( int recordId, int timeValue );
static void sipMsgGenOnhookEvt( int recordId );
static void sipMsgRemoteStatusChange(int endpt,int status,unsigned int globalWavTimeName);

static void remotePlayThread( void *context );
static void remoteRcvRtpThread(void* arg);

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
/* This table is used to define format attributes of a supported codec */
static const CODEC_ATTRIBUTE codecAttributeTable[] =
{
/* { {20, 30, 8}, RTP_PAYLOAD_G7231_53,  CODEC_G7231_53 },
*/
	{ {24, 30, 8}, RTP_PAYLOAD_G7231_63,  CODEC_G7231_63 },
	{ {10, 5,  8}, RTP_PAYLOAD_G726_16,   CODEC_G726_16  },
	{ {15, 5,  8}, RTP_PAYLOAD_G726_24,   CODEC_G726_24  },
	{ {20, 5,  8}, RTP_PAYLOAD_G726_32,   CODEC_G726_32  },
	{ {25, 5,  8}, RTP_PAYLOAD_G726_40,   CODEC_G726_40  },
	{ {10, 10, 8}, RTP_PAYLOAD_G729A,     CODEC_G729A    },
	{ {10, 10, 8}, RTP_PAYLOAD_G729,      CODEC_G729	 },     
	{ {40, 5,  8}, RTP_PAYLOAD_PCMU,      CODEC_PCMU     },
	{ {40, 5,  8}, RTP_PAYLOAD_PCMA,      CODEC_PCMA     },
	{ {0,  0,  0}, RTP_PAYLOAD_NULL,      CODEC_NULL     }    /* end of list */
};
static VOICE_NOTIFY voiceNotifyTable[] =
{
	{ 0, {40, 5,  8}, RTP_PAYLOAD_PCMU,  "recNotifyDef_g711u", 0, 0},
	{ 0, {40, 5,  8}, RTP_PAYLOAD_PCMA,  "recNotifyDef_g711a", 0, 0},
	{ 1, {40, 5,  8}, RTP_PAYLOAD_PCMU,  "recNotifyCustom_g711u.wav", 0, 0},
	{ 1, {40, 5,  8}, RTP_PAYLOAD_PCMA,  "recNotifyCustom_g711a.wav", 0, 0},   	
	{ 0, {0,  0,  0}, RTP_PAYLOAD_NULL,  "", 0, 0} 

};

BOOL l_exitMsgSent[PJSUA_USBVM_MAX_RECORDID];
/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/* 
 * fn		static void strProcessForSpecialChar( char *inputStr, char *outputStr )
 * brief	String process for special character, because these characters can not exist in file name
 * details	
 *
 * param[in]	inputStr:  input string
 * param[out]	outputStr: output string
 *
 * return	
 * retval	
 *
 * note		
 */
static void strProcessForSpecialChar( char *inputStr, char *outputStr )
{
	int i, j;
	char filter[] = {'\\','/',':','*','?','\"','<','>','|'};
	BOOL bTruncated = FALSE;
	
	memset(outputStr, 0, MAX_DIGITS_NUM);
	for (i = 0; i < strlen(inputStr); i++)
	{
		/* If it is printable char */
		if (isprint(*(inputStr + i)))
		{
			for (j = 0; j < sizeof(filter); j++)
			{
				if (*(inputStr + i) == filter[j])
				{
					break;
				}
			}
			if (j < sizeof(filter))
			{
				sprintf(strchr(outputStr, '\0'),"%c%02X", '%', *(inputStr + i));
			}
			else 
			{
				sprintf(strchr(outputStr, '\0'),"%c", *(inputStr + i));
			}
			/* Truncate the string if it exceed the max length allowed */
			if (strlen(outputStr) >= (MAX_DIGITS_NUM - 1))
			{
				bTruncated = TRUE;
				break;
			}
		}
		else
		{
			break;
		}
	
	}
	/* If unprintable char exist, use "NULL" instead */
	if ((i < strlen(inputStr)) && (!bTruncated))
	{
		strcpy(outputStr, "NULL");
	}
}
#if 0 /* not used any more 2012-5-23 */
/* 
 * fn		static void closeRtpStream( pjsua_call *call )
 * brief	Close rtp stream
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
static void closeRtpStream( pjsua_call *call )
{
	if (call && call->session && call->session->stream[0])
	{
		pjmedia_stream_destroy(call->session->stream[0]);
	}
}
#endif

/* 
 * fn		static BOOL chkRtpStreamReady( pjsua_call *call )
 * brief	Check whether rtp send socket is ready
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	ready return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
static BOOL chkRtpStreamReady( pjsua_call *call )
{
	return call && call->session && call->session->stream[0] 
							&& call->session->stream[0]->transport;
}

/* 
 * fn		staic BOOL getNotifyFileByPayLoad( int endpt, unsigned int rtpPayLoadType, BOOL bUseCustom, 
 *												FILE **pfpVoiceNotify, unsigned int *pFileLen )
 * brief	Get voice noitfy by rtp payload type
 * details	
 *
 * param[in]	endpt  endpoint number 
 * param[in]	rtpPayLoadType  rtp packet payload type
 * param[in]	bUseCustom  use custom notify file or not
 * param[out]	**pfpVoiceNotify  pointer of file descriptor
 * param[out]	*pFileLen  pointer of file length
 *
 * return	success return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
static BOOL getNotifyFileByPayLoad( int endpt, unsigned int rtpPayLoadType, int bUseCustom, 
									FILE **pfpVoiceNotify, unsigned int *pFileLen )
{
	int        i = 0;
	BOOL       formatMatch = FALSE;
	char       ntyFilePathName[100];
	
	while (voiceNotifyTable[i].rtpPayLoadType != 0xff)
	{
		if ((voiceNotifyTable[i].rtpPayLoadType == rtpPayLoadType) 
				&& (voiceNotifyTable[i].bUseCustom == bUseCustom))
		{	
			/* Default voice notify file path is different from custom noitfy file */
			if (bUseCustom)
			{				
				sprintf(ntyFilePathName, "%s", USB_NOT_FILE_CUSTOM_DIR);
			}
			else
			{
				strcpy(ntyFilePathName, USB_NOT_FILE_DEF_DIR);
			}
			strcat(ntyFilePathName, voiceNotifyTable[i].voiceNotFile);

			/* Voice notify exist, then get file length and file descriptor */
			if (access(ntyFilePathName, F_OK) == 0) 
			{
				MARK_PRINT("getNotifyFileByPayLoad file %s for record notify\n", ntyFilePathName);
				if (access(ntyFilePathName, R_OK) != 0)
				{
					chmod(ntyFilePathName, 0444);
				}
				if ((voiceNotifyTable[i].voiceNotFileLen == 0) || (bUseCustom))
				{
					voiceNotifyTable[i].voiceNotFileLen = usbvm_getFileSize(ntyFilePathName);
				}

				*pFileLen = voiceNotifyTable[i].voiceNotFileLen;
				if ((*pfpVoiceNotify = fopen(ntyFilePathName, "rb")) == NULL)
				{
					MARK_PRINT("Failed to open an exist file %s for writing\n", ntyFilePathName);
					break;
				}
				if (bUseCustom)
				{
					fseek(*pfpVoiceNotify, WAV_HEADER_LEN, SEEK_SET);
					voiceNotifyTable[i].voiceNotFileLen -= WAV_HEADER_LEN;
					*pFileLen = voiceNotifyTable[i].voiceNotFileLen;
				}
			}
			else if (bUseCustom) /* custom voice notify does't exist, then use default file instead */
			{

				MARK_PRINT("can't find custom notify file:%s\n", ntyFilePathName);
				i -= (sizeof(voiceNotifyTable)/sizeof(VOICE_NOTIFY) - 1)/2;
				
				sprintf(ntyFilePathName, "%s%s", USB_NOT_FILE_DEF_DIR, voiceNotifyTable[i].voiceNotFile);
				
				MARK_PRINT("getNotifyFileByPayLoad file %s for record notify\n", ntyFilePathName);
				if (access(ntyFilePathName, F_OK) == 0)
				{
					if (access(ntyFilePathName, R_OK) != 0)
					{
						chmod(ntyFilePathName, 0444);
					}
					if ((voiceNotifyTable[i].voiceNotFileLen == 0))
					{
						voiceNotifyTable[i].voiceNotFileLen = usbvm_getFileSize(ntyFilePathName);
					}

					*pFileLen = voiceNotifyTable[i].voiceNotFileLen;
					if ((*pfpVoiceNotify = fopen(ntyFilePathName, "rb")) == NULL)
					{
						MARK_PRINT("Failed to open an exist file %s for reading\n", ntyFilePathName);
						break;
					}
				}
				else
				{
					break;
				}
				
			}
			else
			{
				break;
			}

			formatMatch = TRUE;
			break;
		}
		i++;
	}
	return (formatMatch);
}

/* 
 * fn		static BOOL calRtpPacketParams( int ptime, unsigned int rtpPayLoadType, int *pktPeriod,
 *								            int *pktSize, int *tsInterval )
 * brief	Calculate the rtp packet paramters according to ptime and rtp payload type
 * details	
 *
 * param[in]	ptime  packet time interval(eg. 10ms 20ms 30ms) 
 * param[in]	rtpPayLoadType  rtp packet payload type
 * param[out]	*pktPeriod  pointer of packet period to send 
 * param[out]	*pktSize  pointer of packet size to send 
 * param[out]	*tsInterval  pointer of timestamp interval of each packet 
 *
 * return	
 * retval	
 *
 * note		
 */
static BOOL calRtpPacketParams( int ptime, unsigned int rtpPayLoadType, int *pktPeriod,
								int *pktSize, int *tsInterval )
{
	int  i = 0;
	BOOL formatMatch = FALSE;
	while ( codecAttributeTable[i].rtpPayloadType != 0xff )
	{
		if (codecAttributeTable[i].rtpPayloadType == rtpPayLoadType)
		{
			/* We assume the pktPeriod from the caller must be x times of 10 msec */
			UINT16 n = ptime / codecAttributeTable[i].soundFormat.msecPerFrame;
			n = (n == 0) ? 1 : n;
			*pktPeriod = n * codecAttributeTable[i].soundFormat.msecPerFrame; 
			*pktSize = n * codecAttributeTable[i].soundFormat.bytesPerFrame;
			*tsInterval = (*pktPeriod) * codecAttributeTable[i].soundFormat.samplesPerMsec;
			formatMatch = TRUE;
			break;
		}
		i++;
	}
	return (formatMatch);
}

/* 
 * fn		static int getRemoteSdpOfferCodec( pjsip_inv_session *inv, int *rtpPayloadType )
 * brief	Get G.711 rtp payload type that incoming call remote SDP offer can support
 * details	
 *
 * param[in]	*inv  invite session handle
 * param[out]	*rtpPayloadType  pointer of rtp payload type
 *
 * return	enum NEG_STAS
 * retval	
 *
 * note		
 */
static int getRemoteSdpOfferCodec( pjsip_inv_session *inv, int *rtpPayloadType )
{
	unsigned int mediaFmtCount = 0;
	pjmedia_sdp_session *sdpSessionInfo = NULL;
	pjsip_inv_state invState;
	pjmedia_sdp_neg_state negState;
	int rtpPayloadTmp;
	int ret = NEG_ERROR;
	int i;

	if ((inv == NULL))
	{
		return NEG_ERROR;
	}
	
	invState = inv->state;
	MARK_PRINT("%s:%d--invState:%d\n", __FUNCTION__, __LINE__, invState);
	if ((PJSIP_INV_STATE_INCOMING == invState) || (PJSIP_INV_STATE_EARLY == invState))
	{
		
		negState = inv->neg->state;
		MARK_PRINT("%s:%d--negState:%d\n", __FUNCTION__, __LINE__, negState);
		if ((PJMEDIA_SDP_NEG_STATE_REMOTE_OFFER == negState)
			|| (PJMEDIA_SDP_NEG_STATE_WAIT_NEGO == negState))
		{
			sdpSessionInfo = inv->neg->neg_remote_sdp;

			if (sdpSessionInfo != NULL)
			{
				mediaFmtCount = sdpSessionInfo->media[0]->desc.fmt_count;
				for ( i = 0 ; i < mediaFmtCount; i++ )
				{
					rtpPayloadTmp = atoi(sdpSessionInfo->media[0]->desc.fmt[i].ptr);
					if (rtpPayloadTmp == RTP_PAYLOAD_PCMU)
					{

						*rtpPayloadType = rtpPayloadTmp;
						if (i == 0)
						{
							ret = NEG_LOCAL_SDP_NOTNEED_CHANGE;
						}
						else
						{
							ret = NEG_LOCAL_SDP_NEED_CHANGE;
						}
						break;
					}
					else if (rtpPayloadTmp == RTP_PAYLOAD_PCMA)
					{
						*rtpPayloadType = rtpPayloadTmp;
						if (i == 0)
						{
							ret = NEG_LOCAL_SDP_NOTNEED_CHANGE;						
						}
						else
						{
							ret = NEG_LOCAL_SDP_NEED_CHANGE;
						}					
						break;
					}
			   	}
				
				/* Remote SDP doesn't support PCMU or PCMA */
				if ((i == mediaFmtCount) && (0 != mediaFmtCount))
				{
					*rtpPayloadType = RTP_PAYLOAD_NULL;
					
					ret = NEG_REMOTE_SDP_NOTSUPPORT_G711;
				}
			}
		}
		else
		{
			*rtpPayloadType = RTP_PAYLOAD_NULL;
			ret = NEG_REMOTE_SDP_NOTEXIST;  /* Remote ua don't offer SDP in INVITE message */
		}
	}
	else
	{
		*rtpPayloadType = RTP_PAYLOAD_NULL;
		ret = NEG_REMOTE_SDP_NOTEXIST;  /* Remote ua don't offer SDP in INVITE message */
	}

	MARK_PRINT("%s:ret:%d rtp payload type:%d\n", __FUNCTION__, ret, *rtpPayloadType);
	
	return ret;
}

/* 
 * fn		static void getRemoteSdpOfferPtime( pjsua_call *call, int *pTime )
 * brief	Get the ptime in the SDP that we received from remote calls. 
 * details	
 *
 * param[in]	call	the related call from which we get the ptime
 * param[out]	*pTime the buffer to get the ptime
 *
 * return	none
 * retval	
 *
 * note		yuchuwei modify @ 2012-4-4
 */
static void getRemoteSdpOfferPtime( pjsua_call *call, int *pTime )
{
	/*Now, we only support one stream. ycw-usbvm*/
	*pTime = call->session->stream_info[0].ulptime;
	MARK_PRINT("%s:ptime:%d\n", __FUNCTION__, *pTime);
}

/* 
 * fn		static void modifyLocalSdpAnswer( pjsip_inv_session *inv, int rtpPayloadType, NEG_STAS negState )
 * brief	Change media and atrributes of local SDP answer to support G.711 only
 * details	
 *
 * param[in]	*inv  invite session handle
 * param[in]	rtpPayloadType  rtp payload type
 * param[in]	negState  negotiation state
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
static void modifyLocalSdpAnswer( pjsip_inv_session *inv, int rtpPayloadType, NEG_STAS negState )
{
	pjmedia_sdp_session *sdpSessionInfo;
	char mediaStr[10];

	
	if ((inv == NULL)
		|| ((rtpPayloadType != RTP_PAYLOAD_PCMU) && (rtpPayloadType != RTP_PAYLOAD_PCMA)))
	{
		return;
	}

	sdpSessionInfo = inv->neg->neg_local_sdp;
	MARK_PRINT("%s:%d--negState:%d\n", __FUNCTION__, __LINE__, negState);

	/* Here we don't care the rtpPayloadType, enable G.711 PCMU and PCMA both */
	if (NEG_REMOTE_SDP_NOTEXIST == negState)
	{
		
		/* Set media field */
		sdpSessionInfo->media_count = 1;
		sdpSessionInfo->media[0]->desc.fmt_count = 3;
		memset(mediaStr, 0, 10);
		sprintf(mediaStr, "%d", RTP_PAYLOAD_PCMU);
		strcpy(sdpSessionInfo->media[0]->desc.fmt[0].ptr, mediaStr);
		sdpSessionInfo->media[0]->desc.fmt[0].slen = strlen(mediaStr);
		memset(mediaStr, 0, 10);
		sprintf(mediaStr, "%d", RTP_PAYLOAD_PCMA);
		strcpy(sdpSessionInfo->media[0]->desc.fmt[1].ptr, mediaStr);
		sdpSessionInfo->media[0]->desc.fmt[1].slen = strlen(mediaStr);
		
		strcpy(sdpSessionInfo->media[0]->desc.fmt[2].ptr, "96");
		sdpSessionInfo->media[0]->desc.fmt[2].slen = strlen("96");		
		/* Set attribute field */
		strcpy(sdpSessionInfo->media[0]->attr[1]->name.ptr, "rtpmap");
		sdpSessionInfo->media[0]->attr[0]->name.slen = strlen("rtpmap");
		strcpy(sdpSessionInfo->media[0]->attr[1]->value.ptr, "0 PCMU/8000");
		sdpSessionInfo->media[0]->attr[0]->value.slen = strlen("0 PCMU/8000");
		strcpy(sdpSessionInfo->media[0]->attr[2]->name.ptr, "rtpmap");
		sdpSessionInfo->media[0]->attr[1]->name.slen = strlen("rtpmap");
		strcpy(sdpSessionInfo->media[0]->attr[2]->value.ptr, "8 PCMA/8000");
		sdpSessionInfo->media[0]->attr[1]->value.slen = strlen("8 PCMA/8000");

	}
	else if (NEG_LOCAL_SDP_NEED_CHANGE == negState)
	{
		/* Set media field */
		sdpSessionInfo->media_count = 1;
		sdpSessionInfo->media[0]->desc.fmt_count = 2;
		memset(mediaStr, 0, 10);
		sprintf(mediaStr, "%d", rtpPayloadType);
		strcpy(sdpSessionInfo->media[0]->desc.fmt[0].ptr, mediaStr);
		sdpSessionInfo->media[0]->desc.fmt[0].slen = strlen(mediaStr);

		strcpy(sdpSessionInfo->media[0]->desc.fmt[1].ptr, "96");
		sdpSessionInfo->media[0]->desc.fmt[1].slen = strlen("96");	
		
		/* Set attribute field */
		strcpy(sdpSessionInfo->media[0]->attr[1]->name.ptr, "rtpmap");
		sdpSessionInfo->media[0]->attr[1]->name.slen = strlen("rtpmap");
		if (RTP_PAYLOAD_PCMU == rtpPayloadType)
		{
			strcpy(sdpSessionInfo->media[0]->attr[1]->value.ptr, "0 PCMU/8000");
			sdpSessionInfo->media[0]->attr[1]->value.slen = strlen("0 PCMU/8000");
		}
		else if (RTP_PAYLOAD_PCMA == rtpPayloadType)
		{
			strcpy(sdpSessionInfo->media[0]->attr[1]->value.ptr, "8 PCMA/8000");
			sdpSessionInfo->media[0]->attr[1]->value.slen = strlen("8 PCMA/8000");
		}

	}
}

static void getLocalSdpPtime(pjsua_call *call, int* accPtime)
{
	/*ycw-usbvm*/
	*accPtime = (call->acc_id < 0) ? PJSUA_DEFAULT_LOCAL_PTIME : pjsua_var.acc[call->acc_id].cfg.ptime;
}

/* 
 * fn		static void getDisplayName(pjsip_fromto_hdr * phdr, char* displayNameBuf)
 * brief	Get Display name from the From or To header; If there is no display name, then get
 *			the username; If no username ,then get the IP.
 * details	
 *
 * param[in]	phdr  the From or To header
 * param[out]	*displayNameBuf  the buffer to get the displayname.
 *
 * return	None
 * retval	
 *
 * note		yuchuwei create @2012-4-4
 */
static void getDisplayName(pjsip_fromto_hdr * phdr, char* displayNameBuf)
{
	pjsip_name_addr *pNameAddr = NULL;
	pjsip_sip_uri	*pSipUri = NULL;
	pj_str_t displayName = {NULL, 0};
	char nameTmp[MAX_DIGITS_NUM];
	int min;
	
	/*ycw-usbvm*/
	pNameAddr = (pjsip_name_addr*)phdr->uri;
	if (pNameAddr->display.slen)
	{
		displayName = pNameAddr->display;
	}
	else
	{
		pSipUri = (pjsip_sip_uri*)pjsip_uri_get_uri(pNameAddr);
		displayName = (pSipUri->user.slen) ? pSipUri->user : pSipUri->host;
	}
	
	min = (displayName.slen >= (sizeof(nameTmp)-1)) ? (sizeof(nameTmp)-1)
					: displayName.slen;
	memcpy(nameTmp, displayName.ptr, min);
	nameTmp[min] = 0;
		
	strProcessForSpecialChar(nameTmp, displayNameBuf);
}

/* 
 * fn		static void getSrcDstDisplayName( pjsua_call *call, char *srcName, char *dstName )
 * brief	Get source account and destination account display name from URI of From and To 
 * details	
 *
 * param[in]	call  the call from which we get the source name and destination name.
 * param[out]	*srcName  pointer of source name string
 * param[out]	*dstName  pointer of destination name string
 *
 * return	none
 * retval	
 *
 * note		yuchuwei modify @ 2012-4-4
 */
static void getSrcDstDisplayName( pjsua_call *call, char *srcName, char *dstName )
{
	/*ycw-usbvm*/
	pjsip_fromto_hdr *from = NULL;
	pjsip_fromto_hdr *to = NULL;

	from = call->inv->dlg->remote.info;
	to = call->inv->dlg->local.info;
	
	/* Get source display name from URI of From message */
	if (from)
	{
		getDisplayName(from, srcName);
		MARK_PRINT("%s:srcName:%s\n", __FUNCTION__, srcName);
	}
	
	/* Get destination display name from URI of To message */
	if (to)
	{
		getDisplayName(to, dstName);
		MARK_PRINT("%s:dstName:%s\n", __FUNCTION__, dstName);
	}	
}

/* 
 * fn		static void sipMsgModifyCMTimer( int recordId, int timeValue )
 * brief	Modify cm  endpoint's timer via messge
 * details	
 *
 * param[in]	recordId  reocrd ID
 * param[in]	timeValue  time value  
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
static void sipMsgModifyCMTimer( int recordId, int timeValue )
{
	CMSIP_MSG cmSipMsg;
	CMSIP_USBVM_CHANGE_TIMER *pCmTimer = NULL; 
	
	cmSipMsg.type = CMSIP_REQUEST_USBVM_CHANGE_TIMER;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
	pCmTimer = (CMSIP_USBVM_CHANGE_TIMER*)cmSipMsg.body;
	pCmTimer->callIndex = g_usbvm_recordHandle[recordId].cnxId;
	pCmTimer->endpt = g_usbvm_recordHandle[recordId].endpt;
	pCmTimer->timerValue = timeValue;
	cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	#if 0	
	MARK_PRINT("%s:%d:sip sending msg:CMSIP_REQUEST_USBVM_CHANGE_TIMER cid:%d, recordId:%d, timer:%d\n",
		__FUNCTION__, __LINE__, pCmTimer->callIndex,recordId, timeValue);
	#endif
}

/* 
 * fn		static void sipMsgGenOnhookEvt( int recordId )
 * brief	Generate onhook event to cm via message
 * details	
 *
 * param[in]	recordId  record ID
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
static void sipMsgGenOnhookEvt( int recordId )
{
	CMSIP_MSG cmSipMsg;	
	CMSIP_USBVM_GENERATE_EVTONHOOK *pCmEvtOnhook = NULL;
	
	cmSipMsg.type = CMSIP_REQUEST_USBVM_GENERATE_EVTONHOOK;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
	pCmEvtOnhook = (CMSIP_USBVM_GENERATE_EVTONHOOK*)cmSipMsg.body;								
	pCmEvtOnhook->callIndex = g_usbvm_recordHandle[recordId].cnxId;
	pCmEvtOnhook->endpt = g_usbvm_recordHandle[recordId].endpt;
	cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	
	MARK_PRINT("sip sending msg:CMSIP_REQUEST_USBVM_GENERATE_EVTONHOOK. endpt:%d, callId:%d\n",
		pCmEvtOnhook->endpt, pCmEvtOnhook->callIndex);
}
/* 
 * fn		static void sipMsgRemoteStatusChange(int endpt,int status,unsigned int globalWavTimeName)
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
static void sipMsgRemoteStatusChange(int endpt,int status,unsigned int globalWavTimeName)
{
	CMSIP_MSG cmSipMsg;	
	CMSIP_USBVM_REMOTE_STATUS_CHANGE *pRemoteStatusChange = NULL;
	if(endpt<0||endpt>=ENDPT_NUM)
	{
		return;
	}
	cmSipMsg.type = CMSIP_REQUEST_USBVM_REMOTE_STATUS_CHANGE;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
	pRemoteStatusChange = (CMSIP_USBVM_REMOTE_STATUS_CHANGE*)cmSipMsg.body;								
	pRemoteStatusChange->status= status;
	pRemoteStatusChange->endpt = endpt;
	pRemoteStatusChange->globalWavTimeName= globalWavTimeName;
	cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
}

/* add by yuanjp 2015/5/28 */
/*
 * fn			
 *
 * brief		keep voice mail send rtp continuously and 
 *				with correct sequence and timestamp
 *
 * note			now we make all rtp packet in answer machine continuous:
 *					SSRC don't change
 *					sequence increase one by one
 *					timestamp increase correctly
 				rtp packets captured by wireshark seems good.
 				
 *				but if endpoint hook off, then rtp will be processed by DSP,
 					we keep SSRC don't change (900v Broadcom can do this )
 					but now we don't keep sequence and timestmp continuous.
 *
 *
 * param[in]
 *
 */
static void sleepMsWithComfortNoise(int sleepms, PSleepMsWithCNContext context)
{
	RTPDATA rtpPacket;
	int cnPktTime = CNG_TIMEVAL;	/* a comfort noise packet last 200ms */
	int pktNum = sleepms / cnPktTime;	
	int pcmTime = (sleepms - CNG_TIMEVAL * pktNum);
	int pcmNum = pcmTime / context->TX_pktPeriod;
	struct timeval tv;
	int indx;
	UINT16 *seq = context->seq;
	UINT32 *ts = context->ts;
	int cngPeriod = context->TX_pktSize/context->TX_pktPeriod * CNG_TIMEVAL;
	int *maker = context->maker;

	/* firstly:
	 * if sleepms is a little bit long
	 * we should send rtp use Comfort Noise
	 */
	for(indx = 0; indx < pktNum; indx ++)
	{		
		int len = 1;

		*maker = 1;
		
		rtpPacket.RTPheader.v = 2;
		rtpPacket.RTPheader.p = 0;
		rtpPacket.RTPheader.x = 0;
		rtpPacket.RTPheader.cc = 0;
		rtpPacket.RTPheader.m = 0;
		rtpPacket.RTPheader.pt = CNG_PAYLOAD_TYPE_OLD;

		rtpPacket.RTPheader.seq = htons(*seq);
		rtpPacket.RTPheader.ts = htonl(*ts);
		rtpPacket.RTPheader.ssrc = htonl(context->ssrc);

		memset(rtpPacket.RTPdata,0,MAX_USBVM_RTP_LENGTH);
		rtpPacket.RTPdata[0] = (char)CNG_LEVEL; 

		tv.tv_sec = 0;
		tv.tv_usec = cnPktTime * 1000;
		select(0, NULL, NULL, NULL, &tv);
		
		(*ts) += cngPeriod;	/* time elapsed, so increase timestamp here */
		
		PJSUA_LOCK();
		if (!chkRtpStreamReady(context->call))
		{				
			PJSUA_UNLOCK();
			continue;
		}
		/* Send rtp packet */
		pjmedia_transport_send_rtp(context->call->session->stream[0]->transport, (pjmedia_rtp_hdr *)&rtpPacket,
			len + sizeof(pjmedia_rtp_hdr));
		
		PJSUA_UNLOCK();
		
		(*seq) ++;	/* pakcet send, so increase sequence here */
	}

	/* secondly:
	 * other time we should send rtp use PCM with sound value is 0 
	 */
	for (indx = 0; indx < pcmNum; indx++)
	{
		int len = context->TX_pktSize;
		rtpPacket.RTPheader.v = 2;
		rtpPacket.RTPheader.p = 0;
		rtpPacket.RTPheader.x = 0;
		rtpPacket.RTPheader.cc = 0;
		
		if (*maker)
		{
			rtpPacket.RTPheader.m = 1;
			*maker = 0;
		}
		else
		{
			rtpPacket.RTPheader.m = 0;
		}
		
		if (g_usbvm_recordHandle[context->recordId].payloadType == RTP_PAYLOAD_PCMU)
		{
			memset(rtpPacket.RTPdata,PCMU_MUTE_DATA,context->TX_pktSize);
		}
		else if (g_usbvm_recordHandle[context->recordId].payloadType == RTP_PAYLOAD_PCMA)
		{
			memset(rtpPacket.RTPdata,PCMA_MUTE_DATA,context->TX_pktSize);
		}
		
		rtpPacket.RTPheader.seq = htons(*seq);
		rtpPacket.RTPheader.ts = htonl(*ts);
		rtpPacket.RTPheader.ssrc = htonl(context->ssrc);

		tv.tv_sec = 0;
		tv.tv_usec = context->TX_pktPeriod * 1000;
		select(0, NULL, NULL, NULL, &tv);
		
		(*ts) += context->TX_pktSize;	/* why is TX_pktSize not TX_pktPeriod ? */
		
		/* Check if session is being closed */
		PJSUA_LOCK();
		if (!chkRtpStreamReady(context->call))
		{				
			PJSUA_UNLOCK();
			continue;
		}
		
		/* Send rtp packet */
		pjmedia_transport_send_rtp(context->call->session->stream[0]->transport, (pjmedia_rtp_hdr *)&rtpPacket,
			len + sizeof(pjmedia_rtp_hdr));

		PJSUA_UNLOCK();
		
		(*seq) ++;
	}
}
/* dd by yuanjp end */

/* 
 * fn		static void remotePlayThread( void *context )
 * brief	Play voice notify to remote UA and process received DTMF
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
static void remotePlayThread( void *context )
{
	RTPDATA rtpPacket;
	UINT32  ts = 0;
	UINT16  seq = 0;
	UINT32  ssrc = 0;
	int cngPeriod = 0;
	UINT32  currentPos = 0;
	int     rc = NO_ERROR;
	int     len = 0;
	int     TX_pktPeriod = 0;	/* the period between RTP pkts that we transport to the opponent */
	int     TX_pktSize = 0;
	int  	RX_pktPeriod = 0;	/* the period between RTP pkts that we recieve */
	int 	RX_pktSize = 0;
	int     recordId = (int)context;
	int     pinValidateCount = 0;
	int     nodePos = 0;
	int     nodeCount = 0;
	int     nodeNum = 0;
	short	holdCount = 0;
	char    notifyFilePathName[100] ={ 0 };
	char    rtpDataTemp[MAX_USBVM_RTP_LENGTH] = { 0 };
	char    *pPinNum = NULL;
	BOOL    bRecStopNotifyFinished = FALSE;
	BOOL    bPinNotifyFinished = FALSE;
	BOOL    bFirstRecordStart = FALSE;
	/* wlm: add to control remote access voice mail */
	BOOL    isRemoteAccessAllowed = FALSE;
	BOOL    bRecStartNotifyDeNotPlayed = TRUE;
	int     useCustomNotify = 0;
	BOOL    bAddRecSucceed = FALSE;
	UINT32  recDuration = 0;
	UINT16  dtmfNum = 0;
	int cngTimeVal = 0;
	BOOL marker = FALSE;
	struct timeval tv;
	USBVM_REC_FILE_NODE recordNode;
	int  gotoDeinit=0;
	FILE* fpIndex = NULL;
	int lockRet = -1;
	int endpt;
	REMOTE_STAS statusRead;
#ifdef INCLUDE_USBVM_MODULE
	int handle = 0, data;
#endif /* INCLUDE_USBVM_MODULE */
	BOOL pinNumGet = FALSE;
	BOOL pinError = TRUE;
	SleepMsWithCNContext sleepContext;
	
	/* Edited by [Huang Lei], [07Nov12] */
	g_usbvm_recordHandle[recordId].cngSend = FALSE;
	/* Edited End */
	g_usbvm_recordHandle[recordId].playThreadRunning = PJ_TRUE;
	usbvm_remoteStatusLockWrite(REMOTE_START, recordId);
	g_usbvm_recordHandle[recordId].holdCall =FALSE;
	/*Edited by huanglei, 2011/11/21, get both remote and local ptime */
	calRtpPacketParams(g_usbvm_recordHandle[recordId].remotePtime,
			g_usbvm_recordHandle[recordId].payloadType,
			&TX_pktPeriod, &TX_pktSize,
			&g_usbvm_recordHandle[recordId].tsRemoteInterval
			);	
	calRtpPacketParams(g_usbvm_recordHandle[recordId].localPtime,
			g_usbvm_recordHandle[recordId].payloadType,
			&RX_pktPeriod, &RX_pktSize,
			&g_usbvm_recordHandle[recordId].tsInterval
			);
	
	cngPeriod = TX_pktSize/TX_pktPeriod * CNG_TIMEVAL;
	
	/* Get setting value from cmsip.conf file */
	endpt = g_usbvm_recordHandle[recordId].endpt;
	if(endpt==-1)
	{
		MARK_PRINT("HL_DBG: get configeration error! endpt==-1! %s:%d\n",__FUNCTION__,__LINE__);
	}

	useCustomNotify = pjsua_var.usbVmEndptConfig[endpt].useCustomeNotifyForRemote;
	pPinNum = pjsua_var.usbVmEndptConfig[endpt].pin;
	recDuration = g_usbvm_recordHandle[recordId].remoteRecDuration;

	l_exitMsgSent[recordId] = FALSE;

	/* Find stream according to call id */
	pjsua_call *call = &pjsua_var.calls[g_usbvm_recordHandle[recordId].cnxId];

	MARK_PRINT("entering thread %s, pid:%d, recordId:%d, endpt:%d, useCustomNotify %d, cnxId:%d, rtpPayLoadType:%d,TX_pktPeriod %d,TX_pktSize %d, RX_pktPeriod:%d, RX_pktSize:%d, tsInterval:%d\n", 
			__FUNCTION__, getpid(), recordId, endpt, useCustomNotify,
			g_usbvm_recordHandle[recordId].cnxId, g_usbvm_recordHandle[recordId].payloadType,
			TX_pktPeriod, TX_pktSize,
			RX_pktPeriod, RX_pktSize, g_usbvm_recordHandle[recordId].tsInterval);

	sleepContext.call = call;
	sleepContext.recordId = recordId;
	sleepContext.TX_pktPeriod = TX_pktPeriod;
	sleepContext.TX_pktSize = TX_pktSize;
	sleepContext.maker = &marker;
	
	while(1)
	{
		usbvm_remoteStatusLockRead(&statusRead, recordId);
		if(statusRead == REMOTE_STOP)
		{
			gotoDeinit = 1;
			goto deInit;
		}

		if (!g_usbvm_recordHandle[recordId].bNoAnswer)
		{				
			if (statusRead == REMOTE_PLAY)
			{
				cmsip_send_systemlog(CMSIP_SYSTEMLOG_DEBUG, 
					"USB Voice MailBox-- Voice Remote Play Stop!\n");
			}
			usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
			rc = ERROR_STREAM_CLOSED;
			MARK_PRINT("break in remotePlayThread! %s:%d\n",__FUNCTION__,__LINE__);
			break;
		}

		PJSUA_LOCK();
		if(statusRead==REMOTE_PLAY)
		{
			if(call&&call->inv)
			{
				if (((call->inv->state==PJSIP_INV_STATE_CONFIRMED)			
						&& (PJSUA_CALL_MEDIA_NONE== call->media_st))
					||(PJSUA_CALL_MEDIA_REMOTE_HOLD == call->media_st))
				{
					if(g_usbvm_recordHandle[recordId].holdCall == FALSE)
					{
						g_usbvm_recordHandle[recordId].holdCall = TRUE;
						/* only in REMOTE_PLAY, timeout when hold.because we don't know how long
						the records will be played, don't timeout in REMOTE_PLAY.But when hold,
						we still need timeout to release the endpt,so that the others can call in*/
						holdCount++;
						if(holdCount>=3)
						{/*limit the max hold time no longer than 60s, exit after 3 times  */
							sipMsgModifyCMTimer(recordId, 100);
						}
						else
						{
							sipMsgModifyCMTimer(recordId, 20000);
						}
					}
					/* Added by huanglei for debug print, 2012/02/27 */
					MARK_PRINT("HL_DBG: continue!!!g_usbvm_recordHandle[recordId].holdCall=%d [%s:%d]\n",
							g_usbvm_recordHandle[recordId].holdCall,__FUNCTION__,__LINE__);
					/* end huanglei */
					PJSUA_UNLOCK();
					usleep(500 * 1000);
					continue;
				}
				else if(g_usbvm_recordHandle[recordId].holdCall == TRUE)
				{
					MARK_PRINT("HL_DBG: unhold,recover! %s:%d\n",__FUNCTION__,__LINE__);
					sipMsgModifyCMTimer(recordId, 0);
					g_usbvm_recordHandle[recordId].holdCall = FALSE;
				}
			}
		}
		/* If stream is closed or bNoAnswer is not true, exit the thread */
		if (!chkRtpStreamReady(call))
		{ 
			PJSUA_UNLOCK();
			usleep(500*1000);
			continue;
		}
		PJSUA_UNLOCK();

		if(usbvm_checkUSBDevExist() == TRUE)
		{
			/* Handle received dtmf data */ 
			usbvm_remoteStatusLockRead(&statusRead, recordId);
			switch(statusRead)
			{
			case REMOTE_IDLE:
				usleep(200 * 1000);
				break;

			case REMOTE_STOP:
				MARK_PRINT("HL_DBG:REMOTE_STOP,goto deInit! %s:%d\n",__FUNCTION__,__LINE__);
				/* u盘突然被拔掉  */
				goto deInit;
				break;

			case REMOTE_START:
				{
					/* Start to play remote record starting notify */
					ssrc = g_usbvm_recordHandle[recordId].ssrc;
					ts = rand();
					seq = rand();
					currentPos = 0;

					sleepContext.ssrc = ssrc;
					sleepContext.seq = &seq;
					sleepContext.ts = &ts;

					/* close old fp in 'g_usbvm_recordHandle' if it is used,before open new fp */
					usbvm_closeRecFile(recordId);
					if (getNotifyFileByPayLoad(endpt, g_usbvm_recordHandle[recordId].payloadType,
							useCustomNotify, 
							(FILE **)&(g_usbvm_recordHandle[recordId].outputFp),
							(unsigned int *)&(g_usbvm_recordHandle[recordId].fileSize)) == TRUE)
					{
						usbvm_remoteStatusLockWrite(REMOTE_RECORD_START_NOTIFY, recordId);
						/*  [Huang Lei] [08Nov12] ,add marker for first packet */
						marker = TRUE;
						MARK_PRINT("HL_DBG: change timer!!! [%s:%d]\n",__FUNCTION__,__LINE__);
						/* when playing, no matter user hold or not, we will timeout. 
						To avoid that if a user hold again and again, the usbvm never exit. 
						Usually we will not play a notification longer than 20s. 
						This timer will be reset if we start recording. */
						sipMsgModifyCMTimer(recordId, 60000);

						dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
						bFirstRecordStart = TRUE;
/*20130618_update*/
						/*<< BosaZhong@09Jun2013, add, skip dtmf # when recording. */
						usbvm_remoteRecord_clrGotDtmfHashInRecording(recordId);
						/*>> endof BosaZhong@09Jun2013, add, skip dtmf # when recording. */
					}
					else
					{
						rc = ERROR_NOTIFY_NOT_EXIST;
						goto exit;
					}
				}
				break;
				
			case REMOTE_RECORD_START_NOTIFY:
				{
					//* wlmtodo:
					/* wlmusbtodo: add remote access control. */
					if (bFirstRecordStart && pjsua_var.enableVMRemoteAcc)
					{
						/* Enter PIN validation stage with key '*', start to play PIN input notify */
						if (dtmf_FIFOGet(g_usbvm_recordHandle[recordId].cnxId, &dtmfNum) == TRUE)
						{
							if (dtmfNum == RTP_NTE_DTMFS)
							{
								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);
								if (g_usbvm_recordHandle[recordId].pVoiceNotifyBuf != NULL)
								{
									g_usbvm_recordHandle[recordId].pVoiceNotifyBuf = NULL;
								}
								/*						
								ssrc = rand();
								ts = 0;
								seq = 0;
								*/
								currentPos = 0;

								strcpy(notifyFilePathName, USB_NOT_FILE_DEF_DIR);
								if (g_usbvm_recordHandle[recordId].payloadType == 0)
								{
									strcat(notifyFilePathName, "pinInputNotify_g711u");
								}
								else
								{
									strcat(notifyFilePathName, "pinInputNotify_g711a");
								}

								if (access(notifyFilePathName, F_OK) == 0) 
								{
									if (access(notifyFilePathName, R_OK) != 0)
									{
										chmod(notifyFilePathName, 0644);
									}

									g_usbvm_recordHandle[recordId].fileSize = usbvm_getFileSize(notifyFilePathName);
									if ((g_usbvm_recordHandle[recordId].outputFp = fopen(notifyFilePathName, "rb")) == NULL)
									{
										MARK_PRINT("Failed to open an exist file %s for reading\n", notifyFilePathName);
										rc = ERROR_FILE_OPEN;

										gotoDeinit = TRUE;
										/*Edited by huanglei,退出后使cm强制超时，
										这样整个远端usbvm都会退出，而不仅仅是本线程退出 2012/01/11*/
										sipMsgModifyCMTimer(recordId, 100);

										goto deInit;
									}
								}
								else
								{
									rc = ERROR_NOTIFY_NOT_EXIST;
									/*Edited by huanglei,退出后使cm强制超时，
									这样整个远端usbvm都会退出，而不仅仅是本线程退出 2012/01/11*/
									sipMsgModifyCMTimer(recordId, 100);

									goto deInit;
								}

								memset(g_usbvm_recordHandle[recordId].collectPinNum,
										0, sizeof(g_usbvm_recordHandle[recordId].collectPinNum));
								usbvm_remoteStatusLockWrite(REMOTE_PINVALIDATE, recordId);
								pinError = FALSE;
								pinNumGet = FALSE;
								dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);

								/* Notify cm to Change endpoint timer */
								sipMsgModifyCMTimer(recordId, 30000);
							}
						}
					}
				}
				break;
				
			case REMOTE_RECORD:
				{
					/* End record with key '#', start to play remote record stopping notify */
					if (dtmf_FIFOGet(g_usbvm_recordHandle[recordId].cnxId, &dtmfNum) == TRUE)
					{
						if (dtmfNum == RTP_NTE_DTMFH)
						{
							/* Edited by [Huang Lei], [07Nov12] */
							g_usbvm_recordHandle[recordId].cngSend =FALSE;
							/* Edited End */
#ifdef INCLUDE_USBVM_MODULE
							if (handle > 0)
							{
								ioctl(handle, _VOIP_TIMER_STOP, &endpt);
								close(handle);
								handle = 0;
							}
#endif /* INCLUDE_USBVM_MODULE */
							if (g_usbvm_recordHandle[recordId].bNoAnswer)
							{
								bAddRecSucceed = usbvm_remoteRecStop(recordId);
								currentPos = 0;
								strcpy(notifyFilePathName, USB_NOT_FILE_DEF_DIR);
								if (g_usbvm_recordHandle[recordId].payloadType == 0)
								{
									strcat(notifyFilePathName, "recStopNotify_g711u");
								}
								else
								{
									strcat(notifyFilePathName, "recStopNotify_g711a");
								}

								if (access(notifyFilePathName, F_OK) == 0) 
								{
									if (access(notifyFilePathName, R_OK) != 0)
									{
										chmod(notifyFilePathName, 0644);
									}

									g_usbvm_recordHandle[recordId].fileSize = usbvm_getFileSize(notifyFilePathName);
									/* close old fp in 'g_usbvm_recordHandle' if it is used,before open new fp */
									usbvm_closeRecFile(recordId);
									if ((g_usbvm_recordHandle[recordId].outputFp = fopen(notifyFilePathName, "rb")) == NULL)
									{
										MARK_PRINT("Failed to open an exist file %s for reading\n", notifyFilePathName);
										rc = ERROR_FILE_OPEN;
										/*Edited by huanglei,退出后使cm强制超时，
										这样整个远端usbvm都会退出，而不仅仅是本线程退出 2012/01/11*/
										sipMsgModifyCMTimer(recordId, 100);
										goto exit;
									}
								}
								else
								{
									rc = ERROR_NOTIFY_NOT_EXIST;
									/*Edited by huanglei,退出后使cm强制超时，
									这样整个远端usbvm都会退出，而不仅仅是本线程退出 2012/01/11*/
									sipMsgModifyCMTimer(recordId, 100);
									goto deInit;
								}
								usbvm_remoteStatusLockRead(&statusRead, recordId);
								if(statusRead==REMOTE_STOP)
								{
									MARK_PRINT("HL_DBG:REMOTE_STOP change into REMOTE_RECORD_STOP_NOTIFY\n");
								}
								usbvm_remoteStatusLockWrite(REMOTE_RECORD_STOP_NOTIFY, recordId);
								/*  [Huang Lei] [08Nov12]  */
								marker = TRUE;
								dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);

								/* Notify cm to Change endpoint timer */
								sipMsgModifyCMTimer(recordId, 60000);

								bRecStopNotifyFinished = FALSE;
							}
						}
					}

					/* Edited by [Huang Lei], [07Nov12] */
#if 0 /* Debug,here's the old version.*/
					usleep(20 * 2 * 1000); /* Wait for 2 * 20ms */
					//ts += 20 * 2 * 8;
					sleepCount++;
#endif /* Debug */
				}
				break;
				
			case REMOTE_RECORD_STOP_NOTIFY:
				{
					if (dtmf_FIFOGet(g_usbvm_recordHandle[recordId].cnxId, &dtmfNum) == TRUE)
					{
						switch(dtmfNum)
						{
						/* Exit record with key '#' */
						case RTP_NTE_DTMFH:
							{
#ifdef INCLUDE_USBVM_MODULE
								if (handle > 0)
								{
									ioctl(handle, _VOIP_TIMER_STOP, &endpt);
									close(handle);
									handle = 0;
								}
#endif /* INCLUDE_USBVM_MODULE */
								MARK_PRINT("simulate CMEVT_ONHOOK in CMST_TALK\n");
								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);
								if (g_usbvm_recordHandle[recordId].bNoAnswer)
								{
									/* Notify cm to Change endpoint timer */
									sipMsgModifyCMTimer(recordId, 0);
									/* Notify cm to generate ONHOOK event */
									sipMsgGenOnhookEvt(recordId);
									usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
									g_usbvm_recordHandle[recordId].cnxId = -1;
									dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
								}
								pthread_mutex_lock(&g_usbvm_sendMsgLock[recordId]);
								if(l_exitMsgSent[recordId]==FALSE)
								{
									MARK_PRINT("HL_DEBUG:send CMSIP_USBVM_REMOTE_STATE_EXITED msg to cm!\n");
									if((g_usbvm_recordHandle[recordId].globalWav==1)
										&&(g_usbvm_recordHandle[recordId].globalTimeName>0))
									{
										MARK_PRINT("HL_DBG: globalTimeName=(%08x)!  [%s:%d]\n",
												g_usbvm_recordHandle[recordId].globalTimeName,__FUNCTION__,__LINE__);

										sipMsgRemoteStatusChange(endpt,
												CMSIP_USBVM_REMOTE_STATE_EXITED,
												g_usbvm_recordHandle[recordId].globalTimeName);
										g_usbvm_recordHandle[recordId].globalTimeName = -1;
									}
									else
									{
										sipMsgRemoteStatusChange(endpt,
												CMSIP_USBVM_REMOTE_STATE_EXITED, -1);
									}
									l_exitMsgSent[recordId]=TRUE;
								}
								pthread_mutex_unlock(&g_usbvm_sendMsgLock[recordId]);
								goto exit;
							}
							break;
							
						/* Re-record with key '2' */
						case RTP_NTE_DTMF2:
							{
#ifdef INCLUDE_USBVM_MODULE
								if (handle > 0)
								{
									ioctl(handle, _VOIP_TIMER_STOP, &endpt);
									close(handle);
									handle = 0;
								}
#endif /* INCLUDE_USBVM_MODULE */
/*20130618_update*/
								/*<< BosaZhong@09Jun2013, add, skip dtmf # when recording. */
								usbvm_remoteRecord_clrGotDtmfHashInRecording(recordId);
								/*>> endof BosaZhong@09Jun2013, add, skip dtmf # when recording. */
								
								/* Delete last recorded file,and re-record */
								int retInfo = -1;
								lockRet = usbvm_openIndexFpAndLock(endpt,
										&fpIndex, "rb+", WR_LOCK_WHOLE);
								if(lockRet != -1)
								{
									int numPos = usbvm_getRecNumInAll(endpt, fpIndex);
									retInfo = usbvm_delRecIndexListByPos(endpt, numPos,fpIndex);
									lockRet = usbvm_closeIndexFpAndUnlock(endpt, 
											fpIndex, WR_LOCK_WHOLE);
									if(lockRet == -1)
									{
										MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
									}
								}
								else
								{
									MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
								}
								if(retInfo!=0)
								{
									MARK_PRINT("HL_DBG: delete last recorded file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
								}

								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);
								/* Notify cm to Change endpoint timer */
								sipMsgModifyCMTimer(recordId, 60000);

								/*
								ssrc = rand();
								ts = 0;
								seq = 0;
								*/
								currentPos = 0;
								sleepMsWithComfortNoise(1000, &sleepContext);
								// sleep(1);
								//ts += 1000 * 8;
								/* start to play DE sound instead of remote record starting notify */
								usbvm_getVoiceNotifyDe(
										(char **)&(g_usbvm_recordHandle[recordId].pVoiceNotifyBuf), 
										(unsigned int *)&(g_usbvm_recordHandle[recordId].fileSize));

								usbvm_remoteStatusLockWrite(REMOTE_RECORD_START_NOTIFY, recordId);
								bFirstRecordStart = FALSE;
								bRecStartNotifyDeNotPlayed = FALSE;
								dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
							}
							break;
							
						/* Verify record with key '1' */
						case RTP_NTE_DTMF1:
							{
#ifdef INCLUDE_USBVM_MODULE
								if (handle > 0)
								{
									ioctl(handle, _VOIP_TIMER_STOP, &endpt);
									close(handle);
									handle = 0;
								}
#endif /* INCLUDE_USBVM_MODULE */
								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);

								/*
								ssrc = rand();
								ts = 0;
								seq = 0;
								*/
								currentPos = WAV_HEADER_LEN;
								sleepMsWithComfortNoise(1000, &sleepContext);
								//sleep(1);
								//ts += 1000 * 8;
								/* Start to play record verify notify */
								if (bAddRecSucceed)
								{	
									int retInfo = -1;
									lockRet = usbvm_openIndexFpAndLock(endpt,
											&fpIndex, "rb+", WR_LOCK_WHOLE);
									if(lockRet != -1)
									{
										retInfo = usbvm_getRecIndexListNodeInfoAndFd(endpt,
												usbvm_getRecNumInAll(endpt, fpIndex), &recordNode, 
												(FILE **)&g_usbvm_recordHandle[recordId].outputFp, fpIndex);
										lockRet = usbvm_closeIndexFpAndUnlock(endpt, 
												fpIndex, WR_LOCK_WHOLE);
										if(lockRet == -1)
										{
											MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
										}
									}
									else
									{
										MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
									}
									if (retInfo == 0)
									{
										g_usbvm_recordHandle[recordId].fileSize = recordNode.fileSize;
										usbvm_remoteStatusLockWrite(REMOTE_RECORD_VERIFY, recordId);
										/* Change cm endpoint timer */
										/* the timeout is the length of record +10s */
										sipMsgModifyCMTimer(recordId,
												(g_usbvm_recordHandle[recordId].fileSize/REC_SIZE_ONE_SEC + 10)*1000);
									}
									else
									{
										rc = ERROR_RECINFO_GET;
										gotoDeinit = 2;
										/*Edited by huanglei,退出后使cm强制超时，
										这样整个远端usbvm都会退出，而不仅仅是本线程退出 2012/01/11*/
										sipMsgModifyCMTimer(recordId, 100);
										goto deInit;
									}
								}
								else
								{
									MARK_PRINT(" The record file has not saved!!!\n ")
									rc = ERROR_REC_NOT_EXIST;
									sipMsgModifyCMTimer(recordId, 100);
									goto deInit;
								}
								dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
							}
							break;
							
						default:
							break;
						}
					}
					else if (bRecStopNotifyFinished)
					{
						sleepMsWithComfortNoise(50, &sleepContext);
						//usleep(50 * 1000);
						//ts += 50 * 8;
					}
				}
				break;
				
			case REMOTE_RECORD_VERIFY:
				{
					if(dtmf_FIFOGet(g_usbvm_recordHandle[recordId].cnxId, &dtmfNum) == TRUE)
					{
						switch(dtmfNum)
						{
						/* Exit verifing with key '#', play notify again */
						case RTP_NTE_DTMFH:
							{
#ifdef INCLUDE_USBVM_MODULE
								if (handle > 0)
								{
									ioctl(handle, _VOIP_TIMER_STOP, &endpt);
									close(handle);
									handle = 0;
								}
#endif /* INCLUDE_USBVM_MODULE */
								MARK_PRINT("exit REMOTE_RECORD_VERIFY\n");
								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);
								/* Start to play remote record stopping notify */
								/*
								ssrc = rand();
								ts = 0;
								seq = 0;
								*/
								currentPos = 0;
								strcpy(notifyFilePathName, USB_NOT_FILE_DEF_DIR);
								if (g_usbvm_recordHandle[recordId].payloadType == 0)
								{
									strcat(notifyFilePathName, "recStopNotify_g711u");
								}
								else
								{
									strcat(notifyFilePathName, "recStopNotify_g711a");
								}

								if (access(notifyFilePathName, F_OK) == 0) 
								{
									if (access(notifyFilePathName, R_OK) != 0)
									{
										chmod(notifyFilePathName, 0644);
									}

									g_usbvm_recordHandle[recordId].fileSize = usbvm_getFileSize(notifyFilePathName);
									if ((g_usbvm_recordHandle[recordId].outputFp = fopen(notifyFilePathName, "rb")) == NULL)
									{
										MARK_PRINT("Failed to open an exist file %s for reading\n", notifyFilePathName);
										rc = ERROR_FILE_OPEN;
										goto exit;
									}
								}
								usbvm_remoteStatusLockWrite(REMOTE_RECORD_STOP_NOTIFY, recordId);
								/* Notify cm to Change endpoint timer */									
								sipMsgModifyCMTimer(recordId, 60000);
								bRecStopNotifyFinished = FALSE;
							}
							break;
							
						/* Re-record with key '2' */
						case RTP_NTE_DTMF2:
							{
#ifdef INCLUDE_USBVM_MODULE
								if (handle > 0)
								{
									ioctl(handle, _VOIP_TIMER_STOP, &endpt);
									close(handle);
									handle = 0;
								}
#endif /* INCLUDE_USBVM_MODULE */
								/* close fp first */
								MARK_PRINT("exit REMOTE_RECORD_VERIFY\n");
								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);
								/* Delete last recorded file,and re-record */
								int retInfo = -1;
								lockRet = usbvm_openIndexFpAndLock(endpt,
										&fpIndex, "rb+", WR_LOCK_WHOLE);
								if(lockRet != -1)
								{
									int numPos = usbvm_getRecNumInAll(endpt, fpIndex);
									retInfo = usbvm_delRecIndexListByPos(endpt, numPos,fpIndex);
									lockRet = usbvm_closeIndexFpAndUnlock(endpt, 
											fpIndex, WR_LOCK_WHOLE);
									if(lockRet == -1)
									{
										MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
									}
								}
								else
								{
									MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
								}
								if(retInfo!=0)
								{
									MARK_PRINT("HL_DBG: delete last recorded file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
								}

								/* Notify cm to Change endpoint timer */
								sipMsgModifyCMTimer(recordId, 60000);
								currentPos = 0;
								/* sleep 1 sec to start recording */
								sleepMsWithComfortNoise(1000, &sleepContext);
								//sleep(1);
								//ts += 1000 * 8;
								/* start to play DE sound instead of remote record starting notify */
								usbvm_getVoiceNotifyDe(
										(char **)&(g_usbvm_recordHandle[recordId].pVoiceNotifyBuf), 
										(unsigned int *)&(g_usbvm_recordHandle[recordId].fileSize));
								/* Act as REMOTE_RECORD_START_NOTIFY is over,then break*/	
								usbvm_remoteStatusLockWrite(REMOTE_RECORD_START_NOTIFY, recordId);
								bFirstRecordStart = FALSE;
								bRecStartNotifyDeNotPlayed = FALSE;
								dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
							}
							break;
							
						/* Verify record with key '1' */
						case RTP_NTE_DTMF1:
							{
#ifdef INCLUDE_USBVM_MODULE
								if (handle > 0)
								{
									ioctl(handle, _VOIP_TIMER_STOP, &endpt);
									close(handle);
									handle = 0;
								}
#endif /* INCLUDE_USBVM_MODULE */
								g_usbvm_recordHandle[recordId].fileSize = 0;
								usbvm_closeRecFile(recordId);

								currentPos = WAV_HEADER_LEN;
								sleepMsWithComfortNoise(1000,&sleepContext);
								//sleep(1);
								//ts += 1000 * 8;
								/* Start to play record verify notify */
								if (bAddRecSucceed)
								{	
									int retInfo = -1;
									lockRet = usbvm_openIndexFpAndLock(endpt,
											&fpIndex, "rb+", WR_LOCK_WHOLE);
									if(lockRet != -1)
									{
										retInfo = usbvm_getRecIndexListNodeInfoAndFd(endpt,
												usbvm_getRecNumInAll(endpt, fpIndex), &recordNode, 
												(FILE **)&g_usbvm_recordHandle[recordId].outputFp, fpIndex);
										lockRet = usbvm_closeIndexFpAndUnlock(endpt, 
												fpIndex, WR_LOCK_WHOLE);
										if(lockRet == -1)
										{
											MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
										}
									}
									else
									{
										MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
									}
									if (retInfo == 0)
									{
										g_usbvm_recordHandle[recordId].fileSize = recordNode.fileSize;
										usbvm_remoteStatusLockWrite(REMOTE_RECORD_VERIFY, recordId);
										/* Change cm endpoint timer */
										/* the timeout is the length of record +10s */
										sipMsgModifyCMTimer(recordId,
												(g_usbvm_recordHandle[recordId].fileSize/REC_SIZE_ONE_SEC + 10)*1000);
									}
									else
									{
										rc = ERROR_RECINFO_GET;
										gotoDeinit = 2;
										/*Edited by huanglei,退出后使cm强制超时，
										这样整个远端usbvm都会退出，而不仅仅是本线程退出 2012/01/11*/
										sipMsgModifyCMTimer(recordId, 100);
										goto deInit;
									}
								}
								dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
							}
							break;
							
						default:
							break;
						}
					}
				}
				break;
				
			case REMOTE_PINVALIDATE:
				{
					if (dtmf_FIFOGet(g_usbvm_recordHandle[recordId].cnxId, &dtmfNum) == TRUE)
					{
						/* DTMF Got at the first time, to pause playing. */
						if('\0' == g_usbvm_recordHandle[recordId].collectPinNum[0])
						{
							pinNumGet = TRUE;
							bPinNotifyFinished = FALSE;
						}
						/* Input PIN end with '#' */
						if (dtmfNum == RTP_NTE_DTMFH)
						{
							pinNumGet = FALSE;
							g_usbvm_recordHandle[recordId].fileSize = 0;
							usbvm_closeRecFile(recordId);
							/* Notify cm to Change endpoint timer */
							sipMsgModifyCMTimer(recordId, 0);

							MARK_PRINT("collectPinNum:%s, pPinNum:%s\n",
							g_usbvm_recordHandle[recordId].collectPinNum, pPinNum);
							/* If PIN number input is correct */
							if (strcmp(g_usbvm_recordHandle[recordId].collectPinNum, pPinNum) == 0)
							{
								/*
								ssrc = rand();
								ts = 0;
								seq = 0;
								*/
								currentPos = WAV_HEADER_LEN;
								/* If there is some new records, start to play them */
								lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb+", WR_LOCK_WHOLE);
								if(lockRet!=-1)
								{
									if (usbvm_getRecNotReadNum(endpt, fpIndex) > 0)
									{
										nodePos = usbvm_getRecNotReadStartPos(endpt,fpIndex);
										nodeNum = usbvm_getRecNotReadNum(endpt,fpIndex);

										if ((usbvm_getRecIndexListNodeInfoAndFd(endpt, 
												nodePos, &recordNode, 
												(FILE **)&g_usbvm_recordHandle[recordId].outputFp,fpIndex)) == 0)
										{
											usbvm_getVoiceNotifyDe(
												(char **)&g_usbvm_recordHandle[recordId].pVoiceNotifyBuf, 
												(unsigned int *)&g_usbvm_recordHandle[recordId].fileSize);

											usbvm_remoteStatusLockWrite(REMOTE_PLAY, recordId);
											cmsip_send_systemlog(CMSIP_SYSTEMLOG_DEBUG, 
												"USB Voice MailBox-- Voice Remote Play Start!\n");
										}
										else
										{
											rc = ERROR_RECINFO_GET;
											gotoDeinit = 3;
											sipMsgModifyCMTimer(recordId, 100);
											lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
											if(lockRet == -1)
											{
												MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
											}
											goto deInit;
										}

									}
									/* If there is no new record, start to play no record notify */
									else
									{
										strcpy(notifyFilePathName, USB_NOT_FILE_DEF_DIR);
										if (g_usbvm_recordHandle[recordId].payloadType == 0)
										{
											strcat(notifyFilePathName, "noNewRecordNotify_g711u");
										}
										else
										{
											strcat(notifyFilePathName, "noNewRecordNotify_g711a");
										}

										if (access(notifyFilePathName, F_OK) == 0) 
										{
											if (access(notifyFilePathName, R_OK) != 0)
											{
												chmod(notifyFilePathName, 0644);
											}

											g_usbvm_recordHandle[recordId].fileSize = usbvm_getFileSize(notifyFilePathName);
											if ((g_usbvm_recordHandle[recordId].outputFp = fopen(notifyFilePathName, "rb")) == NULL)
											{
												MARK_PRINT("Failed to open an exist file %s for reading\n", notifyFilePathName);
												rc = ERROR_FILE_OPEN;
												sipMsgModifyCMTimer(recordId, 100);
												lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
												if(lockRet == -1)
												{
													MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
												}
												goto exit;
											}
										}
										usbvm_remoteStatusLockWrite(REMOTE_NORECORD, recordId);
									}
									lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
									if(lockRet == -1)
									{
										MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
									}
								}
								else
								{
									MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
								}
							}
							/* To play voice notify about PIN doesn't match */
							else
							{
								pinError = TRUE;
							}
							memset(g_usbvm_recordHandle[recordId].collectPinNum, 0, sizeof(g_usbvm_recordHandle[recordId].collectPinNum));
							dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
						}
						else if ((dtmfNum <= RTP_NTE_DTMF9) && (dtmfNum >= RTP_NTE_DTMF0))
						{
							if (strlen(g_usbvm_recordHandle[recordId].collectPinNum) < sizeof(g_usbvm_recordHandle[recordId].collectPinNum))
							{
								g_usbvm_recordHandle[recordId].collectPinNum \
									[strlen(g_usbvm_recordHandle[recordId].collectPinNum)] = (char)dtmfNum + '0';
								MARK_PRINT("%s collectPinNum:%s\n", __FUNCTION__, g_usbvm_recordHandle[recordId].collectPinNum);
							}
							else
							{
								pinError = TRUE;
							}
						}
						/* To play voice notify about PIN doesn't match */
						if (pinError)
						{
							/* Only three times trying allowed here */
							if (++pinValidateCount >= 3)
							{
								/* Notify cm to Change endpoint timer */
								sipMsgModifyCMTimer(recordId, 100);
								usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
								goto exit;
							}
							currentPos = 0;
							pinNumGet = FALSE;
							pinError = FALSE;

							strcpy(notifyFilePathName, USB_NOT_FILE_DEF_DIR);
							if (g_usbvm_recordHandle[recordId].payloadType == 0)
							{
								strcat(notifyFilePathName, "pinValidErrNotify_g711u");
							}
							else
							{
								strcat(notifyFilePathName, "pinValidErrNotify_g711a");
							}

							if (access(notifyFilePathName, F_OK) == 0) 
							{
								if (access(notifyFilePathName, R_OK) != 0)
								{
									chmod(notifyFilePathName, 0644);
								}

								g_usbvm_recordHandle[recordId].fileSize = usbvm_getFileSize(notifyFilePathName);
								if ((g_usbvm_recordHandle[recordId].outputFp = fopen(notifyFilePathName, "rb")) == NULL)
								{
									MARK_PRINT("Failed to open an exist file %s for reading\n", notifyFilePathName);
									rc = ERROR_FILE_OPEN;
									goto exit;
								}
							}
							/* Notify cm to Change endpoint timer */
							sipMsgModifyCMTimer(recordId, 30000);
							bPinNotifyFinished = FALSE;

							memset(g_usbvm_recordHandle[recordId].collectPinNum, 
								0, sizeof(g_usbvm_recordHandle[recordId].collectPinNum));
							dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);
						}
					}
				}
				break;
			
			default:
				break;
			}

			usbvm_remoteStatusLockRead(&statusRead, recordId);
			if ((statusRead!= REMOTE_IDLE) && (statusRead!= REMOTE_RECORD))
			{	
				if(TRUE == pinNumGet || bPinNotifyFinished)
				{
#ifdef INCLUDE_USBVM_MODULE
					if(handle > 0)
					{
						data = (TX_pktPeriod << 16) | endpt;
						ioctl(handle, _VOIP_TIMER_START, &data);
					}
					else
#endif /* INCLUDE_USBVM_MODULE */
					{
						tv.tv_sec = 0;
						tv.tv_usec = TX_pktPeriod * 1000;
						select(0, NULL, NULL, NULL, &tv);
					}
					/* continue to keep the timer. */
					continue;
				}
				/* fileSize==currentPos for "play finished" */
				if (g_usbvm_recordHandle[recordId].fileSize >= currentPos)
				{
					if ((g_usbvm_recordHandle[recordId].fileSize - currentPos) > TX_pktSize)
					{
						len = TX_pktSize;
					}
					else
					{
						len = g_usbvm_recordHandle[recordId].fileSize - currentPos;
					}
					/* There is still data available for a RTP packet */
					if (len > 0)
					{
						/* Form the RTP packet and send to the remote endpoint module */
						rtpPacket.RTPheader.v = 2;
						rtpPacket.RTPheader.p = 0;
						rtpPacket.RTPheader.x = 0;
						rtpPacket.RTPheader.cc = 0;
						if(marker)
						{
							rtpPacket.RTPheader.m = 1;
							marker = FALSE;
						}
						else
						{
							rtpPacket.RTPheader.m = 0;
						}
						rtpPacket.RTPheader.pt = g_usbvm_recordHandle[recordId].payloadType;
						rtpPacket.RTPheader.seq = htons(seq);
						rtpPacket.RTPheader.ts = htonl(ts);
						rtpPacket.RTPheader.ssrc = htonl(ssrc);						

						ts += len;
						seq++;
						/* Playing DE sound */
						if (g_usbvm_recordHandle[recordId].pVoiceNotifyBuf != NULL)
						{
							if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMU)
							{
								memcpy(rtpPacket.RTPdata, 
									&g_usbvm_recordHandle[recordId].pVoiceNotifyBuf[currentPos], len);
							}
							else if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMA)
							{
								memcpy(rtpDataTemp, 
									&g_usbvm_recordHandle[recordId].pVoiceNotifyBuf[currentPos], len);
								usbvm_G711UToG711A(rtpDataTemp, rtpPacket.RTPdata, len);
							}
						}
						/* Playing record file or notify file */
						else if (g_usbvm_recordHandle[recordId].outputFp != NULL)
						{
							usbvm_remoteStatusLockRead(&statusRead, recordId);
							if (statusRead== REMOTE_PLAY)
							{
								if (recordNode.payloadType == g_usbvm_recordHandle[recordId].payloadType)
								{
									if (fread(rtpPacket.RTPdata, len, 1, g_usbvm_recordHandle[recordId].outputFp) != 1)
									{
										rc = ERROR_FILE_READ;
										gotoDeinit = 4;
										sipMsgModifyCMTimer(recordId,100);
										goto deInit;
									}
								}
								else
								{
									if (fread(rtpDataTemp, len, 1, g_usbvm_recordHandle[recordId].outputFp) != 1)
									{
										rc = ERROR_FILE_READ;
										gotoDeinit = 5;
										sipMsgModifyCMTimer(recordId,100);
										goto deInit;
									}
									if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMU)
									{
										usbvm_G711AToG711U(rtpDataTemp, rtpPacket.RTPdata, len);
									}
									else if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMA)
									{
										usbvm_G711UToG711A(rtpDataTemp, rtpPacket.RTPdata, len);
									}
								}
							}
							else
							{
								if (fread(rtpPacket.RTPdata, len, 1, g_usbvm_recordHandle[recordId].outputFp) != 1)
								{
									rc = ERROR_FILE_READ;
									gotoDeinit = 6;
									sipMsgModifyCMTimer(recordId,100);
									goto deInit;
								}
							}
						}

						/* Packet interval is pktPeriod */
#ifdef INCLUDE_USBVM_MODULE
						if (0 == handle)
						{
							handle = open(_VOIP_DEV_NAME, O_RDWR);
						}
						if (handle > 0)
						{
							data = (TX_pktPeriod << 16) | endpt;
							ioctl(handle, _VOIP_TIMER_START, &data);
						}
						else
#endif /* INCLUDE_USBVM_MODULE */
						{
							tv.tv_sec = 0;
							tv.tv_usec = TX_pktPeriod * 1000;
							select(0, NULL, NULL, NULL, &tv);
						}

						/* Check if session is being closed */
						PJSUA_LOCK();
						if (!chkRtpStreamReady(call))
						{
							currentPos += len;/* 应该播放但播不了，实际上已经读出来了，应该加上len*/
							PJSUA_UNLOCK();
							continue;
						}

						/* Send rtp packet */						
						pjmedia_transport_send_rtp(call->session->stream[0]->transport, 
								(pjmedia_rtp_hdr *)&rtpPacket, len + sizeof(pjmedia_rtp_hdr));

						PJSUA_UNLOCK();
						currentPos += len;
					}
					else   /* Play finished */
					{
#ifdef INCLUDE_USBVM_MODULE
						if (handle > 0)
						{
							ioctl(handle, _VOIP_TIMER_STOP, &endpt);
							close(handle);
							handle = 0;
						}
#endif /* INCLUDE_USBVM_MODULE */
						usbvm_remoteStatusLockRead(&statusRead, recordId);
						if (statusRead!= REMOTE_PLAY)
						{
							g_usbvm_recordHandle[recordId].fileSize = 0;
							usbvm_closeRecFile(recordId);
							g_usbvm_recordHandle[recordId].pVoiceNotifyBuf = NULL;
						}
						/*Edited by huanglei, 2012/03/27, 既然播完就应当置为0*/
						currentPos = 0;		
						switch(statusRead)
						{
						case REMOTE_RECORD_START_NOTIFY:
							{
								/* Start to play De sound if it has not been played */
								if (bRecStartNotifyDeNotPlayed)
								{
									sleepMsWithComfortNoise(1000, &sleepContext);
									//sleep(1);
									//ts += 1000 * 8;
									currentPos = 0;
									usbvm_getVoiceNotifyDe(
										(char **)&g_usbvm_recordHandle[recordId].pVoiceNotifyBuf,
										(unsigned int *)&g_usbvm_recordHandle[recordId].fileSize);
									bRecStartNotifyDeNotPlayed = FALSE;
								}
								else
								{
									/* Notify cm to Change endpoint timer */
									sipMsgModifyCMTimer(recordId, recDuration * 1000);
									usbvm_remoteStatusLockWrite(REMOTE_RECORD, recordId);
									bAddRecSucceed = FALSE;
									g_usbvm_recordHandle[recordId].recordStop = FALSE;
									bRecStartNotifyDeNotPlayed = TRUE;
									usbvm_remoteRecStart( recordId );
									/* Edited by [Huang Lei], [07Nov12] */
									g_usbvm_recordHandle[recordId].cngSend =TRUE;
									marker = TRUE;
									/* Edited End */
								}
							}
							break;
							
						case REMOTE_RECORD_STOP_NOTIFY:
							{
								bRecStopNotifyFinished = TRUE;
							}
							break;
							
						case REMOTE_RECORD_VERIFY:
							{
								/* Start to play remote record stopping notify */
								/*
								ssrc = rand();
								ts = 0;
								seq = 0;
								*/
								currentPos = 0;
								strcpy(notifyFilePathName, USB_NOT_FILE_DEF_DIR);
								if (g_usbvm_recordHandle[recordId].payloadType == 0)
								{
									strcat(notifyFilePathName, "recStopNotify_g711u");
								}
								else
								{
									strcat(notifyFilePathName, "recStopNotify_g711a");
								}

								if (access(notifyFilePathName, F_OK) == 0) 
								{
									if (access(notifyFilePathName, R_OK) != 0)
									{
										chmod(notifyFilePathName, 0644);
									}

									g_usbvm_recordHandle[recordId].fileSize = usbvm_getFileSize(notifyFilePathName);
									/* close old fp in 'g_usbvm_recordHandle' if it is used,before open new fp */
									usbvm_closeRecFile(recordId);
									if ((g_usbvm_recordHandle[recordId].outputFp = fopen(notifyFilePathName, "rb")) == NULL)
									{
										MARK_PRINT("Failed to open an exist file %s for reading\n", notifyFilePathName);
										rc = ERROR_FILE_OPEN;
										goto exit;
									}
								}
								usbvm_remoteStatusLockWrite(REMOTE_RECORD_STOP_NOTIFY, recordId);

								/* Notify cm to Change endpoint timer */									
								sipMsgModifyCMTimer(recordId, 60000);
								bRecStopNotifyFinished = FALSE;
							}
							break;
							
						case REMOTE_PINVALIDATE:
							{
								bPinNotifyFinished = TRUE;
							}
							break;
							
						case REMOTE_NORECORD:
							{
								/* Notify cm to Change endpoint timer */
								sipMsgModifyCMTimer(recordId, 100);
								usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
								goto exit;
							}
							break;
							
						case REMOTE_PLAY:
							{
								g_usbvm_recordHandle[recordId].fileSize = 0;
								/* De sound play finished,start to play record file  */
								if (g_usbvm_recordHandle[recordId].pVoiceNotifyBuf != NULL)
								{
									g_usbvm_recordHandle[recordId].pVoiceNotifyBuf = NULL;
									currentPos = WAV_HEADER_LEN;
									g_usbvm_recordHandle[recordId].fileSize = recordNode.fileSize;
									MARK_PRINT("REMOTE_PLAY endpt:%d, pos:%d, filesize:%d\n", 
										endpt, nodePos, recordNode.fileSize);
								}
								/* Record file play finished */
								else if ((g_usbvm_recordHandle[recordId].outputFp != NULL) && (g_usbvm_recordHandle[recordId].pVoiceNotifyBuf == NULL))
								{
									MARK_PRINT("HL_DBG:Record file play finished %s:%d\n",__FUNCTION__,__LINE__);
									usbvm_closeRecFile(recordId);
									/* Update the infomation in record indexlist file */
									usbvm_setRecCurNode(endpt, nodePos);

									lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb+", WR_LOCK_WHOLE);
									if(lockRet !=-1)
									{
										if (usbvm_updRecIndexListInfoAfterR(endpt, &recordNode, fpIndex) < 0)
										{
											rc = ERROR_RECINFO_UPDATE;
											sipMsgModifyCMTimer(recordId, 100);				
											lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
											if(lockRet == -1)
											{
												MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
											}
											goto deInit;
										}
										lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
										if(lockRet == -1)
										{
											MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
										}
									}
									else
									{
										MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
									}
									nodeCount++; 
									nodePos++;
									/*读取下一条留言并播放*/
									if (nodeCount < nodeNum)
									{
										sleepMsWithComfortNoise(2000, &sleepContext);
										//sleep(2);
										//ts += 2000 * 8;
										/*
										ssrc = rand();
										ts = 0;
										seq = 0;
										*/
										currentPos = 0;
										lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb+", WR_LOCK_WHOLE);
										if(lockRet!=-1)
										{
											nodePos = usbvm_schNotReadNodeFromCurPos(endpt, nodePos, fpIndex);
											if ((nodePos > 0) && ((usbvm_getRecIndexListNodeInfoAndFd(endpt, 
													nodePos, &recordNode, 
													(FILE **)&g_usbvm_recordHandle[recordId].outputFp, fpIndex)) == 0))
											{
												usbvm_getVoiceNotifyDe(
													(char **)&g_usbvm_recordHandle[recordId].pVoiceNotifyBuf,
													(unsigned int *)&g_usbvm_recordHandle[recordId].fileSize);
											}
											else
											{
												/* Notify cm to Change endpoint timer */
												sipMsgModifyCMTimer(recordId, 100);
												usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
												lockRet = usbvm_closeIndexFpAndUnlock(endpt, 
														fpIndex, WR_LOCK_WHOLE);	
												if(lockRet == -1)
												{
													MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
												}
												goto exit;
											}
											lockRet = usbvm_closeIndexFpAndUnlock(endpt, 
													fpIndex, WR_LOCK_WHOLE);
											if(lockRet == -1)
											{
												MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
											}
										}
										else
										{
											MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
										}
									}
									else
									{
										cmsip_send_systemlog(CMSIP_SYSTEMLOG_DEBUG, 
											"USB Voice MailBox-- Voice Remote Play Finished!\n");
										/*没有下一条留言则超时退出*/
										/* Notify cm to Change endpoint timer */
										sipMsgModifyCMTimer(recordId, 100);
										usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
										goto exit;
									}
								}
							}
							break;
							
						default:
							break;
						}

					}
				}
			}

			/* Send CN rtp packet to remote endpoint while recording */
			else if (statusRead== REMOTE_RECORD)
			{
				len = 1;
				/* Form the RTP packet and send to the remote endpoint module */
				rtpPacket.RTPheader.v = 2;
				rtpPacket.RTPheader.p = 0;
				rtpPacket.RTPheader.x = 0;
				rtpPacket.RTPheader.cc = 0;
				if(marker == TRUE)
				{	/* using marker in the first CNG  */
					rtpPacket.RTPheader.m = 1;
					marker = FALSE;
					/* rebuild  timestamp */
			//		seq = rand();
			//		ts = rand();
					
/* disabled by yuanjp
   if change SSRC, some ISP's UAs will drop rtp packet 3 seconds
 */
#if 0	
					ssrc = rand();
#endif
				}
				else
				{
					rtpPacket.RTPheader.m = 0;
				}
				if (g_usbvm_recordHandle[recordId].cngSend)
				{
					/* using old to make it more compatible. */
					rtpPacket.RTPheader.pt = CNG_PAYLOAD_TYPE_OLD; 
				}
				else
				{
					rtpPacket.RTPheader.pt = g_usbvm_recordHandle[recordId].payloadType;
				}

				rtpPacket.RTPheader.seq = htons(seq);
				rtpPacket.RTPheader.ts = htonl(ts);
				rtpPacket.RTPheader.ssrc = htonl(ssrc); 

				ts += TX_pktSize/TX_pktPeriod * CNG_TIMEVAL;
				seq++;

				if (g_usbvm_recordHandle[recordId].cngSend)
				{	/* 78 is refered to other CN packet */
					memset(rtpPacket.RTPdata,0,MAX_USBVM_RTP_LENGTH);
					rtpPacket.RTPdata[0] = (char)CNG_LEVEL;   
				}
				/*
				else if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMU)
				{
					memset(rtpPacket.RTPdata,PCMU_MUTE_DATA,TX_pktSize);
				}
				else if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMA)
				{
					memset(rtpPacket.RTPdata,PCMA_MUTE_DATA,TX_pktSize);
				}
				*/
				cngTimeVal = CNG_TIMEVAL;
				/*
				if(g_usbvm_recordHandle[recordId].cngSend==FALSE)
				{
					cngTimeVal = TX_pktPeriod;
				}
				*/
				/* Packet interval is pktPeriod */
#ifdef INCLUDE_USBVM_MODULE
				if (0 == handle)
				{
					handle = open(_VOIP_DEV_NAME, O_RDWR);
				}
				if (handle > 0)
				{
					data = (cngTimeVal << 16) | endpt;
					ioctl(handle, _VOIP_TIMER_START, &data);
				}
				else
#endif /* INCLUDE_USBVM_MODULE */
				{
					tv.tv_sec = 0;
					tv.tv_usec = cngTimeVal * 1000;
					select(0, NULL, NULL, NULL, &tv);
				}
				/* Check if session is being closed */
				PJSUA_LOCK();
				if (!chkRtpStreamReady(call))
				{				
					PJSUA_UNLOCK();
					continue;
				}
				/* Send rtp packet */
				pjmedia_transport_send_rtp(call->session->stream[0]->transport, (pjmedia_rtp_hdr *)&rtpPacket,
				len + sizeof(pjmedia_rtp_hdr));

				PJSUA_UNLOCK();
			}
		}
		else
		{
			goto deInit;
		}
	}

deInit:

	/*如果是里面goto过来的，执行它。如果不是，说明是退出，则先不执行，由其他线程在使用完全局变量后执行*/
	if(gotoDeinit!=0)
	{
		MARK_PRINT("HL_DBG: gotoDeinit==%d ,%s:%d\n",gotoDeinit,__FUNCTION__,__LINE__);
		gotoDeinit = 0;
		usbvm_closeRecFile(recordId);
	}
	/*如果录音和播放提示音过程中，不按#号，而是直接挂机，则应当在线程退出时清空dtmf队列，不然下次拨打会出错*/
	MARK_PRINT("HL_DBG: reset FIFO! %s:%d\n",__FUNCTION__,__LINE__);
	dtmf_FIFOReset(g_usbvm_recordHandle[recordId].cnxId);

exit:
	/*无论是否goto，都执行以下代码*/
#ifdef INCLUDE_USBVM_MODULE
	if (handle > 0)
	{
		ioctl(handle, _VOIP_TIMER_STOP, &endpt);
		close(handle);
		handle = 0;
	}
#endif /* INCLUDE_USBVM_MODULE */
	if (g_usbvm_recordHandle[recordId].pVoiceNotifyBuf != NULL)
	{
		g_usbvm_recordHandle[recordId].pVoiceNotifyBuf = NULL;
	}
	/* send msg to tell cm that the remote status has changed  */
	if((g_usbvm_recordHandle[recordId].rcvRtpThreadRunning== PJ_FALSE)
			&&(g_usbvm_recordHandle[recordId].recordThreadRunning== PJ_FALSE))
	{
		usbvm_closeRecFile(recordId);
		pthread_mutex_lock(&g_usbvm_sendMsgLock[recordId]);
		if(l_exitMsgSent[recordId]==FALSE)
		{
			MARK_PRINT("HL_DEBUG:send CMSIP_USBVM_REMOTE_STATE_EXITED msg to cm!\n");
			if((g_usbvm_recordHandle[recordId].globalWav==1)&&(g_usbvm_recordHandle[recordId].globalTimeName>0))
			{
				MARK_PRINT("HL_DBG: globalTimeName=(%08x)!  [%s:%d]\n",
				g_usbvm_recordHandle[recordId].globalTimeName,__FUNCTION__,__LINE__);
				sipMsgRemoteStatusChange(endpt, CMSIP_USBVM_REMOTE_STATE_EXITED,
						g_usbvm_recordHandle[recordId].globalTimeName);
				g_usbvm_recordHandle[recordId].globalTimeName = -1;
			}
			else
			{
				sipMsgRemoteStatusChange(endpt, CMSIP_USBVM_REMOTE_STATE_EXITED, -1);
			}
			l_exitMsgSent[recordId]=TRUE;
		}
		pthread_mutex_unlock(&g_usbvm_sendMsgLock[recordId]);
	}
	usbvm_remoteStatusLockRead(&statusRead, recordId);
	if(statusRead!=REMOTE_STOP)
	{	/* 如果是强制退出，要用于判断，不需要重置为IDLE,在后面再重置  */
		usbvm_remoteStatusLockWrite(REMOTE_IDLE, recordId);
	}
	g_usbvm_recordHandle[recordId].playThreadRunning = PJ_FALSE;
	if(l_exitMsgSent[recordId]==TRUE)
	{	/* if exit msg is sent, destroy lock here.*/
		if(pthread_mutex_destroy(&g_usbvm_statusRwLock[recordId])!=0)
		{
			MARK_PRINT("can't destroy g_usbvm_statusRwLock[%d], [%s:%d]",recordId,__FUNCTION__,__LINE__);
		}
	}
#if 0
	g_usbvm_remoteFileInUse[recordId] = FALSE;
#endif
	MARK_PRINT("thread %s,recordId:%d, endpt:%d, exit value:%d\n", 
			__FUNCTION__, recordId,endpt,rc);
}
/* 
 * fn		static void remoteRcvRtpThread(void* arg)
 * brief	Use a thread to get pkts that sip has recieved.Read from the fifolist.
 * details	threads are identified by recordId.Every recordId has a fifolist and a remoteRcvRtpThread.
 *			So the sip will not wait when too many ptks arrived and usbvm's operation is too slow.
 * param[in]	void* arg : recordId's value.Should be a value, not a real pointer.
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		exit code is 4
 */
static void remoteRcvRtpThread(void* arg)
{
	int recordId =(int)arg;
	unsigned short dtmfNum;
/*20130618_update*/
	unsigned short dtmfStartNum;
	CMSIP_DTMF_SEND_TYPE dtmfType;
	
	struct timespec tspec;
	struct timeval tt;
		MARK_PRINT("[%s:%d]enter! pid=[%d]\n",
			__FUNCTION__,__LINE__,getpid());
	int ret = -1;
	int duration =0;
	REMOTE_STAS statusRead;
	g_usbvm_recordHandle[recordId].rcvRtpThreadRunning = PJ_TRUE;
	while (1)
	{
		usbvm_remoteStatusLockRead(&statusRead, recordId);
		if ((statusRead== REMOTE_STOP) || (statusRead== REMOTE_IDLE))
		{
			goto cleanup;
		}
		gettimeofday(&tt,NULL); 
		tspec.tv_sec = tt.tv_sec;
		tspec.tv_nsec = tt.tv_usec * 1000 + WAIT_FOR_RTP_FIFO_TIMEOUT * 1000 * 1000; 
		tspec.tv_sec += tspec.tv_nsec/(1000 * 1000 *1000);
		tspec.tv_nsec %= (1000 * 1000 *1000);

		if (sem_timedwait(&g_usbvm_recordHandle[recordId].semRtpRcvForUsbvm, &tspec) != -1)
		{
			if( g_usbvm_recordHandle[recordId].rtpPacketBuf!=NULL)
			{
				ret = usbvm_rtpPktFifoGet(recordId, g_usbvm_recordHandle[recordId].rtpPacketBuf, 
								&g_usbvm_recordHandle[recordId].rtpPacketBufLen);
				if(ret==0)
				{
					if (g_usbvm_recordHandle[recordId].bNoAnswer)
					{  
/*20130618_update*/
						/*<< BosaZhong@09Jun2013, add, skip dtmf # when recording. */
						dtmfNum = -1;
						dtmfStartNum = -1;
						dtmfType = CMSIP_DTMF_INBAND;
						/*>> endof BosaZhong@09Jun2013, add, skip dtmf # when recording. */
						
						duration = DR_parseDTMFRtpPacket(g_usbvm_recordHandle[recordId].cnxId, 
							(pjmedia_rtp_hdr *)g_usbvm_recordHandle[recordId].rtpPacketBuf,
										g_usbvm_recordHandle[recordId].rtpPacketBufLen, 
										&dtmfNum,
										&dtmfStartNum,
										&dtmfType);
						if(duration!=-1)
						{
							g_usbvm_recordHandle[recordId].dtmfTimeDuration = duration;
						}
						/*20130618_update*/
						/*<< BosaZhong@09Jun2013, modify, skip dtmf # when recording. */
						#if 0
						usbvm_remoteRtpRcv(recordId, 
										g_usbvm_recordHandle[recordId].rtpPacketBuf, 
										g_usbvm_recordHandle[recordId].rtpPacketBufLen);
						#else
						usbvm_remoteRecord_setGotDtmfHashInRecording(recordId, dtmfNum,
									dtmfStartNum, dtmfType);
						
						if (usbvm_remoteRecord_getGotDtmfHashInRecording(recordId) < 0)
						{		
							/* has not got dtmf #, so save packet. */
							usbvm_remoteRtpRcv(recordId, 
										g_usbvm_recordHandle[recordId].rtpPacketBuf, 
										g_usbvm_recordHandle[recordId].rtpPacketBufLen);
						}
						#endif /* 0 */
						/*>> endof BosaZhong@09Jun2013, modify, skip dtmf # when recording. */
					}
				}
				else
				{
					MARK_PRINT("usbvm_rtpPktFifoGet failed! ret=%d,[%s:%d]\n",ret,__FUNCTION__,__LINE__);
				}
			}
		}
		
	}
	cleanup:
		MARK_PRINT("%s exit\n", __FUNCTION__);
		g_usbvm_recordHandle[recordId].rcvRtpThreadRunning =PJ_FALSE;
}

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
BOOL usbvm_getPayloadTypeByCodec( CODEC_TYPE eptCodec, unsigned int *rtpPayLoadType )
{
	int  i = 0;
	BOOL formatMatch = FALSE;

	/* Search the codecFormatTable to find eptCodec and get its attributes */
	while (codecAttributeTable[i].eptCodec != CODEC_NULL)
	{
		if (codecAttributeTable[i].eptCodec == eptCodec)
		{
			*rtpPayLoadType = codecAttributeTable[i].rtpPayloadType;
			formatMatch = TRUE;
			break;
		}
		i++;
	}
	if (!formatMatch)
	{
		*rtpPayLoadType = 0xff;
	}
	return (formatMatch);
}

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
BOOL usbvm_forceNegToG711( pjsip_inv_session *inv )
{
	int rtpPayloadType = 0;
	NEG_STAS negState;
	
	negState = getRemoteSdpOfferCodec(inv, &rtpPayloadType);
	if ((negState == NEG_ERROR) || (negState == NEG_REMOTE_SDP_NOTSUPPORT_G711))
	{
		return FALSE;
	}
	modifyLocalSdpAnswer(inv, rtpPayloadType, negState);

	return TRUE;
}

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
 * note		yuchuwei modify @ 2012-4-4
 */
BOOL usbvm_enterRemoteUSBvoiceMail( CMSIP_USBVM_ENTER *pUsbVmEnter)
{	
	int endpt= pUsbVmEnter->endpt;
	int recordId=pUsbVmEnter->recordId;
	int cid = pUsbVmEnter->callIndex;
	UINT32 recDuration = pUsbVmEnter->duration;
	UINT8 globalWav = pUsbVmEnter->globalWav;
	pjsua_call* call = NULL;

	pj_assert(cid >= 0 && cid < PJSUA_MAX_CALLS);
	MARK_PRINT("entering %s:endpt:%d, cid:%d, recordId:%d\n", __FUNCTION__, endpt, cid, recordId);
	
	g_usbvm_recordHandle[recordId].remoteRecDuration = recDuration;
	g_usbvm_recordHandle[recordId].globalWav = globalWav;

	/*ycw-usbvm*/
	PJSUA_LOCK();
	call = &pjsua_var.calls[cid];
	if ( !(call->inv && call->inv->neg && call->inv->dlg && call->session)
			|| call->inv->state != PJSIP_INV_STATE_CONFIRMED 
			|| call->inv->neg->state != PJMEDIA_SDP_NEG_STATE_DONE)
	{
		PJSUA_UNLOCK();
		return PJ_FALSE;
	}
	getSrcDstDisplayName(call, g_usbvm_recordHandle[recordId].srcNum, g_usbvm_recordHandle[recordId].dstNum);
	getLocalSdpPtime(call, &g_usbvm_recordHandle[recordId].localPtime);
	getRemoteSdpOfferPtime(call, &g_usbvm_recordHandle[recordId].remotePtime);
	PJSUA_UNLOCK();
	
	MARK_PRINT("cid=%d,handle.cid=%d,recordId=%d,localtime=%d, remoteptime=%d ,payloadType=%d\n",
		cid,g_usbvm_recordHandle[recordId].cnxId,recordId,g_usbvm_recordHandle[recordId].localPtime,
		g_usbvm_recordHandle[recordId].remotePtime, g_usbvm_recordHandle[recordId].payloadType);

	g_usbvm_recordHandle[recordId].payloadType = g_DR_payloadType[cid].SdpPayloadType;

	g_usbvm_started[cid]=TRUE;		 /* 当检测dtmf时判断是否启动了usbvm  */
	l_exitMsgSent[recordId]=FALSE;
	/* 刚启动时置为true，录音时置为false，录音保存的函数一运行就置为true */
	g_usbvm_recordHandle[recordId].recordStop = TRUE;	
	usbvm_recFileMangInit();

	if(pthread_mutex_init(&g_usbvm_statusRwLock[recordId],NULL)!=0)
	{
		MARK_PRINT("can't init g_usbvm_statusRwLock[%d], [%s:%d]",recordId,__FUNCTION__,__LINE__);
	}
	if(pthread_mutex_init(&g_usbvm_sendMsgLock[recordId],NULL)!=0)
	{
		MARK_PRINT("can't init g_usbvm_sendMsgLock[%d], [%s:%d]",recordId,__FUNCTION__,__LINE__);
	}
	usbvm_remotePlayThreadCreate(recordId);

	g_dtmf_receiver_switch[cid] = DTMF_RECEIVER_ON;

	g_usbvm_recordHandle[recordId].rtpPacketBuf = g_rtpPacketBuf[recordId];
	g_usbvm_recordHandle[recordId].rtpPacketBufLen = 0;
	sem_init((sem_t*)&g_usbvm_recordHandle[recordId].semRtpRcvForUsbvm,0,0);
	usbvm_rtpPktFifoInit();
	usbvm_remoteRcvRtpThreadCreate(recordId);
	pthread_mutex_lock(&g_usbvm_sendMsgLock[recordId]);
	if(l_exitMsgSent[recordId]!=TRUE)
	{	
/* if remotePlayThread got trouble and has already send an EXIT msg to cm, do not send BUSY. */
	sipMsgRemoteStatusChange(endpt, CMSIP_USBVM_REMOTE_STATE_BUSY, -1);
	}
	pthread_mutex_unlock(&g_usbvm_sendMsgLock[recordId]);
	return PJ_TRUE;
}

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
void usbvm_exitRemoteUSBvoiceMail( int recordId )
{
	BOOL stopRet=0;
	int index;
	pj_status_t pj_status;
	REMOTE_STAS statusRead;
	MARK_PRINT("entering %s,endpt=%d,recordId=%d\n", __FUNCTION__,
	g_usbvm_recordHandle[recordId].endpt,recordId);

	sipMsgModifyCMTimer(recordId, 0);
	usbvm_remoteStatusLockRead(&statusRead, recordId);
	if(statusRead!=REMOTE_STOP)
	{
		MARK_PRINT("HL_DBG: g_usbvm_recordHandle[recordId].remoteStatus =%d %s:%d\n",
		g_usbvm_recordHandle[recordId].remoteStatus,__FUNCTION__,__LINE__);
		/* 挂机摘机都会进入这里 */
		if(g_usbvm_recordHandle[recordId].recordStatus != RECORD_IDLE)
		{	/* 如果正在录音，则停止 */
			stopRet = usbvm_remoteRecStop(recordId);
		}

		MARK_PRINT("HL_DEBUG:stopRet = %d, holdCall= %s\n",
		stopRet, g_usbvm_recordHandle[recordId].holdCall?"TRUE":"FALSE");
		if(statusRead!=REMOTE_IDLE)
		{	/* 不是stop也不是idle，置为stop使其退出 */
			usbvm_remoteStatusLockWrite(REMOTE_STOP, recordId);
		}
	}
	else
	{
		/* play线程状态是REMOTE_STOP退出，同时也要退出录音线程  */
		if(g_usbvm_recordHandle[recordId].recordStatus != RECORD_IDLE)
		{
			g_usbvm_recordHandle[recordId].recordStatus = RECORD_STOP;
		}
	}

	for(index=0;index<PJSUA_USBVM_MAX_THREADID;index++)
	{
		if(pjsua_var.usbvmThread[recordId][index])
		{
			pj_status = pj_thread_join(pjsua_var.usbvmThread[recordId][index]);
			if(pj_status == PJ_SUCCESS)
			{
				MARK_PRINT("HL_DBG:pjsua_var.usbvmThread[%d][%d] exited!!! [%s:%d]\n",
						recordId,index,__FUNCTION__,__LINE__);
			}
		}
	}
	usbvm_closeRecFile(recordId);

	sem_destroy((sem_t*)&g_usbvm_recordHandle[recordId].semRtpRcvForUsbvm);
	usbvm_rtpPktFifoReset(recordId);
	if((g_usbvm_recordHandle[recordId].playThreadRunning == PJ_FALSE)
		&&(g_usbvm_recordHandle[recordId].rcvRtpThreadRunning == PJ_FALSE)
		&&(g_usbvm_recordHandle[recordId].recordThreadRunning == PJ_FALSE))
	{
		/* 先判断是否已经发送，避免错误  */
		/* no need to lock g_usbvm_sendMsgLock, because there is no muti-threads now. */
		MARK_PRINT("HL_DEBUG:l_exitMsgSent[%d]==%d\n",	recordId,l_exitMsgSent[recordId]);
		if(l_exitMsgSent[recordId]==FALSE)
		{
			MARK_PRINT("HL_DEBUG:send CMSIP_USBVM_REMOTE_STATE_EXITED msg to cm!\n");
			if ((g_usbvm_recordHandle[recordId].globalWav == 1)
				&& (g_usbvm_recordHandle[recordId].globalTimeName > 0))
			{
				MARK_PRINT("HL_DBG: globalTimeName=(%08x)!  [%s:%d]\n",
					g_usbvm_recordHandle[recordId].globalTimeName,__FUNCTION__,__LINE__);
				sipMsgRemoteStatusChange(g_usbvm_recordHandle[recordId].endpt,
					CMSIP_USBVM_REMOTE_STATE_EXITED, g_usbvm_recordHandle[recordId].globalTimeName);
				g_usbvm_recordHandle[recordId].globalTimeName = -1;
			}
			else
			{
				sipMsgRemoteStatusChange(g_usbvm_recordHandle[recordId].endpt,
					CMSIP_USBVM_REMOTE_STATE_EXITED, -1);
			}
			l_exitMsgSent[recordId]=TRUE;

			if(pthread_mutex_destroy(&g_usbvm_statusRwLock[recordId])!=0)
			{
				MARK_PRINT("can't destroy g_usbvm_statusRwLock[%d], [%s:%d]",
					recordId,__FUNCTION__,__LINE__);
			}
			if(pthread_mutex_destroy(&g_usbvm_sendMsgLock[recordId])!=0)
			{
				MARK_PRINT("can't destroy g_usbvm_sendMsgLock[%d], [%s:%d]",
					recordId,__FUNCTION__,__LINE__);
			}
		}
	}

	usbvm_recFileMangDeInit();

	if (g_usbvm_recordHandle[recordId].cnxId!= -1)
	{
		DR_resetVar(g_usbvm_recordHandle[recordId].cnxId);
	}

	g_usbvm_started[g_usbvm_recordHandle[recordId].cnxId]=FALSE;		 
	g_usbvm_recordHandle[recordId].bNoAnswer = FALSE;
	if(g_usbvm_recordHandle[recordId].playThreadRunning == PJ_FALSE)
	{	
		g_usbvm_recordHandle[recordId].endpt = -1;
	}
	g_usbvm_recordHandle[recordId].cnxId = -1;
	g_usbvm_recordHandle[recordId].remoteStatus = REMOTE_IDLE;
}

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
void usbvm_remoteRecPlayInit( void )
{

	int status;
	int handle;
	for (handle = 0; handle < PJSUA_USBVM_MAX_RECORDID; handle++)
	{
		/* Initialize RECORD information */
		memset((void *)&g_usbvm_recordHandle[handle], 0, sizeof(USBVM_REMOTE_RECORD_HANDLE));
		g_usbvm_recordHandle[handle].outputFp = NULL;
		g_usbvm_recordHandle[handle].recordStatus = RECORD_IDLE;
		g_usbvm_recordHandle[handle].remoteStatus = REMOTE_IDLE;
		g_usbvm_recordHandle[handle].pRecordBuf = NULL;
		g_usbvm_recordHandle[handle].bNoAnswer = FALSE;
		g_usbvm_recordHandle[handle].cnxId = -1;
		g_usbvm_recordHandle[handle].endpt = -1;
		g_usbvm_recordHandle[handle].pVoiceNotifyBuf = NULL;

		status = pthread_mutex_init((pthread_mutex_t *)&g_usbvm_recordHandle[handle].mutexRecordStop, NULL );
		assert( status == 0 );
		
		status = pthread_mutex_init((pthread_mutex_t *)&g_usbvm_recordHandle[handle].mutexRtpReceive, NULL );
		assert( status == 0 );
	}

	dtmf_FIFOInit();

}

#if 0
/* 
 * fn		void usbvm_remoteRecPlayDeInit(int handle)
 * brief	Deinitialize RTP control block info
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
void usbvm_remoteRecPlayDeInit( void )
{

	int status;
	int i;
	for (i = 0; i < PJSUA_MAX_CALLS; i++)
	{
		/* Destory mutex */
		status = pthread_mutex_destroy((pthread_mutex_t *)&g_usbvm_recordHandle[i].mutexRecordStop);
		assert( status == 0 );

		status = pthread_mutex_destroy((pthread_mutex_t *)&g_usbvm_recordHandle[i].mutexRtpReceive);
		assert( status == 0 );
		
		/* Destory a rtp record thread */
		if (g_usbvm_recordHandle[i].recordThreadId != 0)
		{
			if (pthread_cancel(g_usbvm_recordHandle[i].recordThreadId) !=0 )
			{
				return;
			}
			pthread_testcancel(); /* Set cancellation point */
			pthread_join(g_usbvm_recordHandle[i].recordThreadId, NULL); /* Acknowledge the child's death */
		}
		
		/* Destory a rtp receive timeout process thread */
		if (g_usbvm_recordHandle[i].receiveThreadId != 0)
		{
			if (pthread_cancel(g_usbvm_recordHandle[i].receiveThreadId) !=0 )
			{
				return;
			}
			pthread_testcancel(); /* Set cancellation point */
			pthread_join(g_usbvm_recordHandle[i].receiveThreadId, NULL); /* Acknowledge the child's death */
		}

		/* Destory a rtp play thread */
		if (g_usbvm_recordHandle[i].playThreadId != 0)
		{
			if (pthread_cancel(g_usbvm_recordHandle[i].playThreadId) !=0 )
			{
				return;
			}
			pthread_testcancel(); /* Set cancellation point */
			pthread_join(g_usbvm_recordHandle[i].playThreadId, NULL); /* Acknowledge the child's death */
		}
	}
}
#endif

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
void usbvm_remotePlayThreadCreate( int recordId )
{
	/*
	** Spawn the task.
	*/
	int status;
	pthread_attr_t attributes;
	struct sched_param schedparams;

	status = pthread_attr_init( &attributes );
	pthread_attr_setinheritsched (&attributes, PTHREAD_EXPLICIT_SCHED);
	status = pthread_attr_setschedpolicy( &attributes, SCHED_FIFO );
	schedparams.sched_priority = PTHREAD_PRIO_FIFO_HIGH;
	status = pthread_attr_setschedparam( &attributes, &schedparams );
	status = pj_thread_create_with_attr(pjsua_var.pool, "remotePlay", (pthread_attr_t*)&attributes, 
		(pj_thread_proc *)remotePlayThread, (void *)recordId, 0, 0, &pjsua_var.usbvmThread[recordId][0]);
	
}
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
void usbvm_remoteRcvRtpThreadCreate(int recordId)
{
	int status;
	pthread_attr_t attributes;
	struct sched_param schedparams;

	status = pthread_attr_init( &attributes );
	pthread_attr_setinheritsched (&attributes, PTHREAD_EXPLICIT_SCHED);
	status = pthread_attr_setschedpolicy( &attributes, SCHED_FIFO );
	schedparams.sched_priority = PTHREAD_PRIO_FIFO_HIGH;
	status = pthread_attr_setschedparam( &attributes, &schedparams );
	status = pj_thread_create_with_attr(pjsua_var.pool, "remoteRcvRtp", (pthread_attr_t*)&attributes, 
		(pj_thread_proc *)remoteRcvRtpThread, (void *)recordId, 0, 0, &pjsua_var.usbvmThread[recordId][1]);
}
#if 0/* useless now */

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
int usbvm_checkRemoteRecPlayIsIDLE( void )
{
	int i;
	REMOTE_STAS statusRead;
	/* Get remote record and play status */
	for (i = 0; i < PJSUA_USBVM_MAX_RECORDID; i++)
	{
		usbvm_remoteStatusLockRead(&statusRead, i);
		if ((statusRead != REMOTE_IDLE)
			 || (statusRead != RECORD_IDLE))
		{
			break;
		}
	}
	if (i != PJSUA_USBVM_MAX_RECORDID)
	{
		return -1;
	}

	return 0;
}
#endif

#if 0
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
int usbvm_checkRemoteRecFileIsIdle(void)
{
	int index;
	for (index = 0;index<PJSUA_USBVM_MAX_RECORDID;index++)
	{
		if(g_usbvm_remoteFileInUse[index])
		{
			break;
		}
	}
	if (index != PJSUA_USBVM_MAX_RECORDID)
	{
		return 1;
	}
	return 0;
}
#endif

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

