/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_remoteRecord.h
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		19Jul11
 *
 * history 	\arg	
 */

#ifndef __USBVM_REMOTERECORD_H__
#define __USBVM_REMOTERECORD_H__

#include "usbvm_types.h"
#include <usbvm_glbdef.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <pjmedia/rtp.h>       /* pjmedia_rtp_hdr */
#include <pjmedia/stream.h>    /* pjmedia_stream */
#include <pjsua-lib/pjsua.h>
#include <cmsip_msgdef.h>
#include <usbvm_recFileMang.h>
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
/* 2012-5-24, 仅对VG3631V1: 492 */
/* RTP大小=负载(最大60ms*8Byte/ms)+rtp固定头(12)+csrc(最大值4*15)+扩展(4+自定义长度), RFC3550*/  
/* 根据RFC3551,音视频的RTP无扩展头 */
/* 根据RFC3550,仅有RTP MIXER时才需要CSRC,例如三方通话.usbvm目前不支持. */
/* 3631IAD只支持60ms, 对IAD最大支持的ptime, 没有统一规定, 一般只支持到10~60ms.
	按RFC4733,若我们最大只支持60ms,在协商之后,对方应按我们协商的ptime发包,最大也只能到60ms.
	因此在有sdp协商情况下,对方仍按超出60ms间隔发包,视为其不合规范,不能完全支持.对这种
*/
/* 当usbvm支持的编码方式增加,或功能有变化造成RTP大小变化时,需要修改这个宏 */
#define USBVM_RTP_PKT_SIZE 492 /*60*8 + sizeof(pjmedia_rtp_hdr)=492*/	

#ifndef REC_SIZE_ONE_SEC
#define REC_SIZE_ONE_SEC   8000
#endif

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

typedef enum
{
	DTMF_NULL,
	DTMF_INBAND,
	DTMF_RFC2833,
	DTMF_SIPINFO

} DTMF_MODE;


typedef enum
{
	RECORD_IDLE,
	RECORD_START,
	RECORD_PROCESS,
	RECORD_STOP,
	RECORD_FINISHED,
	RECORD_ERROR

} RECORD_STAS;

typedef enum
{
	REMOTE_IDLE = 0,
	REMOTE_START = 1,
	REMOTE_RECORD_START_NOTIFY =2,    /* 转入语音信箱播放录音开始提示音 */
	REMOTE_RECORD = 3,                 /* 录音 */
	REMOTE_RECORD_STOP_NOTIFY = 4,     /* 按"#"播放录音结束提示音 */
	REMOTE_RECORD_VERIFY = 5,          /* 验证留言 */
	REMOTE_RECORD_REPEAT = 6,          /* 重新录制 */
	REMOTE_RECORD_EXIT = 7,            
	REMOTE_PINVALIDATE = 8,            /* 验证PIN码 */
	REMOTE_NORECORD = 9,               /* 无新留言提示 */
	REMOTE_PLAY_NOTIFY = 10,            /* 播放听取留言操作提示音 */
	REMOTE_PLAY = 11,                    /* 播放留言 */
	REMOTE_STOP = 12		/* 强制退出线程，usb被拔出时还在录音，added by 黄磊  */

} REMOTE_STAS;

