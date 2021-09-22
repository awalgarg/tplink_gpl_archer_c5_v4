/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_remoteRecord.c
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		19Jul11
 *
 * history 	\arg	
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>     /* getpid */
#include <sys/types.h>  /* getpid */
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <arpa/inet.h>  /* htonl ntohl */
#include <pthread.h>
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>

#include <usbvm_remoteRecord.h>	 
#include <usbvm_recFileMang.h>
#include <usbvm_voiceNotify.h>
#include <usbvm_speechEncDec.h>

#include <cmsip_msgdef.h>
#include <cmsip_transport.h>

#ifdef INCLUDE_MAIL
#include "mail.h"
#include <os_lib.h>
#include <os_msg.h>
#endif /* INCLUDE_MAIL */
 
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#if(defined(USBVM_DEBUG)&&(USBVM_DEBUG==1))
#define  MARK_PRINT(args...) {printf("============USBVM RECORD %d============", __LINE__);printf(args);}
#else
#define  MARK_PRINT(args...)
#endif

#define REC_TASK_WAKEUP_MS 2000                   /* 2s */
#define REC_TASK_STACK     1024
#define REC_BUFFER_SIZE    1600                   /* 200ms * 8Bytes/ms */
#define RTP_RCV_TIMEOUT    (REC_BUFFER_SIZE / 8)  /* 200ms */

#define REC_SIZE_HALF_SEC  0						/* 4000 */

#define DTMF_FIFO_SIZE       32

#define RTP_RCV_FIFO_LEN 200	
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
/* 
 * brief	RFC2833 DTMF payload
 */
typedef struct
{
	unsigned char  event; 
	unsigned char  volume;
	unsigned short duration;
} NTE_PAYLOAD;

/* 
 * brief	DTMF FIFO state
 */
typedef enum
{
	DTMF_FIFO_EMPTY,  /* FIFO is empty */
	DTMF_FIFO_DATA,   /* FIFO is available */
	DTMF_FIFO_FULL    /* FIFO is full */
}DTMF_FIFO_STATE;

/* 
 * brief	RTP recieve fifo list status
 */
typedef enum
{
	RTP_FIFO_EMPTY = 0,
	RTP_FIFO_DATA = 1,
	RTP_FIFO_FULL = 2
}RTP_FIFO_STATUS;
/* 
 * brief	USBVM_RTP_FIFO_NODE
 */
typedef struct _USBVM_RTP_FIFO_NODE
{
	char rtpPktBuf[USBVM_RTP_PKT_SIZE];
	int rtpPktLen;
}USBVM_RTP_FIFO_NODE;
/* 
 * brief	DTMF number map with event 
 */
typedef struct DTMFMAP
{
	int id;  /* DTMF number */
	int evt; /* DTMF event */
} DTMFMAP;

/* 
 * brief	Sound format
 */
typedef struct
{
   int bytesPerFrame;       /* size of a base frame */
   int msecPerFrame;        /* duration in ms of a base frame */
   int samplesPerSec;       /* smapling frequency, 8k for narrow band codec, and 16k for wide band codec */
} SOUNDFORMAT;

/* 
 * brief	Codec attributes of encoder and decoder
 */
typedef struct
{	
   SOUNDFORMAT soundFormat;  /* sound format */
   int frameSizeToDecoder;   /* frame size to decoder */
   int rtpPayloadType;       /* rtp payload type */
   CODEC_TYPE eptCodec;      /* codec type */
} CODEC_ATTRIBUTE;

/* 
 * brief	This table is used to define format attributes of a supported codec
 */
static const CODEC_ATTRIBUTE codecAttributeTable[] =
{
   { {24, 30, 8000}, 240, RTP_PAYLOAD_G7231_63,  CODEC_G7231_63 },
   { {10, 5,  8000}, 100, RTP_PAYLOAD_G726_16, CODEC_G726_16  },
   { {15, 5,  8000}, 150, RTP_PAYLOAD_G726_24, CODEC_G726_24  },
   { {20, 5,  8000}, 200, RTP_PAYLOAD_G726_32,  CODEC_G726_32  },
   { {25, 5,  8000}, 250, RTP_PAYLOAD_G726_40, CODEC_G726_40  },
   { {10, 10, 8000}, 10,  RTP_PAYLOAD_G729A, CODEC_G729A    },
   { {40, 5,  8000}, 400, RTP_PAYLOAD_PCMU,  CODEC_PCMU     },
   { {40, 5,  8000}, 400, RTP_PAYLOAD_PCMA,  CODEC_PCMA     },
   { {0,  0,  0},    0,   RTP_PAYLOAD_NULL,  CODEC_NULL     }  /* This line is required to indicate the end of the list */

};


/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

static void remoteRecThreadCreate( int recordId );
#ifdef INCLUDE_MAIL
static int remoteRecPush( USBVM_REC_FILE_NODE *pRec );
#endif /* INCLUDE_MAIL */
/* 
static pthread_t remoteRcvThreadCreate( int handle );
static void remoteRcvThread( void *context );
*/
static void remoteRecThread( void *context );

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
static char l_recFilePathName[ENDPT_NUM][200];
static int l_fifoBufHead[PJSUA_MAX_CALLS];
static int l_fifoBufTail[PJSUA_MAX_CALLS];
static int l_fifoBufState[PJSUA_MAX_CALLS];
static unsigned short l_dtmfFIFOBuf[PJSUA_MAX_CALLS][DTMF_FIFO_SIZE];
	 
static UINT8 *l_recordBuf1[PJSUA_USBVM_MAX_RECORDID];
static UINT8 *l_recordBuf2[PJSUA_USBVM_MAX_RECORDID]; 
static UINT8 *l_pRecBuf[PJSUA_USBVM_MAX_RECORDID];
static UINT32 l_recBufCount[PJSUA_USBVM_MAX_RECORDID];
static UINT32 l_timeStampNew[PJSUA_USBVM_MAX_RECORDID];
static UINT32 l_timeStampOld[PJSUA_USBVM_MAX_RECORDID];
static UINT32 l_rtpLenOld[PJSUA_USBVM_MAX_RECORDID];
static UINT32 l_rtpMutePaddingLen[PJSUA_USBVM_MAX_RECORDID];
static BOOL l_bFirstPacket[PJSUA_USBVM_MAX_RECORDID];
static char l_rtpMuteData[PJSUA_USBVM_MAX_RECORDID][REC_BUFFER_SIZE * 3];

static int l_rtpPktFifoHead[PJSUA_USBVM_MAX_RECORDID];
static int l_rtpPktFifoTail[PJSUA_USBVM_MAX_RECORDID];
static int l_rtpPktFifoStatus[PJSUA_USBVM_MAX_RECORDID];
static USBVM_RTP_FIFO_NODE l_rtpPktFifoList[PJSUA_USBVM_MAX_RECORDID][RTP_RCV_FIFO_LEN];

/* 
unsigned int g_sdpPayloadType[MAX_CALLS];
unsigned int g_sdpTelEvtPayload[MAX_CALLS];
*/
char g_rtpPacketBuf[PJSUA_USBVM_MAX_RECORDID][USBVM_RTP_PKT_SIZE];
USBVM_REMOTE_RECORD_HANDLE g_usbvm_recordHandle[PJSUA_USBVM_MAX_RECORDID];

pthread_mutex_t g_usbvm_statusRwLock[PJSUA_USBVM_MAX_RECORDID];
pthread_mutex_t g_usbvm_sendMsgLock[PJSUA_USBVM_MAX_RECORDID];
#if 0
BOOL g_usbvm_remoteFileInUse[PJSUA_USBVM_MAX_RECORDID];
#endif


const DTMFMAP TelEvtToneMap[] =
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
   { UNKNOWN, UNKNOWN } /* end of map */
};


/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/* 
 * fn		static pthread_t remoteRecThreadCreate( int handle )
 * brief	Create remote record task
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	task id
 * retval	
 *
 * note		
 */
static void remoteRecThreadCreate( int recordId )
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
	status = pj_thread_create_with_attr(pjsua_var.pool, "remoteRec", (pthread_attr_t*)&attributes, 
		(pj_thread_proc *)remoteRecThread, (void *)recordId, 0, 0, &pjsua_var.usbvmThread[recordId][2]);

	MARK_PRINT("HL_DBG: remoteRecThread create!![%s:%d]\n",__FUNCTION__,__LINE__);
}

#ifdef INCLUDE_MAIL
/* 
 * fn		static void remoteRecPush( int recordId )
 * brief	send a message to cos to push
 *
 * param[in]	an usb record file node
 */
static int remoteRecPush(USBVM_REC_FILE_NODE *pRec)
{
	MARK_PRINT("lichao debug usbvm remote rec push in\n");
	CMSG_FD msgFd;
	CMSG_BUFF msg;
	
	memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msg, 0 , sizeof(CMSG_BUFF));

	msg.type = MAIL_ACTION_SEND_MAIL;

	/* make sure that sizeof(msg.content) >= sizeof(*pRec) ! */
	memcpy(msg.content, pRec ,sizeof(*pRec));

	msg.priv = MAIL_USBVM;

	if (msg_connCliAndSend(CMSG_ID_MAIL, &msgFd, &msg) != 0)
	{
		return -1;
	}
	return 0;
}
#endif /* INCLUDE_MAIL */

/* 
 * fn		static void remoteRecThread( void *context )
 * brief	Remote record task
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
static void remoteRecThread( void *context )
{
	int recordId = (int)context;
	int status = 0;
	struct timespec tspec;
	struct timeval tt;

	MARK_PRINT("entering %s pid(%d)\n", __FUNCTION__, getpid());

	g_usbvm_recordHandle[recordId].recordThreadRunning = PJ_TRUE;
	while ( sem_wait((sem_t *)&g_usbvm_recordHandle[recordId].semRecord) );
	while (1)
	{
		if ((g_usbvm_recordHandle[recordId].recordStatus == RECORD_STOP)
			|| (g_usbvm_recordHandle[recordId].recordStatus == RECORD_IDLE)
			|| g_usbvm_recordHandle[recordId].playThreadRunning == PJ_FALSE)
		{
			status = 0;
			goto cleanup;
		}
		else
		{
			g_usbvm_recordHandle[recordId].recordStatus = RECORD_PROCESS;
		}

		gettimeofday(&tt,NULL); 
		tspec.tv_sec = tt.tv_sec + REC_TASK_WAKEUP_MS/1000;
		tspec.tv_nsec = tt.tv_usec * 1000; 
		tspec.tv_sec += tspec.tv_nsec/(1000 * 1000 *1000);
		tspec.tv_nsec %= (1000 * 1000 *1000);
		/* Wait for rtp is available, timeout is 2s */
		if (sem_timedwait(&g_usbvm_recordHandle[recordId].semRtpAvailable, &tspec) == 0)
		{

			if ((g_usbvm_recordHandle[recordId].recordBufLen > 0) && (g_usbvm_recordHandle[recordId].pRecordBuf != NULL))
			{ 
				if(g_usbvm_recordHandle[recordId].outputFp == NULL)
				{	/* We must not close outputFp when this thread is running!!! Sometimes we 
					are going to exit this thread, but outputFp may be closed first out of this 
					thread, then we should break here. */		
						status = 2;
						break;
				}
				if (fwrite(g_usbvm_recordHandle[recordId].pRecordBuf, g_usbvm_recordHandle[recordId].recordBufLen,
					1, g_usbvm_recordHandle[recordId].outputFp) != 1)
				{
					MARK_PRINT("Error writing the recorded data to file descriptor\n");
					status = 1;
					usbvm_remoteStatusLockWrite(REMOTE_STOP, recordId);
					break;
				}				
				g_usbvm_recordHandle[recordId].fileSize += g_usbvm_recordHandle[recordId].recordBufLen;
				g_usbvm_recordHandle[recordId].recordBufLen = 0;
				g_usbvm_recordHandle[recordId].pRecordBuf = NULL;
			}

			sem_post((sem_t *)&g_usbvm_recordHandle[recordId].semRtpSaved);
		}
		else
		{
			MARK_PRINT("%s semRtpAvailable time out, recordStatus=%d\n",
				__FUNCTION__,g_usbvm_recordHandle[recordId].recordStatus);
		}
	}

	cleanup:
	/* exit the record thread */ 
#if 0
	/*Edited by huanglei, 2011/10/14*/
	g_usbvm_remoteFileInUse[recordId]=FALSE;
	/*end huanglei*/
#endif
	
	g_usbvm_recordHandle[recordId].recordStatus = RECORD_IDLE;
	g_usbvm_recordHandle[recordId].recordThreadRunning = PJ_FALSE;

	g_usbvm_recordHandle[recordId].pRecordBuf = NULL;

	MARK_PRINT("remoteRecThread exit:%d \n",status);
	return;
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/* 
 * fn		void dtmf_FIFOInit( void )
 * brief	Initialize the DTMF FIFO buffer
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
void dtmf_FIFOInit( void )
{
	int i = 0;

	/* initialize fifo */
	for (i = 0; i < PJSUA_MAX_CALLS; i++)
	{
		l_fifoBufHead[i] = 0;
		l_fifoBufTail[i] = 0;
		l_fifoBufState[i] = DTMF_FIFO_EMPTY;
	}

}

/* 
 * fn		void dtmf_FIFOPut( int cnxId, unsigned short dtmfNum )
 * brief	Put DTMF number into FIFO
 * details	
 *
 * param[in]	cnxId  the current connection id 
 * param[in]	dtmfNum  the dtmf put into FIFO
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void dtmf_FIFOPut( int cnxId, unsigned short dtmfNum ,CMSIP_DTMF_SEND_TYPE	 dtmfType)
{
	CMSIP_DETECT_REMOTE_DTMF *pCmdetectedDTMF = NULL;
	CMSIP_MSG cmSipMsg;
	
	if ((cnxId < 0) || (cnxId >= PJSUA_MAX_CALLS))
	{
		return ;
	}

		/* Notify cm that sip has detected a remote DTMF */
		cmSipMsg.type = CMSIP_REQUEST_DETECT_REMOTE_DTMF;
		cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
		pCmdetectedDTMF = (CMSIP_DETECT_REMOTE_DTMF*)cmSipMsg.body;
		pCmdetectedDTMF->callIndex = cnxId;
		pCmdetectedDTMF->dtmfNum = dtmfNum;
		pCmdetectedDTMF->type = dtmfType;
		cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
		MARK_PRINT("sip sending msg:CMSIP_REQUEST_DETECT_REMOTE_DTMF. callId:%d, dtmf:%d, type:%s\n", 
			cnxId, dtmfNum, (dtmfType==0)?"INBAND":((dtmfType==1)?"RFC2833":"SIPINFO"));
	if (l_fifoBufState[cnxId] != DTMF_FIFO_FULL)
	{
		l_dtmfFIFOBuf[cnxId][l_fifoBufTail[cnxId]] = dtmfNum;
		l_fifoBufState[cnxId] = DTMF_FIFO_DATA;

		l_fifoBufTail[cnxId]++;
		if (l_fifoBufTail[cnxId] == DTMF_FIFO_SIZE)
		{
			l_fifoBufTail[cnxId] = 0;
		}

		if (l_fifoBufTail[cnxId] == l_fifoBufHead[cnxId])
		{
			MARK_PRINT("HL_DBG:DTMF_FIFO_FULL %s:%d\n",__FUNCTION__,__LINE__);
			l_fifoBufState[cnxId] = DTMF_FIFO_FULL;
		}
	}
	else
	{
		MARK_PRINT("HL_DBG:DTMF_FIFO_FULL,can't put!!! %s:%d\n",__FUNCTION__,__LINE__);
	}
}