typedef struct _USBVM_REMOTE_RECORD_HANDLE
{
	BOOL recordThreadRunning;
	BOOL playThreadRunning;
	BOOL rcvRtpThreadRunning;
	FILE *outputFp;
	char *pVoiceNotifyBuf;
	unsigned int fileTimName;
	unsigned int fileSize;
	unsigned int payloadType;
	char srcNum[MAX_DIGITS_NUM];	
	char dstNum[MAX_DIGITS_NUM];
	REMOTE_STAS remoteStatus;
	RECORD_STAS recordStatus;
	pthread_mutex_t mutexRecordStop;
	pthread_mutex_t mutexRtpReceive;
	sem_t semRecord;
	sem_t semRtpAvailable;
	sem_t semRtpSaved;
	sem_t semRtpRcvForUsbvm;
	UINT8 *pRecordBuf;
	UINT32 recordBufLen;
	char *rtpPacketBuf;
	int rtpPacketBufLen;
	BOOL bNoAnswer;
	BOOL bVoiceNotify;
	int cnxId;						/*call's index,do not mean connection's id */
	int endpt;
	char collectPinNum[10];
	BOOL cngSend;
	char cngData;
	int remotePtime;			/* The ptime in sdp,that the opponent tells us.We use it to send RTP */
	int localPtime;				/* The ptime in sdp,that we tell the opponent.We use it to recieve RTP */
	int tsRemoteInterval;		/* The interval we send RTP,but sometimes it's used for receive pkts,decided by the remote IAD */
	int tsInterval;
	UINT32 remoteRecDuration;
	UINT8 globalWav;
	unsigned int globalTimeName;
	int dtmfTimeDuration;		/* To check rtp lost after a dtmf is recieved */
	BOOL holdCall;
	BOOL recordStop;
/*20130618_update*/
	/*<< BosaZhong@09Jun2013, add, set when receive # in REMOTE_RECORD status. */
	int nGotDtmfHashInRecording;	/* -1--not got, other CMSIP_DTMF_SEND_TYPE values. */
	/*>> endof BosaZhong@09Jun2013, add, set when receive # in REMOTE_RECORD status. */
	UINT32 ssrc;	/* allocated by voip client, to keep answer machine and voip server same */
} USBVM_REMOTE_RECORD_HANDLE;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
extern USBVM_REMOTE_RECORD_HANDLE g_usbvm_recordHandle[PJSUA_USBVM_MAX_RECORDID];
#if 0
extern BOOL g_usbvm_remoteFileInUse[PJSUA_USBVM_MAX_RECORDID];
#endif
extern char g_rtpPacketBuf[PJSUA_USBVM_MAX_RECORDID][USBVM_RTP_PKT_SIZE];
extern pthread_mutex_t g_usbvm_statusRwLock[PJSUA_USBVM_MAX_RECORDID];
extern pthread_mutex_t g_usbvm_sendMsgLock[PJSUA_USBVM_MAX_RECORDID];
/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* I have already included <semaphore.h> why sem_timedwait is implicit declaration??? */
extern int sem_timedwait(sem_t * sem,const struct timespec * abs_timeout);
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
void dtmf_FIFOInit( void );

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
void dtmf_FIFOPut( int cnxId, unsigned short dtmfNum, CMSIP_DTMF_SEND_TYPE	 dtmfType );

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
BOOL dtmf_FIFOGet( int cnxId, unsigned short *dtmfNum );

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
void dtmf_FIFOReset( int cnxId );

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
 * note		
 */
BOOL usbvm_matchRecordId( pjmedia_stream *stream, int *recordId );

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
void usbvm_remoteRtpCopy(int recordId, char *rtpPacket, int rtpPacketLen);
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
void usbvm_rtpPktFifoInit(void);
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
int usbvm_rtpPktFifoPut(int recordId, char*rtpPktBuf,int rtpPktLen);
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
int usbvm_rtpPktFifoGet(int recordId, char* rtpPktBuf, int* rtpPktLen);
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
void usbvm_rtpPktFifoReset(int recordId);
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
void usbvm_remoteRtpRcv( int recordId, char *rtpPacket, int rtpPaketLen );


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
int usbvm_remoteRecStart( int recordId );

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
BOOL usbvm_remoteRecStop( int recordId );

/* 
 * fn		void usbvm_closeRecFile(int recordId) 
 * brief	close record file and release fp
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
void usbvm_closeRecFile(int recordId);
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
void usbvm_remoteStatusLockRead(REMOTE_STAS* pStatus,int recordId);

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
void usbvm_remoteStatusLockWrite(REMOTE_STAS status,int recordId);


/*20130618_update*/
/* 
 * brief  void usbvm_remoteRecord_setGotDtmfHashInRecording(int recordId, 
 *				unsigned short dtmfNum, unsigned short dtmfStartNum, int dtmfType)
 *
 * note		1. BosaZhong@09Jun2013, add, access bGotDtmfHashInRecording.
 */
void usbvm_remoteRecord_setGotDtmfHashInRecording(int recordId, 
				unsigned short dtmfNum, unsigned short dtmfStartNum, int dtmfType);

/* 
 * brief  BOOL usbvm_remoteRecord_getGotDtmfHashInRecording(int recordId)
 *
 * note		1. BosaZhong@09Jun2013, add, access bGotDtmfHashInRecording
 */
BOOL usbvm_remoteRecord_getGotDtmfHashInRecording(int recordId);

/* 
 * brief  void usbvm_remoteRecord_clrGotDtmfHashInRecording(int recordId)
 *
 * note		1. BosaZhong@09Jun2013, add, access bGotDtmfHashInRecording
 */
void usbvm_remoteRecord_clrGotDtmfHashInRecording(int recordId);


/*
 * brief	 void usbvm_remoteRecord_truncateFile(int recordId)
 *
 * note		1. BosaZhong@09Jun2013, add, truncateFile when DTMF MODE is INBAND.
 */
void usbvm_remoteRecord_truncateFile(int recordId);


/*
 * brief    int usbvm_remoteRecord_getRecordIdFromCnxId(int cnxId)
 *
 * note    1. BosaZhong@09Jun2013, add, get recordId from cnxId
 */
int usbvm_remoteRecord_getRecordIdFromCnxId(int cnxId);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif	/* __USBVM_REMOTERECORD_H__ */