/* 
 * fn		BOOL dtmf_FIFOGet( int cnxId, unsigned short *dtmfNum )
 * brief	Get DTMF number from FIFO
 * details	
 *
 * param[in]	cnxId  the current connection id  
 * param[out]	*dtmfNum  pointer of DTMF number
 *
 * return	DTMF is available return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
BOOL dtmf_FIFOGet( int cnxId, unsigned short *dtmfNum )
{
	if ((cnxId < 0) || (cnxId >= PJSUA_MAX_CALLS))
	{
		return FALSE;
	}
	/* l_wavFIFOBuf[][] is not empty */
	if (l_fifoBufState[cnxId] != DTMF_FIFO_EMPTY)		
	{

		*dtmfNum = l_dtmfFIFOBuf[cnxId][l_fifoBufHead[cnxId]];
		l_fifoBufState[cnxId] = DTMF_FIFO_DATA;

		l_fifoBufHead[cnxId]++;
		if (l_fifoBufHead[cnxId] == DTMF_FIFO_SIZE)
		{
			l_fifoBufHead[cnxId] = 0;
		}

		if (l_fifoBufTail[cnxId] == l_fifoBufHead[cnxId])
		{
			l_fifoBufState[cnxId] = DTMF_FIFO_EMPTY;
		}

		return TRUE;
	}

	return FALSE;
}

/* 
 * fn		void dtmf_FIFOReset( int cnxId )
 * brief	Reset of DTMF FIFO buffer
 * details	
 *
 * param[in]	cnxId  the current connection id 
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void dtmf_FIFOReset( int cnxId )
{
	if ((cnxId < 0) || (cnxId >= PJSUA_MAX_CALLS))
	{
		return ;
	}
	l_fifoBufHead[cnxId] = 0;
	l_fifoBufTail[cnxId] = 0;
	l_fifoBufState[cnxId] = DTMF_FIFO_EMPTY;

}

/* 
 * fn		BOOL usbvm_matchRecordId( pjmedia_stream *stream, int *recordId )
 * brief	Match record id with stream
 * details	
 *
 * param[in]	stream  pointer of media stream
 * param[out]	recordId  record id
 *
 * return	match return TRUE, else return FALSE
 * retval	
 *
 * note		Need to add a variable callId in pjmedia_stream first
 */
BOOL usbvm_matchRecordId( pjmedia_stream *stream, int *recordId )
{

	BOOL bmatch = FALSE;
	int i;
	for (i = 0; i < PJSUA_USBVM_MAX_RECORDID; i++)
	{
		/* to be implemented */
		if ((g_usbvm_recordHandle[i].cnxId != -1)			
			&& (stream->callId == g_usbvm_recordHandle[i].cnxId))
		{
			*recordId = i;
			break;
		}
	}

	if (i < PJSUA_USBVM_MAX_RECORDID)
	{
		bmatch = TRUE;
	}

	return bmatch;

}

	
/* 
 * fn		void usbvm_remoteRtpCopy(int recordId, char *rtpPacket, int rtpPaketLen) 
 * brief	When a RTP is recieved, copy it to a fifolist that usbvm will read from it.
 * details	
 *
 * param[in]	int recordId: record index number.
 *				char *rtpPacket: a pointer to the rtp pkt buf.
 *				int rtpPaketLen: the length of the rtp pkt.
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_remoteRtpCopy(int recordId, char *rtpPacket, int rtpPacketLen)
{
	int ret =-1;
	int semRet;
	if(g_usbvm_recordHandle[recordId].playThreadRunning == PJ_TRUE)
	{
			if(rtpPacketLen>USBVM_RTP_PKT_SIZE)
			{
				/* if the buffer is not enough, cut the packet */
				MARK_PRINT("HL_DBG: rtpPktLen is %d,bigger than USBVM_RTP_PKT_SIZE(%d)[%s:%d]\n",
					rtpPacketLen,USBVM_RTP_PKT_SIZE,__FUNCTION__,__LINE__);
				rtpPacketLen = USBVM_RTP_PKT_SIZE;
			}
		ret = usbvm_rtpPktFifoPut(recordId, rtpPacket, rtpPacketLen);
		if(ret == 0)
		{
			semRet=sem_post((sem_t*)&g_usbvm_recordHandle[recordId].semRtpRcvForUsbvm);
			if(semRet!=0)
			{
				MARK_PRINT("sem_post: semRtpRcvForUsbvm failed!!! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
		}
		else
		{
			MARK_PRINT("HL_DBG: usbvm_rtpPktFifoPut() failed!!! ret = %d [%s:%d]\n",ret,__FUNCTION__,__LINE__);
		}
	
	}
}
/* 
 * fn		void usbvm_rtpPktFifoInit(void) 
 * brief	Initialize the fifolist of usbvm to save rtp pkts.
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
void usbvm_rtpPktFifoInit(void)
{
	int i,j;
	for(i=0;i<PJSUA_USBVM_MAX_RECORDID;i++)
	{
		l_rtpPktFifoHead[i] = 0;
		l_rtpPktFifoTail[i] = 0;
		l_rtpPktFifoStatus[i] = RTP_FIFO_EMPTY;
		for(j=0;j<RTP_RCV_FIFO_LEN;j++)
		{
			memset(l_rtpPktFifoList[i][j].rtpPktBuf,0,USBVM_RTP_PKT_SIZE);
			l_rtpPktFifoList[i][j].rtpPktLen = 0;
		}
	}
}
/* 
 * fn		int usbvm_rtpPktFifoPut(int recordId, char*rtpPktBuf,int rtpPktLen) 
 * brief	Add a pkt to the usbvm rtp fifolist, if list is not full.
 * details	
 *
 * param[in]	int recordId: record index number.
 *				char *rtpPktBuf: a pointer to the rtp pkt buf.
 *				int rtpPaketLen: the length of the rtp pkt.
 * param[out]	
 *
 * return	int 
 * retval	-1: recordId is wrong
 *			-2: can't copy, there is a NULL pointer
 *			-3: the list is full
 *			0: success
 * note		
 */
int usbvm_rtpPktFifoPut(int recordId, char*rtpPktBuf,int rtpPktLen)
{
	if((recordId < 0)||(recordId >= PJSUA_USBVM_MAX_RECORDID))
	{
		return -1;
	}
	if(l_rtpPktFifoStatus[recordId]!= RTP_FIFO_FULL)
	{
		if((l_rtpPktFifoList[recordId][l_rtpPktFifoTail[recordId]].rtpPktBuf != NULL)&&(rtpPktBuf!=NULL))
		{
			memcpy(l_rtpPktFifoList[recordId][l_rtpPktFifoTail[recordId]].rtpPktBuf,rtpPktBuf,rtpPktLen);
			l_rtpPktFifoList[recordId][l_rtpPktFifoTail[recordId]].rtpPktLen = rtpPktLen;
			l_rtpPktFifoStatus[recordId] = RTP_FIFO_DATA;
			l_rtpPktFifoTail[recordId]++;
			if (l_rtpPktFifoTail[recordId] == RTP_RCV_FIFO_LEN)
			{
				l_rtpPktFifoTail[recordId] = 0;
			}
#if 1	/* DEBUG for rtp fifo list's max length, to be deleted */
				int length = l_rtpPktFifoTail[recordId] - l_rtpPktFifoHead[recordId];
				length = (length>0)?length:(RTP_RCV_FIFO_LEN + length);
				if(length > 100)
				{
					MARK_PRINT("put FIFO length now is %d >100\n",length);
				}
#endif
			if (l_rtpPktFifoTail[recordId] == l_rtpPktFifoHead[recordId])
			{
				l_rtpPktFifoStatus[recordId] = RTP_FIFO_FULL;
			}
			return 0;
		}
		else
		{
			return -2;
		}
	}
	else
	{
MARK_PRINT("HL_DBG: usbvm_rtpPktFifoPut failed, List is FULL!!![%s:%d]\n",__FUNCTION__,__LINE__);
		return -3;
	}
}
/* 
 * fn		int usbvm_rtpPktFifoGet(int recordId, char* rtpPktBuf, int* rtpPktLen) 
 * brief	Get a pkt from fifolist, if the list is not empty.
 * details	
 *
 * param[in]	int recordId: record index number.	
 * param[out]	char *rtpPktBuf: a pointer to the rtp pkt buf.	
 *				int* rtpPaketLen: a pointer to the length.
 *
 * return	int
 * retval	-1: recordId is wrong
 *			-2: can't copy, rtpPktBuf is a NULL pointer
 *			-3: the list is empty
 *			-4: the rtpPktLen is a NULL pointer
 *			0: success
 *
 * note		
 */
int usbvm_rtpPktFifoGet(int recordId, char* rtpPktBuf, int* rtpPktLen)
{
	if((recordId < 0)||(recordId >= PJSUA_USBVM_MAX_RECORDID))
	{
		return -1;
	}

	if(l_rtpPktFifoStatus[recordId]!=RTP_FIFO_EMPTY)
	{
		if(rtpPktBuf!=NULL)
		{
			memcpy(rtpPktBuf, 
				l_rtpPktFifoList[recordId][l_rtpPktFifoHead[recordId]].rtpPktBuf,
				l_rtpPktFifoList[recordId][l_rtpPktFifoHead[recordId]].rtpPktLen);
		
			if(rtpPktLen!=NULL)
			{
				*rtpPktLen = l_rtpPktFifoList[recordId][l_rtpPktFifoHead[recordId]].rtpPktLen;
			}
			else
			{
				return -4;
			}
/* 
			memset(l_rtpPktFifoList[recordId][l_rtpPktFifoHead[recordId]].rtpPktBuf,0,USBVM_RTP_PKT_SIZE);
*/
			l_rtpPktFifoList[recordId][l_rtpPktFifoHead[recordId]].rtpPktLen = 0;
			l_rtpPktFifoStatus[recordId] = RTP_FIFO_DATA;
			l_rtpPktFifoHead[recordId]++;
			if(l_rtpPktFifoHead[recordId] == RTP_RCV_FIFO_LEN)
			{
				l_rtpPktFifoHead[recordId] = 0;
			}
			if(l_rtpPktFifoHead[recordId] == l_rtpPktFifoTail[recordId])
			{
				l_rtpPktFifoStatus[recordId] = RTP_FIFO_EMPTY;
			}
			return 0;
		}
		else
		{
			return -2;
		}
	}
	else
	{
		return -3;
	}
}
/* 
 * fn		void usbvm_rtpPktFifoReset(int recordId) 
 * brief	Reset the fifolist to default value, when it's not used any more.
 * details	
 *
 * param[in]	int recordId: record index number.	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_rtpPktFifoReset(int recordId)
{
	int i;
	if((recordId < 0)||(recordId >= PJSUA_USBVM_MAX_RECORDID))
	{
		return;
	}
	for(i=0;i<RTP_RCV_FIFO_LEN;i++)
	{
		if(l_rtpPktFifoList[recordId][i].rtpPktLen != 0)
		{
			memset(l_rtpPktFifoList[recordId][i].rtpPktBuf,0,USBVM_RTP_PKT_SIZE);
			l_rtpPktFifoList[recordId][i].rtpPktLen = 0;
		}
		
	}
	l_rtpPktFifoHead[recordId] = 0;
	l_rtpPktFifoTail[recordId] =0;
	l_rtpPktFifoStatus[recordId] = RTP_FIFO_EMPTY;
}
/* 
 * fn		void usbvm_remoteRtpRcv( int recordId, char *rtpPacket, int rtpPaketLen )
 * brief	Receive remote rtp packet and put into the buffer
 * details	
 *
 * param[in]	recordId  record id
 * param[in]	rtpPacket  pointer of rtp packet received
 * param[in]	rtpPaketLen  rtp packet length
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_remoteRtpRcv( int recordId, char *rtpPacket, int rtpPaketLen )
{
	pjmedia_rtp_hdr *rtpHeader;
	struct timespec tspec;
	struct timeval tt;

	int tsDifference=0;
	static int diffPtimeCount[PJSUA_USBVM_MAX_RECORDID];
	static int tsDtmfAdd[PJSUA_USBVM_MAX_RECORDID];

	unsigned short wExtensionByte = 0;	/* extension octet				   */
	unsigned short wCSRCByte = 0;		/* CSRC octet					   */
	char *pPayload;
	int payloadLen = 0;		
	if ((recordId < 0) || (recordId >= PJSUA_USBVM_MAX_RECORDID))
	{
		return ;
	}
	if (g_usbvm_recordHandle[recordId].bNoAnswer)
	{  
		if ((g_usbvm_recordHandle[recordId].recordStatus == RECORD_START)
			|| (g_usbvm_recordHandle[recordId].recordStatus == RECORD_PROCESS)) 
		{
			if (pthread_mutex_lock((pthread_mutex_t *)&g_usbvm_recordHandle[recordId].mutexRtpReceive) == 0)
			{
				rtpHeader = (pjmedia_rtp_hdr *)rtpPacket;
				if (l_bFirstPacket[recordId])
				{
					l_timeStampOld[recordId] = ntohl(rtpHeader->ts);

					if (rtpHeader->pt == CNG_PAYLOAD_TYPE || rtpHeader->pt == CNG_PAYLOAD_TYPE_OLD)
					{
						l_rtpLenOld[recordId] = 0;
					}
					else
					{
						l_rtpLenOld[recordId] = rtpPaketLen - sizeof(pjmedia_rtp_hdr);
					}
					l_bFirstPacket[recordId] = FALSE;
				}
				l_timeStampNew[recordId] = ntohl(rtpHeader->ts);

				#if 0
				/* Added by huanglei for debug print, 2011/09/21 */
				MARK_PRINT("HL_DBG: l_timeStampNew=%u,l_timeStampOld[recordId]=%u,rtpHeader->ts=%x,rtpHeader->pt=%d\n",
				l_timeStampNew[recordId],l_timeStampOld[recordId],rtpHeader->ts,rtpHeader->pt);
				/* end huanglei */
				#endif
				/* If the interval between rtp packets is longer than 1*tsInterval, or the packet is cng
				then fill with mute voice data */

/* If the ptime is different from that we use , change to the real ptime after consecutive pkts */
			tsDifference=l_timeStampNew[recordId] - l_timeStampOld[recordId];
			if((tsDifference == 0)&&(rtpHeader->pt>=96)&&(rtpHeader->pt<=127))
			{		/*rfc2833  dtmf包ts相同，但不能作为静音填入,记录一个时间。*/
				tsDtmfAdd[recordId] = g_usbvm_recordHandle[recordId].dtmfTimeDuration;
			}
			else
			{

				if ((tsDifference!= g_usbvm_recordHandle[recordId].tsInterval)&&
				(tsDifference==g_usbvm_recordHandle[recordId].tsRemoteInterval))
				{	/* 连续五个包都是按另一个ptime收的，而不是丢包，说明确实是用另一种实现，应该更改丢包判断所使用的值 */
					/* 只有当双方ptime不同，且对方没有按RFC4733实现才会出现此种情况 */
					diffPtimeCount[recordId]++;
					if(diffPtimeCount[recordId]>=5)
					{/* 交换tsInterval和tsRemoteInterval，次数为5考虑到要及时转换到真正的ptime，且5次中也可能丢包，不能太久，可适当修改 */
						g_usbvm_recordHandle[recordId].tsRemoteInterval = g_usbvm_recordHandle[recordId].tsInterval;
						g_usbvm_recordHandle[recordId].tsInterval = tsDifference;
						MARK_PRINT("HL_DBG:the tsInterval changed from %d to %d, %s:%d\n",
							g_usbvm_recordHandle[recordId].tsRemoteInterval,
							g_usbvm_recordHandle[recordId].tsInterval,__FUNCTION__,__LINE__);
						diffPtimeCount[recordId]=0;
					}
				}
				else if(diffPtimeCount[recordId]!=0)
				{	/* 如果不是连续的出现，则说明不一定是按另一个ptime收包 */
					/* 例如20的间隔丢2个包等于60的情况，连续五次都恰好丢且只丢两个包，概率很小，就算出现了，也可以再次换回.*/
					diffPtimeCount[recordId]=0;
				}

			}
	
		
				if((tsDifference!=0)&&(tsDtmfAdd[recordId]!=0))
				{
					MARK_PRINT("HL_DBG:tsDifference(%d), tsDtmfAdd(%d)[%s:%d]\n",tsDifference,tsDtmfAdd[recordId],__FUNCTION__,__LINE__);
					/* 如果中间是dtmf包，它们的ts相同，可能重发可能丢包，而且end包不算在内*/
					/* 举例来说，如果tsDtmfAdd是60，那么tsDifference>60则丢包。转换成一个包的间隔，
					即把上述所有包视为一个，再继续下面的判断 */
					tsDifference = tsDifference - tsDtmfAdd[recordId] + g_usbvm_recordHandle[recordId].tsInterval;
					tsDtmfAdd[recordId] = 0;
				}

				if ((rtpHeader->pt == CNG_PAYLOAD_TYPE||rtpHeader->pt == CNG_PAYLOAD_TYPE_OLD) 
					|| (tsDifference > 1 * g_usbvm_recordHandle[recordId].tsInterval))
				{
					/* Fill with the mute voice data */
					MARK_PRINT("rtp packet lost, fill with mute data,the pkt inteval:%d,the interval in sdp:%d rtpHeader->pt:%d\n",
					tsDifference,g_usbvm_recordHandle[recordId].tsInterval,rtpHeader->pt);
			
					l_rtpMutePaddingLen[recordId] = l_timeStampNew[recordId] - l_timeStampOld[recordId] - l_rtpLenOld[recordId];
					if ((l_rtpMutePaddingLen[recordId] > 0) && (l_rtpMutePaddingLen[recordId] <= 3 * REC_BUFFER_SIZE))
					{
						if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMU)
						{
							memset(l_rtpMuteData[recordId], PCMU_MUTE_DATA, l_rtpMutePaddingLen[recordId]);
						}
						else if (g_usbvm_recordHandle[recordId].payloadType == RTP_PAYLOAD_PCMA)
						{
							memset(l_rtpMuteData[recordId], PCMA_MUTE_DATA, l_rtpMutePaddingLen[recordId]);
						}
						memcpy(l_pRecBuf[recordId] + l_recBufCount[recordId], l_rtpMuteData[recordId], l_rtpMutePaddingLen[recordId]);
						l_recBufCount[recordId] += l_rtpMutePaddingLen[recordId];
					}
					else
					{	/* Now the mutedata that only less than 600ms can be saved. if it's longer, save it here*/
						/* tbd, save mute data,then recount, and save again until all lost pkts is replaced by mutedata*/
					}

					}

				l_timeStampOld[recordId] = l_timeStampNew[recordId];

	/* 录音前去掉rtp头，取出负载 */
	/* Find CSRC source */
	if (rtpHeader->cc) /* cc == 0 to 15,RFC 3550 */
	{
	  wCSRCByte = (unsigned short)rtpHeader->cc;
	  wCSRCByte <<= 2;	 /* cc * 4; */
	}
	/* Find extension byte */
	if (rtpHeader->x) /* x == 1 means has a extension header*/
	{
	  unsigned short extlen;
	
	  /* Extension length */
	  extlen = *(unsigned short*)&rtpPacket[sizeof(pjmedia_rtp_hdr) + wCSRCByte + 2];
	
	  /*
	  ** number of 16 bit byte = (number of 32 bit word + 1) x 2
	  ** (+1 comes from the extension descriptor.  Please see section 5.3.1 RFC1889)
	  */
	  /* 4字节扩展头的头部，再加上extlen*32bit的扩展内容，RFC 3550，8bit=1字节 */
	  wExtensionByte = (unsigned short) (((unsigned short)(extlen) + 1) << 2);
	}
	pPayload = rtpPacket + sizeof(pjmedia_rtp_hdr) + wCSRCByte + wExtensionByte;
	payloadLen = rtpPaketLen - (sizeof(pjmedia_rtp_hdr) + wCSRCByte + wExtensionByte);
		
				if (rtpHeader->pt == CNG_PAYLOAD_TYPE||rtpHeader->pt == CNG_PAYLOAD_TYPE_OLD)
				{
					l_rtpLenOld[recordId] = 0;
				}
				else
				{
					l_rtpLenOld[recordId] = rtpPaketLen - sizeof(pjmedia_rtp_hdr);
				}

				/* Fill with rtp voice data */
				if (g_usbvm_recordHandle[recordId].payloadType == rtpHeader->pt)
				{
					memcpy(l_pRecBuf[recordId] + l_recBufCount[recordId], pPayload, payloadLen);
					l_recBufCount[recordId] += payloadLen;
				}

				/* Receive Buffer data can be saved */
				if ((l_recBufCount[recordId] >= REC_BUFFER_SIZE) && (l_recBufCount[recordId] <= 4 * REC_BUFFER_SIZE))
				{
					gettimeofday(&tt,NULL); 
					tspec.tv_sec = tt.tv_sec + REC_TASK_WAKEUP_MS/1000;
					tspec.tv_nsec = tt.tv_usec * 1000; 
					tspec.tv_sec += tspec.tv_nsec/(1000 * 1000 *1000);
					tspec.tv_nsec %= (1000 * 1000 *1000);
					if (sem_timedwait(&g_usbvm_recordHandle[recordId].semRtpSaved, &tspec) == 0)
					{
						g_usbvm_recordHandle[recordId].pRecordBuf = l_pRecBuf[recordId];
						g_usbvm_recordHandle[recordId].recordBufLen = l_recBufCount[recordId];
						sem_post((sem_t *)&g_usbvm_recordHandle[recordId].semRtpAvailable);
						if (l_pRecBuf[recordId] ==  l_recordBuf1[recordId])
						{
							l_pRecBuf[recordId] = l_recordBuf2[recordId];
						}
						else
						{
							l_pRecBuf[recordId] = l_recordBuf1[recordId];
						}
						l_recBufCount[recordId] = 0;
					}
					else
					{
						MARK_PRINT("%s semRtpSaved time out\n", __FUNCTION__);
					}		
				}

				pthread_mutex_unlock(&g_usbvm_recordHandle[recordId].mutexRtpReceive);
				
			}
		}
		else
		{/* 此时record线程不再录音，我们也不再写入buffer，不管record线程是否已经退出*/
			l_recBufCount[recordId] = 0;
		}
	}
}


/* 
 * fn		void usbvm_remoteRecStart( int recordId )
 * brief	Start remote record including create record file and record task
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
int usbvm_remoteRecStart( int recordId )
{	
	int i = 0;
	BOOL formatMatch = FALSE;
	int formatTag = 0;
	int sampleRate = 0;
	int resolution = 0;
	char wavName[100] = {0};
	char cmd[100];
	pj_assert(g_usbvm_recordHandle[recordId].endpt>=0&&g_usbvm_recordHandle[recordId].endpt<ENDPT_NUM);
	cmsip_send_systemlog(CMSIP_SYSTEMLOG_DEBUG, 
		"USB Voice MailBox-- Voice Remote Record Start!\n");
	if (sem_init((sem_t *)&g_usbvm_recordHandle[recordId].semRecord, 0, 0) != 0)
	{
		 MARK_PRINT("SEM semRecord initilization failed!");
		 return -1;		
	}

	if (sem_init((sem_t *)&g_usbvm_recordHandle[recordId].semRtpAvailable, 0, 0) != 0)
	{
		 MARK_PRINT("SEM semRtpAvailable initilization failed!");
		 return -1;		
	}
	if (sem_init((sem_t *)&g_usbvm_recordHandle[recordId].semRtpSaved, 0, 1) != 0)
	{
		 MARK_PRINT("SEM semRtpSaved initilization failed!");
		 return -1;		
	}
	l_recordBuf1[recordId] = (UINT8 *)malloc(4 * REC_BUFFER_SIZE);
	l_recordBuf2[recordId] = (UINT8 *)malloc(4 * REC_BUFFER_SIZE);
	l_pRecBuf[recordId] = l_recordBuf1[recordId];
	l_bFirstPacket[recordId] = TRUE;
	l_rtpLenOld[recordId] = 0;
	l_recBufCount[recordId] = 0;

	g_usbvm_recordHandle[recordId].recordStatus = RECORD_START;

	/* Open a new file for record according to the current time */
	g_usbvm_recordHandle[recordId].fileTimName = (unsigned int)time(NULL);
	g_usbvm_recordHandle[recordId].fileSize = 0;

	sprintf(l_recFilePathName[g_usbvm_recordHandle[recordId].endpt],
		"%s", USB_REC_FILE_DIR);

	/* Mkdir if need */
	if (access(l_recFilePathName[g_usbvm_recordHandle[recordId].endpt], F_OK) !=0 )
	{
	   memset(cmd, 0, sizeof(cmd));
	   sprintf(cmd,"mkdir -p %s",l_recFilePathName[g_usbvm_recordHandle[recordId].endpt]);
	   system(cmd);
	   chmod(l_recFilePathName[g_usbvm_recordHandle[recordId].endpt], 0777);
	} 

	usbvm_wavNameFormat2(g_usbvm_recordHandle[recordId].fileTimName, 
						g_usbvm_recordHandle[recordId].srcNum, 
						g_usbvm_recordHandle[recordId].dstNum, 
						wavName);
	strcat(l_recFilePathName[g_usbvm_recordHandle[recordId].endpt], wavName);
	
	/* Remove the record file which name has already eixst */
	if (access(l_recFilePathName[g_usbvm_recordHandle[recordId].endpt], F_OK) == 0)
	{
		char tmpFileName[200] = {0};

		strcpy(tmpFileName, l_recFilePathName[g_usbvm_recordHandle[recordId].endpt]);
		usbvm_escapeCharHandle(tmpFileName);

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"rm -f %s", tmpFileName);
		system(cmd);
		MARK_PRINT("Remove file %s which has the same name with the current record file\n",
			l_recFilePathName[g_usbvm_recordHandle[recordId].endpt]);
#if 0
		g_usbvm_remoteFileInUse[recordId]=FALSE;
#endif
	}
	if (g_usbvm_recordHandle[recordId].fileTimName != 0)
	{
	/* close old fp in 'g_usbvm_recordHandle' if it is used,before open new fp */
		usbvm_closeRecFile(recordId);
		g_usbvm_recordHandle[recordId].outputFp = fopen(l_recFilePathName[g_usbvm_recordHandle[recordId].endpt], "wb+");
		if (g_usbvm_recordHandle[recordId].outputFp == NULL)
		{
			MARK_PRINT("Failed to open %s for writing\n", l_recFilePathName[g_usbvm_recordHandle[recordId].endpt]);
			usbvm_remoteStatusLockWrite(REMOTE_STOP, recordId);
			g_usbvm_recordHandle[recordId].recordStatus = RECORD_STOP;
			return -1;
		}

		while (codecAttributeTable[i].eptCodec != CODEC_NULL)
		{
			if (codecAttributeTable[i].rtpPayloadType == g_usbvm_recordHandle[recordId].payloadType)
			{
				formatMatch = TRUE;
				break;
			}
			i++;
		}
		/* If the codec is supported, then do transformation */
		if (formatMatch)
		{
			sampleRate = codecAttributeTable[i].soundFormat.samplesPerSec;
			switch (codecAttributeTable[i].eptCodec)
			{
			case CODEC_PCMU:
				formatTag = PCMU_FORMAT_TAG;
				resolution = PCM_RESOLUTION_8;
				break;
			case CODEC_PCMA:
				formatTag = PCMA_FORMAT_TAG;
				resolution = PCM_RESOLUTION_8;
				break;
			default:
				formatTag = PCM_FORMAT_TAG;
				resolution = PCM_RESOLUTION_16;
				break;
			}
		}
		
		usbvm_createWavHeader(g_usbvm_recordHandle[recordId].outputFp, formatTag, 
				PCM_CHANNEL_MONO, sampleRate, resolution);	
		
		fseek(g_usbvm_recordHandle[recordId].outputFp, WAV_HEADER_LEN, SEEK_SET);
	}
	/* add the file header's length */
	g_usbvm_recordHandle[recordId].fileSize += WAV_HEADER_LEN;
	/* Create a thread for rtp recording */
	if(g_usbvm_recordHandle[recordId].recordThreadRunning==PJ_FALSE)
	{
		remoteRecThreadCreate(recordId);
	}
	sem_post((sem_t *)&g_usbvm_recordHandle[recordId].semRecord);
	return 0;
}

/* 
 * fn		BOOL usbvm_remoteRecStop( int recordId )
 * brief	Stop remote record and add record node to record list
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	Record file's length greater than 1s return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
BOOL usbvm_remoteRecStop( int recordId )
{
	USBVM_REC_FILE_NODE recNode;
	USBVM_WAV_FIFO_INFO wavIndexInfo;
	char recFilePathName[100];
	char cmd[100];
	BOOL rc = FALSE;
	BOOL gblWavEn = FALSE;
	FILE* fpIndex = NULL;
	int lockRet = -1;
	int i;
	int recIndexNumInAll;
	USBVM_REC_FILE_NODE cmpNode;
	pj_status_t pj_status;
	UINT64 delta = 0;
	BOOL plus = TRUE;
	if(g_usbvm_recordHandle[recordId].recordStop == TRUE)
	{/* 已经执行过一次 */
		return FALSE;
	}
	else
	{/* 一调用到这里就置为true，直到下次启动usbvm才改为FALSE */
		g_usbvm_recordHandle[recordId].recordStop = TRUE;
	}

	if (pthread_mutex_lock((pthread_mutex_t *)&g_usbvm_recordHandle[recordId].mutexRecordStop) == 0)
	{
		cmsip_send_systemlog(CMSIP_SYSTEMLOG_DEBUG, 
			"USB Voice MailBox-- Voice Remote Record Stop!\n");	
		if (g_usbvm_recordHandle[recordId].recordStatus == RECORD_IDLE)
		{
		    pthread_mutex_unlock((pthread_mutex_t *)&g_usbvm_recordHandle[recordId].mutexRecordStop);
		    return FALSE;
		}
		if (g_usbvm_recordHandle[recordId].recordThreadRunning == PJ_TRUE)
		{

			MARK_PRINT("HL_DBG: record thread is running,stop it, %s:%d\n",__FUNCTION__,__LINE__);
		
			g_usbvm_recordHandle[recordId].recordStatus = RECORD_STOP;
			sem_post(&g_usbvm_recordHandle[recordId].semRtpAvailable);
			
			pj_status = pj_thread_join(pjsua_var.usbvmThread[recordId][2]);
			if(pj_status!=PJ_SUCCESS)
			{
				MARK_PRINT("HL_DBG: pj_thread_join(recordThread) error %s:%d\n",
					__FUNCTION__,__LINE__);
			}
			else
			{
				MARK_PRINT("HL_DBG: exit record Thread! %s:%d\n",
					__FUNCTION__,__LINE__);
			}
		}
		
		
		memset(&recNode,'\0',sizeof(USBVM_REC_FILE_NODE));
		memset(&cmpNode,'\0',sizeof(USBVM_REC_FILE_NODE));
/*20130618_update*/
		/*<< BosaZhong@09Jun2013, modify, skip dtmf # when recording. */
		usbvm_remoteRecord_truncateFile(recordId);
		/*>> endof BosaZhong@09Jun2013, modify, skip dtmf # when recording. */
		
#if 0
		MARK_PRINT("usbvm_remoteRecStop size:%d, src:%s, dst:%s, payloadtype:%d\n",
			g_usbvm_recordHandle[recordId].fileSize, 
			g_usbvm_recordHandle[recordId].srcNum,
			g_usbvm_recordHandle[recordId].dstNum, 
			g_usbvm_recordHandle[recordId].payloadType);
#endif

		/* Add record index node and copy the record file to USB */	
		if (g_usbvm_recordHandle[recordId].fileSize >= REC_SIZE_ONE_SEC)
		{
			recNode.fileTimName = g_usbvm_recordHandle[recordId].fileTimName;
			wavIndexInfo.fileTimeName = recNode.fileTimName;
			recNode.fileSize = g_usbvm_recordHandle[recordId].fileSize;	
			recNode.fileDuration = recNode.fileSize / REC_SIZE_ONE_SEC;
			if (recNode.fileSize % REC_SIZE_ONE_SEC >= REC_SIZE_HALF_SEC)
			{	/* if it will exeed the limits of record duration in configeration, don't plus 1 */
				recNode.fileDuration += 1;
			}
			if(recNode.fileDuration > g_usbvm_recordHandle[recordId].remoteRecDuration)
			{	
				recNode.fileDuration = g_usbvm_recordHandle[recordId].remoteRecDuration;
			}
			strcpy(recNode.srcNum, g_usbvm_recordHandle[recordId].srcNum);	
			strcpy(recNode.dstNum, g_usbvm_recordHandle[recordId].dstNum);
			recNode.payloadType = g_usbvm_recordHandle[recordId].payloadType;
			recNode.readFlag = FALSE;	
			wavIndexInfo.endpt = g_usbvm_recordHandle[recordId].endpt;
			usbvm_calChkSum(&recNode);	
			
			lockRet = usbvm_openIndexFpAndLock(
					g_usbvm_recordHandle[recordId].endpt,
					&fpIndex, 
					"rb+", 
					WR_LOCK_WHOLE);
					
		if(lockRet != -1)
		{
			/* Check for repeat nodes, if it exists, delete it and add to the list tail*/
			recIndexNumInAll = usbvm_getRecNumInAll(g_usbvm_recordHandle[recordId].endpt, fpIndex);
			for (i = 1; i <= recIndexNumInAll; i++)
			{
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE +  (i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return FALSE;
				}
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&cmpNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return FALSE;
				}
				/* Find the insert position according to the record time of the record file */
				if (recNode.fileTimName == cmpNode.fileTimName)
				{
					break;
				}
			}
			if(i <= recIndexNumInAll)
			{
				MARK_PRINT("HL_DBG: find repeated node! Delete it [%s:%d]\n",__FUNCTION__,__LINE__);
				int ret = usbvm_delRecIndexListOnly(g_usbvm_recordHandle[recordId].endpt, i, fpIndex);
				if(ret < 0)
				{
					MARK_PRINT("HL_DBG: Delete repeat node failed! [%s:%d]\n",__FUNCTION__,__LINE__);
				}
				if(cmpNode.fileSize > recNode.fileSize)
				{	/* the old file is bigger, reduce size */
					delta = cmpNode.fileSize - recNode.fileSize;
					plus = FALSE;
				}
				else
				{	/* the new file is bigger, increase size */
					delta = recNode.fileSize - cmpNode.fileSize;
					plus = TRUE;
				}
			}
			else
			{	/* no file with same name, add the size of index node. */
				delta = recNode.fileSize + sizeof(USBVM_REC_FILE_NODE);
				plus = TRUE;
			}
			if(usbvm_addRecIndexList(g_usbvm_recordHandle[recordId].endpt, &recNode,fpIndex)<0)
			{
				MARK_PRINT("HL_DBG: usbvm_addRecIndexList failed!![%s:%d]\n",__FUNCTION__,__LINE__);
			}
			else
			{
				usbvm_rewriteUsedSize(delta,plus);
			}
			lockRet = usbvm_closeIndexFpAndUnlock(
					g_usbvm_recordHandle[recordId].endpt,
					fpIndex, 
					WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
		}
		else
		{
			MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			wavIndexInfo.fileTimeName= -1;
		}
			if (recNode.fileTimName < 0)
			{
				usbvm_closeRecFile(recordId);
				g_usbvm_recordHandle[recordId].fileTimName = 0;
				g_usbvm_recordHandle[recordId].fileSize = 0;
				pthread_mutex_unlock((pthread_mutex_t *)&g_usbvm_recordHandle[recordId].mutexRecordStop);
				return FALSE;

			}
			rc = TRUE;

			gblWavEn = g_usbvm_recordHandle[recordId].globalWav;
			if (gblWavEn)
			{
				g_usbvm_recordHandle[recordId].globalTimeName = wavIndexInfo.fileTimeName;
				MARK_PRINT("HL_DBG:globalTimeName(%08x) [%s:%d]\n",
					g_usbvm_recordHandle[recordId].globalTimeName,
					__FUNCTION__,__LINE__);
			}
#if 0
			g_usbvm_remoteFileInUse[recordId]=TRUE;
#endif
			if (g_usbvm_recordHandle[recordId].outputFp != NULL)
			{
				if (fseek(g_usbvm_recordHandle[recordId].outputFp, 0, SEEK_SET) == -1)
				{
					usbvm_closeRecFile(recordId);
					g_usbvm_recordHandle[recordId].fileTimName = 0;
					g_usbvm_recordHandle[recordId].fileSize = 0;
					pthread_mutex_unlock((pthread_mutex_t *)&g_usbvm_recordHandle[recordId].mutexRecordStop);
					return FALSE;
				}
/*20130618_update*/
				
				if (usbvm_updateWavHeader(g_usbvm_recordHandle[recordId].outputFp) != 0)
				{
					usbvm_closeRecFile(recordId);
					g_usbvm_recordHandle[recordId].fileTimName = 0;
					g_usbvm_recordHandle[recordId].fileSize = 0;
					pthread_mutex_unlock((pthread_mutex_t *)&g_usbvm_recordHandle[recordId].mutexRecordStop);
					return FALSE;
				}
				/* no need to fflush before fclose */
				usbvm_closeRecFile(recordId);
#ifdef INCLUDE_MAIL
				remoteRecPush(&recNode);
#endif /* INCLUDE_MAIL */
			}
		}
		else
		{
			char wavName[200] = {0};
			char tmpFileName[200] = {0};
			
			/* Delete the record file if the duration is less than 1s */
			usbvm_closeRecFile(recordId);

			usbvm_wavNameFormat2(g_usbvm_recordHandle[recordId].fileTimName, 
					g_usbvm_recordHandle[recordId].srcNum, 
					g_usbvm_recordHandle[recordId].dstNum, 
					wavName);
			
			memset(recFilePathName, 0, 100);		
			sprintf(recFilePathName, "%s%s", USB_REC_FILE_DIR, wavName);

			strcpy(tmpFileName, recFilePathName);
			usbvm_escapeCharHandle(tmpFileName);

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd,"rm -f %s", tmpFileName);

			system(cmd);
			MARK_PRINT("usbvm_remoteRecStop delete file:%s,%s:%d rc %d\n", recFilePathName,__FUNCTION__,__LINE__, rc);
			
		}

		sem_destroy((sem_t *)&g_usbvm_recordHandle[recordId].semRecord);
   		sem_destroy((sem_t *)&g_usbvm_recordHandle[recordId].semRtpAvailable);
   		sem_destroy((sem_t *)&g_usbvm_recordHandle[recordId].semRtpSaved); 
		if (l_recordBuf1[recordId] != NULL)
		{
			free(l_recordBuf1[recordId]);
			l_recordBuf1[recordId] = NULL;
		}
		if (l_recordBuf2[recordId] != NULL)
		{
			free(l_recordBuf2[recordId]);
			l_recordBuf2[recordId] = NULL;
		}
#if 0
		g_usbvm_remoteFileInUse[recordId]=FALSE;
#endif
		g_usbvm_recordHandle[recordId].fileTimName = 0;
		g_usbvm_recordHandle[recordId].fileSize = 0;
		pthread_mutex_unlock(&g_usbvm_recordHandle[recordId].mutexRecordStop);
	}
	
	sync();
	return rc;
}

/* 
 * fn		int usbvm_closeRecFile()
 * brief	close record file and release fp
 * details	
 *
 * param[in]	int recordId
 * param[out]	
 *
 * return 
 * retval	
 *
 * note		Added by huanglei
 */
void usbvm_closeRecFile(int recordId)
{
	int ret_fc;
	if (g_usbvm_recordHandle[recordId].outputFp != NULL)
	{
		ret_fc=fclose(g_usbvm_recordHandle[recordId].outputFp);
		if(ret_fc!=0)
		{
			MARK_PRINT("HL_DBG:fclose() failed! ret=%d, %s:%d\n",ret_fc,__FUNCTION__,__LINE__);
		}
		g_usbvm_recordHandle[recordId].outputFp = NULL;
	}
}
/* 
 * fn		void usbvm_remoteStatusLockRead(REMOTE_STAS* pStatus,int recordId) 
 * brief	lock global variable, read from it,then unlock.
 * details	g_usbvm_recordHandle[recordId].remoteStatus.For this var is used frequently,so I make 
 *			the codes in a func.
 * param[in]	int recordId, index of recording process.
 * param[out]	REMOTE_STAS* pStatus, pointer to status that get from global var.
 *
 * return	void
 * retval	void
 *
 * note		
 */
void usbvm_remoteStatusLockRead(REMOTE_STAS* pStatus,int recordId)
{
	if(pthread_mutex_lock(&g_usbvm_statusRwLock[recordId])!=0)
	{	
		MARK_PRINT("lock remoteStatus failed!!! recordId=%d\n",recordId);
	}
	else
	{
		*pStatus = g_usbvm_recordHandle[recordId].remoteStatus;
		if(pthread_mutex_unlock(&g_usbvm_statusRwLock[recordId])!=0)
		{
			MARK_PRINT("unlock remoteStatus failed!!! recordId=%d\n",recordId);
		}
	}	
}

/* 
 * fn		void usbvm_remoteStatusLockWrite(REMOTE_STAS status,int recordId) 
 * brief	lock global variable, write to it,then unlock.
 * details	g_usbvm_recordHandle[recordId].remoteStatus.For this var is used frequently.
 *
 * param[in]	REMOTE_STAS status, the satus value that will be written.
 *				int recordId, index of recording process.	
 * param[out]	
 *
 * return	void	
 * retval	void
 *
 * note		
 */
void usbvm_remoteStatusLockWrite(REMOTE_STAS status,int recordId)
{
	if(pthread_mutex_lock(&g_usbvm_statusRwLock[recordId])!=0)
	{
		MARK_PRINT("lock remoteStatus failed!!! recordId=%d status=%d\n",recordId,status);
	}
	else
	{	/* the REMOTE_STOP should not be changed into other states except REMOTE_IDLE  */
		if((g_usbvm_recordHandle[recordId].remoteStatus!=REMOTE_STOP)
			||(status==REMOTE_IDLE))
		{
			g_usbvm_recordHandle[recordId].remoteStatus = status;
		}
		if(pthread_mutex_unlock(&g_usbvm_statusRwLock[recordId])!=0)
		{
			MARK_PRINT("unlock remoteStatus failed!!! recordId=%d status=%d\n",recordId,status);
		}
	}	
}
/*20130618_update*/
/* 
 * brief  void usbvm_remoteRecord_setGotDtmfHashInRecording(int recordId, 
 *				unsigned short dtmfNum, unsigned short dtmfStartNum, int dtmfType)
 *
 * note		1. BosaZhong@09Jun2013, add, access bGotDtmfHashInRecording.
 */
void usbvm_remoteRecord_setGotDtmfHashInRecording(int recordId, 
				unsigned short dtmfNum, unsigned short dtmfStartNum, int dtmfType)
{
	if ((RTP_NTE_DTMFH == dtmfNum || RTP_NTE_DTMFH == dtmfStartNum)
	 && REMOTE_RECORD == g_usbvm_recordHandle[recordId].remoteStatus
	 && g_usbvm_recordHandle[recordId].nGotDtmfHashInRecording < 0)
	{
		g_usbvm_recordHandle[recordId].nGotDtmfHashInRecording = dtmfType;

		TT_PRINT("-----recordId(%d) localPtime(%d)-- gotH(0 -> 1)",
			recordId, g_usbvm_recordHandle[recordId].localPtime);
		TT_PRINT("-----dtmf(%d/%d) dtmfStart(%d/%d) remoteStatus(%d/3) "
			  "pkgLen(%d) dtmfDur(%d) gotH(%d:0-in,1-2833,2-info)",
			dtmfNum, RTP_NTE_DTMFH,
			dtmfStartNum, RTP_NTE_DTMFH,
			g_usbvm_recordHandle[recordId].remoteStatus,
			g_usbvm_recordHandle[recordId].rtpPacketBufLen,
			g_usbvm_recordHandle[recordId].dtmfTimeDuration,
			g_usbvm_recordHandle[recordId].nGotDtmfHashInRecording);
	}

	return ;
}

/* 
 * brief  BOOL usbvm_remoteRecord_getGotDtmfHashInRecording(int recordId)
 *
 * note		1. BosaZhong@09Jun2013, add, access bGotDtmfHashInRecording
 */
BOOL usbvm_remoteRecord_getGotDtmfHashInRecording(int recordId)
{
	return g_usbvm_recordHandle[recordId].nGotDtmfHashInRecording;
}


/* 
 * brief  void usbvm_remoteRecord_clrGotDtmfHashInRecording(int recordId)
 *
 * note		1. BosaZhong@09Jun2013, add, access bGotDtmfHashInRecording
 */
void usbvm_remoteRecord_clrGotDtmfHashInRecording(int recordId)
{
	g_usbvm_recordHandle[recordId].nGotDtmfHashInRecording = -1;
}


/*
 * brief	 void usbvm_remoteRecord_truncateFile(int recordId)
 *
 * note		1. BosaZhong@09Jun2013, add, truncateFile when DTMF MODE is INBAND.
 */
void usbvm_remoteRecord_truncateFile(int recordId)
{
#define _USBVM_PACKET_UNIT			8	
	/* Note: make sure you can acess record file before invoke this function. */
	int ret = 0;
	int dtmfType = 0;
	int packetTime = 0;
	int skipPacketNum = 0; /* the number of skip a rtp data packet. */
	int truncateSize = 0;
	int originalFileSize = 0;
	FILE * targetFp = NULL;

	unsigned int fileTimName = 0;

	dtmfType = g_usbvm_recordHandle[recordId].nGotDtmfHashInRecording;
	packetTime = g_usbvm_recordHandle[recordId].localPtime;
	targetFp = g_usbvm_recordHandle[recordId].outputFp;
	originalFileSize = g_usbvm_recordHandle[recordId].fileSize;

	fileTimName = g_usbvm_recordHandle[recordId].fileTimName;

	/* note: important!!! 
	 *    if packetTime is 20 ms, truncate unit is 160 Bytes.
	 */
	skipPacketNum = 0;
	if (CMSIP_DTMF_SIPINFO == dtmfType)
	{
		skipPacketNum = 1;
	}
	else if (CMSIP_DTMF_INBAND == dtmfType)
	{
		skipPacketNum = 1;
	}
	else if (CMSIP_DTMF_RFC2833 == dtmfType)
	{
		skipPacketNum = 1;
	}
	
	truncateSize = (_USBVM_PACKET_UNIT * packetTime) * skipPacketNum;

	if (CMSIP_DTMF_SIPINFO == dtmfType
	 || CMSIP_DTMF_INBAND == dtmfType
	 || CMSIP_DTMF_RFC2833 == dtmfType)
	{
		if (targetFp != NULL)
		{
			ret = ftruncate(fileno(targetFp), originalFileSize - truncateSize);
			if (ret >= 0)
			{
				g_usbvm_recordHandle[recordId].fileSize -= truncateSize;
			}
		}
	}

	
	TT_PRINT("ret(%d) errno(%d)- truncateFile: recordId(%d) fileno(%d) fileTimName(%x) "
			"originalSize(%d) truncateSize(%d) ptime(%d) hashType(%d)",
			ret, errno,
			recordId, fileno(targetFp), fileTimName,
			originalFileSize, truncateSize,
			packetTime, dtmfType);

	return ;
#undef _USBVM_PACKET_UNIT
}


/*
 * brief    int usbvm_remoteRecord_getRecordIdFromCnxId(int cnxId)
 *
 * note    1. BosaZhong@09Jun2013, add, get recordId from cnxId
 */
int usbvm_remoteRecord_getRecordIdFromCnxId(int cnxId)
{
	int idx = 0;

	for (idx = PJSUA_USBVM_MAX_RECORDID - 1; idx >= 0 ; idx--)
	{
		if (g_usbvm_recordHandle[idx].cnxId == cnxId)
		{
			break;
		}
	}

	return idx;
}



/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
